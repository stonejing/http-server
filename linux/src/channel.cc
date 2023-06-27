#include "channel.h"
#include "event_loop.h"

Channel::Channel(EventLoop* loop, int fd):
    loop_(loop),
    fd_(fd),
    events_(0),
    current_events_(0)
{ }

void Channel::HandleEvent()
{
    if ((current_events_ & EPOLLHUP) && !(current_events_ & EPOLLIN))
    {
        log_err("EPOLL UP error");
        return;
    }
    else if (current_events_ & EPOLLERR)
    {
        if (error_callback_) error_callback_();
        log_err("epoll err event");
        return;
    }
    else if (current_events_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
    {
        read_callback_();
    }
    else if (current_events_ & EPOLLOUT)
    {
        write_callback_();
    }
    else
    {
        log_err("epoll err");
        return;
    }
}