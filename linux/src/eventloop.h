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
#include <sys/eventfd.h>

#include "http.h"
#include "channel.h"
#include "log.h"
#include "utils.h"

#define MAX_EVENT_NUMBER 5000

class Channel;
class Http;

class EventLoop
{
public:
    EventLoop() : 
        epollfd_(epoll_create1(EPOLL_CLOEXEC)), 
        evfd_(eventfd(0, EFD_NONBLOCK)),
        socket_channel_map_()
    {

    }

    ~EventLoop()
    {
        LOG_INFO("event loop released close epollfd:", epollfd_);
        close(epollfd_);
        close(evfd_);
    }
    void setNewChannel(int fd, std::shared_ptr<Channel> channel);

    void test()
    {
        cout << "loop test" << endl;
    }

    void delChannel(int fd)
    {
        socket_channel_map_.erase(fd);
        epollDelFd(fd);
    }

    void setNewSocket(int fd);
    void handleSocketQueue();
    void loop(); 
    void epollAddFd(int fd);
    void epollModFd(int fd, int ev);
    void epollDelFd(int fd);

private:
    CLogger& Log = CLogger::getInstance();
    EventLoop* loop_;
    std::mutex mut_;
    queue<int> socket_queue_;
    std::map<int, std::unique_ptr<Http>> socket_http_map_;
    std::map<int, std::shared_ptr<Channel>> socket_channel_map_;

    epoll_event events_[MAX_EVENT_NUMBER];
    int epollfd_;
    int evfd_;
};