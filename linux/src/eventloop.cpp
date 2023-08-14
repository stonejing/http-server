#include "eventloop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <thread>

void EventLoop::loop()
{
    CLogger& Log = CLogger::getInstance();          // class 创建完成，但是内部成员可能没有初始化完成
    epollAddFd(epollfd, evfd);
    while(1)
    {
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
                // LOG_INFO("event handle epoll event: ", sockfd);
                socket_http_map[sockfd]->handle_event();
            }
        }
    } 
}