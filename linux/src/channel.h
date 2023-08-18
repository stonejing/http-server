#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "log.h"

class EventLoop;
class Http;

class Channel
{
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel()
    {
        // LOG_INFO("remove channel: ", fd);
    }

    void handleEvent();
    void set_read_callback(EventCallback&& cb)
    {
        read_callback_ = cb; 
    }
    void set_write_callback(EventCallback&& cb)
    {
        write_callback_ = cb;
    }
    void set_error_callback(EventCallback&& cb)
    {
        error_callback_ = cb;
    }

    void set_read();
    void set_write();

private:
    CLogger& Log = CLogger::getInstance();
    EventLoop* loop_;
    int fd_;
    int event_;
    // for poll ,there is revent, but for epoll, it is no use.
    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback error_callback_;    
};