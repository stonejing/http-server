#pragma once

#include "dbg.h"
#include "utils.h"
#include "thread_pool.h"
#include "event_loop.h"
#include <unistd.h>
#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include "http_connection.h"
#include <map>
#include <sys/epoll.h>

const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 2;
const int TCP_BUFFER_SIZE = 512;
const int UDP_BUFFER_SZIE = 1024;

class WebServer
{
private:
    std::string root_path_;
    int listen_fd_;
    std::unique_ptr<ThreadPool> thread_pool_;
    EventLoop* loop_;
    std::shared_ptr<Channel> accept_channel_;
    std::map<int, std::shared_ptr<HttpConnection>> requests_;

public:
    WebServer(EventLoop* loop);
    ~WebServer() {}   

    void EventListen();
    void Start();

    void HandleNewConnection();
};