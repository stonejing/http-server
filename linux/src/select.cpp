#include "utils.h"
#include <cstring>
#include <sys/select.h>
#include <sys/socket.h>
#include <iostream>

int main(void)
{
    int listen_fd = socket_bind_listen_tcp_v4(8000);
    reusesocket(listen_fd);
    assert(listen_fd != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);

    char buff[1024];
    int maxfd = listen_fd;
    fd_set read_fd;
    fd_set exception_fd;
    fd_set read_tmp;
    FD_ZERO(&read_fd);
    FD_ZERO(&read_tmp);
    FD_ZERO(&exception_fd);

    FD_SET(listen_fd, &read_fd);

    printf("START SELECT SERVER: 127.0.0.1:8000, listen fd: %d\n", listen_fd);
    std::cout << maxfd << std::endl;
    while(1)
    {
        read_tmp = read_fd;
        int ret = select(maxfd + 1, &read_tmp, NULL, NULL, NULL);
        if(ret < 0)
        {
            printf("selection failure.\n");
            break;
        }
        if(FD_ISSET(listen_fd, &read_tmp))
        {
            int accept_fd = accept(listen_fd, (struct sockaddr*)&client_address, &client_addrlength);
            if(accept_fd < 0)
            {
                printf("accpet failure.\n");
            }
            else 
            {
                FD_SET(accept_fd, &read_fd);
                maxfd = accept_fd > maxfd ? accept_fd : maxfd;
            }
        }
        for(int i = 3; i < maxfd + 1; i++)
        {
            if(i != listen_fd && FD_ISSET(i, &read_tmp))
            {
                int len = read(i, buff, sizeof(buff));
                if(len == 0)
                {
                    printf("客户端 %d 关闭了连接...\n", i);
                    FD_CLR(i, &read_fd);
                    close(i);
                }
                else if(len > 0)
                {
                    write(i, buff, strlen(buff)+1);
                }
                else
                {
                    perror("read");
                }
            }
        }
    }

    return 0;
}