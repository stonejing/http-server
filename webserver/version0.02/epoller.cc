#include "epoller.h"
#include <assert.h>
#include <iostream>
#include "dbg.h"

const int kEventNumber = 4096;

Epoller::Epoller() : 
    epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(kEventNumber)
{
    log_info("create epoll fd");
    assert(epoll_fd_ > 0);
}

Epoller::~Epoller()
{ }

void Epoller::EpollAdd(SharedChannel request)
{
    int fd = request->get_fd();
    log_info("epoll add fd: %d", request->get_fd());
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->get_events();
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        std::cout << "epoll add error." << std::endl;
        channels_.erase(fd);  
    }
    channels_[fd] = request;
}

void Epoller::EpollModify(SharedChannel request)
{
    int fd = request->get_fd();
    log_info("epoll modify fd: %d", request->get_fd());
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->get_events();
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        std::cout << "epoll modify error." << std::endl;
        channels_.erase(fd);  
    }
}

void Epoller::EpollDelete(SharedChannel request)
{
    int fd = request->get_fd();
    log_info("epoll delete fd: %d", request->get_fd());
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->get_events();
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        std::cout << "epoll delete error." << std::endl;  
    }
    channels_.erase(fd);
}

std::vector<SharedChannel> Epoller::poll()
{
    log_info("epoll wait.");
    int event_number = epoll_wait(epoll_fd_, &*events_.begin(), events_.size(), -1);
    std::vector<SharedChannel> result_channel;
    for(int i = 0; i < event_number; i++)
    {
        int fd = events_[i].data.fd;
        log_info("epoll event fd: %d", fd);
        SharedChannel current_channel = channels_[fd];
        if(current_channel)
        {
            current_channel->set_current_events(events_[i].events);
            result_channel.push_back(current_channel);
        }
        else
        {
            std::cout << ("shared channel current invalid.\n");   
        }
    }
    return result_channel;
}