#include "utils.h"
#include "log.h"
#include <WinSock2.h>
#include <winerror.h>

#define MAX_READ_BUFFER_SIZE 4096

// -1 的结果就是关闭 socket，其它的情况都是在 select 中继续监听
int recvn(SOCKET sockfd, std::vector<char>& buffer)
{
    int r = 0;
    int result = 0;
    char temp[MAX_READ_BUFFER_SIZE];
    while(true)
    {
        r = recv(sockfd, temp + result, MAX_READ_BUFFER_SIZE - result, 0);
        // remote close the socket
        if(r == 0)
        {
            return -1;
        }
        else if(r == SOCKET_ERROR)
        {
            if(WSAGetLastError() == WSAEINTR)
            {
                LOG_WARN << "READ SOCKET WSAEINTR: " << WSAGetLastError() << "\n";
                continue;
            }
            // non block socket, current no data available
            if(WSAGetLastError() == WSAEWOULDBLOCK)
            {
                // LOG_INFO << "READ SOCKET NONBLOCK NO DATA.\n";
                buffer = std::vector<char>(temp, temp + result);
                return result;
            }
            else
            {
                LOG_ERROR << "READ SOCKET ERROR: " << WSAGetLastError() << "\n";
                return -1;
            }
        }
        result += r;
    }


    buffer = std::vector<char>(temp, temp + result);
    return result;
}

int sendn(SOCKET sockfd, std::vector<char>& buffer, int buffer_len)
{
    int r = 0;
    int sent = 0;
    int left = buffer_len;

    while(left > 0)
    {
        r = send(sockfd, buffer.data() + sent, left, 0);
        if(r == 0)
        {
            LOG_ERROR << "SOCKET SEND RETRUN 0: " << WSAGetLastError() << "\n";
            return -1;
        }
        else if(r == SOCKET_ERROR)
        {
            if(WSAGetLastError() == WSAEWOULDBLOCK)
            {
                LOG_INFO << "WRITE SOCKET CURRENTLY BLOCK, TOO MUCH DATA.\n";
                return sent;
            }
            else 
            {
                LOG_ERROR << "WRITE SOCKET ERROR: " << WSAGetLastError() << "\n";
                return -1;
            }
        }
        sent += r;
        left -= r;
    }
    return sent;
}