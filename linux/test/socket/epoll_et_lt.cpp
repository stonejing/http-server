#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool enable_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    if(enable_et)
        event.events |= EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void lt(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for(int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            printf("%d handle connection, %s.\n", sockfd, __TIME__);
            struct sockaddr_in client_address;
            socklen_t len = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*)&client_address, &len);
            addfd(epollfd, connfd, false);
        }
        else if(events[i].events & EPOLLIN)
        {
            printf("%d, event trigger once, %s.\n", events[i].data.fd, __TIME__);
            memset(buf, '\0', BUFFER_SIZE);
            int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
            if(ret <= 0)
            {
                close(sockfd);
                continue;
            }
            printf("get %d bytes of content: %s\n", ret, buf);
        }
        else
        {
            printf("something error happened.\n");
        }
    }
}

void et(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for(int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            printf("%d handle connection, %s.\n", sockfd, __TIME__);
            struct sockaddr_in client_address;
            socklen_t len = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*)&client_address,
                                &len);
            addfd(epollfd, connfd, true);
        }
        else if(events[i].events & EPOLLIN)
        {
            printf("%d, event trigger once, %s.\n", events[i].data.fd, __TIME__);
            while(1)
            {
                memset(buf, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if(ret < 0)
                {
                    if((errno == EAGAIN) || (errno == EWOULDBLOCK))
                    {
                        printf("read later.\n");
                        break;
                    }
                    close(sockfd); 
                    break;
                }
                else if(ret == 0)
                {
                    close(sockfd);
                }
                else
                {
                    printf("get %d bytes of content: %s\n", ret, buf);
                }
            }
        }
        else
        {
            printf("something error happened.\n");
        }
    }
}

int main(void)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8000);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);

    int reuse = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, true);

    while(1)
    {
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0)
        {
            printf("epoll failure.\n");
            break;
        }
        lt(events, ret, epollfd, listenfd);
        // et(events, ret, epollfd, listenfd);
    }

    close(listenfd);
    return 0;
}