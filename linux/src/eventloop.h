#pragma once

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <sys/socket.h>
#include <thread>
#include <iostream>
#include <random>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <map>

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "log.h"
#include "http.h"
#include "utils.h"

#define MAX_EVENT_NUMBER 5000

class EventLoop
{
public:
    EventLoop() : epollfd(epoll_create1(EPOLL_CLOEXEC)), evfd(eventfd(0, EFD_NONBLOCK))
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
        LOG_INFO("set new socket fd: %d", fd);
        // create HTTP object at here
        std::lock_guard<std::mutex> socket_queue_lock(mut);
        socket_queue.push(fd);
        socket_http_map[fd] = make_shared<Http>(epollfd, fd);
        eventfd_write(evfd, 1);
    }

    // 将 socket fd 加入到 epoll 中
    void handleSocketQueue()
    {
        LOG_INFO("handle socket queue");
        std::lock_guard<std::mutex> lock(mut);
        while(!socket_queue.empty())
        {
            epollAddFd(epollfd, socket_queue.front());
            socket_queue.pop();
        } 
    }

    void loop(); 

private:
    CLogger& Log = CLogger::getInstance();

    std::mutex mut;
    queue<int> socket_queue;
    std::map<int, std::shared_ptr<Http>> socket_http_map;

    int epollfd;
    epoll_event events[MAX_EVENT_NUMBER];
    int evfd;       // eventfd
};