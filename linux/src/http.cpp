#include "http.h"
#include "eventloop.h"
#include "channel.h"

#include "httpproxy.h"

Http::Http(EventLoop* loop, int fd) : sockfd_(fd), 
                    buffer_(std::vector<char>(8096)),
                    channel_(std::make_shared<Channel>(loop, fd)),
                    loop_(loop)
{
    LOG_INFO("http constructed");
    channel_->set_read();
    channel_->set_read_callback(std::bind(&Http::handleRead, this));
    channel_->set_write_callback(std::bind(&Http::HTTPWrite, this));
    // channel_->set_error_callback(std::bind(&Http::handleError, this));
}

void Http::registerChannel()
{
    loop_->setNewChannel(sockfd_, channel_);
}

void Http::init()
{
    LOG_INFO("http init");
    channel_->set_read();
    channel_->set_read_callback(std::bind(&Http::handleRead, this));
    channel_->set_write_callback(std::bind(&Http::HTTPWrite, this));
}

shared_ptr<Channel> Http::getChannel()
{
    return channel_;
}

EventLoop* Http::getLoop()
{
    return loop_;
}

/*
    support three functions:
    1. http request
    2. http proxy
    3. https proxy
    HttpRequest status: -1 error, 0 not determined, 1 http request, 2 http proxy, 3 https proxy
    according to the status, choose different function to handle the request
*/
void Http::proxyRead()
{
    request_.get_information(keep_alive_, URL_, headers_);
    keep_alive_ = false;
    http_proxy_.init(recv_buffer_, headers_["host"], loop_->get_ares_channel(), loop_, this);
}


void Http::handleRead()
{
    LOG_INFO("handle read ", sockfd_);
    
    if(bufferRead())
    {
        request_.add_buffer(recv_buffer_);
        int ret = request_.get_parse_status();
        switch(ret)
        {
            case 1:
            {
                LOG_INFO("http request");
                recv_buffer_.clear();
                request_.get_information(keep_alive_, URL_, headers_);
                response_.set_information(keep_alive_, URL_);
                http_response_ = std::move(response_.get_response());
                channel_->set_write();
                break;
            }
            case 2:
            {
                LOG_INFO("http proxy");

                // http_proxy_.set_information(headers_["host"], URL_);
                // http_response_ = std::move(http_proxy_.get_response());
                proxyRead();
                channel_->set_read();
                // loop_->epollDelFd(sockfd_);
                break;
            }
            case 3:
            {
                // LOG_INFO("https proxy");
                // channel_->set_write();
                loop_->epollDelFd(sockfd_);
                break;
            }
            case -1:
            {
                // parse error
                LOG_ERR("http request error, epoll delte sockfd: ", sockfd_);
                loop_->epollDelFd(sockfd_);
                break;
            }
            case 0:
            {
                // parse not finished
                channel_->set_read();
                break;
            }
        }
    }
    else
    {
        LOG_ERR("http read error, epoll delte sockfd: ", sockfd_);
        loop_->epollDelFd(sockfd_);
    }
}

void Http::HTTPWrite()
{
    // LOG_INFO("http write");
    if(bufferWrite())
    {
        if(bytes_have_sent_ == 0)
        {
            if(keep_alive_)
            {
                channel_->set_read();
            }
            else
            {
                LOG_INFO("close connection: ", sockfd_);
                loop_->epollDelFd(sockfd_);
            }
        }
        else
        {
            LOG_WARN("http write buffer not finished");
            return;
        }
    }
    else
    {
        LOG_ERR("http write error, epoll delte sockfd ", sockfd_);
        loop_->epollDelFd(sockfd_);
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
                channel_->set_write();
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