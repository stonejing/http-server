#include "channel.h"

Channel::Channel(EventLoop* loop, int fd) : 
        fd(fd), 
        loop_(loop)
{
    // LOG_INFO("add channel: ", fd);
}

int Channel::get_event()
{
    return event;
}
void Channel::add_event()
{
    loop_->epollAddFd(fd);
}
void Channel::set_event(int ev)
{
    loop_->epollModFd(fd, ev);
    event = ev;
}

void Channel::handleEvent()
{
    if(event & EPOLLIN)
    {
        read_callback();
    }
    else if(event & EPOLLOUT)
    {
        write_callback();
    }
    else if(event & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        error_callback();
    }
    else 
    {
        LOG_ERR("channel handle event unexpected. %d 神仙难救", fd);
    }
}