#define FD_SETSIZE 1024

#include <WinSock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <stdio.h>

#define PORT 5150
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
    INT buffer_len;
    _SOCKET_INFORMATION* prev;
    _SOCKET_INFORMATION* next;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

// Prototypes
BOOL Createsocket_information(LPSOCKET_INFORMATION& tail, SOCKET accept_socket, SOCKET remote_socket);
void Freesocket_information(LPSOCKET_INFORMATION current, LPSOCKET_INFORMATION result);

int main(int argc, char** argv)
{
    WSADATA wsadata;
	SOCKET listen_socket;
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
    LPSOCKET_INFORMATION current = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    LPSOCKET_INFORMATION tail = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    tail = dummy;
    
	if ((ret = WSAStartup(0x0202, &wsadata)) != 0)
	{
		printf("WSAStartup() failed with error %d\n", ret);
		WSACleanup();
		return 1;
	}
	else
		printf("WSAStartup() is fine!\n");
	// Prepare a socket to listen for connections
	if ((listen_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("WSASocket() is OK!\n");
	
	local_server.sin_family = AF_INET;
	local_server.sin_addr.s_addr = htonl(INADDR_ANY);
	local_server.sin_port = htons(PORT);
	
	if (bind(listen_socket, (PSOCKADDR)&local_server, sizeof(local_server)) == SOCKET_ERROR)
	{
		printf("bind() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("bind() is OK!\n");
	if (listen(listen_socket, 5))
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("listen() is OK!\n");
	// Change the socket mode on the listening socket from blocking to
	// non-block so the application will not block waiting for requests
	if (ioctlsocket(listen_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("ioctlsocket() is OK!\n");

    // FD_ZERO(&read_set);
    // FD_ZERO(&write_set);
    // FD_SET(listen_socket, &read_set);

    FD_SET temp_set;

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
		// FD_ZERO(&temp_set);
        // temp_set = read_set;
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(listen_socket, &read_set);
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
		if (FD_ISSET(listen_socket, &read_set))
		{
			if ((accept_socket = accept(listen_socket, NULL, NULL)) != INVALID_SOCKET)
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

                int len;
                memset(buffer, 0, 1024);
                len = recv(accept_socket, buffer, 1024, 0);
                // printf("len: %d bufer: %02x %02x %02x\n", len, buffer[0], buffer[1], buffer[2]);
                /* server send methdo reply */
                send(accept_socket, response_init, 2, 0);
                /* recv address port from local(accpet socket) */
                memset(buffer, 0, 1024);
                len = recv(accept_socket, buffer, 1024, 0);
                /* send reply to local */
                // printf("reply len: %d\n", len);
                buffer[1] = 0x00;
                send(accept_socket, buffer, len, 0);
                // printf("rtysp: %hhu\n", buffer[3]);
                if(buffer[3] == 0x01)
                {
                    SOCKET remote_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    struct sockaddr_in remote_server;
                    remote_server.sin_family = AF_INET;
                    memset(ip, 0, in_addr_len);
                    // struct in_addr* ip = (struct in_addr*)malloc(in_addr_len);
                    memcpy(ip, buffer + 4, in_addr_len);
                    // inet_ntop(AF_INET, (const void *)(buffer + 4), ip, in_addr_len);
                    unsigned short int port = ntohs(*(u_short*)(buffer + 4 + in_addr_len));

	                memset(address, 0, 16);
                    snprintf(address, 16, "%hhu.%hhu.%hhu.%hhu", ip[0], ip[1], ip[2], ip[3]);

                    remote_server.sin_addr.S_un.S_addr = inet_addr(address);
                    remote_server.sin_port = htons(port);
                    // printf("connect to %s:%hu\n", address, port);

                    if (connect(remote_socket, (SOCKADDR *)&remote_server, sizeof(remote_server)) < 0)
                    {
                        printf("Connection Failed \n");
                        continue;
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
                int len = recv(current->recv_socket, buffer, 1024, 0);
                // printf("recieve len: %d socket: %d\n", len, current->recv_socket);
                if(len > 0)
                {
                    send(current->send_socket, buffer, len, 0);
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
                // printf("recieve len: %d socket: %d\n", len, current->recv_socket);
                if(len > 0)
                {
                    send(current->recv_socket, buffer, len, 0);
                }
                else
                {
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
    tail = SI;

	return(TRUE);
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