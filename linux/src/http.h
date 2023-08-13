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

// 处理 http 请求的类，在类中处理 read 和 write 事件
// buffer 只需要一个，因为同时只能有 read 和 write 一件事件发生
class Http
{
public: 
    Http(int epollfd, int fd);
    ~Http()
    {
        LOG_INFO("Http destructed");
    }

    std::shared_ptr<Channel> get_channel()
    {
        return channel_;
    }

private:    
    void handleRead();
    void HTTPWrite();
    void handleError();

    /*
        set buffer_ size to 4096, append buffer_ to HTTP request     
    */
    bool bufferRead();
    bool bufferWrite();

private:
    std::shared_ptr<Channel> channel_; 
    int sockfd_;
    int epollfd_;
    
    vector<char> buffer_;
    string http_response_;
    int bytes_have_sent_ = 0;

    string file_path_;

    CLogger& Log = CLogger::getInstance();
    HttpRequest request_;
    HttpResponse response_;

    string URL_;
    bool keep_alive_;
    map<string, string> headers_;
};