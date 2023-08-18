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
#include <ares.h>

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
    EventLoop();

    ~EventLoop()
    {
        LOG_INFO("event loop released close epollfd:", epollfd_);
        close(epollfd_);
        close(evfd_);
    }

    static int ares_socket_create_callback(int sock, int type, void *data);

    static void dns_callback(void* arg, int status, int timeouts, struct hostent* host)
    {
        if (status == ARES_SUCCESS)
        {
            puts(host->h_name);
            puts(inet_ntoa(*(struct in_addr*)host->h_addr));
        }
        else
            std::cout << "lookup failed: " << ares_strerror(status) << std::endl;
    }

    void setDnsChannel(int sock);
    void handleDnsRead(int sock);

    void setNewChannel(int fd, std::shared_ptr<Channel> channel);

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
    // DNS query
    ares_channel ares_channel_;
};