/*************************************************************************
*
* VULCANFORMS CONFIDENTIAL
* __________________
*  Copyright, VulcanForms Inc.
*  [2016] - [2020] VulcanForms Incorporated
*  All Rights Reserved.
*
*  "VulcanForms", "Vulcan", "Fusing the Future"
*       are trademarks of VulcanForms, Inc.
*
* NOTICE:  All information contained herein is, and remains
* the property of VulcanForms Incorporated and its suppliers,
* if any.  The intellectual and technical concepts contained
* herein are proprietary to VulcandForms Incorporated
* and its suppliers and may be covered by U.S. and Foreign Patents,
* patents in process, and are protected by trade secret or copyright law.
* Dissemination of this information or reproduction of this material
* is strictly forbidden unless prior written permission is obtained
* from VulcanForms Incorporated.
*/
#pragma once

//#include "Logger.h"
#include "Threadpool.h"
#include <cassert>

#define ALLOW_THREADED 1

// The thread priority lets us increase priority of any
// child tasks, so that we don't explode the number of tasks in flight
// before actually doing any work.
static thread_local int threadPriority = 0;

Threadpool::Threadpool(unsigned int nThreads) : mbStopnow(false), mNumJobsSent(0), mNumJobsComplete(0)
{
    const uint32_t hardThreads    = std::thread::hardware_concurrency();
    uint32_t       poolThreads    = 0; // Default to single-threaded mode
    const auto numThreadsReserved = 4; // We don't want to render the system unusable

#if ALLOW_THREADED
    if (hardThreads > numThreadsReserved) {
        poolThreads = std::clamp(nThreads, 0u, hardThreads - numThreadsReserved);
    }
#endif

    for (unsigned int i=0; i < poolThreads; ++i) {
        mPool.push_back(std::thread(&Threadpool::infinite_loop,this));
    }
}

Threadpool::~Threadpool()
{
    shutdown();
}


Threadpool& Threadpool::instance()
{
    static Threadpool local;
    return local;
}

Threadpool::Waitable Threadpool::submit(Task job, bool threaded)
{
    std::future<void> ret;
    if (job) {
        auto task = std::packaged_task<void()>(job);
        ret = task.get_future();

        Threadpool& t = instance();
        if (threaded) {
            {
                std::unique_lock<std::mutex> lock(t.mQMutex);
                t.mQJobs.emplace(make_pair(threadPriority, move(task)));
                t.mNumJobsSent.fetch_add(1);
            }
            t.mJobAlert.notify_one();
        }
        else {
            t.mNumJobsSent.fetch_add(1);
            task();
            t.mNumJobsComplete.fetch_add(1);
        }
    }

    return ret;
}

Threadpool::WaitableList Threadpool::submit(TaskList job_list, bool threaded)
{
    std::vector<std::future<void>> ret;
    std::vector<std::packaged_task<void()>> tasks;
    ret.reserve(job_list.size());
    tasks.reserve(job_list.size());

    for (auto& job : job_list) {
        if (job) {
            tasks.emplace_back(job);
            ret.emplace_back(tasks.back().get_future());
        }
    }

    Threadpool& t = instance();
    if (threaded) {
        size_t numTasks = tasks.size();
        {
            std::unique_lock<std::mutex> lock(t.mQMutex);
            for (auto& task : tasks) {
                t.mQJobs.emplace(make_pair(threadPriority, move(task)));
            }
            t.mNumJobsSent.fetch_add(tasks.size());
        }
        t.mJobAlert.notify_all(); // Might send some unlucky threads right back to sleep without grabbing work?
    }
    else {
        t.mNumJobsSent.fetch_add(tasks.size());
        for (auto& task : tasks) {
            task();
            t.mNumJobsComplete.fetch_add(1);
        }
    }

    return ret;
}

void Threadpool::submitAndJoin(Task job, bool threaded, LoggingFunc loggingFunc, size_t inFlightLimit)
{
    if (inFlightLimit == SIZE_MAX) {
        join(submit(std::move(job), threaded), loggingFunc);
    }
    else {
        Threadpool& t = instance();
        t.waitForInFlight(inFlightLimit);
        join(submit(std::move(job), threaded), loggingFunc);
    }
}

