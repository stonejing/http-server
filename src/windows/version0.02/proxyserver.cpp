#include "proxyserver.h"

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

    if((listen_socket_ = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        log_err("WSASocket() failed with error %d", WSAGetLastError());
        return -1;
    }

    if(bind(listen_socket_, (PSOCKADDR)&local_server_, sizeof(local_server_)) == SOCKET_ERROR)
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
        auto ss = make_shared<Shadowsocks>(accept_socket_, connect_socket_);
        sts_[accept_socket_] = ss;
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

        for(auto& kv : sts_)
        {
            FD_SET(kv.second->get_accept_socket(), &read_set_);
            FD_SET(kv.second->get_connect_socket(), &read_set_);
        }

        if((select_total_ = select(0, &read_set_, &write_set_, NULL, NULL)) == SOCKET_ERROR)
        {
            log_err("select() return with error %d", WSAGetLastError());
            return -1;
        }

        if(FD_ISSET(listen_socket_, &read_set_))
        {
            if(HandleListenSocket() == -1)
                continue;
        }

        for(auto it = sts_.begin(); it != sts_.end();)
        {
            auto ss = it->second;
            if(FD_ISSET(ss->get_accept_socket(), &read_set_))
            {
                int ret = ss->ShadowsocksHandleLocal();
                if(ret == -1)
                {
                    it = sts_.erase(it);
                    continue;
                }
            }
            if(FD_ISSET(ss->get_connect_socket(), &read_set_))
            {
                int ret = ss->ShadowsocksHandleRemote();
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