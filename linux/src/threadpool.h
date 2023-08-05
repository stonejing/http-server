#pragma once
#include <chrono>
#include <future>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>

#include "eventloop.h"
#include "log.h"

using std::vector;
using std::thread;
using std::shared_ptr;
using std::mutex;

class ThreadPool
{
public:
    ThreadPool(int thread_numbers = 4) : thread_numbers(thread_numbers)
    {
        for(int i = 0; i < thread_numbers; i++)
        {
            shared_ptr<EventLoop> loop(make_shared<EventLoop>());
            threads.emplace_back(&EventLoop::loop, loop.get());
            loops.push_back(std::move(loop));
            eventloop_size++;
        }
        status = true;
    }

    ~ThreadPool() 
    {
        for(auto& thread : threads)
            thread.join();
    }

    bool threadPoolStatus()
    {
        if(status) return true;
        while(!status)
        {
            if(eventloop_size == thread_numbers)
            {
                LOG_INFO("event pool created.");
                return true;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        return false;
    }

    // 并不是线程安全，但是调用这个函数的不是多线程
    std::shared_ptr<EventLoop> getNextLoop()
    {
        next = (next + 1) % thread_numbers;
        LOG_INFO("get %dth thread.", next);
        return loops[next];
    }

private:
    CLogger& Log = CLogger::getInstance();
    mutex mut;
    vector<shared_ptr<EventLoop>> loops;
    vector<thread> threads;
    int eventloop_size = 1;
    int next = 0;
    int thread_numbers;
    bool status = false;
};