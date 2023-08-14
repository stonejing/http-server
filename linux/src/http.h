#include "log.h"
#include "channel.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpproxy.h"

#include <future>
#include <vector>
#include <string>
#include <memory>
#include <unistd.h>

const int BUFFER_SIZE = 4096;

class Http
{
public: 
    Http(int epollfd, int fd);
    ~Http()
    {
        LOG_INFO("http destructed");
    }
    void init()
    {
        LOG_INFO("http init");
        channel_->set_event(EPOLLIN);
        channel_->set_read_callback(std::bind(&Http::handleRead, this));
        channel_->set_write_callback(std::bind(&Http::HTTPWrite, this));
    }

    void handle_event()
    {
        channel_->handleEvent();
    }

private:    
    void handleRead();
    void HTTPWrite();
    void handleError();

    void proxyRead();
    void proxyWrite();

    // set buffer_ size to 4096, append buffer_ to HTTP request     
    bool bufferRead();
    bool bufferWrite();

private:
    std::unique_ptr<Channel> channel_; 
    int sockfd_;
    int epollfd_;
    
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