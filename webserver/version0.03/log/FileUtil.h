#pragma once
#include <string>
#include "MutexLock.h"

class FileUtil
{
public:
    explicit FileUtil(std::string filename, int flushEveryN = 1024);
    ~FileUtil();
    void WriteToBuffer(const char* logline, const size_t len);
    void Flush();

private:
    FILE* fp_;
    char buffer_[64 * 1024];
    int count_;
    MutexLock mutex_;
    int flushEveryN_;
};