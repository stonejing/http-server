#include "eventloop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>

void EventLoop::loop()
{
    // std::random_device rd;
    // std::mt19937 gen(rd());
    // std::uniform_int_distribution<int> dist(1, 5);  // Random duration between 1 and 5 seconds
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    CLogger& Log = CLogger::getInstance();          // class 创建完成，但是内部成员可能没有初始化完成
    epollAddFd(epollfd, evfd);
    LOG_INFO("loop function start");
    while(1)
    {
        LOG_INFO("epoll_wait once.");
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if((number < 0 ) && (errno != EINTR))
        {
            LOG_ERR("epoll failure.");
            break;
        }

        for(int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == evfd)
            {
                eventfd_t value;
                eventfd_read(evfd, &value);
                handleSocketQueue();
            }
            else 
            {
                LOG_INFO("event handle epoll event: %d", sockfd);
                socket_http_map[sockfd]->get_channel()->handleEvent();
            }
        }
    } 
}