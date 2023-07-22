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

#include <string>
#include <iostream>

#include "bind_and_listen.h"

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024

void et(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buffer[BUFFER_SIZE];
    for(int i = 0; i < number; i++)
    {
        int sockfd = events[i].data.fd;
        if(sockfd == listenfd)
        {
            printf("%d handle connection, %s.\n", sockfd, __TIME__);
            struct sockaddr_in client_address;
            socklen_t len = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*)&client_address, &len);
            addfd(epollfd, connfd, true);
        }
        else if(events[i].events & EPOLLIN)
        {
            // printf("%d, event trigger once, %s.\n", events[i].data.fd, __TIME__);
            ssize_t len = 0;
            while(1)
            {
                memset(buffer, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
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
                    continue;
                }
                len += ret;
            }
            // Handle client socket events
            std::cout << "set sockfd to epollout" << std::endl;
            modfd(epollfd, sockfd, EPOLLOUT);

            // ssize_t bytesRead = recv(sockfd, buffer, sizeof(buffer), 0);
            // if (bytesRead == -1) {
            //     std::cerr << "Failed to receive data from client" << std::endl;
            //     close(sockfd);
            //     continue;
            // } else if (bytesRead == 0) {
            //     // Client closed the connection
            //     std::cout << "Client closed connection" << std::endl;
            //     close(sockfd);
            //     continue;
            // }

            // Parse request and send response
            // ssize_t bytesRead = len;
            // std::cout << bytesRead << " " << len << std::endl;
            // std::string request(buffer, bytesRead);
            // std::cout << "Received request: " << request << std::endl;

            // // Simulate processing time
            // // sleep(1);
            // std::string response = "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Length: 5\r\n\r\nHello";
            // ssize_t bytesSent = send(sockfd, response.c_str(), response.length(), 0);
            // if (bytesSent == -1) {
            //     std::cerr << "Failed to send response to client" << std::endl;
            // }

            // // Check for Keep-Alive header
            // if (request.find("Connection: keep-alive") == std::string::npos) {
            //     // Keep-Alive header not found, close the connection
            //     std::cout << "Closing connection" << std::endl;
            //     epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, 0);
            //     close(sockfd);
            // }
        }   
        else if(events[i].events & EPOLLOUT)
        {
            std::cout << "epoll out" << std::endl;
            std::string response = "HTTP/1.1 200 OK\r\nserver: nginx122312312\r\n"
                                "Content-Type: text/html\r\nConnection: Keep-Alive\r\n"
                                "Keep-Alive: timeout=5, max=1000\r\n"
                                "Content-Length: 0\r\n"         
                                "\r\n";
            std::cout << response.length() << std::endl;
            int res = send(sockfd, response.c_str(), response.length(), 0);
            std::cout << res << std::endl;
            modfd(epollfd, sockfd, EPOLLIN);
            std::cout << "epoll out send over." << std::endl;
        }
        else
        {
            printf("something error happened.\n");
        }
    }
}

int main(void)
{
    int listenfd = bind_and_listen();

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create1(0);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, true);

    while(1)
    {
        std::cout << "epoll wait one time" << std::endl;
        int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(ret < 0)
        {
            printf("epoll failure.\n");
            break;
        }
        et(events, ret, epollfd, listenfd);
    }

    close(listenfd);
    return 0;
}