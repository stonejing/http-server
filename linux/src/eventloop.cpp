#include "eventloop.h"
#include <thread>

void EventLoop::loop()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 5);  // Random duration between 1 and 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(3));
    CLogger& Log = CLogger::getInstance();          // class 创建完成，但是内部成员可能没有初始化完成
    LOG_INFO("loop function start");
    epoll_event events[MAX_EVENT_NUMBER];
    while(1)
    {
        // int duration = dist(gen);
        // std::this_thread::sleep_for(std::chrono::seconds(3));
        // LOG_INFO("duration: %d get epollfd", duration);

        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        LOG_INFO("EPOLL WAIT.");
        if((number < 0 ) && (errno != EINTR))
        {
            LOG_ERR("epoll failure.");
            break;
        }

        for(int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if(events[i].events & EPOLLIN)
            {
                handleRead(sockfd);
            }
            else if(events[i].data.fd == evfd)
            {
                handleSocketQueue();
            }
            else if(events[i].events & EPOLLOUT)
            {
                handleWrite(sockfd);
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                LOG_INFO("close %d connection.", sockfd);
            }
            else 
            {
                LOG_ERR("epoll unseen event failure.");
            }
        }
    } 
}