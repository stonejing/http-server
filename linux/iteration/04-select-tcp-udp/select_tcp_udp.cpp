#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void dg_echo(int sockfd, struct sockaddr* pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char mesg[1024];

    // for (;;) {
        len = clilen;
        n = recvfrom(sockfd, mesg, 1024, 0, pcliaddr, &len);
        mesg[n] = '\0';
        printf("recvfrom %s\n", mesg);
        sendto(sockfd, mesg, n, 0, pcliaddr, len);
        printf("sendto over.\n");
    // }
}

void str_echo(int fd, fd_set& read_set)
{
    char buff[1024];
    int n = recv(fd, buff, 1024, 0);
    if(n == 0)
    {
        printf("client closed connection.\n");
        FD_CLR(fd, &read_set);
        close(fd);
    }
    else if(n > 0)
    {
        printf("get message from %d: %s\n", fd, buff);
        send(fd, buff, n, 0);
    }
    else {
        printf("error.\n");
    }    
}

int main()
{
    const int on = -1;
    int listenfd, connfd, udpfd;

    struct sockaddr_in servaddr, clientaddr;
    socklen_t len = sizeof(clientaddr);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8000);

    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(ret == -1) printf("tcp bind error: %d\n", ret);
    ret = listen(listenfd, 5);

    udpfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8000);
    ret = bind(udpfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(ret == -1) printf("udp bind error: %d\n", ret);

    int maxfd = (listenfd > udpfd ? listenfd : udpfd) + 1;
    fd_set read_set;
    fd_set read_temp;
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);
    FD_SET(udpfd, &read_set);

    pid_t pid;

    int count = 0;
    for(;;)
    {
        read_temp = read_set;
        int num = select(maxfd + 1, &read_temp, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &read_temp))
        {
            connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len); 
            FD_SET(connfd, &read_set);
            maxfd = connfd > maxfd ? connfd : maxfd;
        }
        else if(FD_ISSET(udpfd, &read_temp))
        {
            socklen_t len = sizeof(clientaddr);
            dg_echo(udpfd, (struct sockaddr*)&clientaddr, len);
        }

        for(int i = 3; i < maxfd + 1; i++)
        {
            if(i != listenfd && i != udpfd && FD_ISSET(i, &read_temp))
            {
                str_echo(i, read_set);
            }
        }
    }

    return 0;
}