#include "eventloop.h"

EventLoop::EventLoop(SOCKET listen_socket, string &address, int remote_port)
    : address_(address), remote_port_(remote_port), accept_loop_(0),
      listen_socket_(listen_socket), quit_(false) {}

EventLoop::~EventLoop() {}

void EventLoop::Loop() 
{
    // SOCKET dumb = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    while (!quit_) 
    {
        FD_ZERO(&read_set_);
        FD_ZERO(&write_set_);

        FD_SET(listen_socket_, &read_set_);

        std::vector<SOCKET> write_;

        for (auto &kv : sts_) 
        {
            if (!kv.second->get_write_status()) 
            {
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

        int ret = select(0, &read_set_, &write_set_, NULL, NULL);

        if (ret == SOCKET_ERROR) 
        {
            log_err("select() return with error %d", WSAGetLastError());
            abort();
        }

        for (int i = 0; i < write_.size(); i++) 
        {
            if (FD_ISSET(write_[i], &write_set_)) 
            {
                sts_[write_[i]]->ShadowsocksSend(write_[i]);
            }
            if (FD_ISSET(write_[i], &write_set_)) 
            {
                sts_[write_[i]]->ShadowsocksSend(write_[i]);
            }
        }

        for (auto it = sts_.begin(); it != sts_.end();) 
        {
            if (FD_ISSET(it->second->get_accept_socket(), &read_set_)) 
            {
                int ret = it->second->ShadowsocksHandleLocal();
                if (ret == -1) 
                {
                    it = sts_.erase(it);
                    continue;
                }
            }

            if (FD_ISSET(it->second->get_connect_socket(), &read_set_)) 
            {
                int ret = it->second->ShadowsocksHandleRemote();
                if (ret == -1) 
                {
                    it = sts_.erase(it);
                    continue;
                }
            }
            it++;
        }

        HandleListenSocket();
    }
}

void EventLoop::IncreaseAccept() 
{
    std::lock_guard<std::mutex> guard(mutex_);
    accept_loop_++;
}

int EventLoop::HandleListenSocket() {
    int num;

    {
        std::lock_guard<std::mutex> guard(mutex_);
        num = accept_loop_;
        accept_loop_ = 0;
    }

    while (num != 0) 
    {
        if ((accept_socket_ = accept(listen_socket_, NULL, NULL)) !=
            INVALID_SOCKET) 
        {
            connect_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            auto ss = make_unique<Shadowsocks>(accept_socket_, connect_socket_,
                                                address_, remote_port_);
            sts_[accept_socket_] = std::move(ss);
            return 1;
        } 
        else 
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK) 
            {
                log_err("accept() failed with error %d", WSAGetLastError());
                return -1;
            }
        }
        --num;
    }
    return 1;
}
