/*
    a test program that use epoll and c-ares to exect asynchronous DNS query
    and connect to the IP address returned by DNS query using non-blocking socket
*/

#include <ares.h>
#include <arpa/inet.h>
#include <cstdio>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <cstring>
#include <string>
#include <iostream>
#include <assert.h>
#include <sys/types.h>
#include <udns.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>

using namespace std;

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 10

struct dns_res_t
{
    const char* error_info;
    void* address;
    ssize_t address_len;
};

void dns_callback(void* arg, int status, int timeouts, struct hostent* host) 
{
    if (status == ARES_SUCCESS)
    {
        puts(host->h_name);
        puts(inet_ntoa(*(struct in_addr*)host->h_addr));
    }
    else
        std::cout << "lookup failed: " << ares_strerror(status) << std::endl;
}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addInFd(int epollfd, int fd, bool enable_et)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN;
    // if(enable_et)
    //     event.events |= EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void modInFd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void addOutFd(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

int main(void)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(9000);

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
    addInFd(epollfd, listenfd, true);
    addInFd(epollfd, STDIN_FILENO, true);
    ares_channel channel;
    ret = ares_init(&channel);
    if(ret != ARES_SUCCESS)
    {
        printf("ares_init error.\n");
        return 0;
    }
    struct sockaddr_in sa = {};
    string domain = "www.google.com";
    dns_res_t res = {NULL, &sa.sin_addr.s_addr, sizeof(sa.sin_addr.s_addr)};
    ares_gethostbyname(channel, domain.c_str(), AF_INET, dns_callback, &res);
    ares_socket_t socks[1];
    int dns_sockfd;
    int bitmask = ares_getsock(channel, socks, 1);
    for (int i = 0; i < 1; i++)
    {
        cout << "socks: " << socks[i] << endl;
        dns_sockfd = socks[0];
        if (ARES_GETSOCK_READABLE(bitmask, i))
        {
            addInFd(epollfd, socks[i], true);
        }
        if (ARES_GETSOCK_WRITABLE(bitmask, i))
        {
            addOutFd(epollfd, socks[i]);
        }
    }
    
    while(1)
    {
        cout << "EPOLL WAIT" << endl;
        modInFd(epollfd, dns_sockfd);
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0)
        {
            printf("epoll failure.\n");
            break;
        }

        char buf[BUFFER_SIZE];
        for(int i = 0; i < ret; i++)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == listenfd)
            {
                printf("%d handle connection, %s.\n", sockfd, __TIME__);
                struct sockaddr_in client_address;
                socklen_t len = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address,
                                    &len);
                addInFd(epollfd, connfd, true);
            }
            else if(sockfd == STDIN_FILENO)
            {
                char buffer[1024];
                ssize_t num_read = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (num_read == -1) {
                    perror("read");
                    return 1;
                } else if (num_read == 0) {
                    printf("End of input. Exiting...\n");
                    return 0;
                } else {
                    buffer[num_read] = '\0';  // Null-terminate the string
                    printf("Read from input: %s", buffer);
                }
                ares_gethostbyname(channel, buffer, AF_INET, dns_callback, &res);              
            }
            else if(sockfd == dns_sockfd)
            {
                printf("%d, dns_sock trigger once, %s.\n", sockfd, __TIME__);
                ares_process_fd(channel, sockfd, ARES_SOCKET_BAD);
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

    close(listenfd);
    return 0;
}