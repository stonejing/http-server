#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <signal.h>
#include <netdb.h>
#include <openssl/evp.h>
#include <mbedtls/md.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <mbedtls/cipher.h>
#include <pthread.h>

/*
    岂可修，那个单纯用来转发的程序被自己改没了
*/

#define BUFFER_SIZE 1024

typedef mbedtls_cipher_info_t cipher_kt_t;
typedef mbedtls_cipher_context_t cipher_evp_t;
typedef mbedtls_md_info_t digest_type_t;
#define MAX_MD_SIZE MBEDTLS_MD_MAX_SIZE
#define max(a,b) (((a)>(b))?(a):(b))

static uint8_t enc_key[32];
static int enc_key_len;
static int enc_iv_len;

struct enc_ctx
{
    uint8_t init;
    EVP_CIPHER_CTX* evp;
};

struct para
{
    fd_set* readfds;
    int* fd_array;
    int m_listenfd;
};

struct server
{
    int fd_recv;
    char buf[BUFFER_SIZE];
    int buf_len;
    char stage;     /* socks5 握手阶段 */
    bool status;    /* 选择数据 encrypt 还是 decrypt */
    int fd_send;    /* 远程 fd 接收，加密，本地 fd 发送；本地 fd 接收， 解密， 远程 fd 发送 */
};

struct data_map
{
    int fd;
    char buf[BUFFER_SIZE];
    bool status;
};

void enc_ctx_init(struct enc_ctx *ctx, int enc)
{
    const EVP_CIPHER *cipher = EVP_get_cipherbyname("aes-256-cfb");
    if (cipher == NULL)
    {
        printf("Cipher %s not found in OpenSSL library", "aes-256-cfb");
    }
    memset(ctx, 0, sizeof(struct enc_ctx));
    ctx->evp = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX *evp = ctx->evp;
    EVP_CIPHER_CTX_init(evp);
    if (!EVP_CipherInit_ex(evp, cipher, NULL, NULL, NULL, enc))
    {
        printf("Cannot initialize cipher %s", "aes-256-cfb");
        exit(EXIT_FAILURE);
    }
    if (!EVP_CIPHER_CTX_set_key_length(evp, enc_key_len))
    {
        EVP_CIPHER_CTX_cleanup(evp);
        printf("Invalid key length: %d", enc_key_len);
        exit(EXIT_FAILURE);
    }
    EVP_CIPHER_CTX_set_padding(evp, 1);
}

void enc_key_init(const char *pass)
{
    OpenSSL_add_all_algorithms();
    uint8_t iv[EVP_MAX_IV_LENGTH];
    const EVP_CIPHER *cipher = EVP_get_cipherbyname("aes-256-cfb");
    if (cipher == NULL)
    {
        perror("method no supported.");
        return;
    }

    enc_key_len = EVP_BytesToKey(cipher, EVP_md5(), NULL, (uint8_t *)pass,
                                 strlen(pass), 1, enc_key, iv);
    enc_iv_len = EVP_CIPHER_iv_length(cipher);
}

char* ss_encrypt(int buf_size, char *plaintext, ssize_t *len, struct enc_ctx *ctx)
{
        int c_len = *len + 32;
        int iv_len = 0;
        int err = 0;
        char *ciphertext = (char*)malloc(max(iv_len + c_len, buf_size));

        if (!ctx->init)
        {
            uint8_t iv[EVP_MAX_IV_LENGTH];
            iv_len = enc_iv_len;
            RAND_bytes(iv, iv_len);
            EVP_CipherInit_ex(ctx->evp, NULL, NULL, enc_key, iv, 1);
            memcpy(ciphertext, iv, iv_len);
            ctx->init = 1;
        }

        err = EVP_EncryptUpdate(ctx->evp, (uint8_t*)(ciphertext+iv_len),
                                &c_len, (const uint8_t *)plaintext, *len);
        if (!err)
        {
            free(ciphertext);
            free(plaintext);
            return NULL;
        }

        *len = iv_len + c_len;
        free(plaintext);
        return ciphertext;
}

