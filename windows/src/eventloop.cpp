#include "eventloop.h"
#include <WinSock2.h>
#include <memory>
#include <mutex>
#include <openssl/crypto.h>
#include <thread>
#include <winsock2.h>

EventLoop::EventLoop(SOCKET listen_socket, string &address, int remote_port)
    : address_(address), remote_port_(remote_port),
      listen_socket_(listen_socket), quit_(false), first_buffer_len(0),
      first_buffer(std::vector<char>(1024))
{}

EventLoop::~EventLoop() {}

void EventLoop::IncreaseAccept(SOCKET accept_socket)
{
    std::lock_guard<std::mutex> lk(socket_queue_mutex_);
    if(socket_type_.empty())
    {
        socket_queue_.push(accept_socket);
        socket_variable.notify_one();
    }
    else 
    {
        socket_queue_.push(accept_socket);
    }
}

void EventLoop::Loop() 
{
    while (!quit_) 
    {
        if(socket_type_.empty())
        {
            // if socket_type is empty, socket_queue is also empty
            // just use one socket into select
            std::unique_lock<std::mutex> lk(socket_queue_mutex_);
            socket_variable.wait(lk, [this]{return !socket_queue_.empty();});
            socket_type_[socket_queue_.front()] = 3;
            socket_queue_.pop();
            lk.unlock();
        }
        else 
        {
            FD_ZERO(&read_set_);
            FD_ZERO(&write_set_);

            for(auto& i : socket_type_)
            {
                FD_SET(i.first, &read_set_);
            }

            int ret = select(0, &read_set_, &write_set_, NULL, NULL);

            if (ret == SOCKET_ERROR)
            {
                LOG_ERROR << "select() return with error" << WSAGetLastError() <<"\n";
                abort();
            }
            for(auto it = socket_type_.begin(); it != socket_type_.end();)
            {
                if(FD_ISSET(it->first, &read_set_))
                {
                    switch(it->second)
                    {
                        case 1:
                        {
                            // shadowsocks handle event
                        }
                        case 2:
                        {
                            if(sth_[it->first]->Read())
                                sth_[it->first]->Write();
                            else
                            {
                                sth_.erase(it->first);
                                it = socket_type_.erase(it);
                                continue;
                            }
                            break;
                        }
                        // dispatch the socket to different class according to first buffer type
                        // local input only have two types, socks5 and http
                        case 3:
                        {
                            if((first_buffer_len = recvn(it->first, first_buffer)) == -1)
                            {
                                closesocket(it->first);
                                it = socket_type_.erase(it);
                                continue;
                            }
                            if(first_buffer_len > 10)
                            {
                                sth_[it->first] = make_unique<HttpConnection>(it->first, first_buffer, first_buffer_len);
                                sth_[it->first]->Write();
                                socket_type_[it->first] = 2;
                            }
                            else 
                            {
                                // sts_[it->first] = make_unique<Shadowsocks>(it->first);
                                // sts_[it->first]->ShadowsocksHandleLocal();
                                // shadowsocks handle
                            }
                            break;
                        }
                        default:
                            LOG_ERROR << "unknow case occured.\n"; 
                            abort();
                    }
                }
                it++;
            }

            {
                // add accept socket in socket to socket_type
                std::lock_guard<std::mutex> lk(socket_queue_mutex_);
                while(!socket_queue_.empty())
                {
                    socket_type_[socket_queue_.front()] = 3;
                    socket_queue_.pop();
                }
            }
        }

        // for (auto &kv : sts_) 
        // {
        //     if (!kv.second->get_write_status()) 
        //     {
        //         write_.push_back(kv.second->get_accept_socket());
        //         write_.push_back(kv.second->get_connect_socket());
        //         FD_SET(kv.second->get_accept_socket(), &write_set_);
        //         FD_SET(kv.second->get_connect_socket(), &write_set_);
        //     } 
        //     else 
        //     {
        //         FD_SET(kv.second->get_accept_socket(), &read_set_);
        //         FD_SET(kv.second->get_connect_socket(), &read_set_);
        //     }
        // }    
        // for(auto &kv : socket_type_)
        // {
        //     LOG_INFO << "socket: " << kv.first << "\n";
        //     FD_SET(kv.first, &read_set_);
        // }     


        // for (int i = 0; i < write_.size(); i++) 
        // {
        //     if (FD_ISSET(write_[i], &write_set_)) 
        //     {
        //         sts_[write_[i]]->ShadowsocksSend(write_[i]);
        //     }
        //     if (FD_ISSET(write_[i], &write_set_)) 
        //     {
        //         sts_[write_[i]]->ShadowsocksSend(write_[i]);
        //     }
        // }

        // for (auto it = sts_.begin(); it != sts_.end();) 
        // {
        //     if (FD_ISSET(it->second->get_accept_socket(), &read_set_)) 
        //     {
        //         int ret = it->second->ShadowsocksHandleLocal();
        //         if (ret == -1) 
        //         {
        //             it = sts_.erase(it);
        //             continue;
        //         }
        //     }
        //     if (FD_ISSET(it->second->get_connect_socket(), &read_set_)) 
        //     {
        //         int ret = it->second->ShadowsocksHandleRemote();
        //         if (ret == -1) 
        //         {
        //             it = sts_.erase(it);
        //             continue;
        //         }
        //     }
        //     it++;
        // }        
    }
}