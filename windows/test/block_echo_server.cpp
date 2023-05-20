#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <vector>

using namespace std;

int main(void)
{
    WSADATA wsaData;
    WORD sockVersion = MAKEWORD(2, 2);
    if(WSAStartup(sockVersion, &wsaData) != 0)
    {
        return 0;
    }
    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_socket == INVALID_SOCKET)
    {
        cout << "socket error!" << endl;
        return 0;
    }

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(5150);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if(bind(listen_socket, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        cout << "bind error" << endl;
        closesocket(listen_socket);
        return 0;
    }
    if(listen(listen_socket, 5) == SOCKET_ERROR)
    {
        cout << "listen error" << endl;
        closesocket(listen_socket);
        return 0;
    }
    struct sockaddr_in remoteAddr;
    SOCKET client_socket;
    int addrlen = sizeof(remoteAddr);
    char recvData[255];
    std::vector<char> recv_data(1024);
    while(1)
    {
        client_socket = accept(listen_socket, (SOCKADDR*)&remoteAddr, &addrlen);
        if(client_socket == SOCKET_ERROR)
        {
            cout << "accept error." << endl;
            continue;
        }
        // cout << "get a new connection: " << inet_ntoa(remoteAddr.sin_addr) << endl;
        int ret = recv(client_socket, recv_data.data(), 1024, 0);
        if(ret > 0)
        {
            // recvData[ret] = 0x00;
            // printf(recvData);
            for(int i = 0; i < ret; i++)
                std::cout << recv_data[i];
        }
        char* buff = (char*)"HTTP/1.1 200 OK\r\nContent-length: 17\r\n\r\nTHIS IS A TEST.\r\n";
        int s = send(client_socket, buff, strlen(buff), 0);
        if(s != strlen(buff))
        {
            cout << "WSA ERROR: " << WSAGetLastError() << endl;
        }
        closesocket(client_socket);
    }
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}