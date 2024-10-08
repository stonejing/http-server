#include <sys/socket.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <fcntl.h>

#define BUFFER_SIZE 64

int main(void)
{
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8000);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    if(connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        printf("connection failed.\n");
        close(sockfd);
        return -1;
    }

    pollfd fds[2];
    // standard input file descriptor: 0
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents =0;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents =0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);

    while(1)
    {
        ret = poll(fds, 2, -1);
        if(ret < 0)
        {
            printf("poll failure.\n");
            break;
        }
        if(fds[1].revents & POLLRDHUP)
        {
            printf("server close the connection.\n");
            break;
        }
        else if(fds[1].revents & POLLIN)
        {
            memset(read_buf, '\0', BUFFER_SIZE);
            recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
            printf("recv: %s\n", read_buf);
        }
        if(fds[0].revents & POLLIN)
        {
            ret = splice(0, NULL, pipefd[1], NULL,
                32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL, sockfd, NULL,
                32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(sockfd);
    return 0;
}