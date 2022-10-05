#define FD_SETSIZE 1024
#include "dbg.hpp"

#include <WinSock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "encrypt.hpp"

#define PORT 5001
#define DATA_BUFSIZE 1024

#pragma comment(lib, "ws2_32")

/* 
    目前单线程，接收阻塞，没有写满的问题
    一次性加一个 socket，因为没有关联先后关系
    recv 为 0，buffer_len 为 0，关闭两个 socket
    write_status 为 1， send == buffer_len，关闭 send socket
*/
typedef struct _SOCKET_INFORMATION {
	// CHAR buffer[DATA_BUFSIZE];
	WSABUF data_buf;
	SOCKET recv_socket;
	SOCKET send_socket;
    BOOL write_status; /* write_status 为 1，将 send_socket 送入 write_set，*/
    struct _SOCKET_INFORMATION* prev;
    struct _SOCKET_INFORMATION* next;
    enc_ctx*   enc_ctx_encrypt;
    enc_ctx*   enc_ctx_decrypt;                           
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

// Prototypes
BOOL Createsocket_information(LPSOCKET_INFORMATION& tail, SOCKET accept_socket, SOCKET remote_socket);
void Freesocket_information(LPSOCKET_INFORMATION current, LPSOCKET_INFORMATION result);

SOCKET create_and_bind(INT Port)
{
    SOCKET          ListenSocket;
    WSADATA         wsaData;
    SOCKADDR_IN     InternetAddr;
    ULONG           NonBlock = 1;

    if(WSAStartup(0x0202, &wsaData) != 0)
    {
        log_err("WSAStartup() failed with error %d", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    else
    {
        log_info("WSAStartup() is OK!");
    }

    if((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        log_err("WSASocket() failed with error %d",  WSAGetLastError());
        return 1;
    }
    else
    {
        log_info("WSASocket() is OK!");
    }
    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_port = htons(Port);
    InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if(bind(ListenSocket, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
    {
        log_err("bind() failed with error %d", WSAGetLastError());
        return 1;
    }
    else
    {
        log_info("bind() is OK!");
    }
    if(listen(ListenSocket, 5))
    {
        log_err("listen() failed with error %d", WSAGetLastError());
        return 1;
    }
    else
    {
        log_info("listen() is OK!");
    }
	// Change the socket mode on the listening socket from blocking to
	// non-block so the application will not block waiting for requests
    if(ioctlsocket(ListenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        log_err("ioctlsocket() failed with error %d", WSAGetLastError());
        return 1;
    }
    else
    {
        log_info("ioctlsocket() is OK!");
    }
    log_info("start server in 0.0.0.0:%d", Port);
    return ListenSocket;
}


int main(int argc, char** argv)
{
	SOCKET accept_socket;
	FD_SET write_set;
	FD_SET read_set;

    SOCKADDR_IN local_server;
	INT ret;
    INT i;

    DWORD flags = 0;
    ULONG nonblock = 1;
    ULONG block = 0;
    DWORD total = 0;

    LPSOCKET_INFORMATION dummy = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    memset(dummy, 0, sizeof(SOCKET_INFORMATION));
    dummy->prev = NULL;
    dummy->next = NULL;
    dummy->enc_ctx_decrypt = NULL;
    dummy->enc_ctx_encrypt = NULL;
    LPSOCKET_INFORMATION current = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    LPSOCKET_INFORMATION tail = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    tail = dummy;

    enc_key_init("stonejing");

    INT    Port         = 5001; 
    SOCKET ListenSocket = create_and_bind(Port);

    char* buffer = (char*)malloc(1024);
    char response_init[2] = {0x05, 0x00};
    int in_addr_len = sizeof(struct in_addr);
    char* ip = (char*)malloc(in_addr_len);
    char* address = (char*)malloc(16);

    LPSOCKET_INFORMATION tmp = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));

    struct addrinfo aiHints;
    struct addrinfo* res = NULL;
    int retVal;

    aiHints.ai_family = AF_UNSPEC;
    aiHints.ai_flags = AI_CANONNAME;
    aiHints.ai_socktype = SOCK_STREAM;
    aiHints.ai_protocol = IPPROTO_TCP;
	
	while (TRUE)
	{
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(ListenSocket, &read_set);
        current = dummy->next;

        tmp = dummy->next;
        int tmp_num =  0;
        while(tmp)
        {
            if(tmp->write_status == 1)
            {
                printf("set write.\n");
                FD_SET(tmp->send_socket, &write_set);
            }
            else
            {
                // if(tmp->recv_socket)
                // {
                    // printf("FD_SET local socket: %d\n", tmp->recv_socket);
                    FD_SET(tmp->recv_socket, &read_set);
                    FD_SET(tmp->send_socket, &read_set);
                // }
            }
            tmp_num++;
            tmp = tmp->next;
        }
        // printf("tmp num is: %d\n", tmp_num++);

		if ((total = select(0, &read_set, &write_set, NULL, NULL)) == SOCKET_ERROR)
		{
			printf("select() returned with error %d\n", WSAGetLastError());
			return 1;
		}
        // printf("total is : %d\n", total);
		// Check for arriving connections on the listening socket.
		if (FD_ISSET(ListenSocket, &read_set))
		{
			if ((accept_socket = accept(ListenSocket, NULL, NULL)) != INVALID_SOCKET)
			{
				// Set the accepted socket to non-blocking mode so the server will
				// not get caught in a blocked condition on WSASends
				if (ioctlsocket(accept_socket, FIONBIO, &block) == SOCKET_ERROR)
				{
					printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
					return 1;
				}
				// else
				// 	printf("ioctlsocket(FIONBIO) is OK!\n");

                SOCKET remote_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                struct sockaddr_in remote_server;
                remote_server.sin_family = AF_INET;
                remote_server.sin_addr.S_un.S_addr=  inet_addr("127.0.0.1");
                remote_server.sin_port = htons(5000);

                if (connect(remote_socket, (SOCKADDR *)&remote_server, sizeof(remote_server)) < 0)
                {
                    log_err("Connection Failed.");
                    continue;
                }

                log_info("connection remote socket: %d", remote_socket);

                if (Createsocket_information(tail, accept_socket, remote_socket) == FALSE)
                {
                    log_err("Createsocket_information(AcceptSocket) failed!");
                    return 1;
                }

                log_info("create socket information over.");
                
                int len;
                memset(buffer, 0, 1024);
                len = recv(accept_socket, buffer, 1024, 0);
                // printf("len: %d bufer: %02x %02x %02x\n", len, buffer[0], buffer[1], buffer[2]);
                /* server send methdo reply */
                send(accept_socket, response_init, 2, 0);
                /* recv address port from local(accpet socket) */
                memset(buffer, 0, 1024);
                len = recv(accept_socket, buffer, 1024, 0);
                /* send fake reply to local */
                // printf("reply len: %d\n", len);
                buffer[1] = 0x00;
                send(accept_socket, buffer, len, 0);
                // printf("rtysp: %hhu\n", buffer[3]);
                // ATYP 
                if(buffer[3] == 0x01)
                {
                    // memset(ip, 0, in_addr_len);
                    // struct in_addr* ip = (struct in_addr*)malloc(in_addr_len);
                    // memcpy(ip, buffer + 4, in_addr_len);
                    // inet_ntop(AF_INET, (const void *)(buffer + 4), ip, in_addr_len);
                    // unsigned short int port = ntohs(*(u_short*)(buffer + 4 + in_addr_len));
	                // memset(address, 0, 16);
                    // snprintf(address, 16, "%hhu.%hhu.%hhu.%hhu", ip[0], ip[1], ip[2], ip[3]);

                    unsigned char* addr_to_send = (unsigned char*)malloc(1024);
                    int addr_len = 0;
                    addr_to_send[addr_len++] = 0x01;
                    size_t in_addr_len = sizeof(struct in_addr);
                    memcpy(addr_to_send+addr_len, buffer+4, in_addr_len + 2);
                    addr_len += in_addr_len + 2;

                    unsigned char ciphertext[1024];
                    int cipherlen; 

                    // addr_to_send = ss_encrypt(, addr_to_send, &addr_len, )
                    log_info("ss encrypt start.");
                    log_info("tail enc %d", tail->enc_ctx_encrypt->init);
                    cipherlen =  ss_encrypt(addr_to_send, addr_len, ciphertext, tail->enc_ctx_encrypt);
                    log_info("ss encrypt over.");
                    if(cipherlen == -1) continue;
                    printf("send shadows handshake.\n");
                    send(remote_socket, (char*)ciphertext, cipherlen, 0);
                    // send(remote_socket, (char*)addr_to_send, addr_len, 0);
                    free(addr_to_send);
            
                    if (ioctlsocket(accept_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
                    {
                        printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
                        // return 1;
                    }
                    // else
                    //     printf("ioctlsocket(FIONBIO) is OK!\n");

                    if (ioctlsocket(remote_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
                    {
                        printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
                        // return 1;
                    }
                    // else
                    //     printf("ioctlsocket(FIONBIO) is OK!\n");

                    // else
                    // 	printf("Createsocket_information() is OK!\n");
                }
                else if(buffer[3] == 0x03)
                {
                    unsigned int domain_len =  buffer[4];
                    char* domain = (char*)malloc(domain_len + 1);
                    memset(domain, 0, domain_len + 1);
                    memcpy(domain, buffer + 5, domain_len);
                    unsigned short int port = ntohs(*(u_short*)(buffer + 4 + domain_len + 1));
                    // printf("domain len is %d\n", domain_len);
                    printf("connect to %s : %d\n", domain, port);
                    
                    int retVal;
                    memset(&aiHints, 0, sizeof(aiHints));
                    char portaddr[6];
                    snprintf(portaddr, 6, "%d", port);
                    if((retVal = getaddrinfo(domain, portaddr, &aiHints, &res)) != 0)
                    {
                        printf("getaddrinfo() failed.\n");
                        continue;
                    }
                    SOCKET remote_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                    if (remote_socket == INVALID_SOCKET)
                    {
                                // socket API failed
                    }
                    int rc = connect(remote_socket, res->ai_addr, res->ai_addrlen);
                    if (rc == SOCKET_ERROR)
                    {
                                // connect API failed
                    }

                    if (ioctlsocket(accept_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
                    {
                        printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
                        // return 1;
                    }
                    // else
                    //     printf("ioctlsocket(FIONBIO) is OK!\n");

                    if (ioctlsocket(remote_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
                    {
                        printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
                        // return 1;
                    }
                    // else
                    //     printf("ioctlsocket(FIONBIO) is OK!\n");

                    if (Createsocket_information(tail, accept_socket, remote_socket) == FALSE)
                    {
                    	printf("Createsocket_information(AcceptSocket) failed!\n");
                    	return 1;
                    }

                }
                else
                {
                    printf("unsupported protocol.\n");
                }
			}
			else
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					printf("accept() failed with error %d\n", WSAGetLastError());
					return 1;
				}
				else
					printf("accept() is fine!\n");
			}
		}

        while(current)
        {
            // printf("current socket: %d %d\n", current->recv_socket, current->send_socket);
            if(FD_ISSET(current->recv_socket, &read_set))
            {
                memset(buffer, 0, 1024);
                unsigned char ciphertext[1024];
                int len = recv(current->recv_socket, buffer, 1024, 0);
                printf("recieve recv socket len: %d socket: %d\n", len, current->recv_socket);
                len = ss_encrypt((unsigned char*)buffer, len, ciphertext, current->enc_ctx_encrypt);
                printf("encrypt over.\n");

                if(len > 0)
                {
                    send(current->send_socket, (char*)ciphertext, len, 0);
                    printf("send over.%d\n", WSAGetLastError());
                }
                else
                {
                    printf("delete socket %d %d\n", current->send_socket, current->recv_socket);
                    if(current->next)
                    {
                        current->prev->next = current->next;
                        current->next->prev = current->prev;
                    }
                    else
                    {
                        current->prev->next = NULL;
                        tail = current->prev;
                    }
                    if (WSAGetLastError())
				    {
                        printf("socket failed with error %d\n", WSAGetLastError());
                    }
                    closesocket(current->recv_socket);
                    closesocket(current->send_socket);
                    total--;
                    LPSOCKET_INFORMATION temp = (LPSOCKET_INFORMATION)malloc(sizeof(struct _SOCKET_INFORMATION));
                    temp = current;
                    current = current->next;
                    delete temp;
                    continue;
                }
            }
            if(FD_ISSET(current->send_socket, &read_set))
            {
                memset(buffer, 0, 1024);
                int len = recv(current->send_socket, buffer, 1024, 0);
                unsigned char plaintext[1024];
                printf("recieve send socket len: %d socket: %d\n", len, current->recv_socket);
                len = ss_decrypt((unsigned char*)buffer, len, plaintext, current->enc_ctx_decrypt);
                if(len > 0)
                {
                    send(current->recv_socket, (char*)plaintext, len, 0);
                }
                else
                {
                    // continue;
                    printf("delete socket %d %d\n", current->recv_socket, current->send_socket);
                    if(current->next)
                    {
                        current->prev->next = current->next;
                        current->next->prev = current->prev;
                    }
                    else
                    {
                        current->prev->next = NULL;
                        tail = current->prev;
                    }
                    if (WSAGetLastError())
				    {
                        printf("socket failed with error %d\n", WSAGetLastError());
                    }
                    closesocket(current->recv_socket);
                    closesocket(current->send_socket);
                    total--;
                    LPSOCKET_INFORMATION temp = (LPSOCKET_INFORMATION)malloc(sizeof(struct _SOCKET_INFORMATION));
                    temp = current;
                    current = current->next;
                    delete temp;
                    continue;
                }
            }
            current = current->next;
        }
	}
}

BOOL Createsocket_information(LPSOCKET_INFORMATION& tail, SOCKET local_socket, SOCKET remote_socket)
{
	LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
	SI->recv_socket = local_socket;
    SI->send_socket = remote_socket;
    tail->next = SI;
    SI->prev = tail;
    SI->next = NULL;
    SI->enc_ctx_encrypt = (enc_ctx*)malloc(sizeof(enc_ctx));
    SI->enc_ctx_decrypt = (enc_ctx*)malloc(sizeof(enc_ctx));
    SI->enc_ctx_encrypt->evp = EVP_CIPHER_CTX_new();
    SI->enc_ctx_encrypt->init = 0;
    SI->enc_ctx_decrypt->evp = EVP_CIPHER_CTX_new();
    SI->enc_ctx_decrypt->init = 0;

    enc_ctx_init(SI->enc_ctx_encrypt, 1);
    enc_ctx_init(SI->enc_ctx_decrypt, 0);

    tail = SI;

	return TRUE;
}

void Freesocket_information(LPSOCKET_INFORMATION current, LPSOCKET_INFORMATION result)
{
    result = current->prev;
    current->next->prev = current->prev;
    current->prev->next = current->next;
	closesocket(current->recv_socket);
	closesocket(current->send_socket);
	printf("Closing socket number %d\n", current->recv_socket);
	// GlobalFree(SI);
    free(current);
}