#include "proxyserver.h"
#include <iostream>
#include "eventloop.h"
#include <string>
#include <memory>
#include <thread>


using std::cerr;

// void thread_test()
// {
//     std::string address_ = "1";
//     int remote_port_ = 1;
//     std::shared_ptr<EventLoop> t = std::make_shared<EventLoop>(address_, remote_port_);
//     t->Loop();
// }

ProxyServer::ProxyServer(string& address, int remote_port,
                        string& password, int method,
                        int local_port) : 
    address_(address), remote_port_(remote_port),
    password_(password), method_(method),
    local_port_(local_port), select_total_(0)
{
    local_server_.sin_family = AF_INET;
    local_server_.sin_port = htons(local_port_);
    local_server_.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
}

int ProxyServer::EventListen()
{
    WSADATA wsaData;
    ULONG NonBlock = 1;

    if(WSAStartup(0x0202, &wsaData) != 0)
    {
        log_err("WSAStartup() failed with error %d", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    if((listen_socket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 
                            0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        log_err("WSASocket() failed with error %d", WSAGetLastError());
        return -1;
    }

    if(bind(listen_socket_, (PSOCKADDR)&local_server_, sizeof(local_server_)) 
        == SOCKET_ERROR)
    {
        log_err("WSASocket() failed with error %d", WSAGetLastError());
        return -1;
    }

    if(listen(listen_socket_, 5))
    {
        log_err("listen() failed with error %d", WSAGetLastError());
        return -1;
    }

    if(ioctlsocket(listen_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        log_err("can not set socket to nonblock %d", WSAGetLastError());
        return -1;
    }

    thread_pool_ = make_unique<ThreadPool>(std::thread::hardware_concurrency() - 2, 
                                            listen_socket_, address_, remote_port_);

    log_info("server started in 0.0.0.0:%d", local_port_);
    return 1;
}

int ProxyServer::ServerStart()
{
    while(true)
    {
        FD_ZERO(&read_set_);
        FD_SET(listen_socket_, &read_set_);

        // select 是阻塞的
        if((select_total_ = select(0, &read_set_, NULL, NULL, NULL)) 
                == SOCKET_ERROR)
        {
            log_err("select() return with error %d", WSAGetLastError());
            return -1;
        }

        if(FD_ISSET(listen_socket_, &read_set_))
        {
            thread_pool_->GetNextThread()->IncreaseAccept();
        }
    }
}