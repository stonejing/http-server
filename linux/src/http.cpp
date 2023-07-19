#include "http.h"
#include "utils.h"
#include <asm-generic/errno.h>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Http::Http(int epollfd, int fd) : sockfd(fd), 
                    channel(make_shared<Channel>(epollfd, fd)), 
                    buffer(std::vector<char>(1024)),
                    read_idx(0),
                    write_idx(0),
                    epollfd(epollfd)
{
    channel->set_event(EPOLLIN);
    channel->set_read_callback(std::bind(&Http::handleRead, this));
    channel->set_write_callback(std::bind(&Http::handleWrite, this));
    // channel->set_error_callback(std::bind(&Http::handleError, this));
}
// 对于一个无协议的 echo 服务器来说，就是收到多少发送多少

void Http::handleRead()
{
    LOG_INFO("http handle read");
    // get read buffer
    if(bufferRead())
    {
        channel->set_event(EPOLLOUT);
        // process buffer
        std::string s(buffer.begin(), buffer.begin() + read_idx);
        // LOG_INFO("%s", s);
        cout << s << endl;
        LOG_INFO("read len: %d", read_idx);
    }
    else 
    {
        LOG_INFO("http epoll delte sockfd %d", sockfd);
        epollDelFd(epollfd, sockfd);
    }
}

void Http::handleWrite()
{
    // get processed buffer
    // send processed buffer
    LOG_INFO("http start write the buffer");
    string response = "HTTP/1.1 200 OK\r\nserver: nginx1223123 \
        12\r\nContent-Type: text/html\r\nConnection: Keep-Alive\r\nKeep-Alive: timeout=5, max=1000\r\n\r\nTEST";
    read_idx = response.size();
    buffer = std::vector<char>(response.begin(), response.end()); 
    // bufferWrite();
    int res = send(sockfd, response.c_str(), response.size(), 0);
    cout << res << endl;
    read_idx = 0;
    channel->set_event(EPOLLIN | EPOLLET);
    // epollDelFd(epollfd, sockfd);
    // channel->set_event(EPOLLIN);
    // LOG_INFO("%d", write_idx);
}

// nonblock file descriptor 
// false close socket
// 有三种状态：
// 1. 错误，对方关闭 socket             -1 
// 2. 缓冲区读完了，但是对方没有发送完     0
// 3. 对方发送完了，也读完了   

// 只有两种返回状态：缓冲区 buffer 读完了，出现错误或socket关闭
bool Http::bufferRead()
{
    write_idx = 0;
    if(read_idx >= buffer_size)
    {
        return false;
    }

    int bytes_read = 0;
    // epoll ET mode, should read all buffer in the kernel
    while(true)
    {
        bytes_read = recv(sockfd, buffer.data() + read_idx, buffer_size - read_idx, 0);
        // read error happened
        if(bytes_read == -1)
        {
            // no message the recv return -1
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return false;
        }
        // remove close socket
        else if(bytes_read == 0)
        {
            return false;
        }
        read_idx += bytes_read;
    }
    return true;
}

// non block write
bool Http::bufferWrite()
{
    int bytes_send = 0;

    while(true)
    {
        if(read_idx == write_idx) 
        {
            read_idx = 0;
            channel->set_event(EPOLLIN);
            return true;
        }
        bytes_send = send(sockfd, buffer.data() + write_idx, read_idx - write_idx, 0);
        if(bytes_send == -1)
        {
            // write buffer full, wait for another chance
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                channel->set_event(EPOLLOUT); // 可以不这样设置
                break;
            }
            return false;
        }
        else if(bytes_send == 0) return false;
        write_idx += bytes_send;
    }  
    return true;
}