char* ss_decrypt(int buf_size, char *ciphertext, ssize_t *len, struct enc_ctx *ctx)
{

        int p_len = *len + 32;
        int iv_len = 0;
        int err = 0;
        char *plaintext = (char*)malloc(max(p_len, buf_size));

        if (!ctx->init)
        {
            uint8_t iv[EVP_MAX_IV_LENGTH];
            iv_len = enc_iv_len;
            memcpy(iv, ciphertext, iv_len);
            EVP_CipherInit_ex(ctx->evp, NULL, NULL, enc_key, iv, 0);
            ctx->init = 1;
        }

        err = EVP_DecryptUpdate(ctx->evp, (uint8_t*)plaintext, &p_len,
                                (const uint8_t*)(ciphertext + iv_len), *len - iv_len);

        if (!err)
        {
            free(ciphertext);
            free(plaintext);
            return NULL;
        }

        *len = p_len;
        free(ciphertext);
        return plaintext;
}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int readn(int fd, char *buf, int n)
{
	int nread, left = n;
	while (left > 0) {
		if ((nread = read(fd, buf, left)) == -1) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
		} else {
			if (nread == 0) {
				return 0;
			} else {
				left -= nread;
				buf += nread;
			}
		}
	}
	return n;
}

int writen(int fd, char *buf, int n)
{
    int left = n;
	while (left > 0) 
    {
        // printf("write fd: %d buf: %hhx: left: %d\n", fd, buf[0], left);
        int nwrite = write(fd, buf, left);
		if (nwrite == -1)
        {
            if(errno == EPIPE)
            {
                perror("remote connection closed.\n");
                return -1;
            }
			else if (errno == EINTR || errno == EAGAIN) 
            {
                perror("errno write: \n");
				continue;
			}
            else
            {
                perror("write error.\n");
            }
		} 
        else 
        {
			if (nwrite == n) 
            {
				return 0;
			} 
            else 
            {
				left -= nwrite;
				buf += nwrite;
			}
		}
	}
	return n;
}

/* 返回 connect fd */
int app_connect(int type, char* ip, unsigned short int portnum)
{
    int fd;
    struct sockaddr_in remote;
    char address[16];
    memset(address, 0, sizeof(address));

    if(type == 1)
    {
        snprintf(address, sizeof(address), "%hhu.%hhu.%hhu.%hhu",
                ip[0], ip[1], ip[2], ip[3]);
        memset(&remote, 0, sizeof(remote));
        remote.sin_family = AF_INET;
        remote.sin_addr.s_addr = inet_addr(address);
        remote.sin_port = portnum;

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(fd, (struct sockaddr *)&remote, sizeof(remote)) < 0) 
        {
            close(fd);
            return -1;
        }
        return fd;
    }
    else if(type == 2)
    {
		char portaddr[6];
		struct addrinfo *res;
		snprintf(portaddr, sizeof(portaddr), "%d", ntohs(portnum));
		// printf("getaddrinfo: %s %s\n", ip, portaddr);
		int ret = getaddrinfo(ip, portaddr, NULL, &res);
		if (ret == EAI_NODATA) 
        {
			return -1;
		} 
        else if (ret == 0) 
        {
			struct addrinfo *r;
			for (r = res; r != NULL; r = r->ai_next) 
            {
				fd = socket(r->ai_family, r->ai_socktype,
					    r->ai_protocol);
                if (fd == -1) 
                {
                    continue;
                }
				ret = connect(fd, r->ai_addr, r->ai_addrlen);
				if (ret == 0) 
                {
					freeaddrinfo(res);
					return fd;
                } 
                else 
                {
                    close(fd);
                }
			}
		}
		freeaddrinfo(res);
		return -1;        
    }
    return -1;
}

