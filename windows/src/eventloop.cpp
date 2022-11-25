#include "eventloop.h"
#include <WinSock2.h>
#include <mutex>
#include <openssl/crypto.h>
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
        LOG_INFO << "socket_type_ empty: " << socket_type_.empty() << "\n";
        if(socket_type_.empty())
        {
            LOG_INFO << "socket type is empty.\n";
            std::unique_lock<std::mutex> lk(socket_queue_mutex_);
            socket_variable.wait(lk, [this]{return !socket_queue_.empty();});
            LOG_INFO << "get socket: " << socket_queue_.front() << "\n";
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
        
            LOG_INFO << "select get new connection.\n";

            if (ret == SOCKET_ERROR)
            {
                LOG_ERROR << "select() return with error" << WSAGetLastError() <<"\n";
                abort();
            }
            for(auto it = socket_type_.begin(); it != socket_type_.end();)
            {
                if(FD_ISSET(it->first, &read_set_))
                {
                    // if(ReadFirstBuffer() == -1)
                    // {
                    //     socket_type_.erase(it->first);
                    //     continue;
                    // }
                    LOG_INFO << "FD SET socket: " << it->first << "\n";
                    int ret = recv(it->first, first_buffer.data(), first_buffer.size(), 0);
                    LOG_INFO << "recv data length: " << ret << "\n"; 
                    if(ret > 0)
                    {
                        for(int i = 0; i < ret; i++)
                        {
                            std::cout << first_buffer[i];
                        }
                        std::cout << std::endl;

                        char* buff = (char*)"HTTP/1.1 200 OK\r\nContent-length: 17\r\n\r\nTHIS IS A TEST.\r\n";
                        int s = send(it->first, buff, strlen(buff), 0);
                        if(s != strlen(buff))
                        {
                            LOG_ERROR << "WSA ERROR: " << WSAGetLastError() << "\n";
                        }
                    }
                    else 
                    {
                        closesocket(it->first);
                        socket_type_.erase(it);
                        continue;
                    }
                }
                it++;
            }

            {
                std::lock_guard<std::mutex> lk(socket_queue_mutex_);
                while(!socket_queue_.empty())
                {
                    socket_type_[socket_queue_.front()] = 3;
                    socket_queue_.pop();
                }
            }
        }
        // FD_ZERO(&read_set_);
        // FD_ZERO(&write_set_);

        // FD_SET(listen_socket_, &read_set_);
        // LOG_INFO << "listen socket: " << listen_socket_ << "\n";

        // std::vector<SOCKET> write_;

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

        // if(FD_ISSET(listen_socket_, &read_set_))
        // {
        //     HandleListenSocket();
        // }

        // for(auto it = socket_type_.begin(); it != socket_type_.end();)
        // {
        //     if(FD_ISSET(it->first, &read_set_))
        //     {
        //         if(ReadFirstBuffer() == -1)
        //         {
        //             socket_type_.erase(it->first);
        //             continue;
        //         }

        //         char* buff = (char*)"HTTP/1.1 200 OK\r\nContent-length: 17\r\n\r\nTHIS IS A TEST.\r\n";
        //         int s = send(it->first, buff, strlen(buff), 0);
        //         if(s != strlen(buff))
        //         {
        //             LOG_ERROR << "WSA ERROR: " << WSAGetLastError() << "\n";
        //         }
        //     }
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

// void EventLoop::IncreaseAccept() 
// {
//     std::lock_guard<std::mutex> guard(mutex_);
//     accept_loop_++;
// }

int EventLoop::ReadFirstBuffer()
{
    while(true)
    {
        int r = recv(accept_socket_, first_buffer.data() + first_buffer_len, 
                        1024 - first_buffer_len, 0);
        LOG_INFO << "recv length: " << r << "\n";
        if(r == 0)
        {
            closesocket(accept_socket_);
            return -1;
        }
        else if(r == SOCKET_ERROR)
        {
            if(WSAGetLastError() == WSAEWOULDBLOCK)
            {
                LOG_INFO << "e would block." << "\n";
                return 1;
            }
            else
            {
                closesocket(accept_socket_);
                return -1;
            }
        }
        first_buffer_len += r;
    }

    return 1;
}

// int EventLoop::HandleListenSocket() {
//     if ((accept_socket_ = accept(listen_socket_, NULL, NULL)) !=
//         INVALID_SOCKET) 
//     {
//         LOG_INFO << "accept_socket: " << accept_socket_ << "\n";
//         ULONG NonBlock = 1;
//         if(ioctlsocket(accept_socket_, FIONBIO, &NonBlock) == SOCKET_ERROR)
//         {
//             LOG_ERROR << "can not set socket to nonblock" << WSAGetLastError() << "\n";
//         }
//         socket_type_[accept_socket_] = 3;
//     } 
//     else 
//     {
//         if (WSAGetLastError() != WSAEWOULDBLOCK) 
//         {
//             LOG_ERROR << "accept() failed with error" << WSAGetLastError() <<"\n";
//             return -1;
//         }
//     }
//     return 1;
// }