void Threadpool::submitAndJoin(TaskList job_list, bool threaded, LoggingFunc loggingFunc, size_t inFlightLimit)
{
    if (inFlightLimit == SIZE_MAX) {
        join(submit(std::move(job_list), threaded), loggingFunc);
    }
    else {
        size_t totalJobs        = job_list.size();
        size_t lastSeenComplete = 0;
        auto   startTime        = std::chrono::high_resolution_clock::now();

        WaitableList endWaitables;
        Threadpool& t = instance();
        for (auto taskItr = job_list.begin(); taskItr != job_list.end(); taskItr++) {
            t.waitForInFlight(inFlightLimit);
            endWaitables.emplace_back(submit(std::move(*taskItr), threaded));

            // Find those that are done
            size_t newComplete = 0;
            for (auto eraseItr = std::begin(endWaitables); eraseItr != std::end(endWaitables);) {
                if (!eraseItr->valid() || is_ready(*eraseItr)) {
                    newComplete++;
                    eraseItr->get(); // This should return void, or throw an exception

                    eraseItr = endWaitables.erase(eraseItr);
                }
                else {
                    eraseItr++;
                }
            }

            // If any completed while we were doing work, log that some completed
            if (loggingFunc && newComplete) {
                auto endTime         = std::chrono::high_resolution_clock::now();
                auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

                lastSeenComplete += newComplete;

                loggingFunc(totalJobs - lastSeenComplete, newComplete, timeSinceUpdate.count());

                startTime = std::chrono::high_resolution_clock::now();
            }
        }

        join(std::move(endWaitables), loggingFunc);
    }
}

void Threadpool::join(LoggingFunc loggingFunc)
{
    Threadpool& t = instance();

    int    timeout          = 0;
    size_t lastSeenComplete = t.mNumJobsComplete.load();
    auto   startTime        = std::chrono::high_resolution_clock::now();
    // While we wait, this thread can also act as a worker thread
    // unless there's no work to do, in which case we'll spin-lock
    while (t.mNumJobsSent.load() > t.mNumJobsComplete.load()) {
        if (timeout <= 0) {
            if (!t.do_work_nonblocking(loggingFunc ? false : true)) {
                // There wasn't work, so back off for a bit and then try again
                timeout = TimeoutReset;
            }
        }
        else {
            std::this_thread::yield(); // Spin lock while waiting for another thread to complete
            --timeout;
        }

        if (loggingFunc && (t.mNumJobsComplete.load() != lastSeenComplete)) {
            auto endTime         = std::chrono::high_resolution_clock::now();
            auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            const size_t curComplete  = t.mNumJobsComplete.load();
            const size_t diffComplete = curComplete - lastSeenComplete;
            const size_t leftToFinish = t.mNumJobsSent - curComplete;

            loggingFunc(leftToFinish, diffComplete, timeSinceUpdate.count());

            lastSeenComplete = curComplete;
            startTime        = std::chrono::high_resolution_clock::now();
        }
    }
}

void Threadpool::join(Waitable&& waitable, LoggingFunc loggingFunc)
{
    Threadpool& t = instance();

    int    timeout          = 0;
    size_t lastSeenComplete = t.mNumJobsComplete.load();
    auto   startTime        = std::chrono::high_resolution_clock::now();
    // While we wait, this thread can also act as a worker thread
    // unless there's no work to do, in which case we'll spin-lock
    while (waitable.valid() && !is_ready(waitable)) {
        if (timeout <= 0) {
            if (!t.do_work_nonblocking(false)) {
                // There wasn't work, so back off for a bit and then try again
                timeout = TimeoutReset;
            }
        }
        else {
            std::this_thread::yield(); // Spin lock while waiting for another thread to complete
            --timeout;
        }
    }

    if (loggingFunc) {
        auto endTime         = std::chrono::high_resolution_clock::now();
        auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        const size_t curComplete  = 1; // We were only waiting on a single task
        const size_t diffComplete = 1;
        const size_t leftToFinish = 0;

        loggingFunc(leftToFinish, diffComplete, timeSinceUpdate.count());

        lastSeenComplete = curComplete;
        startTime        = std::chrono::high_resolution_clock::now();
    }

    waitable.get(); // This should return void, or throw an exception
}

void Threadpool::join(WaitableList&& waitables, LoggingFunc loggingFunc)
{
    Threadpool& t = instance();

    size_t totalJobs        = waitables.size();
    int    timeout          = 0;
    auto   startTime        = std::chrono::high_resolution_clock::now();

    // We'd normally start waiting in the back as we'd expect those to complete closer to last,
    // but because we're logging, we want to track the progress of which waitable is available,
    // so we start from the beginning
    while (!waitables.empty()) {
        // While we wait, this thread can also act as a worker thread
        // unless there's no work to do, in which case we'll spin-lock
        if (waitables.front().valid() && !is_ready(waitables.front())) {
            if (timeout <= 0) {
                if (!t.do_work_nonblocking(loggingFunc ? false : true)) {
                    // There wasn't work, so back off for a bit and then try again
                    timeout = TimeoutReset;
                }
            }
            else {
                std::this_thread::yield(); // Spin lock while waiting for another thread to complete
                --timeout;
            }
        }

        // Find those that are done
        size_t newComplete = 0;
        for (auto eraseItr = std::begin(waitables); eraseItr != std::end(waitables);) {
            if (!eraseItr->valid() || is_ready(*eraseItr)) {
                newComplete++;
                eraseItr->get(); // This should return void, or throw an exception

                eraseItr = waitables.erase(eraseItr);
            }
            else {
                eraseItr++;
            }
        }

        // If any completed while we were doing work, log that some completed
        if (loggingFunc && newComplete) {
            auto endTime         = std::chrono::high_resolution_clock::now();
            auto timeSinceUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            loggingFunc(waitables.size(), newComplete, timeSinceUpdate.count());

            startTime = std::chrono::high_resolution_clock::now();
        }
    }
}

