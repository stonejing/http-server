#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include "dbg.hpp"

#define MAX_EVENTS 10
#define PORT 2566
#define BUFFSIZE 1024

#define BUFFER_SIZE 2048

void setnonblocking(int sockfd)
{
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if(opts < 0)
    {
        perror("fcntl(F_GETFL).\n");
        exit(1);
    }
    opts = (opts | O_NONBLOCK);  
    if(fcntl(sockfd, F_SETFL, opts) < 0) {  
        perror("fcntl(F_SETFL)\n");  
        exit(1);  
    }  
}

/* 主状态机两种可能状态，分析请求行和分析头部字段 */
enum CHECK_STATE { CHECK_STATE_REQUEST = 0, CHECK_STATE_HEADER };
/* 从状态机，行读取状态，读取到完整行，行出错，行数据不完整 */
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
/*  服务器处理 HTTP 请求的结果 
    NO_REQUEST          请求不完整，需要继续读取客户数据
    GET_REQUEST         获得完整客户请求，GET 方法
    BAD_REQUEST         客户 HTTP 请求有语法错误
    FORBIDDEN_REQUEST   客户对访问资源没有权限
    INTERNAL_ERROR      服务器内部错误
    CLOSED_CONNECTION   客户端已经关闭连接
*/
enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST,
                FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
/*
    随便发送了一个信息
*/
static const char* szret[] = { "I get a correct result\n", "Something wrong.\n" };

