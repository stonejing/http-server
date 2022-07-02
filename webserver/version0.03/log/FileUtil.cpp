#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

FileUtil::FileUtil(std::string filename, int flushEveryN)
    : fp_(fopen(filename.c_str(), "ae")),
    flushEveryN_(flushEveryN),
    count_(0)
{
    setbuffer(fp_, buffer_, sizeof(buffer_));   
}

FileUtil::~FileUtil()
{
    fclose(fp_);
}

/*
    write log to buffer
    multiple producer single consumer
*/
void FileUtil::WriteToBuffer(const char* logline, const size_t len)
{
    MutexLockGuard lock(mutex_);
    size_t remain = len;
    while(remain > 0)
    {
        size_t write_len = fwrite_unlocked(logline, sizeof(char), remain, fp_);
        if(write_len == 0)
        {
            int err = ferror(fp_);
            if(err)
                fprintf(stderr, "FileUtil Append file failed.\n");
            break;
        }
        remain -= write_len;
    }
    count_++;
    if(count_ >= flushEveryN_)
    {
        // printf("print flush time: %d\n", flushEveryN_);
        count_ = 0;
        fflush(fp_);
    }
}

/*
    write buffer in memory to disk file
*/
void FileUtil::Flush()
{
    MutexLockGuard lock(mutex_);
    fflush(fp_);
}