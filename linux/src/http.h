#include "log.h"
#include "channel.h"
#include "httprequest.h"
#include "httpresponse.h"

#include <future>
#include <vector>
#include <string>
#include <memory>
#include <unistd.h>

class Http
{
public: 
    Http(int epollfd, int fd);
    ~Http()
    {
        LOG_INFO("Http destructed");
    }

    void handleRead();
    void handleWrite();
    void handleError();

    int bufferRead();
    bool bufferWrite();

    std::shared_ptr<Channel> get_channel()
    {
        return channel;
    }

private:
    std::shared_ptr<Channel> channel; 
    int sockfd;
    int epollfd;
    
    int read_idx;
    int write_idx;
    
    vector<char> buffer;

    const int buffer_size = 1024;

    string file_path;

    CLogger& Log = CLogger::getInstance();
    HttpRequest request;
    HttpResponse response;

    string URL_;
    bool keep_alive_;
};