#include "http.h"
#include "utils.h"
#include <asm-generic/errno.h>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Http::Http(int epollfd, int fd) : sockfd(fd), 
                    channel(make_shared<Channel>(epollfd, fd)), 
                    buffer(std::vector<char>(8096)),
                    read_idx(0),
                    write_idx(0),
                    epollfd(epollfd)
{
    channel->set_event(EPOLLIN);
    channel->set_read_callback(std::bind(&Http::handleRead, this));
    channel->set_write_callback(std::bind(&Http::handleWrite, this));
    // channel->set_error_callback(std::bind(&Http::handleError, this));
}

// 对于一个无协议的 echo 服务器来说，就是收到多少发送多少，不需要等到协议解析
void Http::handleRead()
{
    LOG_INFO("http handle read");
    // get read buffer
    int ret = bufferRead();
    if(ret == -1 || ret == 0)
    {
        LOG_INFO("http epoll delte sockfd %d", sockfd);
        epollDelFd(epollfd, sockfd);
    }
    else
    {
        string res = std::string(buffer.begin(), buffer.begin() + read_idx);
        cout << res << endl;
        request.add_buffer(res);    // 添加并且 parse 
        read_idx = 0;
        if(request.get_parse_status() == 0)
        {
            request.get_information(keep_alive_, URL_);
            channel->set_event(EPOLLOUT);
        }
    }
}

void Http::handleWrite()
{
    LOG_INFO("http start write the buffer");
    // bufferWrite();
    response.set_information(true, URL_);
    string res = response.get_response(read_idx);
    cout << res.size() << endl;
    buffer = std::vector<char>(res.begin(), res.end());
    bufferWrite();
    channel->set_event(EPOLLIN | EPOLLET);
    buffer.resize(1024);
    read_idx = 0;
}

// nonblock file descriptor 
// false close socket
// 有三种状态：
// 1. 错误，对方关闭 socket             -1 
// 2. 缓冲区读完了，但是对方没有发送完     0
// 3. 对方发送完了，也读完了   

// 只有两种返回状态：缓冲区 buffer 读完了，出现错误或socket关闭
// buffer read 最多读取 8096 字节，返回三种状态：（出错 -1，远程关闭连接 0，）可以合并成一个；
// （缓冲区还可以读 1，缓冲区已经没有了 2）也可以合并成一个
// 第一种情况：直接关闭连接，删除 HTTP 对象就好了
// 第二种情况：都是直接将收到的 buffer 传入到 httprequest 对象，
//      EPOLLOUT event 设定需要从 request parse 判断，http request 有没有收取完毕；
int Http::bufferRead()
{
    write_idx = 0;
    int bytes_read = 0;
    // epoll ET mode, should read all buffer in the kernel
    int loop = 4;
    while(loop--)
    {
        bytes_read = ::recv(sockfd, buffer.data() + read_idx, buffer_size, 0);
        // read error happened
        if(bytes_read == -1)
        {
            // no message in the buffer, wait the next EPOLLIN event
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return 2;
            }
            return -1;
        }
        // remote close the connection
        else if(bytes_read == 0)
        {
            return 0;
        }
        read_idx += bytes_read;
    }
    return 1;
}

// non block write，一次性将 response 全部发送；暂时没有考虑到 EPOLLOUT 写缓冲区满时的事件
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