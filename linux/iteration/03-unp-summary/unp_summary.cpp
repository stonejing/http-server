#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <string>
#include <errno.h>


int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

struct fd_map {
    int fd;
    fd_set* read_set;
};

ssize_t readn(int fd, void* buf, size_t nbytes)
{
    return 0;
}

ssize_t writen(int fd, void* buf, size_t nbytes)
{
    return 0;
}

void* str_echo(void* data)
{
    char buff[1024];
    fd_map* temp = (struct fd_map*) data;
    int fd = temp->fd;
    fd_set* read_set = temp->read_set;
    int n = recv(fd, buff, 1024, 0);
    if(n == 0)
    {
        printf("client closed connection.\n");
        FD_CLR(fd, read_set);
        close(fd);
    }
    else if(n > 0)
    {
        printf("get message from %d: %s\n", fd, buff);
        send(fd, buff, n, 0);
    }
    else {
        printf("error: %d.\n", errno);
        FD_CLR(fd, read_set);
        close(fd);
    }
    return 0; 
}

int main()
{
    char IP[10] = "127.0.0.1";
    int PORT = 8000;
    struct sockaddr_in servaddr, clientaddr;
    socklen_t len = sizeof(clientaddr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
    {
        printf("make socket error.\n");
        return 0;
    }


    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_addr.s_addr = inet_addr(IP);
    servaddr.sin_port = htons(PORT);
    // for test only, fast resue bind
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(ret == -1) 
    {
        printf("bind error: %d\n", ret);
        return 0;
    }
    ret = listen(listenfd, 5);
    if(ret == -1)
    {
        printf("listen error: %d\n", ret);
        return 0;
    }
    int maxfd = listenfd;
    fd_set read_set;
    fd_set read_temp;
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);

    for(;;)
    {
        read_temp = read_set;
        int num = select(maxfd + 1, &read_temp, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &read_temp))
        {
            int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len); 
            setnonblocking(connfd);
            FD_SET(connfd, &read_set);
            maxfd = connfd > maxfd ? connfd : maxfd;
        }

        for(int i = 3; i < maxfd + 1; i++)
        {
            if(i != listenfd && FD_ISSET(i, &read_temp))
            {
                struct fd_map a{i, &read_set};
                // str_echo(&a);
                pthread_t thread;
                pthread_create(&thread, NULL, str_echo, (void*)&a);
            }
        }
    }

    return 0;
}