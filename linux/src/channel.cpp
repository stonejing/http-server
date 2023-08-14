#include "channel.h"

Channel::Channel(int epollfd, int fd) : 
        fd(fd), 
        epollfd(epollfd), 
        event(0)
{
    // LOG_INFO("add channel: ", fd);
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