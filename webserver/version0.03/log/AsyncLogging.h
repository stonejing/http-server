#pragma once
#include <functional>
#include <pthread.h>
#include <string>
#include <vector>
#include "LogStream.h"
#include "MutexLock.h"
#include <memory>
#include <thread>

void* StartThread(void* arg);

class AsyncLogging
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 10);
    ~AsyncLogging()
    {
        // pthread_join(&threadId_, NULL);
    }

    void Append(const char* logline, int len);

    static void* start(void* arg)
    {
        AsyncLogging* alog = static_cast<AsyncLogging*>(arg);
        alog->threadFunc();
        return alog;
    }

    void stop()
    {
        running_ = false;
        // pthread_join(threadId_, NULL);   
    }
private:
    bool running_;
    void threadFunc();
    typedef FixedBuffer<kLargeBuffer> Buffer;
    typedef std::shared_ptr<Buffer> BufferPtr;
    typedef std::vector<BufferPtr> BufferVector;

    std::string basename_;
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
    const int flushInterval_;
    MutexLock mutex_;

    pthread_t threadId_;
};