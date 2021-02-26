
/*************************************************************************
*
* VULCANFORMS CONFIDENTIAL
* __________________
*  Copyright, VulcanForms Inc.
*  [2016] - [2021] VulcanForms Incorporated
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


#include <condition_variable>
#include <future>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

class Threadpool
{
public:
    // [](size_t jobsCntLeft, size_t cycleCntJobs, int64_t cycleTimeMs)->void {}
    using LoggingFunc  = std::function<void(size_t, size_t, int64_t)>;
    using Task         = std::function<void()>;
    using TaskList     = std::vector<Task>;
    using Waitable     = std::future<void>;
    using WaitableList = std::vector<Waitable>;


    static Waitable submit(Task job, bool threaded = true);
    static WaitableList submit(TaskList job_list, bool threaded = true); // Doesn't lock mutex more than once, but perhaps has some thread sleeping issues

    // How many are left to go, How many finished since last update, How much time passed since last update
    // Recommended signature:
    static void join(LoggingFunc loggingFunc = LoggingFunc());                                             // Wait for all current tasks. WARNING: This does not propogate exceptions to the calling thread, exceptions are dropped
    static void join(Waitable&&     waitable,  LoggingFunc loggingFunc = LoggingFunc());               // Wait for a single task
    static void join(WaitableList&& waitables, LoggingFunc loggingFunc = LoggingFunc()); // Wait for many tasks

    // Use submitAndJoin when you need to limit the number of tasks in flight for example, file IO that needs to restrict number of active file handles
    static void submitAndJoin(Task job,          bool threaded = true, LoggingFunc loggingFunc = LoggingFunc(), size_t inFlightLimit = SIZE_MAX); // Might block on call for a long time so logging is used as if joining previous results
    static void submitAndJoin(TaskList job_list, bool threaded = true, LoggingFunc loggingFunc = LoggingFunc(), size_t inFlightLimit = SIZE_MAX); // Might block on call for a long time so logging is used as if joining previous results

    Threadpool(const Threadpool&)     = delete;
    void operator=(const Threadpool&) = delete;

private:
    static const int TimeoutReset = 512;

    Threadpool(unsigned int nThreads = 64);
    ~Threadpool();

    // If we want to be returning as soon as possible, eg for logging,
    // we don't want to allow long tasks to run on this thread, so we
    // avoid doing priority 0 tasks.
    bool do_work_nonblocking(bool allowLongWork = true); // Returns if it was able to do work or not
    bool do_work();

    void waitForInFlight(size_t numInFlight);

    static bool is_ready(const std::future<void>& waitable);
    static bool has_jobs();
    static size_t num_jobs();

    static Threadpool& instance();

    void infinite_loop();
    void shutdown();

    class compare_priority {
    public:
        bool operator()(const std::pair<int, std::packaged_task<void()>>& a, const std::pair<int, std::packaged_task<void()>>& b)
        {
            return a.first < b.first;
        }
    };
    using PriorityTask = std::pair<int, std::packaged_task<void()>>;

    std::atomic<bool>                      mbStopnow;
    std::condition_variable                mJobAlert;
    std::mutex                             mQMutex;
    std::priority_queue<PriorityTask, std::vector<PriorityTask>, compare_priority> mQJobs;
    std::atomic<size_t>                    mNumJobsSent;
    std::atomic<size_t>                    mNumJobsComplete;
    std::vector<std::thread>               mPool;
};
