#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include "socks5.h"
#include "dbg.h"

#define MAX_EVENTS 10
#define PORT 2344
#define BUFFSIZE 1024

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

int hex2int(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

enum server_state { IDLE = 0, RESPONSE, RUN, REPLY, STOP };

server_state s = RESPONSE;
const int BUF_SIZE = 4096;

void forward_data(int src_sock, int dst_sock)
{
    char buffer[4096];
    ssize_t n;
    n = recv(src_sock, buffer, 4096, 0);
    if(n > 0)
    {
        log_info("forward data.");
        send(dst_sock, buffer, n, 0);
    }
}

void print_hex(char* buf, int length)
{
    for(int i = 0; i < length; i++)
    {
        log_info("buf: %02x", buf[i]);
    }
}

void server_recv_cb(int server_sockfd)
{
    char buf[BUF_SIZE];
    memset(buf, '\0', BUF_SIZE);

    int remote_sockfd = 0;
    struct sockaddr_in serv_addr;
    char serv_ip[16];
    char serv_port[4];

    remote_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(remote_sockfd != -1);

    int maxfd, ret;
    fd_set rd_set;
    size_t n_read;
    char buffer_r[4096];

    maxfd = (server_sockfd > remote_sockfd) ? server_sockfd : remote_sockfd;

    while(1)
    {
        ssize_t r = recv(server_sockfd, buf, BUF_SIZE, 0);
        if(r <= 0) continue;
        switch (s)
        {
        case IDLE:
        {
            s = RESPONSE;
            break;
        }
        case RESPONSE:
        {
            struct method_select_response response_se;
            struct method_select_request request_se;

            // struct method_select_request* request_se = (struct method_select_request*)buf;
            log_info("size of request_se: %ld", sizeof(request_se));
            char test[5] = {0x01, 0x05, 0x08, '4', '5'};

            memcpy(&request_se, test, sizeof(request_se));
            memcpy(&request_se, buf, sizeof(request_se));

            log_info("method_select_request ver: %02x, char: %02x", request_se.ver, buf[0]);
            log_info("method_select_request nmethods: %02x", request_se.nmethods);
            log_info("method_select_request methods: %02x", request_se.methods[0]);
            response_se.ver = SVERSION;
            response_se.method = 0x00;
            char* send_buf_response = (char*)&response_se;
            send(server_sockfd, send_buf_response, sizeof(response_se), 0);
            s = REPLY;
            break;
        }
        case RUN:
        {
            log_info("run.");
            while(1)
            {
                FD_ZERO(&rd_set);
                FD_SET(server_sockfd, &rd_set);
                FD_SET(remote_sockfd, &rd_set);
                ret = select(maxfd + 1, &rd_set, NULL, NULL, NULL);
                if(ret < 0 && errno == EINTR) continue;
                if(FD_ISSET(server_sockfd, &rd_set))
                {
                    n_read = recv(server_sockfd, buffer_r, 4096, 0);
                    if(n_read <= 0)
                    {
                        log_err("server_sockfd.");
                        break;
                    }
                    send(remote_sockfd, (const void*)buffer_r, n_read, 0);
                }
                if(FD_ISSET(remote_sockfd, &rd_set))
                {
                    n_read = recv(remote_sockfd, buffer_r, 4096, 0);
                    if(n_read <= 0)
                    {
                        log_err("remote_sockfd.");
                        break;
                    }
                    send(server_sockfd, (const void*)buffer_r, n_read, 0);
                }
            }
            log_info("bye.");
            s = STOP;
            break;     
        }
        case REPLY:
        {
            log_info("REPLY event.");
            struct socks5_response response;
            struct socks5_request* request = (struct socks5_request*)buf;

            log_info("ver: %02x", request->ver);
            log_info("cmd: %02x", request->cmd);
            log_info("rsv: %02x", request->rsv);
            log_info("atyp: %02x", request->atyp);
            char addr[4];
            char port[2];

            size_t in_addr_len = sizeof(struct in_addr);
            log_info("in_addr_len: %ld", in_addr_len);
            memcpy(addr, buf + 4, in_addr_len);
            memcpy(port, buf + in_addr_len + 4, 2);

            for(int i = 0; i < 4; i++)
            {
                log_info("addr[%d]: %04x", i, (unsigned char)addr[i]);
            }

            // serv_addr.s_addr = htol()
            int number = (int)strtol(addr, NULL, 16);

            char test = 0x61;
            log_info("test char: %c", test);
            log_info("test int: %d", (int)test);
            log_info("ip 1: %d", (int)(unsigned char)addr[0]);

            if(0x95 > 'a')
            {
                log_info("hex compare test.");
            }
            int num1 = (int)(unsigned char)addr[0];
            int num2 = (int)(unsigned char)addr[1];
            int num3 = (int)(unsigned char)addr[2];
            int num4 = (int)(unsigned char)addr[3];
            // log_info("IP: %d %d %d %d", num1, num2, num3, num4);

            char ip_addr[16];
            char port_string[4];
            snprintf( serv_ip, sizeof(serv_ip), "%d.%d.%d.%d", 
            (unsigned char)addr[0], (unsigned char)addr[1], (unsigned char)addr[2], (unsigned char)addr[3]);

            log_info("IP: %s", serv_ip);

            int port_num = (int)(unsigned char)port[0] * 100 + (int)(unsigned char)port[1];
            sprintf(serv_port, "%d", port_num);
            log_info("PORT: %s", serv_port);

            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port_num);
            if(inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr)<=0) 
            {
                printf("\nInvalid address/ Address not supported \n");
            }
        
            if (connect(remote_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            {
                printf("\nConnection Failed \n");
            }
            
            char addr_to_send[256];
            unsigned char addr_len = 0;
            addr_to_send[addr_len++] = request->atyp;
            response.ver = SVERSION;
            response.rep = 0;
            response.rsv = 0;
            response.atyp = 1;
            char* send_buf_REPLY = (char*)&response;
            char send_all[256];

            struct in_addr sin_addr;
            inet_aton("0.0.0.0", &sin_addr);

            memcpy(send_all, &response, 4);
            memcpy(send_all + 4, &sin_addr, sizeof(struct in_addr));
            *((unsigned short *)(send_all + 4 + sizeof(struct in_addr))) 
                = (unsigned short) htons(atoi("2345"));

            int reply_size = 4 + sizeof(struct in_addr) + sizeof(unsigned short);
            int r = send(server_sockfd, send_all, reply_size, 0);

            log_info("send response.");
            send(server_sockfd, send_buf_REPLY, sizeof(response), 0);
            log_info("send completed.");
            s = RUN;
            break;
        }
        default:
            break;
        }
        memset(buf, '\0', BUF_SIZE);
    }
}

int main()
{
    struct epoll_event ev, events[MAX_EVENTS];  
    int addrlen, listenfd, conn_sock, nfds, epfd, fd, i, nread, n;  
    struct sockaddr_in local, remote;  
    char buf[BUFSIZ];  
  
    //创建listen socket  
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
        perror("sockfd\n");  
        exit(1);  
    }

    // 重用 bind 地址，方便调试，一般正式程序不能添加
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &reuse, sizeof(reuse));

    setnonblocking(listenfd);  
    bzero(&local, sizeof(local)); 

    local.sin_family = AF_INET;  
    local.sin_addr.s_addr = htonl(INADDR_ANY);;  
    local.sin_port = htons(PORT);  

    if( bind(listenfd, (struct sockaddr *) &local, sizeof(local)) < 0) {  
        perror("bind\n");  
        exit(1);  
    }  
    int ret = listen(listenfd, 20);  
    assert(ret != -1);

    /* epoll 的使用过程 */
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
            printf("EPOLL START.\n");
            fd = events[i].data.fd; 
            printf("fd is %d.\n", fd); 
            if (fd == listenfd) {  
                while ((conn_sock = accept(listenfd,(struct sockaddr *) &remote,   
                                (socklen_t*)&addrlen)) > 0) {  
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
                log_info("server the socks5.");
                server_recv_cb(fd);
                n = 0;
                if(read(fd, buf + n, BUFSIZ - 1) <= 0)
                {
                    printf("close connection.\n");
                    close(fd);
                    continue;
                }
                while ((nread = read(fd, buf + n, BUFSIZ-1)) > 0) {  
                    n += nread;  
                }  
                if (nread == -1 && errno != EAGAIN) {  
                    perror("read error");  
                }  
                ev.data.fd = fd;  
                ev.events = events[i].events | EPOLLOUT;
                if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {  
                    perror("epoll_ctl: mod");  
                }
                printf("epoll in.\n");  
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
                close(fd);
            }  
            else
            {
                printf("undefined error.\n");
            }
        }  
    }  
  
    return 0; 
}