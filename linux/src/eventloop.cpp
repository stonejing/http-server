#include "eventloop.h"
#include "channel.h"
#include <ares.h>
#include <cstddef>
#include <memory>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <thread>

EventLoop::EventLoop() : 
        epollfd_(epoll_create1(EPOLL_CLOEXEC)), 
        evfd_(eventfd(0, EFD_NONBLOCK)),
        socket_channel_map_(),
        ares_channel_(NULL)
{
    LOG_INFO("event loop created ", epollfd_, " ", evfd_);
    struct ares_options options;
    int optmask = ARES_OPT_FLAGS;
    options.flags = ARES_FLAG_NOCHECKRESP;
    options.flags |= ARES_FLAG_STAYOPEN;
    options.flags |= ARES_FLAG_IGNTC; // UDP only

    int status = ares_init_options(&ares_channel_, &options, optmask);
    if(status != ARES_SUCCESS)
    {
        LOG_ERR("ares_init_options error: ", ares_strerror(status));
        assert(0);
    }
    // socket create: only when ares_gethostname called, then the callback will run
    // ares_gethostname dns_callback should not be NULL
    ares_set_socket_callback(ares_channel_, &EventLoop::ares_socket_create_callback, this);
}

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

int EventLoop::ares_socket_create_callback(int sock, int type, void *data)
{
    cout << "event loop ares socket create callback: " << sock << endl;
    // create dns query socket
    EventLoop* loop = (EventLoop*)data;
    loop->setDnsChannel(sock);
    return 0;
}

void EventLoop::setDnsChannel(int sock)
{
    epollAddFd(sock);
    std::shared_ptr<Channel> channel = std::make_shared<Channel>(this, sock);
    channel->set_read();
    channel->set_read_callback(std::bind(&EventLoop::handleDnsRead, this, sock));
    socket_channel_map_[sock] = channel;
}

void EventLoop::handleDnsRead(int sock)
{
    LOG_INFO("handle dns read ", sock);
    ares_process_fd(ares_channel_, sock, ARES_SOCKET_BAD);
}

void EventLoop::setNewChannel(int fd, std::shared_ptr<Channel> channel)
{
    socket_channel_map_[fd] = channel;
    // dns_callback will execute after ares_process or ares_process_fd called 
    ares_gethostbyname(ares_channel_, "example.com", AF_INET, dns_callback, NULL);
    epollAddFd(fd);
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

    socket_channel_map_.erase(fd);
    if(socket_http_map_.count(fd))
    {
        socket_http_map_.erase(fd);
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