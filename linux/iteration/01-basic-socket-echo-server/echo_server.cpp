#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

ssize_t writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = (const char*)vptr;
    
    return 0;
}

ssize_t readn()
{
    return 0;
}

int main(void)
{
    char IP[10] = "127.0.0.1";
    int PORT = 8000;
    int listenfd, connfd;
    struct sockaddr_in servaddr, clientaddr;
    socklen_t len = sizeof(clientaddr);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

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
    int count = 0;
    for(;;)
    {
        connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
        char buff[1024];
        int n = recv(connfd, buff, 1024, 0);
        send(connfd, buff, n, 0);
        printf("connection from %d\n", connfd);
        close(connfd);
    }

    return 0;
}