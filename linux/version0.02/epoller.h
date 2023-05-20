#pragma once

#include <sys/epoll.h>
#include <memory>
#include <vector>
#include "channel.h"
#include <map>

using SharedChannel = std::shared_ptr<Channel>;

class Epoller
{
public:

    Epoller();
    ~Epoller();
    void EpollAdd(SharedChannel request);
    void EpollModify(SharedChannel request);
    void EpollDelete(SharedChannel request);

    std::vector<SharedChannel> poll();

    int get_epoll_fd_()
    {
        return epoll_fd_;
    }

private:
    static const int kMaxFd = 10000;
    int epoll_fd_;
    std::vector<epoll_event> events_;
    std::map<int, SharedChannel> channels_;
};