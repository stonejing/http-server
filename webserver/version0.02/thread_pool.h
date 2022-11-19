#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <iostream>
#include "event_loop.h"

class ThreadPool {
public:
    ThreadPool(int thread_numbers);
    ~ThreadPool();
    void StartLoop();
    std::shared_ptr<EventLoop> GetNextThread();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    std::vector<std::shared_ptr<EventLoop>> loops_;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    int thread_numbers_;
    int next_;
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(int thread_numbers)
    :   stop(false), thread_numbers_(thread_numbers), next_(0)
{
    for(int i = 0; i < thread_numbers_; i++)
        workers.emplace_back(
            [this]
            {
                this->StartLoop();
            }
        );
}

inline void ThreadPool::StartLoop()
{
    std::shared_ptr<EventLoop> t(new EventLoop());
    loops_.emplace_back(t);
    t->loop();
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    // {
    //     std::unique_lock<std::mutex> lock(queue_mutex);
    //     stop = true;
    // }
    // condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}