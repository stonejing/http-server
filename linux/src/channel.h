#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

#include "log.h"
#include "eventloop.h"
#include "utils.h"

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
        read_callback = cb; 
    }
    void set_write_callback(EventCallback&& cb)
    {
        write_callback = cb;
    }
    void set_error_callback(EventCallback&& cb)
    {
        error_callback = cb;
    }

    int get_event();
    void add_event();
    void set_event(int ev);

private:
    CLogger& Log = CLogger::getInstance();
    EventLoop* loop_;
    int fd;
    int epollfd;
    int event;
    // for poll ,there is revent, but for epoll, it is no use.
    EventCallback read_callback;
    EventCallback write_callback;
    EventCallback error_callback;    
};