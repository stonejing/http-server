#pragma once
#ifndef _STRAY_DOG
#define _STRAY_DOG

#define FD_SETSIZE 1024
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#include "crypto.hpp"
#include "aead_encrypt.hpp"

#define PORT 5005
#define DATA_BUFSIZE 8192

typedef struct _SOCKET_INFORMATION
{
    CHAR Buffer[DATA_BUFSIZE];
    WSABUF DataBuf;
    SOCKET AcceptSocket;          // used for accept
    SOCKET ConnectSocket;    // used for connect
    DWORD BytesSEND;
    DWORD BytesRECV;
    // enc_ctx* enc_ctx_encrypt;
    // enc_ctx* enc_ctx_decrypt;
    cipher_ctx_t* ctx_enc;
    cipher_ctx_t* ctx_dec;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

// Prototypes
int shadowsocks_handshake(char* shadowsocks_handshake_buffer, int len, char* cipher_buffer, LPSOCKET_INFORMATION SI);
LPSOCKET_INFORMATION CreateSocketInformation(SOCKET& AcceptSocket, SOCKET& ConnectSocket);

void FreeSocketInformation(DWORD Index);

# endif