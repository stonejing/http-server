#include "utils.h"
#include "config.h"
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

int socket_bind_listen_tcp_v4(int port)
{
    if(port < 0 || port > 65535)
        return -1;
    
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    reusesocket(listen_fd);
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((unsigned short)port);
    int ret;
    if((ret = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1))
        return -1;

    if(listen(listen_fd, 5) == -1)
        return -1;
    
    if(listen_fd == -1)
    {
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}