void *socks5_handshake(void* arg)
{
    para *tmp = (struct para*)arg;
    int m_listenfd = tmp->m_listenfd;
    fd_set* readfds = tmp->readfds;
    int* fd_array = tmp->fd_array;

    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address); 
    int client_sockfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_len); 
    if(client_sockfd < 2)
    {
        pthread_exit(0);
    } 
    /* 
        socks5 handshake
        多次 read 肯定会造成性能下降
        最佳做法是读到不能再读，然后对 buffer 进行处理
        激进的是，读写线程分离，使用 buffer 共享数据
    */
    /* recv client method select message*/
    char init[3] = {0};
    char response_init[2] = {0x05, 0x00};
    int nread = readn(client_sockfd, init, 3);
    printf("%hhx %hhx %hhx\n", init[0], init[1], init[2]);
    /* only support socks5 protocol */
    if(init[0] != 0x05) pthread_exit(0);
    /* send server methdos select message */
    write(client_sockfd, response_init, 2);
    /* recv client requests command */
    char command[4] = {0};
    readn(client_sockfd, command, 4);
    printf("Command: %hhu %hhu %hhu %hhu\n", command[0], command[1], command[2], command[3]);
    /* recv client requests IP */
    if(command[3] == 0x01)
    {
        // printf("cliend sockfd: %d\n", client_sockfd);
        char ip[4];
        readn(client_sockfd, ip, 4);
        printf("IP %hhu.%hhu.%hhu.%hhu\n", ip[0], ip[1], ip[2], ip[3]);
        unsigned short int p;
        readn(client_sockfd, (char*)&p, sizeof(p));
        printf("port %hu\n", ntohs(p));
        int inet_fd = app_connect(1, ip, p);
        // printf("remote connected: %d\n", inet_fd);

        if(inet_fd <= 0) 
        {
            printf("wo ye bu zhi dao wei shen me hui shi bai.\n");
            pthread_exit(0);
        }

        char response[4] = {0x05, 0x00, 0x00, 0x01};
        writen(client_sockfd, &response[0], 4);
        writen(client_sockfd, ip, 4);
        writen(client_sockfd, (char*)&p, sizeof(p));
        
        fd_array[client_sockfd] = inet_fd;
        fd_array[inet_fd] = client_sockfd;

        FD_SET(client_sockfd, readfds);
        FD_SET(inet_fd, readfds);
        printf("add accept fd: %d connect fd: %d\n", client_sockfd, inet_fd);
    }
    else if (command[3] == 0x03)
    {
        // printf("read domain.\n");
        unsigned char s;
        int ret = readn(client_sockfd, (char *)&s, sizeof(s));
        char *address = (char *)malloc((sizeof(char) * s) + 1);
        readn(client_sockfd, (char *)address, (int)s);
        address[s] = 0;
        printf("Address %s\n", address);
        unsigned short int p;
        readn(client_sockfd, (char*)&p, sizeof(p));
        printf("port %hu\n", ntohs(p));
        printf("remote connect.\n");
        int remote_fd = app_connect(2, address, p);
        printf("remote fd: %d\n", remote_fd);

        char response[4] = {0x05, 0x00, 0x00, 0x03};
        writen(client_sockfd, &response[0], 4);
        writen(client_sockfd, (char*)&s, sizeof(s));
        writen(client_sockfd, address, s * sizeof(char));
        writen(client_sockfd, (char*)&p, sizeof(p));
        
        fd_array[client_sockfd] = remote_fd;
        fd_array[remote_fd] = client_sockfd;

        FD_SET(client_sockfd, readfds);
        FD_SET(remote_fd, readfds);
    } 
    printf("----pthread exit.----\n");   
    pthread_exit(0);
    return NULL;
}

/*
    从 connect fd recv 数据，encrypt，从 accept fd send 数据
*/
void server_recv(struct server* data)
{
    char* buf = data->buf;
    int* buf_len = &data->buf_len;

    while(1)
    {
        ssize_t r= recv(data->fd_recv, buf, BUFFER_SIZE, 0);
        if(r == 0)
        {
            return;
        }
        else if(r < 0)
        {

        }
    }
    switch (data->stage)
    {
    case 0x01:
    {
        break;
    }
    case 0x02:
        break;
    default:
        break;
    }
}

