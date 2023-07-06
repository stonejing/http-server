#pragma once

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <string>
#include <iostream>

inline int create_tcp_v4_listen_socket()
{
    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listen_fd != -1);
    return listen_fd;
}

int socket_bind_listen_tcp_v4(int port);

// blocked read, but we can not do this.
inline ssize_t readn(int fd, void* buff, size_t n)
{
    size_t nleft = n;
    ssize_t nread = 0;
    char *ptr = (char*)buff;
    std::cout << "read n" << std::endl;
    while(nleft > 0)
    {
        if((nread = read(fd, ptr, nleft)) < 0)
        {
            if(errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if(nread == 0)
            break;
        
        nleft -= nread;
        ptr += nread;
    }
    return (n - nleft);
}

// blocked write
inline ssize_t writen(int fd, void* buff, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten = 0;
    char * ptr = (char*)buff;
    while(nleft > 0)
    {
        if((nwritten = write(fd, ptr, nleft)) < 0)
        {
            if(errno == EINTR)
                nwritten = 0;
            else
                return -1;
        } else if(nwritten == 0)
            break;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

// non block read
inline int nonblock_read(int fd, void* buff, size_t n)
{
    return 0;
}

// non block write
inline int nonblock_write(int fd, void* buff, size_t n)
{
    return 0;
}


inline int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

inline void reusesocket(int fd)
{
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
}

inline void setsocketnodelay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

inline void setsocketnolinger(int fd)
{
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char*)&linger_, sizeof(linger_));
}