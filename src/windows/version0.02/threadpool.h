#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <string>
#include <iostream>

#include "eventloop.h"
#include "windows.h"
#include "winsock2.h"

class ThreadPool {
public:
    ThreadPool(int thread_numbers, SOCKET listen_socket, std::string& address, int remote_port);
    ~ThreadPool();

    void StartLoop();
    std::shared_ptr<EventLoop> GetNextThread();

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;

    std::vector<std::shared_ptr<EventLoop>> loops_;

    int thread_numbers_;
    int next_;

    std::string address_;
    int remote_port_;
    SOCKET listen_socket_;
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(int thread_numbers, SOCKET listen_socket, std::string& address, int remote_port)
    :   thread_numbers_(thread_numbers), next_(0), address_(address), remote_port_(remote_port),
        listen_socket_(listen_socket)
{
    for (int i = 0; i < thread_numbers_; i++)
    {
        workers.emplace_back(
            [this]
            {
                this->StartLoop();
            }
           );
        workers[i].detach();
    }
}

inline void ThreadPool::StartLoop()
{
    std::shared_ptr<EventLoop> t = std::make_shared<EventLoop>(listen_socket_, address_, remote_port_);
    loops_.emplace_back(t);
    t->Loop();
}

inline std::shared_ptr<EventLoop> ThreadPool::GetNextThread()
{
    // std::shared_ptr<EventLoop> loop = loops_[next_];

    auto &loop = loops_[next_];
    next_ = (next_ + 1) % thread_numbers_;

    return loop;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    //for(std::thread &worker: workers)
    //    worker.join();
}