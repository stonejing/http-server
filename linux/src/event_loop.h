#pragma once

#include <thread>
#include <vector>
#include <string>
#include "epoller.h"
#include <iostream>

class EventLoop
{
public:
    EventLoop();
    ~EventLoop(){}

    void loop();
    void AddtoPoller(std::shared_ptr<Channel> channel);
    void UpdatePoller(std::shared_ptr<Channel> channel);
    void RemovePoller(std::shared_ptr<Channel> channel);
    
    bool IsInLoopThread() const
    {
        return thread_id_ == std::this_thread::get_id();
    }

private:
    std::shared_ptr<Epoller> poller_;
    std::thread::id thread_id_;
};