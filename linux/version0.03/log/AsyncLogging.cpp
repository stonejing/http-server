#include "AsyncLogging.h"
#include "FileUtil.h"
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <functional>
#include <thread>

// void* StartThread(void* arg)
// {
//     AsyncLogging* alog = static_cast<AsyncLogging*>(arg);
//     printf("start thread func.\n");
//     alog->threadFunc();
//     return nullptr;
// }

AsyncLogging::AsyncLogging(std::string LogFileName, int flushInterval)
    : flushInterval_(flushInterval),
      basename_(LogFileName),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      running_(false)
{
    assert(LogFileName.size() > 0);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
    pthread_create(&threadId_, NULL, start, this);
    printf("pthread create.\n");
    pthread_detach(threadId_);
}

void AsyncLogging::Append(const char* logline, int len)
{
    /*
        两处同样的锁，其实可以省略，将两处合并为一处
        其它的就是更好的 log 策略了
    */
    MutexLock lock(mutex_);
    if(currentBuffer_->avail() > len)
        currentBuffer_->Append(logline, len);
    else
    {
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        if(nextBuffer_)
            currentBuffer_ = nextBuffer_;
        else
            currentBuffer_ = std::move(nextBuffer_);
        currentBuffer_->Append(logline, len);
    }
}

void AsyncLogging::threadFunc()
{

    // assert(running_ == true);
    running_ = true;

    FileUtil logfile(basename_, flushInterval_);

    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        {
            MutexLock lock(mutex_);
            // if(buffers_.empty())
            // {
            //     continue;
            // }
            buffers_.push_back(currentBuffer_);
            currentBuffer_.reset();

            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if(!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }
        assert(!buffersToWrite.empty());
        if(buffersToWrite.size() > 25)
        {
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }
        for(size_t i = 0; i < buffersToWrite.size(); ++i)
        {
            logfile.WriteToBuffer(buffersToWrite[i]->data(), buffersToWrite[i]->length());
        }
        if(buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }
        
        if(!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if(!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        logfile.Flush();
    }
    logfile.Flush();
}