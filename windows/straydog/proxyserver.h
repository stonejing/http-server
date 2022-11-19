#ifndef PROXYSERVER_H_
#define PROXYSERVER_H_

#define FD_SETSIZE 1024

#include "winsock2.h"
#include "windows.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "shadowsocks.h"
#include "dbg.h"
#include "threadpool.h"

using std::string;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using std::unordered_map;

class ProxyServer
{
public:
    ProxyServer(string& address, int remote_port, 
                string& password, int method,
                int local_port);

    ~ProxyServer()
    {
        closesocket(listen_socket_);
    }

    int EventListen();
    int ServerStart();

    void ResetThreadpool();

private:
    int HandleListenSocket();

private:
    SOCKET          listen_socket_;
    SOCKADDR_IN     local_server_;

    string          address_;
    int             remote_port_;
    string          password_;
    int             method_;

    FD_SET          read_set_;

    int             local_port_;
    int             select_total_;

    unique_ptr<ThreadPool> thread_pool_;

    int reset_;
};

#endif