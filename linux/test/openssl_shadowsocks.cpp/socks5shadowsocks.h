#ifndef SOCKS5SHADOWSOCKS_H
#define SOCKS5SHADOWSOCKS_H

#define FD_SETSIZE 1024

#include <winsock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include "encrypt.h"
#include <string>

#define PORT 5150
#define DATA_BUFSIZE 1024

typedef struct _SOCKET_INFORMATION {
    // CHAR buffer[DATA_BUFSIZE];
    WSABUF data_buf;
    SOCKET recv_socket;
    SOCKET send_socket;
    BOOL write_status; /* write_status 为 1，将 send_socket 送入 write_set，*/
    INT buffer_len;
    enc_ctx* e_enc;
    enc_ctx* d_enc;
    _SOCKET_INFORMATION* prev;
    _SOCKET_INFORMATION* next;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

// Prototypes
BOOL Createsocket_information(LPSOCKET_INFORMATION& tail, LPSOCKET_INFORMATION& SI, SOCKET accept_socket, SOCKET remote_socket);
void Freesocket_information(LPSOCKET_INFORMATION& current);
int run(std::string server_address, std::string port, std::string method, std::string password);

#endif // SOCKS5TRASMIT_H
