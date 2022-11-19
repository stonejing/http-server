#include "encrypt.h"
#include "socks5shadowsocks.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int run(std::string server_address, std::string port, std::string method, std::string password)
{
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    std::cout << "address : " << server_address << std::endl;
    std::cout << "port : " << port << std::endl;
    std::cout << "method: " << method << std::endl;
    std::cout << "password: " << password << std::endl;

    WSADATA wsadata;
    SOCKET listen_socket;
    SOCKET accept_socket;
    FD_SET write_set;
    FD_SET read_set;

    SOCKADDR_IN local_server;
    INT ret;

    ULONG nonblock = 1;
    ULONG block = 0;
    int total = 0;

    LPSOCKET_INFORMATION dummy = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    memset(dummy, 0, sizeof(SOCKET_INFORMATION));
    dummy->prev = NULL;
    // LPSOCKET_INFORMATION current = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));LPSOCKET_INFORMATION current = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    LPSOCKET_INFORMATION current;
    LPSOCKET_INFORMATION tail = dummy;
    LPSOCKET_INFORMATION add_set;

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

    BOOL bopt = TRUE;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&bopt, sizeof(BOOL));

    local_server.sin_family = AF_INET;
    local_server.sin_addr.s_addr = htonl(INADDR_ANY);
    local_server.sin_port = htons(5150);

    if (bind(listen_socket, (PSOCKADDR)&local_server, sizeof(local_server)) == SOCKET_ERROR)
    {   
       printf("bind() failed with error %d\n", WSAGetLastError());
       return 1;
    }
    else
    {
        std::cout << "bind is OK!" << std::endl;
    }

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

    char* buffer = (char*)malloc(1024);            /* transmit buffer */
    char response_init[2] = {0x05, 0x00};

    int in_addr_len = sizeof(struct in_addr);
    char* addr_to_send = (char*)malloc(1024);   /* handshake message with remote ss server */

    enc_key_init(password.c_str(), method.c_str());

    while (TRUE)
    {
        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_SET(listen_socket, &read_set);
        current = dummy->next;
        add_set = dummy->next;

        while(add_set)
        {
            if(add_set->write_status == 1)
            {
                FD_SET(add_set->send_socket, &write_set);
            }
            else
            {
                FD_SET(add_set->recv_socket, &read_set);
                FD_SET(add_set->send_socket, &read_set);
            }
            add_set = add_set->next;
        }

        if ((total = select(0, &read_set, &write_set, NULL, NULL)) == SOCKET_ERROR)
        {
            printf("select() returned with error %d\n", WSAGetLastError());
            return 1;
        }

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
                    return 0;
                }

                struct sockaddr_in remote_server;
                remote_server.sin_family = AF_INET;
                remote_server.sin_addr.S_un.S_addr = inet_addr(server_address.c_str());
                remote_server.sin_port = htons(std::stoi(port));
                SOCKET remote_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (connect(remote_socket, (SOCKADDR *)&remote_server, sizeof(remote_server)) < 0)
                {
                    printf("Connection Failed \n");
                    continue;
                }

                int len;
                memset(buffer, 0, 1024);
                len = recv(accept_socket, buffer, 1024, 0);
                send(accept_socket, response_init, 2, 0);
                memset(buffer, 0, 1024);
                len = recv(accept_socket, buffer, 1024, 0);
                buffer[1] = 0x00;
                send(accept_socket, buffer, len, 0);

                ssize_t addr_len = 0;
                memset(addr_to_send, 0, 1024);
                addr_to_send[addr_len++] = buffer[3];

                LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
                SI->e_enc = (enc_ctx*)malloc(sizeof(enc_ctx));
                SI->d_enc = (enc_ctx*)malloc(sizeof(enc_ctx));
                enc_ctx_init(SI->e_enc, 1, method.c_str());
                enc_ctx_init(SI->d_enc, 0, method.c_str());

                if(buffer[3] == 0x01)
                {
                    memcpy(addr_to_send + addr_len, buffer + 4, in_addr_len + 2);
                    addr_len += in_addr_len + 2;
                    addr_to_send = ss_encrypt(1024, addr_to_send, &addr_len, SI->e_enc);
                    send(remote_socket, addr_to_send, addr_len, 0);
                }
                else if(buffer[3] == 0x03)
                {
                    uint8_t domain_len =  (uint8_t)buffer[4];
                    addr_to_send[addr_len++] = domain_len;
                    memcpy(addr_to_send + addr_len, buffer + 5, domain_len + 2);
                    addr_len += domain_len + 2;
                    addr_to_send = ss_encrypt(1024, addr_to_send, &addr_len, SI->e_enc);
                    send(remote_socket, addr_to_send, addr_len, 0);
                }
                else
                {
                    printf("unsupported protocol.\n");
                }

                if (ioctlsocket(accept_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
                {
                    printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
                    return 0;
                }
                if (ioctlsocket(remote_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
                {
                    printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
                    return 0;
                }
                if (Createsocket_information(tail, SI, accept_socket, remote_socket) == FALSE)
                {
                    printf("Createsocket_information(AcceptSocket) failed!\n");
                    return 0;
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
            if(FD_ISSET(current->recv_socket, &read_set))
            {
                memset(buffer, 0, 1024);
                int len = recv(current->recv_socket, buffer, 1024, 0);
                // printf("local buffer len: %d socket: %d\n", len, current->recv_socket);
                if(len > 0)
                {
                    buffer = ss_encrypt(1024, buffer, (ssize_t*)&len, current->e_enc);
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
                    LPSOCKET_INFORMATION temp = (LPSOCKET_INFORMATION)malloc(sizeof(struct _SOCKET_INFORMATION));
                    temp = current;
                    current = current->next;
                    free(temp);
                    continue;
                }
            }
            if(FD_ISSET(current->send_socket, &read_set))
            {
                // printf("start recv remote.\n");
                memset(buffer, 0, 1024);
                int len = recv(current->send_socket, buffer, 1024, 0);
                // printf("remote buffer len: %d socket: %d\n", len, current->send_socket);
                if(len > 0)
                {
                    buffer = ss_decrypt(1024, buffer, (ssize_t*)&len, current->d_enc);
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
                    LPSOCKET_INFORMATION temp = (LPSOCKET_INFORMATION)malloc(sizeof(struct _SOCKET_INFORMATION));
                    temp = current;
                    current = current->next;
                    free(temp);
                    continue;
                }
            }
            current = current->next;
        }
    }
}

BOOL Createsocket_information(LPSOCKET_INFORMATION& tail, LPSOCKET_INFORMATION& SI, SOCKET local_socket, SOCKET remote_socket)
{
    // LPSOCKET_INFORMATION SI = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION));
    // memset(SI, 0, sizeof(SOCKET_INFORMATION));
    // printf("add sockets local: %d remote: %d\n", local_socket, remote_socket);
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
