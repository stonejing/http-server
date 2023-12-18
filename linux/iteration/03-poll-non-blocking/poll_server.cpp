#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define USER_LIMIT 1000
#define BUFFER_SIZE 1024
#define FD_LIMIT 65535

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

    pollfd fds[USER_LIMIT + 1];
    int user_counter = 0;
    for(int i = 1; i < USER_LIMIT; i++)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }   
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    for(;;)
    {
        ret = poll(fds, user_counter + 1, -1);
        if (ret < 0)
        {
            printf("poll error: %d\n", ret);
            break;
        }
        for(int i = 0; i < user_counter + 1; ++i)
        {
            if((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))
            {
                int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len); 
                if(connfd < 0)
                {
                    printf("accept error: %d\n", connfd);
                    continue;
                }
                if(user_counter >= USER_LIMIT)
                {
                    printf("too many users.\n");
                    close(connfd);
                    continue;
                }
                user_counter++;
                setnonblocking(connfd);
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_counter].revents = 0;
                printf("new client: %d\n", connfd);
            }
            else if(fds[i].revents & POLLERR)
            {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, 0, sizeof(errors));
                socklen_t len = sizeof(errors);
                if(getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0)
                {
                    printf("get socket option failed.\n");
                }
                continue;
            }
            else if(fds[i].revents & POLLRDHUP)
            {
                printf("client closed connection.\n");
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                i--;
                user_counter--;
            }
            else if(fds[i].revents & POLLIN)
            {
                char buff[1024];
                int n = recv(fds[i].fd, buff, 1024, 0);
                if(n == 0)
                {
                    printf("client closed connection.\n");
                    close(fds[i].fd);
                    fds[i] = fds[user_counter];
                    i--;
                    user_counter--;
                }
                else if(n > 0)
                {
                    printf("get message from %d: %s\n", fds[i].fd, buff);
                    send(fds[i].fd, buff, n, 0);
                }
                else {
                    printf("error: %d.\n", errno);
                    close(fds[i].fd);
                    fds[i] = fds[user_counter];
                    i--;
                    user_counter--;
                }
            }
            else {
                printf("something else happened.\n");
            }
        }
    }

    return 0;
}