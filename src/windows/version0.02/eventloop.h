#pragma once

#define FD_SETSIZE 1024
#include "shadowsocks.h"
#include "winsock2.h"
#include "windows.h"

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <atomic>

#include <mutex> 

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

    void quit();

    void restart();

    void IncreaseAccept();

    void IncreaseAtomicNum()
    {
        atomic_num_++;
    }

private:

    int HandleListenSocket();
    void HandleAcceptSocket();

    unordered_map<SOCKET, unique_ptr<Shadowsocks>> sts_;

    std::mutex mutex_;

    FD_SET read_set_;
    FD_SET write_set_;

    SOCKET listen_socket_;
    SOCKET accept_socket_;
    SOCKET connect_socket_;

    int accept_loop_;
    std::atomic<int> atomic_num_;

    string address_;
    int    remote_port_;
};