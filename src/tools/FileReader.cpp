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
#include "FileReader.h"

#include <cassert>

FileReader::FileReader(const fs::path& filePath) :
    m_fileBuf(),
    m_stringBuf(),
    m_fs(nullptr)
{
    if (!filePath.empty() && (filePath.string().size() <= _MAX_PATH))
    {
        // Try to open file
        m_fileBuf.open(filePath, std::fstream::in | std::fstream::binary);

        if (m_fileBuf.is_open()) {
            m_fs.rdbuf(&m_fileBuf);

            // Find file size
            m_fs.seekg(0, std::fstream::end);
            const size_t fileSize = m_fs.tellg();
            m_fs.seekg(0, std::fstream::beg);

            assert(fileSize < SIZE_MAX);

            // If the file is small enough, go ahead and read it
            // all into memory to start and switch stream buffers
            if (fileSize > 0 && fileSize < LARGE_FILE_SIZE) {
                m_buf.resize(fileSize);
                m_fs.read(m_buf.data(), fileSize);

//                m_stringBuf.pubsetbuf(m_buf.data(), m_buf.size());
                m_stringBuf.str(m_buf);

                m_fs.rdbuf(&m_stringBuf);
                m_fs.seekg(0, std::fstream::beg);
                m_fileBuf.close();
            }

            assert(m_fs);
        }
    }
}

FileReader::~FileReader() {
    close();
}

FileReader& FileReader::getc(char& c) {
    m_fs.get(c);
    return *this;
}

FileReader& FileReader::getline(std::string& line) {
    std::getline(m_fs, line);
    return *this;
}

FileReader& FileReader::ignore(const size_t n, const int delim) {
    m_fs.ignore(n, delim);
    return *this;
}

bool FileReader::eof() const {
    return m_fs.eof();
}

bool FileReader::readable() const {
    return m_fs && !m_fs.eof();
}

void FileReader::close() {
    if (m_fileBuf.is_open()) { m_fileBuf.close(); }
    m_fs.rdbuf(nullptr);
    m_buf.clear();
}
