#include "proxyserver.h"
#include <iostream>

using std::cerr;

ProxyServer::ProxyServer(string& address, int remote_port,
                        string& password, int method,
                        int local_port) : 
    address_(address), remote_port_(remote_port),
    password_(password), method_(method),
    local_port_(local_port), select_total_(0),
    thread_pool_(make_unique<ThreadPool>(std::thread::hardware_concurrency() - 1))
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
    log_info("server started in 0.0.0.0:%d", local_port_);
    return 1;
}

int ProxyServer::HandleListenSocket()
{
    if((accept_socket_ = accept(listen_socket_, NULL, NULL)) != INVALID_SOCKET)
    {
        connect_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        auto ss = make_unique<Shadowsocks>(accept_socket_, connect_socket_, 
                                            address_, remote_port_);
        sts_[accept_socket_] = std::move(ss);
        return 1;
    }
    else
    {
        if(WSAGetLastError() != WSAEWOULDBLOCK)
        {
            log_err("accept() failed with error %d", WSAGetLastError());
            return -1;
        }
    }
    return 1;
}

int ProxyServer::ServerStart()
{
    while(true)
    {
        FD_ZERO(&read_set_);
        FD_ZERO(&write_set_);
        FD_SET(listen_socket_, &read_set_);

        std::vector<SOCKET> write_;

        // windows select 时间复杂度要比 linux 的高，但是更为简便了

        for(auto& kv : sts_)
        {
            if(!kv.second->get_write_status())
            {
                log_info("write set");
                write_.push_back(kv.second->get_accept_socket());
                write_.push_back(kv.second->get_connect_socket());
                FD_SET(kv.second->get_accept_socket(), &write_set_);
                FD_SET(kv.second->get_connect_socket(), &write_set_);
            }
            else
            {
                FD_SET(kv.second->get_accept_socket(), &read_set_);
                FD_SET(kv.second->get_connect_socket(), &read_set_);
            }
        }

        // select 是阻塞的
        if((select_total_ = select(0, &read_set_, &write_set_, NULL, NULL)) 
                == SOCKET_ERROR)
        {
            log_err("select() return with error %d", WSAGetLastError());
            return -1;
        }

        if(FD_ISSET(listen_socket_, &read_set_))
        {
            if(HandleListenSocket() == -1)
                continue;
        }

        for(int i = 0; i < write_.size(); i++)
        {
            if(FD_ISSET(write_[i], &write_set_))
            {
                sts_[write_[i]]->ShadowsocksSend(write_[i]);
            }
            if(FD_ISSET(write_[i], &write_set_))
            {
                sts_[write_[i]]->ShadowsocksSend(write_[i]);
            }
        }

        for(auto it = sts_.begin(); it != sts_.end();)
        {
            if(FD_ISSET(it->second->get_accept_socket(), &read_set_))
            {
                int ret = it->second->ShadowsocksHandleLocal();
                if(ret == -1)
                {
                    it = sts_.erase(it);
                    continue;
                }
            }

            if(FD_ISSET(it->second->get_connect_socket(), &read_set_))
            {
                int ret = it->second->ShadowsocksHandleRemote();
                if(ret == -1)
                {
                    it = sts_.erase(it);
                    continue;
                }
            }
            it++;
        }
    }
}