/*
    从状态机，用于找到一行的起始和终止位置
*/
LINE_STATUS parse_line(char* buffer, int& checked_index, int& read_index)
{
    char temp;
    for(; checked_index < read_index; ++checked_index)
    {
        temp = buffer[checked_index];
        if(temp == '\r')
        {
            if((checked_index + 1) == read_index)
            {
                return LINE_OPEN;
            }
            else if(buffer[checked_index + 1] == '\n')
            {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(temp == '\n')
        {
            if((checked_index > 1) && buffer[checked_index - 1] == '\r')
            {
                buffer[checked_index-1] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/*
    分析请求行
*/
HTTP_CODE parse_requestline(char* temp, CHECK_STATE& checkstate)
{
    char* url = strpbrk(temp, " \t");
    if(!url)
    {
        return BAD_REQUEST;
    }
    *url++ = '\0';

    char* method = temp;
    if(strcasecmp(method, "GET") == 0)
    {
        printf("The request method is GET.\n");
    }
    else
    {
        return BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char* version = strpbrk(url, " \t");
    if(!version)
    {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    if(strcasecmp(version, "HTTP/1.1") != 0)
    {
        return BAD_REQUEST;
    }
    if(strncasecmp(url, "http://", 7) == 0)
    {
        printf("impossible.\n");
        url += 7;
        url = strchr(url, '/');
    }
    if(!url || url[0] != '/')
    {
        return BAD_REQUEST;
    }
    // printf("The request URL is: %s.\n", url);
    log_info("The request URI is: %s", url);
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}
/*
    分析头部字段
*/
HTTP_CODE parse_headers(char* temp)
{
    if(temp[0] == '\0')
    {
        return GET_REQUEST;
    }
    else if(strncasecmp(temp, "Host:", 5) == 0)
    {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is: %s.\n", temp);
    }
    else
    {
        printf("I can not handle this header.\n");
    }
    return NO_REQUEST;
}
/*
    分析 HTTP 请求的入口函数
*/
HTTP_CODE parse_content(char* buffer, int& checked_index, CHECK_STATE& checkstate, int& read_index, int& start_line)
{
    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;
    while((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK)
    {
        char* temp = buffer + start_line;
        start_line = checked_index;
        switch (checkstate)
        {
        case CHECK_STATE_REQUEST:
            retcode = parse_requestline(temp, checkstate);
            if(retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            break;
        case CHECK_STATE_HEADER:
            retcode = parse_headers(temp);
            if(retcode == BAD_REQUEST)
            {
                return BAD_REQUEST;
            }
            else if(retcode == GET_REQUEST)
            {
                return GET_REQUEST;
            }
            break;
        
        default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    if(linestatus == LINE_OPEN)
    {
        return NO_REQUEST;
    }
    else
    {
        return BAD_REQUEST;
    }
}

int main()
{
    struct epoll_event ev, events[MAX_EVENTS];  
    int listenfd, conn_sock, nfds, epfd, fd, i, nread, n;  
    
    struct sockaddr_in local, remote;  
    socklen_t addrlen = sizeof(remote);
    char buf[BUFFER_SIZE];  
  
    //创建listen socket  
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
        perror("sockfd\n");  
        exit(1);  
    }

    // 重用 bind 地址，方便调试，一般正式程序不能添加
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    setnonblocking(listenfd);  
    bzero(&local, sizeof(local));  
    local.sin_family = AF_INET;  
    local.sin_addr.s_addr = htonl(INADDR_ANY);;  
    local.sin_port = htons(PORT);  
    if( bind(listenfd, (struct sockaddr *) &local, sizeof(local)) < 0) {  
        perror("bind\n");  
        exit(1);  
    }  
    listen(listenfd, 20);  
  
    epfd = epoll_create(MAX_EVENTS);  
    if (epfd == -1) {  
        perror("epoll_create");  
        exit(EXIT_FAILURE);  
    }  
  
    ev.events = EPOLLIN;  
    ev.data.fd = listenfd;  
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {  
        perror("epoll_ctl: listen_sock");  
        exit(EXIT_FAILURE);  
    }  
  
    for (;;) {  
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);  
        if (nfds == -1) {  
            perror("epoll_pwait");  
            exit(EXIT_FAILURE);  
        }  
  
        for (i = 0; i < nfds; ++i) {  
            fd = events[i].data.fd;  
            if (fd == listenfd) {  
                while ((conn_sock = accept(listenfd, (struct sockaddr *) &remote,   
                                &addrlen)) > 0) {  
                    setnonblocking(conn_sock);  
                    ev.events = EPOLLIN | EPOLLET;  
                    ev.data.fd = conn_sock;  
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock,  
                                &ev) == -1) {  
                        perror("epoll_ctl: add");  
                        exit(EXIT_FAILURE);  
                    }
                    printf("ADDED CONN SOCK.\n");  
                }  
                if (conn_sock == -1) {  
                    if (errno != EAGAIN && errno != ECONNABORTED   
                            && errno != EPROTO && errno != EINTR)   
                        perror("accept");  
                }
                printf("added sockfd.\n");  
                continue;  
            }    
            else if (events[i].events & EPOLLIN) {  
                n = 0;
                int read_index = 0;
                int checked_index = 0;
                int start_line = 0;
                int data_read = 0;
                CHECK_STATE checkstate = CHECK_STATE_REQUEST;
                printf("epoll in.\n");

                while(1)
                {
                    data_read = recv(fd, buf + read_index, BUFFER_SIZE - read_index, 0);
                    if(data_read <= 0) break;
                    read_index += data_read;
                    // printf("data_read: %d.\n", data_read);
                    log_info("data_read length: %d", data_read);
                    // printf("read buf:\n%s\ndata_read length: %d.\n", buf, data_read);
                    log_info("read buf:\n%s", buf);
                }
                        
                while(1)
                {
                    HTTP_CODE result = parse_content(buf, checked_index, checkstate, read_index, start_line); 

                    if(result == NO_REQUEST)
                    {
                        // printf("NOE REQUEST.\n");
                        log_warn("NO REQUEST.");
                        continue;
                    }
                    else if(result == GET_REQUEST)
                    {
                        ev.data.fd = fd;  
                        ev.events = events[i].events | EPOLLOUT;
                        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {  
                            perror("epoll_ctl: mod");  
                        }
                        printf("epoll in.\n");
                        break;  
                    }
                    else
                    {
                        ev.data.fd = fd;  
                        ev.events = events[i].events | EPOLLOUT;
                        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {  
                            perror("epoll_ctl: mod");  
                        }
                        printf("epoll error.\n");
                        break;                         
                    }
                }
            }  
            else if (events[i].events & EPOLLOUT) {  
                sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nHello World", 11);  
                int nwrite, data_size = strlen(buf);  
                n = data_size;  
                while (n > 0) {  
                    nwrite = write(fd, buf + data_size - n, n);  
                    if (nwrite < n) {  
                        if (nwrite == -1 && errno != EAGAIN) {  
                            perror("write error");  
                        }  
                        break;  
                    }  
                    n -= nwrite;  
                }
                printf("epoll out.\n");
                // events[i].events = EPOLLIN;
                ev.data.fd = fd;
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
                // 这里写完都直接关闭连接了，每一次都是新的连接  
                log_info("close connection.");
                close(fd);
            }  
            else
            {
                log_err("undefined error.");
            }
        }  
    }  
  
    return 0; 
}