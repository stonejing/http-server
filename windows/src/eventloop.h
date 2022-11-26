#pragma once

#define FD_SETSIZE 1024
#include "shadowsocks.h"
#include "winsock2.h"
#include "windows.h"
#include "processthreadsapi.h"

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <iostream>
#include <queue>
#include <mutex> 
#include <condition_variable>

#include "log.h"
#include "http.h"
#include "utils.h"

using std::unordered_map;
using std::unique_ptr;
using std::make_unique;
using std::vector;
using std::string;

class EventLoop
{
public:
    EventLoop(SOCKET listen_socket, string& address, int remote_port);
    ~EventLoop();

    void Loop();

    inline void quit()
    {
        quit_ = true;
    }

    void IncreaseAccept(SOCKET accept_socket);

private:

    int HandleListenSocket();
    void HandleAcceptSocket();

    int ReadFirstBuffer(SOCKET sockfd);

    unordered_map<SOCKET, unique_ptr<Shadowsocks>> sts_;
    unordered_map<SOCKET, unique_ptr<HttpConnection>> sth_;
    unordered_map<SOCKET, int> socket_type_; // 1 sts_, 2 sth_, 3 read first buffer

    std::mutex mutex_;

    FD_SET read_set_;
    FD_SET write_set_;

    SOCKET listen_socket_;
    SOCKET accept_socket_;
    SOCKET connect_socket_;

    std::queue<SOCKET> socket_queue_;
    std::mutex socket_queue_mutex_;
    std::condition_variable socket_variable;

    // according to the first buffer type, choose different handler
    std::vector<char> first_buffer;
    int first_buffer_len;

    string address_;
    int    remote_port_;

    bool quit_;

    bool accept_;
};