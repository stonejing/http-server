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

    }

    ~EventLoop()
    {
        LOG_INFO("event loop released close epollfd:", epollfd);
        close(epollfd);
        close(evfd);
    }

    void setNewSocketFd(int fd)
    {
        LOG_INFO("set new socket fd: ", fd);
        std::lock_guard<std::mutex> socket_queue_lock(mut);
        socket_queue.push(fd);
        eventfd_write(evfd, 1);
    }

    void handleSocketQueue()
    {
        std::lock_guard<std::mutex> lock(mut);
        while(!socket_queue.empty())
        {
            int fd = socket_queue.front();
            // 为什么这里不能用make_unique，会导致 accept faulure: Bad file descriptor
            // HTTP 类创建了就不删除了，只是将其初始化
            if(socket_http_map.count(fd))
            {
                socket_http_map[fd]->init();
            }
            else 
            {
                socket_http_map[fd] = std::make_unique<Http>(epollfd, fd);
            }
            epollAddFd(epollfd, fd);
            socket_queue.pop();
        }
    }

    void loop(); 

private:
    CLogger& Log = CLogger::getInstance();

    std::mutex mut;
    queue<int> socket_queue;
    std::map<int, std::unique_ptr<Http>> socket_http_map;

    int epollfd;
    epoll_event events[MAX_EVENT_NUMBER];
    int evfd;       // eventfd
};