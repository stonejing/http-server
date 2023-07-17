#pragma once

#include <chrono>
#include <cstdint>
#include <thread>
#include <iostream>
#include <random>
#include <mutex>
#include <vector>
#include <queue>

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "log.h"

#define MAX_EVENT_NUMBER 5000

class EventLoop
{
public:
    EventLoop() : epollfd(epoll_create1(EPOLL_CLOEXEC)), evfd(eventfd(0, 0))
    {
        LOG_INFO("event loop created epollfd: %d eventfd: %d", epollfd, evfd);
    }

    ~EventLoop()
    {
        LOG_INFO("event loop released close epollfd: %d", epollfd);
        close(epollfd);
        close(evfd);
    }

    // 加锁性能什么的不重要，将所有的功能先正确的实现非常重要
    void setNewSocketFd(int fd)
    {
        std::lock_guard<std::mutex> socket_queue_lock(mut);
        socket_queue.push(fd);
        write(evfd, "1", 1);
    }

    // 将 socket fd 加入到 epoll 中
    void handleSocketQueue()
    {
        LOG_INFO("handle socket queue");
        std::lock_guard<std::mutex> lock(mut);
        while(!socket_queue.empty())
        {
            epollAddFd(socket_queue.front());
            socket_queue.pop();
        } 
    }

    void epollAddFd(int fd)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    }

    void epollModFd(int fd, int ev)
    {
        epoll_event event;
        event.data.fd = fd;
        event.events = ev | EPOLLET | EPOLLRDHUP;
        epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
    }

    void epollDelFd(int fd)
    {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
        close(fd);
    }

    void handleRead(int sockfd)
    {

    }

    void handleWrite(int sockfd)
    {

    }

    void loop(); 

private:
    CLogger& Log = CLogger::getInstance();

    std::mutex mut;
    queue<int> socket_queue;

    int epollfd;
    epoll_event events[MAX_EVENT_NUMBER];
    int evfd;       // eventfd
};