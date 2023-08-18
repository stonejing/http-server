#include "channel.h"

Channel::Channel(EventLoop* loop, int fd) : 
        fd_(fd), 
        loop_(loop),
        event_(EPOLLIN)
{
    // LOG_INFO("add channel: ", fd);
}

void Channel::set_read()
{
    loop_->epollModFd(fd_, EPOLLIN);
    event_ = EPOLLIN;
}

void Channel::set_write()
{
    loop_->epollModFd(fd_, EPOLLOUT);
    event_ = EPOLLOUT;
}

void Channel::handleEvent()
{
    if(event_ & EPOLLIN)
    {
        read_callback_();
    }
    else if(event_ & EPOLLOUT)
    {
        write_callback_();
    }
    else if(event_ & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        error_callback_();
    }
    else 
    {
        LOG_ERR("channel handle event unexpected. %d 神仙难救", fd_);
    }
}