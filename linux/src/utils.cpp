#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include "log.h"
#include "utils.h"

int socket_bind_listen_tcp_v4(int port)
{
    CLogger& Log = CLogger::getInstance();
    if(port < 0 || port > 65535)
        return -1;
    
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        LOG_ERR("server create socket fd: %d", listen_fd);
        return -1;
    }
    LOG_INFO("server create socket fd: %d", listen_fd);

    reusesocket(listen_fd);
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((unsigned short)port);
    int ret;
    if((ret = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1))
    {
        LOG_ERR("server bind err: %d", listen_fd);
        return -1;
    }
    LOG_INFO("server bind socket: %d", listen_fd);
    
    if(listen(listen_fd, 5) == -1)
    {
        LOG_ERR("server listen error: %d", listen_fd);
        return -1; 
    }
    
    if(setnonblocking(listen_fd) == -1)
    {
        LOG_ERR("set listen fd non blocking.", listen_fd); 
        abort();
    }
    
    return listen_fd;
}