/* 
    服务器，从远程 fd 接收数据，发送数据到本地 fd 
    recv data from connect fd, send data to accept fd
    这个只是单纯的 send，因为可能各种原因，send 不成功
*/
void server_send(struct server* data)
{
    if(data->buf_len == 0)
    {
        return;
    }
    else
    {
        ssize_t r = send(data->fd_send, data->buf, data->buf_len, 0);
        if(r < 0)
        {
            perror("send");
            if(errno != EAGAIN)
            {
                return;
            }
            return;
        }
        if(r < data->buf_len)
        {
            char* pt;
            for(pt = data->buf; pt < pt + std::min((int)r, BUFFER_SIZE); pt++)
            {
                *pt = *(pt + r);
            }
            data->buf_len -= r;
        }
        else
        {
            /* all send out */
            return;
        }
    }
}

int main()
{
    // 网络编程基础步骤，设置 socket 的基础步骤，TCP 连接
    int m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    signal(SIGPIPE, SIG_IGN);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(1081);
    address.sin_addr.s_addr = INADDR_ANY;

    int reuse = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct linger tmp = {1, 0};
    setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(m_listenfd, 5);
    assert(ret != -1);

    int fdopt = setnonblocking(m_listenfd);

    enc_key_init("stonejing");

    // select 初始化
    fd_set readfds;
    fd_set tmpfds;
    FD_ZERO(&readfds);
    FD_SET(m_listenfd, &readfds);
    struct sockaddr_in client_address;
    int fd_array[FD_SETSIZE] = {0};
    int encrypt_array[FD_SETSIZE] = {0};
    int decrypt_array[FD_SETSIZE] = {0};
    char response_init[2] = {0x05, 0x00};

    /* buffer recv from socket*/
    char* buffer = (char*)malloc(BUFFER_SIZE);
    /* encrypt handshake buffer send to remote */
    char* addr_to_send = (char*)malloc(BUFFER_SIZE);
    /* server reply */
    char* server_reply = (char*)malloc(BUFFER_SIZE);
    /* indicate which fd should encrypt or decrypt */
    struct enc_ctx* encrypt_ctx[BUFFER_SIZE];
    for(int i = 0; i < BUFFER_SIZE; i++)
    {
        encrypt_ctx[i] = (struct enc_ctx*)malloc(sizeof(struct enc_ctx*));
    }

    while(1)
    {
        FD_ZERO(&tmpfds);
        tmpfds = readfds;
        int result = select(FD_SETSIZE, &tmpfds, NULL, NULL, NULL);
        if(result < 1)
        {
            perror("select error.");
            exit(1);
        }
        /*
            不使用 result，只是遍历所有数组，再使用 FD_ISSET，其实效果不算太好，有多余操作
        */
        for(int fd = 0; fd < FD_SETSIZE; fd++)
        {
            if(FD_ISSET(fd, &tmpfds))
            {
                if(fd == m_listenfd)
                {
                    /*
                        第一次实现简单 socks5 握手之后，转为多线程可以操作。
                        第二次，先单线程，在之前基础上，实现和远程 shadowsocks 服务通信。
                        这一次还要加上之前的对于数据一次性处理，每次都只是发一个完整的包，所以不会有不完整的数据包。
                        完全不需要循环 recv，需要处理的是错误信号的问题。
                    */
                    // pthread_t worker;
                    // struct para tmp_para;
                    // tmp_para.m_listenfd = m_listenfd;
                    // tmp_para.fd_array = fd_array;
                    // tmp_para.readfds = &readfds;
                    // if(pthread_create(&worker, NULL, &socks5_handshake, (void*)&tmp_para) == 0)
                    // {
                    //     printf("pthread create sucess.\n");
                    //     pthread_detach(worker);
                    // }
                    // else
                    // {
                    //     perror("pthread create error.");
                    // }
                    /* 
                        接收到了一个新的本地连接            accept fd
                        需要发起一个新的和远程服务器的连接  connect fd
                    */
                    printf("socks handshake.\n");
                    struct sockaddr_in remote_addr;
                    remote_addr.sin_family = AF_INET;
                    remote_addr.sin_port = htons(2345);
                    int remote_sock = -1;
                    if ((remote_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                    {
                        printf("\n Socket creation error \n");
                        return -1;
                    }
                    if (connect(remote_sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0)
                    {
                        printf("\nConnection Failed \n");
                        return -1;
                    }
                    
                    socklen_t client_len = sizeof(client_address); 
                    int client_sockfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_len); 
                    if(client_sockfd < 2)
                    {
                        continue;
                    } 

                    // char* client_buf = (char*)malloc(BUFFER_SIZE);
                    memset(buffer, 0, BUFFER_SIZE);
                    ssize_t r = recv(client_sockfd, buffer, BUFFER_SIZE, 0);
                    /* only support socks5 protocol and none method*/
                    if(r != 3) continue;
                    if(buffer[0] != 0x05) continue;
                    /* send server methdos select message to the accept fd*/
                    send(client_sockfd, response_init, 2, 0);
                    /* recv from accept fd */
                    memset(buffer, 0, BUFFER_SIZE);
                    r = recv(client_sockfd, buffer, BUFFER_SIZE, 0);
                    /* remote send handshake buffer */
                    ssize_t addr_len = 0;
                    memset(addr_to_send, 0, BUFFER_SIZE);
                    addr_to_send[addr_len++] = buffer[3];
                    size_t in_addr_len = sizeof(struct in_addr);
                    
                    /* recv client requests IP， IPV4 support */
                    if(buffer[3] == 0x01)
                    {
                        /* copy IP and port */
                        memcpy(addr_to_send + addr_len, buffer + 4, in_addr_len + 2);
                        addr_len += in_addr_len + 2;
                        /* print ip and port */
                        // char ip[4];
                        // memcpy(ip, buffer + 4, 4);
                        // printf("IP %hhu.%hhu.%hhu.%hhu\t", ip[0], ip[1], ip[2], ip[3]);
                        // unsigned short int p;
                        // memcpy((char*)&p, buffer + 8, sizeof(p));
                        // printf("port %hu\n", ntohs(p));

                        /* send encrypted ip and port handshake to server */
                        memset(encrypt_ctx[client_sockfd], 0, sizeof(struct enc_ctx*));
                        enc_ctx_init(encrypt_ctx[client_sockfd], 1);
                        addr_to_send = ss_encrypt(BUFFER_SIZE, addr_to_send, &addr_len, encrypt_ctx[client_sockfd]);
                        r = send(remote_sock, addr_to_send, addr_len, 0);
                        printf("addr to send len: %ld\n", addr_len);
                        
                        char response[4] = {0x05, 0x00, 0x00, 0x01};
                        memset(server_reply, 0, BUFFER_SIZE);
                        memcpy(server_reply, response, 4);
                        memcpy(server_reply + 4, buffer + 4, 6);
                        send(client_sockfd, server_reply, 10, 0); 
                        // send(client_sockfd, buffer, 10, 0);
                    }
                    /* domain support */
                    else if (buffer[3] == 0x03)
                    {
                        printf("read domain.\n");
                        uint8_t name_len = *(uint8_t*)(buffer + 4);
                        printf("name len: %d\n", name_len);
                        addr_to_send[addr_len++] = name_len;
                        memcpy(addr_to_send + addr_len, buffer + 5, name_len + 2);
                        addr_len += name_len + 2;
                        char host[256];
                        uint16_t port = ntohs(*(uint16_t *)(buffer + 4 + 1 + name_len));
                        memcpy(host, buffer + 4 + 1, name_len);
                        host[name_len] = '\0';
                        printf("connect to %s:%d\n", host, port);
                        /* send encrypted domain and port handshake to server */
                        memset(encrypt_ctx[client_sockfd], 0, sizeof(struct enc_ctx*));
                        enc_ctx_init(encrypt_ctx[client_sockfd], 1);
                        addr_to_send = ss_encrypt(BUFFER_SIZE, addr_to_send, &addr_len, encrypt_ctx[client_sockfd]);
                        r = send(remote_sock, addr_to_send, addr_len, 0);

                        char response[4] = {0x05, 0x00, 0x00, 0x03};
                        memset(server_reply, 0, BUFFER_SIZE);
                        memcpy(server_reply, response, 4);
                        memcpy(server_reply + 4, buffer + 4, name_len + 2 + 1);
                        send(client_sockfd, server_reply, name_len + 3 + 4, 0);
                    }
                    else
                    {
                        close(client_sockfd);
                        close(remote_sock);
                        continue;
                        printf("currently unsupported.\n");
                    }
                    /* 
                        设置一对对应关系，不需要哈希表，甚至，如果 fd 过大，只需要重新分配一个大一点的数组就好
                        另一种更好的方式，设置一个数组类
                    */
                    fd_array[client_sockfd] = remote_sock;
                    fd_array[remote_sock] = client_sockfd;
                    /*
                        设置对应 fd 的加解密方式
                    */
                    memset(encrypt_ctx[remote_sock], 0, sizeof(struct enc_ctx*));
                    enc_ctx_init(encrypt_ctx[remote_sock], 0);
                    /*
                        标记 fd 是 client 还是 remote
                    */
                    encrypt_array[client_sockfd] = 1;
                    decrypt_array[remote_sock] = 1;

                    FD_SET(client_sockfd, &readfds);
                    FD_SET(remote_sock, &readfds);
                    printf("add accept fd: %d connect fd: %d\n", client_sockfd, remote_sock);
                }
                else
                {
                    /* 
                        select 返回 fd 的状态时，不知道 fd 是 local 还是 remote，也就不确定是加密还是解密
                        单线程，公用一个 buffer 就好，可惜的就是，会在同一个地方频繁读写
                        这里没有处理一些情况
                        send 缓冲区不够了，没有处理，下次就不知道去哪里了
                        recv 还是一样，后面 len == 0 处理的就是 recv
                    */
                    memset(buffer, 0, BUFFER_SIZE);
                    int len = recv(fd, buffer, BUFFER_SIZE, 0);
                    if(len > 0)
                    {
                        if(encrypt_array[fd])
                        {
                            buffer = ss_encrypt(BUFFER_SIZE, buffer, (ssize_t*)&len, encrypt_ctx[fd]);
                            send(fd_array[fd], buffer, len, 0);
                        }
                        if(decrypt_array[fd])
                        {
                            buffer = ss_decrypt(BUFFER_SIZE, buffer, (ssize_t*)&len, encrypt_ctx[fd]);
                            send(fd_array[fd], buffer, len, 0);
                        }
                        // printf("loop over.\n");
                    }
                    else if(len == 0)
                    {
                        printf("client closed connection.\n");
                        FD_CLR(fd, &readfds);
                        FD_CLR(fd_array[fd], &readfds);
                        
                        close(fd);
                        close(fd_array[fd]);
                        int temp = fd_array[fd];
                        fd_array[fd] = 0;
                        fd_array[temp] = 0;
                        encrypt_array[fd] = 0;
                        encrypt_array[temp] = 0;
                        decrypt_array[fd] = 0;
                        decrypt_array[temp] = 0;
                    }
                    else
                    {
                        printf("error select.\n");
                        if(errno == EAGAIN) break;
                        FD_CLR(fd, &readfds);
                        FD_CLR(fd_array[fd], &readfds);
                        close(fd);
                        close(fd_array[fd]);
                        int temp = fd_array[fd];
                        fd_array[fd] = 0;
                        fd_array[temp] = 0;
                        encrypt_array[fd] = 0;
                        encrypt_array[temp] = 0;
                        decrypt_array[fd] = 0;
                        decrypt_array[temp] = 0;
                    }
                }
            }
        }
    }
    return 0;
}