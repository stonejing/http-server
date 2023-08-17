#include "eventloop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>

void EventLoop::setNewSocket(int fd)
{
    LOG_INFO("set new socket fd: ", fd);
    std::lock_guard<std::mutex> socket_queue_lock(mut_);
    socket_queue_.push(fd);
    eventfd_write(evfd_, 1);
}

void EventLoop::handleSocketQueue()
{
    std::lock_guard<std::mutex> lock(mut_);
    while(!socket_queue_.empty())
    {
        int fd = socket_queue_.front();
        socket_http_map_[fd] = std::make_unique<Http>(this, fd);
        socket_http_map_[fd]->registerChannel();
        socket_queue_.pop();
    }
}

void EventLoop::setNewChannel(int fd, std::shared_ptr<Channel> channel)
{
    cout << "set new channel: " << fd << endl;
    cout << channel.use_count() << endl;
    socket_channel_map_[fd] = nullptr;

    epollAddFd(fd);
    cout << "set new channel fd: " << fd << endl;
}

void EventLoop::epollAddFd(int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event);
}

void EventLoop::epollModFd(int fd, int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLRDHUP;
    epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event);
}

void EventLoop::epollDelFd(int fd)
{
    epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, 0);
    if(socket_channel_map_.count(fd))
    {
        socket_channel_map_.erase(fd);
        close(fd);
    }
    if(socket_http_map_.count(fd))
    {
        socket_http_map_.erase(fd);
        close(fd);
    }
}

void EventLoop::loop()
{
    CLogger& Log = CLogger::getInstance();          // class 创建完成，但是内部成员可能没有初始化完成
    epollAddFd(evfd_);
    while(1)
    {
        int number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1);
        if((number < 0 ) && (errno != EINTR))
        {
            LOG_ERR("epoll failure.");
            break;
        }

        for(int i = 0; i < number; i++)
        {
            int sockfd = events_[i].data.fd;
            if(sockfd == evfd_)
            {
                eventfd_t value;
                eventfd_read(evfd_, &value);
                handleSocketQueue();
            }
            else 
            {
                LOG_INFO("event handle epoll event: ", sockfd);
                socket_channel_map_[sockfd]->handleEvent();
            }
        }
    } 
}