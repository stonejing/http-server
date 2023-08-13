#include "http.h"
#include "httpproxy.h"
#include "utils.h"
#include <cerrno>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <udns.h>

Http::Http(int epollfd, int fd) : sockfd_(fd), 
                    channel_(make_shared<Channel>(epollfd, fd)), 
                    buffer_(std::vector<char>(8096)),
                    epollfd_(epollfd)
{
    channel_->set_event(EPOLLIN);
    channel_->set_read_callback(std::bind(&Http::handleRead, this));
    channel_->set_write_callback(std::bind(&Http::HTTPWrite, this));
    // channel->set_error_callback(std::bind(&Http::handleError, this));
}

/*
    support three functions:
    1. http request
    2. http proxy
    3. https proxy
    HttpRequest status: -1 error, 0 not determined, 1 http request, 2 http proxy, 3 https proxy
    according to the status, choose different function to handle the request
*/
void Http::handleRead()
{
    if(bufferRead())
    {
        request_.add_buffer(recv_buffer_);
        int ret = request_.get_parse_status();
        switch(ret)
        {
            case 1:
            {
                LOG_INFO("http request");
                request_.get_information(keep_alive_, URL_, headers_);
                response_.set_information(keep_alive_, URL_);
                http_response_ = std::move(response_.get_response());
                channel_->set_event(EPOLLOUT | EPOLLET);
                break;
            }
            case 2:
            {
                LOG_INFO("http proxy");
                request_.get_information(keep_alive_, URL_, headers_);
                http_proxy_.set_request(recv_buffer_);
                http_proxy_.set_information(headers_["host"], URL_);
                
                http_response_ = std::move(http_proxy_.get_response());

                cout << http_response_ << endl;

                channel_->set_event(EPOLLOUT | EPOLLET);
                break;
            }
            case 3:
            {
                LOG_INFO("https proxy");
                channel_->set_event(EPOLLOUT | EPOLLET);
                break;
            }
            case -1:
            {
                // parse error
                LOG_ERR("http request error, epoll delte sockfd: ", sockfd_);
                epollDelFd(epollfd_, sockfd_);
                break;
            }
            case 0:
            {
                // parse not finished
                channel_->set_event(EPOLLIN);
                break;
            }
        }
    }
    else
    {
        LOG_ERR("http read error, epoll delte sockfd: ", sockfd_);
        epollDelFd(epollfd_, sockfd_);
    }
}

void Http::HTTPWrite()
{
    LOG_INFO("http write");
    if(bufferWrite())
    {
        if(bytes_have_sent_ == 0)
        {
            if(keep_alive_)
            {
                channel_->set_event(EPOLLIN);
            }
            else
            {
                LOG_INFO("close connection");
                epollDelFd(epollfd_, sockfd_);
            }
        }
        else
        {
            LOG_INFO("http write buffer not finished");
            return;
        }
    }
    else
    {
        LOG_ERR("http write error, epoll delte sockfd ", sockfd_);
        epollDelFd(epollfd_, sockfd_);
    }
}

bool Http::bufferRead()
{
    while(true)
    {
        int bytes_read = ::recv(sockfd_, buffer_.data(), BUFFER_SIZE, 0);
        if(bytes_read == -1)
        {
            // read buffer empty, wait for another chance
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return true;
            }
            return false;
        }
        // remote close connection
        else if(bytes_read == 0)
        {
            return false;
        }   
        recv_buffer_.append(std::string(buffer_.begin(), buffer_.begin() + bytes_read));
    }
}

bool Http::bufferWrite()
{
    int bytes_send = 0;
    while(true)
    {
        bytes_send = ::send(sockfd_, http_response_.c_str() + bytes_have_sent_, http_response_.size() - bytes_have_sent_, 0);
        if(bytes_send == -1)
        {
            // write buffer full, wait for another chance
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // remain buffer to write
                channel_->set_event(EPOLLOUT | EPOLLET);
                return true;
            }
            return false;
        }
        // only happends when ssize_t n == 0
        else if(bytes_send == 0) return false;
        bytes_have_sent_ += bytes_send;
        if(bytes_have_sent_ == http_response_.size())
        {
            bytes_have_sent_ = 0;
            return true;
        }
    }  
}