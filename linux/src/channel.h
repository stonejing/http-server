#pragma once

#include <functional>
#include <sys/epoll.h>
#include "dbg.h"
#include <memory>
#include <iostream>

class EventLoop;

class Channel
{
/* 
    cb: callback 
*/
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel()
    {
        log_info("remove channel, fd: %d", fd_);
    }

    void HandleEvent();
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

    int get_fd() const
    {
        return fd_;
    }

    __uint32_t& get_events()
    {
        return events_;
    }

    void set_current_events(__uint32_t ev)
    {
        current_events_ = ev;
    }
    void set_events(__uint32_t ev)
    {
        events_ = ev;
    }

private:
    void update(); /* update what */

    EventLoop* loop_;
    int fd_;
    __uint32_t events_;
    __uint32_t current_events_;   /* what is this */     

    EventCallback read_callback_;
    EventCallback write_callback_;
    EventCallback error_callback_;
};