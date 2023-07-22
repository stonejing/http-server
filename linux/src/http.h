#include "log.h"
#include "channel.h"

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

    bool bufferRead();
    bool bufferWrite();

    std::shared_ptr<Channel> get_channel()
    {
        return channel;
    }

private:
    std::shared_ptr<Channel> channel; 
    int sockfd;
    int epollfd;
    std::vector<char> buffer; 
    // char buffer[1024];
    int read_idx;
    int write_idx;
    static const int buffer_size = 1024;

    CLogger& Log = CLogger::getInstance();
};