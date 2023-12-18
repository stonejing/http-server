#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

void dg_echo(int sockfd, struct sockaddr* pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char mesg[1024];

    for (;;) {
        len = clilen;
        n = recvfrom(sockfd, mesg, 1024, 0, pcliaddr, &len);
        mesg[n] = '\0';
        printf("recvfrom %s\n", mesg);
        sendto(sockfd, mesg, n, 0, pcliaddr, len);
    }
}

int main(void)
{
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //
    servaddr.sin_port        = htons(9000);      //
    int ret = bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (ret < 0) {
        printf("bind error.\n");
        return -1;
    }

    dg_echo(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}