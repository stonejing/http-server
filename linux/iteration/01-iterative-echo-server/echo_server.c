#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

// 查看 linux 缓冲区
// cat /proc/sys/net/core/rmem_default
// 通常网络通信都需要 readn 和 writen，但是当前需求不大，暂时放置。

/*
    read 得到的字节数比请求的数据少，缓冲区数据已满
    read 缓冲区满了之后，read 只能返回缓冲区大小的数据，而不能返回 n
*/
ssize_t readn(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nread;

    const char *ptr;
    ptr = (const char*)vptr;
    nleft = n;
    while(nleft > 0)
    {
        if((nread = read(fd, (void*)ptr, nleft)) < 0)
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        else if(nread == 0)
            break;
        nleft -= nread;
        ptr   += nread;
    }
    return (n - nleft);
}
/*
    阻塞的时候，writen 和 write 没区别
    非阻塞的时候，write 立即返回；再 write 的时候，linux 缓冲区会满，write 不能再写，返回值和 n 不同
*/
ssize_t writen(int fd, const void* vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char* ptr;
    ptr = (const char*)vptr;
    nleft = n;
    while(nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if(nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

// void str_echo(int sockfd)
// {
// 	ssize_t n;
// 	char buf[MAXLINE];
// again:
// 	while((n = read(sockfd, buf, MAXLINE)) > 0)
// 		writen(sockfd, buf, n);
// 	if (n < 0 && errno == EINTR)
// 		goto again;
// 	else if (n < 0)
// 		printf("str_echo: read error");
// 		exit(-1);
// }

void str_echo(int sockfd)
{
    char buff[1024];
    int n = recv(sockfd, buff, 1024, 0);
    buff[n] = '\0';
    printf("connection from %d, %s\n", sockfd, buff);
    send(sockfd, buff, n, 0);
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
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // servaddr.sin_addr.s_addr = inet_addr(IP);
    inet_pton(AF_INET, IP, &servaddr.sin_addr);
    servaddr.sin_port = htons(PORT);

    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(ret == -1) printf("bind error: %d\n", ret);
    ret = listen(listenfd, 5);
    int count = 0;
    pid_t pid;
    for(;;)
    {
        connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
        if((pid = fork()) == 0)
        {
            printf("child process %d\n", ++count);
            close(listenfd);
            str_echo(connfd);
            close(connfd);
            exit(0);
        }
        // str_echo(connfd);
        close(connfd);
    }

    return 0;
}