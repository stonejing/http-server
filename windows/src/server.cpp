#include <string>
#include <memory>
#include <thread>

#include "eventloop.h"
#include "server.h"

Server::Server(string& address, int remote_port,
                        string& password, int method,
                        int local_port) : 
    address_(address), remote_port_(remote_port),
    password_(password), method_(method),
    local_port_(local_port), select_total_(0),
    reset_(0)
{
    local_server_.sin_family = AF_INET;
    local_server_.sin_port = htons(local_port_);
    local_server_.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
}

int Server::EventListen()
{
    WSADATA wsaData;
    ULONG NonBlock = 1;

    if(WSAStartup(0x0202, &wsaData) != 0)
    {
        LOG_ERROR << "WSAStartup() failed with error" << WSAGetLastError() << "\n";
        WSACleanup();
        return -1;
    }

    if((listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        LOG_ERROR << "WSASocket() failed with error" <<  WSAGetLastError() << "\n";
        return -1;
    }

    if(bind(listen_socket_, (PSOCKADDR)&local_server_, sizeof(local_server_)) 
        == SOCKET_ERROR)
    {
        LOG_ERROR << "WSASocket() failed with error" <<  WSAGetLastError() << "\n";
        return -1;
    }

    if(listen(listen_socket_, 5))
    {
        LOG_ERROR << "listen() failed with error" <<  WSAGetLastError() << "\n";
        return -1;
    }

    if(ioctlsocket(listen_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        LOG_ERROR << "can not set socket to nonblock" << WSAGetLastError() << "\n";
        return -1;
    }

    // thread_pool_ = make_unique<ThreadPool>(std::thread::hardware_concurrency() - 4,
    //                                         listen_socket_, address_, remote_port_);
    thread_pool_ = make_unique<ThreadPool>(4, listen_socket_, address_, remote_port_);

    LOG_INFO << "server started in 0.0.0.0:" << local_port_ << "\n";
    return 1;
}

// main thread listen on the port for connections
int Server::ServerStart()
{
    while(true)
    {
        FD_ZERO(&read_set_);
        FD_SET(listen_socket_, &read_set_);

        // select 是阻塞的，listen_socket 不是阻塞的，可能也没有必要
        if((select_total_ = select(0, &read_set_, NULL, NULL, NULL)) 
                == SOCKET_ERROR)
        {
            LOG_ERROR << "select() return with error" << WSAGetLastError() << "\n";
            return -1;
        }

        if(reset_ == 1)
        {
            unique_ptr<ThreadPool> temp = make_unique<ThreadPool>(std::thread::hardware_concurrency() - 4,
                                            listen_socket_, address_, remote_port_);
            thread_pool_->StopLoop();
            thread_pool_.reset(temp.release());
            LOG_INFO << "reset thread pool for new configurations.\n";
            reset_ = 0;
            continue;
        }

        if(FD_ISSET(listen_socket_, &read_set_))
        {
            auto eventloop = thread_pool_->GetNextThread();
            
            if(HandleListenSocket() == 1)
                eventloop->IncreaseAccept(accept_socket_);
        }
    }
}

int Server::HandleListenSocket()
{
    if ((accept_socket_ = accept(listen_socket_, NULL, NULL)) !=
        INVALID_SOCKET) 
    {
        // LOG_INFO << "accept_socket: " << accept_socket_ << "\n";
        ULONG NonBlock = 1;
        if(ioctlsocket(accept_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
            LOG_ERROR << "can not set socket to nonblock" << WSAGetLastError() << "\n";
            return -1;
        }
    } 
    else 
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK) 
        {
            LOG_ERROR << "accept() failed with error" << WSAGetLastError() <<"\n";
            return -1;
        }
    }
    return 1;
}