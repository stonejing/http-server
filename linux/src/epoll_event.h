#pragma once

#include <sys/epoll.h>  /* epoll */
#include <fcntl.h>      /* fcntl */
#include <unistd.h>     /* close */
#include "utils.h"
#include "timer_wheel.h"
// #include "timer_min_heap.h"

class EpollEvent
{
public:
    EpollEvent()
    {
        m_epollfd = epoll_create(5);
    }

    ~EpollEvent(){};

    void AddFd(int fd, bool one_shot)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        if(one_shot)
        {
            event.events |= EPOLLONESHOT;
        }
        epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event);
        setnonblocking(fd);
    }

    void RemoveFd(int fd)
    {
        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }

    void ModifyFd(int fd, int ev)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
        epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &event);
    }

    int GetEpollfd()
    {
        return m_epollfd;
    }

private:
    int m_epollfd;
};