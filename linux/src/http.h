#include "log.h"
#include "channel.h"
#include "httprequest.h"
#include "httpresponse.h"

#include <future>
#include <vector>
#include <string>
#include <memory>
#include <unistd.h>

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
    void handleWrite();
    void handleError();

    int bufferRead();
    int bufferWrite();

private:
    std::shared_ptr<Channel> channel_; 
    int sockfd_;
    int epollfd_;
    
    int read_idx_;
    int write_idx_;
    
    vector<char> buffer_;

    const int buffer_size = 1024;

    string file_path_;

    CLogger& Log = CLogger::getInstance();
    HttpRequest request_;
    HttpResponse response_;

    string URL_;
    bool keep_alive_;
    map<string, string> headers_;
};