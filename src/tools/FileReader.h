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

#include "VfCommon.h"

#include <fstream>
#include <sstream>
#include <iostream>

class FileReader {
public:
    virtual bool readAndParse() = 0;

    explicit FileReader(const std::filesystem::path& filePath);

    virtual ~FileReader();

    explicit operator bool() const { return bool(m_fs); }
    bool operator!() const { return !m_fs; }

protected:
    static const size_t LARGE_FILE_SIZE = 1073741824; // 1GB
    using ReadMode = enum { ReadModeAscii,
                            ReadModeBinary};

    // Status Checks
    bool eof() const;
    bool readable() const;
    void close();

    // Low-level get and movements
    FileReader& getc(char& c);
    FileReader& getline(std::string& line);
    FileReader& ignore(const size_t n, const int delim = -1);
    char        peek() { return m_fs.peek(); }

    // Generic Parse Data Formats
    template <ReadMode M, typename T>
    FileReader& parse(T& data) {
        if constexpr (M == ReadModeAscii) {
            m_fs >> data;
            m_fs.ignore(1, ',');
        } else {
            m_fs.read(reinterpret_cast<char*>(&data), sizeof(data));
        }

        return *this;
    }
    // Need special case for string because comma separated values count as one value using the >> operator
    template<>
    FileReader& parse<ReadModeAscii, std::string>(std::string& data) {
        char c;
        bool delim = false;
        while(!delim && m_fs.get(c)) {
            switch (c) {
            case '\n':
            case '\r':
            case ',':
            case std::streambuf::traits_type::eof():
                delim = true;
                break;
            default:
                data += c;
            }
        }
        return *this;
    }

    template <typename T>
    FileReader& parse_runtime(T& data, const ReadMode m) {
        if (m == ReadModeAscii) {
            return parse<ReadModeAscii>(data);
        } else {
            return parse<ReadModeBinary>(data);
        }

        return *this;
    }
private:
    std::iostream     m_fs;

    // Storage
    std::filebuf      m_fileBuf;
    std::stringbuf    m_stringBuf;
//    std::vector<char> m_buf;
    std::string       m_buf;
};
