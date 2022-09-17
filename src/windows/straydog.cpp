#include "dbg.hpp"
// #include "stream_encrypt.hpp"
#include "straydog.hpp"
#include <signal.h>


// Global var
DWORD TotalSockets = 0;
LPSOCKET_INFORMATION SocketArray[FD_SETSIZE];

SOCKET create_and_bind(INT Port)
{
    SOCKET ListenSocket;
    WSADATA wsaData;
    SOCKADDR_IN InternetAddr;
    ULONG NonBlock = 1;

    if (WSAStartup(0x0202, &wsaData) != 0)
    {
        log_err("WSAStartup() failed with error %d", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    else
    {
        log_info("WSAStartup() is OK!");
    }

    if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        log_err("WSASocket() failed with error %d", WSAGetLastError());
        return 1;
    }
    else
    {
        log_info("WSASocket() is OK!");
    }
    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_port = htons(Port);
    InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if (bind(ListenSocket, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
    {
        log_err("bind() failed with error %d", WSAGetLastError());
        return 1;
    }
    else
    {
        log_info("bind() is OK!");
    }
    if (listen(ListenSocket, 5))
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
    if (ioctlsocket(ListenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
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

int shadowsocks_handshake(char* shadowsocks_handshake_buffer, int len, char* cipher_buffer, LPSOCKET_INFORMATION SI)
{
    memset(cipher_buffer, 0, 2048);
    int ciphertext_len;
    aead_encrypt(SI->ctx_enc, (uint8_t*)shadowsocks_handshake_buffer + 3, len - 3, (uint8_t*)cipher_buffer, &ciphertext_len);

    send(SI->ConnectSocket, cipher_buffer, ciphertext_len, 0);
    log_info("send ss handshake.");
    return 1;
    
    // if(shadowsocks_handshake_buffer[3] == 0x01)
    // {
    //     size_t in_addr_len = sizeof(struct in_addr);
    //     // int len = ss_encrypt((unsigned char*)shadowsocks_handshake_buffer + 3, in_addr_len + 3, (unsigned char*)cipher_buffer, SI->enc_ctx_encrypt);
    //     int ciphertext_len;
    //     // log_info("in_addr_len: %d", in_addr_len);
    //     aead_encrypt(SI->ctx_enc, (uint8_t*)shadowsocks_handshake_buffer + 3, in_addr_len + 3, (uint8_t*)cipher_buffer, &ciphertext_len);

    //     // log_info("ciphertext_len: %d", ciphertext_len);
    //     // printf("ciphertext: ");
    //     // debug_hex((const uint8_t *)cipher_buffer, ciphertext_len);

    //     send(SI->ConnectSocket, cipher_buffer, ciphertext_len, 0);

    //     log_info("send ss handshake.");
    //     return 1;
    // }
    // else if(shadowsocks_handshake_buffer[3] == 0x03)
    // {
    //     return 1;
    // }
    // else if(shadowsocks_handshake_buffer[3] == 0x04)
    // {
    //     return 1;
    // }
    // else
    // {
    //     log_err("unsupported protocol.");
    //     return -1;
    // }
}

int main(int argc, char **argv)
{
    SOCKET ListenSocket = create_and_bind(5005);
    SOCKET AcceptSocket;
    SOCKADDR_IN InternetAddr;
    WSADATA wsaData;
    INT Ret;
    FD_SET WriteSet;
    FD_SET ReadSet;
    DWORD i;
    DWORD Total;
    ULONG NonBlock;
    DWORD Flags;
    DWORD SendBytes;
    DWORD RecvBytes;

    char* socks_handshake_buffer = (char*)malloc(1024);
    memset(socks_handshake_buffer, 0, 1024);
    char socks_response_init[2] = { 0x05, 0x00 };

    char* shadowsocks_handshake_buffer = (char*)malloc(1024);
    memset(shadowsocks_handshake_buffer, 0, 1024);

    char* transfer_buffer = (char*)malloc(8192);
    char* cipher_buffer = (char*)malloc(20 * 1024);

    // enc_key_init("stonejing");

    // signal(SIGPIPE, SIG_IGN);

    while (TRUE)
    {
        // Prepare the Read and Write socket sets for network I/O notification
        FD_ZERO(&ReadSet);
        FD_ZERO(&WriteSet);
        // Always look for connection attempts
        FD_SET(ListenSocket, &ReadSet);
        // Set Read and Write notification for each socket based on the
        // current state the buffer.  If there is data remaining in the
        // buffer then set the Write set otherwise the Read set
 
        for (i = 0; i < TotalSockets; i++)
            // if (SocketArray[i]->BytesRECV > SocketArray[i]->BytesSEND)
            //     FD_SET(SocketArray[i]->AcceptSocket, &WriteSet);
            // else
        {
            FD_SET(SocketArray[i]->AcceptSocket, &ReadSet);
            FD_SET(SocketArray[i]->ConnectSocket, &ReadSet);
        }
        if ((Total = select(0, &ReadSet, &WriteSet, NULL, NULL)) == SOCKET_ERROR)
        {
            log_err("select() returned with error %d", WSAGetLastError());
            return 1;
        }

        // Check for arriving connections on the listening socket.
        if (FD_ISSET(ListenSocket, &ReadSet))
        {
            Total--;
            if ((AcceptSocket = accept(ListenSocket, NULL, NULL)) != INVALID_SOCKET)
            {
                log_warn("create new socket connection**********");
                // socks5 protocol 握手
                int recv_local_len = 0;
                recv_local_len = recv(AcceptSocket, socks_handshake_buffer, 1024, 0);
                log_info("recv_local_len: %d", recv_local_len);
                if(recv_local_len == -1) continue;
                log_info("recv_local_len: %d    ", recv_local_len);
                // print_binary(socks_handshake_buffer, recv_local_len);

                int send_status = send(AcceptSocket, socks_response_init, 2, 0);
                if(send_status == -1) continue;
                log_info("send socks init response: %d", send_status);

                recv_local_len = recv(AcceptSocket, socks_handshake_buffer, 1024, 0);
                if(recv_local_len == -1) continue;
                log_info("recv_local_len: %d", recv_local_len);
                // print_binary(socks_handshake_buffer, recv_local_len);
                socks_handshake_buffer[1] = 0x00;
                send_status = send(AcceptSocket, socks_handshake_buffer, recv_local_len, 0);
                if(send_status == -1) continue;

                // shadowsocks 握手连接，获取 address
                SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                struct sockaddr_in RemoteServer;
                RemoteServer.sin_family = AF_INET;
                RemoteServer.sin_addr.S_un.S_addr=  inet_addr("127.0.0.1");
                RemoteServer.sin_port = htons(5000);

                if (connect(ConnectSocket, (SOCKADDR *)&RemoteServer, sizeof(RemoteServer)) < 0)
                {
                    log_err("Connection Failed.");
                    // return 0;
                    continue;
                }
                // log_info("connection remote socket: %d", ConnectSocket);
                // 依据 socks5 协议构建 shadowsocks 协议
                // 就是传输一个地址而已，有三种情况
                // 这都是线性堵塞的，可以优化，但是需要先实现功能，再来谈优化
                // 使用 c++ 就是为了高性能，高性能又是需要优化的

                // Set the accepted socket to non-blocking mode so the server will
                // not get caught in a blocked condition on WSASends
                NonBlock = 1;
                if (ioctlsocket(AcceptSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
                {
                    log_err("ioctlsocket(FIONBIO) failed with error %d", WSAGetLastError());
                    return 1;
                }
                else
                    log_info("ioctlsocket(FIONBIO) is OK!");

                LPSOCKET_INFORMATION SocketInfo = CreateSocketInformation(AcceptSocket, ConnectSocket);
                shadowsocks_handshake(socks_handshake_buffer, recv_local_len, cipher_buffer, SocketInfo);
                log_info("shadowsocks_handshake is OK!");
            }
            else
            {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    log_err("accept() failed with error %d", WSAGetLastError());
                    return 1;
                }
                else
                    log_info("accept() is fine!");
            }
        }
        // Check each socket for Read and Write notification until the number
        // of sockets in Total is satisfied
        for (i = 0; Total > 0 && i < TotalSockets; i++)
        {
            
            LPSOCKET_INFORMATION SocketInfo = SocketArray[i];
            // If the ReadSet is marked for this socket then this means data
            // is available to be read on the socket
            if (FD_ISSET(SocketInfo->AcceptSocket, &ReadSet))
            {
             
                Total--;
                int transfer_buffer_len = recv(SocketInfo->AcceptSocket, transfer_buffer, 1024, 0);
                
                if(transfer_buffer_len <= 0)
                {
                    FreeSocketInformation(i);
                    continue;
                }
                else
                {
                    // int cipher_buffer_len = ss_encrypt((unsigned char*)transfer_buffer, transfer_buffer_len, (unsigned char*)cipher_buffer, SocketInfo->enc_ctx_encrypt);
                    int cipher_buffer_len = 0;
             
                    aead_encrypt(SocketInfo->ctx_enc, (uint8_t*)transfer_buffer, transfer_buffer_len, (uint8_t*)cipher_buffer, &cipher_buffer_len);
            
                    if(!send(SocketInfo->ConnectSocket, cipher_buffer, cipher_buffer_len, 0))
                    {
                        log_err("send() failed with error %d", WSAGetLastError());
                    }
                    // log_info("send encrypted buffer: %d", cipher_buffer_len);
                }
            }
            
            if (FD_ISSET(SocketInfo->ConnectSocket, &ReadSet))
            {
                Total--;

                int transfer_buffer_len = recv(SocketInfo->ConnectSocket, transfer_buffer, 1024, 0);

                if(transfer_buffer_len <= 0)
                {
                    FreeSocketInformation(i);
                    continue;
                }
                else
                {
                    // int cipher_buffer_len = ss_decrypt((unsigned char*)transfer_buffer, transfer_buffer_len, (unsigned char*)cipher_buffer, SocketInfo->enc_ctx_decrypt);

                    int cipher_buffer_len = 0;
                    aead_decrypt(SocketInfo->ctx_dec, (uint8_t*)transfer_buffer, transfer_buffer_len, (uint8_t*)cipher_buffer, &cipher_buffer_len);

                    if(cipher_buffer_len == 0) continue;

                    if(cipher_buffer_len == -1)
                    {
                        log_info("return -1, quit.");
                        FreeSocketInformation(i);
                        return -1;
                    }
                    int send_len = send(SocketInfo->AcceptSocket, cipher_buffer, cipher_buffer_len, 0);
                    if(send_len != cipher_buffer_len)
                    {
                        log_err("WSARecv() failed with error %d", WSAGetLastError());
                    }
                    // log_info("send local: %d", send_len);
                }
            }
        }
    }
}

LPSOCKET_INFORMATION CreateSocketInformation(SOCKET& AcceptSocket, SOCKET& ConnectSocket)
{
    LPSOCKET_INFORMATION SI;
    log_info("Create Socket Information Accept: %d, Connect: %d", AcceptSocket, ConnectSocket);
    // if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    // {
    //     log_info("GlobalAlloc() failed with error %d", GetLastError());
    //     return FALSE;
    // }
    // else
    //     log_info("GlobalAlloc() for SOCKET_INFORMATION is OK!");
    if((SI = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION))) == NULL)
    {
        log_info("GlobalAlloc() failed with error %d", GetLastError());
        return FALSE;
    }
    else
        log_info("GlobalAlloc() for SOCKET_INFORMATION is OK!");
    // Prepare SocketInfo structure for use
    SI->AcceptSocket = AcceptSocket;
    SI->ConnectSocket = ConnectSocket;
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;

    // SI->enc_ctx_encrypt = (enc_ctx*)malloc(sizeof(enc_ctx));
    // SI->enc_ctx_decrypt = (enc_ctx*)malloc(sizeof(enc_ctx));
    // SI->enc_ctx_encrypt->evp = EVP_CIPHER_CTX_new();
    // SI->enc_ctx_encrypt->init = 0;
    // SI->enc_ctx_decrypt->evp = EVP_CIPHER_CTX_new();
    // SI->enc_ctx_decrypt->init = 0;

    SI->ctx_enc = (cipher_ctx_t*)malloc(sizeof(cipher_ctx_t));
    SI->ctx_dec = (cipher_ctx_t*)malloc(sizeof(cipher_ctx_t));

    SI->ctx_enc->init = 0;
    SI->ctx_dec->init = 0;

    SI->ctx_enc->evp = EVP_CIPHER_CTX_new();
    SI->ctx_dec->evp = EVP_CIPHER_CTX_new();

    SI->ctx_dec->chunk = (buffer_t*)malloc(sizeof(buffer_t));

    memset(SI->ctx_dec->chunk, 0, sizeof(buffer_t));

    SI->ctx_dec->chunk->len = 0;
    SI->ctx_dec->chunk->idx = 0;
    SI->ctx_dec->chunk->data = (uint8_t*)malloc(20 * 1024);

    // memset(SI->ctx_dec->chunk->data, 0, 16 * 1024 - 1);
    // enc_ctx_init(SI->enc_ctx_encrypt, 1);
    // enc_ctx_init(SI->enc_ctx_decrypt, 0);

    SocketArray[TotalSockets] = SI;
    TotalSockets++;
    return SI;
}

void FreeSocketInformation(DWORD Index)
{
    LPSOCKET_INFORMATION SI = SocketArray[Index];
    DWORD i;
    closesocket(SI->AcceptSocket);
    closesocket(SI->ConnectSocket);
    // log_info("Closing socket number %d, %d", SI->AcceptSocket, SI->ConnectSocket);
    // free(SI->enc_ctx_decrypt);
    // free(SI->enc_ctx_encrypt);

    free(SI->ctx_dec->chunk->data);
    free(SI->ctx_dec->chunk);
    free(SI->ctx_enc);
    free(SI->ctx_dec);
    free(SI);
    // Squash the socket array
    for (i = Index; i < TotalSockets; i++)
    {
        SocketArray[i] = SocketArray[i + 1];
    }
    TotalSockets--;
    // log_info("Free over.");
}