bool Threadpool::is_ready(const Waitable& waitable)
{
    using namespace std::chrono_literals;
    return waitable.wait_for(0s) == std::future_status::ready;
}

bool Threadpool::has_jobs()
{
    Threadpool& t = instance();
    bool ret = false;
    std::unique_lock<std::mutex> lock(t.mQMutex);
    ret = !t.mQJobs.empty();
    return ret;
}

size_t Threadpool::num_jobs()
{
    Threadpool& t = instance();
    size_t ret = 0;
    std::unique_lock<std::mutex> lock(t.mQMutex);
    ret = t.mQJobs.size();
    return ret;
}

bool Threadpool::do_work_nonblocking(bool allowLongWork)
{
    bool ret = false;
    std::packaged_task<void()> job;
    {
        std::unique_lock<std::mutex> lock(mQMutex);

        // If we're allowed to do long-running work, go ahead and take a task if possible
        // If there are no worker threads available in the pool, go ahead and do work
        // If we're only allowed to do short work, and there's a high priority task on the queue, go ahead
        if (!mQJobs.empty() && (allowLongWork || mPool.empty() || (mQJobs.top().first > 0))) {
            // At this point the queue has work and this thread was the first to see it,
            // so pull the next job off the list
            auto pair = move(const_cast<PriorityTask&>(mQJobs.top())); // const_cast is necessary to maintain heap structure under pqueue, but we must immediately pop the object
            job = move(pair.second);
            mQJobs.pop();
//            printf("%llu jobs left, max priority %d\n", mQJobs.size(), pair.first);
        }
        // Unlocked mQMutex
    }

    if (job.valid()) {
        // Bracket the job with increasing priority so we do the child tasks first
        // We increase more here because we want to get back to any logging function asap
        threadPriority += 3;
        // Execute our job
        job();
        threadPriority -= 3;
        // Notify that a job has been completed
        mNumJobsComplete.fetch_add(1);
        ret = true;
    }
    return ret;
}

bool Threadpool::do_work()
{
    std::packaged_task<void()> job;
    {
        std::unique_lock<std::mutex> lock(mQMutex);

        while (mQJobs.empty() && !mbStopnow.load()) {  // Only block if there's no work to do
            mJobAlert.wait(lock);                      // Unlocks mQMutex until there's work
        }

        if (mbStopnow.load()) return false;    // If the threadpool is closing down, we need to exit the infinite loop

        // At this point the queue has work and this thread was the first to see it,
        // so pull the next job off the list
        auto pair = move(const_cast<PriorityTask&>(mQJobs.top())); // const_cast is necessary to maintain heap structure under pqueue, but we must immediately pop the object
        job = move(pair.second);
        mQJobs.pop();
        // Unlocked mQMutex
    }

    assert(job.valid());

    // Execute our job
    ++threadPriority;
    job();
    --threadPriority;
    // Notify that a job has been completed
    mNumJobsComplete.fetch_add(1);
    return true;
}

void Threadpool::waitForInFlight(size_t numInFlight)
{
    assert(numInFlight > 0);

    int timeout = 0;
    size_t curInFlight = mNumJobsSent - mNumJobsComplete;
    while (curInFlight >= numInFlight) {
        if (timeout <= 0) {
            if (!do_work_nonblocking(false)) {
                // There wasn't work, so back off for a bit and then try again
                timeout = TimeoutReset;
            }
        }
        else {
            std::this_thread::yield(); // Spin lock while waiting for another thread to complete
            --timeout;
        }
        curInFlight = mNumJobsSent - mNumJobsComplete;
    }
}


void Threadpool::shutdown()
{
    {
        mbStopnow = true;

        mJobAlert.notify_all();
    }

    {
#if THREADPOOL_LOGGING
        std::ostringstream toLog;
#endif
        for (auto& child : mPool)
        {
            mJobAlert.notify_all(); // Continue to notify all to avoid join()ing a sleeping thread
            if (child.joinable()) {
#if THREADPOOL_LOGGING
                toLog.str(""); // Clear buffer
                toLog.clear(); // Clear errors
                toLog << "Waiting on [" << std::distance(begin(mPool), itr) << "] " << itr->get_id() << "\n";
                Logger::log(toLog, LogLevel::Debug);
#endif

                child.join();
            }
        }
        mPool.clear();
    }
}

void Threadpool::infinite_loop()
{
    while (do_work()) {
        continue;
    }
}

