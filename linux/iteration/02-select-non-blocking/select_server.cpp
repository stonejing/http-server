#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
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
    char IP[10] = "127.0.0.1";
    int PORT = 8000;
    struct sockaddr_in servaddr, clientaddr;
    socklen_t len = sizeof(clientaddr);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_addr.s_addr = inet_addr(IP);
    servaddr.sin_port = htons(PORT);

    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(ret == -1) printf("bind error: %d\n", ret);
    ret = listen(listenfd, 5);
    int maxfd = listenfd;
    fd_set read_set;
    fd_set read_temp;
    FD_ZERO(&read_set);
    FD_SET(listenfd, &read_set);

    int count = 0;
    for(;;)
    {
        read_temp = read_set;
        int num = select(maxfd + 1, &read_temp, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &read_temp))
        {
            int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len); 
            FD_SET(connfd, &read_set);
            maxfd = connfd > maxfd ? connfd : maxfd;
        }

        for(int i = 3; i < maxfd + 1; i++)
        {
            if(i != listenfd && FD_ISSET(i, &read_temp))
            {
                str_echo(i, read_set);
            }
        }
    }

    return 0;
}