#include "http.h"
#include "httpproxy.h"
#include "utils.h"
#include <asm-generic/errno.h>
#include <cerrno>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

Http::Http(int epollfd, int fd) : sockfd_(fd), 
                    channel_(make_shared<Channel>(epollfd, fd)), 
                    buffer_(std::vector<char>(8096)),
                    read_idx_(0),
                    write_idx_(0),
                    epollfd_(epollfd)
{
    channel_->set_event(EPOLLIN);
    channel_->set_read_callback(std::bind(&Http::handleRead, this));
    channel_->set_write_callback(std::bind(&Http::handleWrite, this));
    // channel->set_error_callback(std::bind(&Http::handleError, this));
}

void Http::handleRead()
{
    LOG_INFO("http handle read");
    int ret = bufferRead();
    if(ret == -1 || ret == 0)
    {
        LOG_ERR("http read error, epoll delte sockfd: ", sockfd_);
        epollDelFd(epollfd_, sockfd_);
    }
    else
    {
        string res = std::string(buffer_.begin(), buffer_.begin() + read_idx_);
        // HTTP request
        // cout << res << endl;
        request_.add_buffer(res);    // 添加 buffer 并且 parse 
        read_idx_ = 0;
        // 如果 parse 完成，设置 channel 的 event 为 EPOLLOUT；否则继续 read buffer
        if(request_.get_parse_status() == 0)
        {
            request_.get_information(keep_alive_, URL_, headers_);
            request_.request_reset();

            string host_ = "";
            if(headers_.count("host"))
            {
                host_ = headers_["host"];
            }            

            if(host_ == HOST)
            {
                LOG_INFO("http start write the buffer");
                response_.set_information(true, URL_);
                string res = response_.get_response();
                read_idx_ = res.size();
                // HTTP response
                // cout << res << endl;
                buffer_ = std::vector<char>(res.begin(), res.end());
                channel_->set_event(EPOLLOUT);
            }
            else
            {
                LOG_INFO("using http proxy");
                // string res = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\n\r\nX";
                // read_idx_ = res.size();
                // buffer_ = std::vector<char>(res.begin(), res.end());
                // channel_->set_event(EPOLLOUT);
                string proxy_host = headers_["host"];
                string proxy_url = URL_;
                HttpProxy proxy;
                proxy.set_information(proxy_host, proxy_url);
                string http_request = request_.get_request();
                proxy.set_request(http_request);

                string res = proxy.get_response();
                read_idx_ = res.size();

                buffer_ = std::vector<char>(res.begin(), res.end());
                channel_->set_event(EPOLLOUT);
            }
        }
    }
}

void Http::handleWrite()
{
    int ret = bufferWrite();
    if(ret == 1)
    {
        channel_->set_event(EPOLLIN | EPOLLET);
        buffer_.resize(8096);
        read_idx_ = 0;
    }
    else if(ret == 2)
    {
        LOG_INFO("http write buffer not finished");
        channel_->set_event(EPOLLOUT | EPOLLET);
    }
    else
    {
        LOG_ERR("http write error, epoll delte sockfd ", sockfd_);
        epollDelFd(epollfd_, sockfd_);
    }
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
// epoll ET mode, should read all buffer in the kernel
int Http::bufferRead()
{
    write_idx_ = 0;
    int bytes_read = 0;
    int loop = 4;
    while(loop--)
    {
        bytes_read = ::recv(sockfd_, buffer_.data() + read_idx_, buffer_size, 0);
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
        read_idx_ += bytes_read;
    }
    return 1;
}

// non block write，一次性将 response 全部发送；暂时没有考虑到 EPOLLOUT 写缓冲区满时的事件
int Http::bufferWrite()
{
    int bytes_send = 0;
    while(true)
    {
        bytes_send = send(sockfd_, buffer_.data() + write_idx_, read_idx_ - write_idx_, 0);
        if(bytes_send == -1)
        {
            // write buffer full, wait for another chance
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                channel_->set_event(EPOLLOUT); // 可以不这样设置
                return 2;
            }
            return -1;
        }
        else if(bytes_send == 0) return 0;
        write_idx_ += bytes_send;
        if(write_idx_ == read_idx_) break;
    }  
    return 1;
}