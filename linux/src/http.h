#pragma once

#include "log.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpproxy.h"

#include <future>
#include <vector>
#include <string>
#include <memory>
#include <unistd.h>

const int BUFFER_SIZE = 4096;

class EventLoop;
class HttpProxy;
class Channel;

class Http
{
public: 
    Http(EventLoop* loop, int fd);
    ~Http()
    {
        LOG_INFO("http destructed");
        close(sockfd_);
    }
    
    void init();
    shared_ptr<Channel> getChannel();
    void registerChannel();
    EventLoop* getLoop();
public:
    void handleRead();
    void HTTPWrite();
    void handleError();

    void proxyRead();
    void proxyWrite();

    // set buffer_ size to 4096, append buffer_ to HTTP request     
    bool bufferRead();
    bool bufferWrite();

    void set_response(string& response)
    {
        http_response_ = response;
    }

private:
    std::shared_ptr<Channel> channel_; 

    int sockfd_;
    int epollfd_;

    EventLoop* loop_;
    
    vector<char> buffer_;
    string recv_buffer_;
    string http_response_;
    int bytes_have_sent_ = 0;

    string file_path_;

    CLogger& Log = CLogger::getInstance();
    HttpRequest request_;
    HttpResponse response_;
    HttpProxy http_proxy_;

    string URL_;
    bool keep_alive_;
    map<string, string> headers_;
};