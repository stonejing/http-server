#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>

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
    while(1)
    {
        client_socket = accept(listen_socket, (SOCKADDR*)&remoteAddr, &addrlen);
        if(client_socket == SOCKET_ERROR)
        {
            cout << "accept error." << endl;
            continue;
        }
        cout << "get a new connection: " << inet_ntoa(remoteAddr.sin_addr) << endl;
        int ret = recv(client_socket, recvData, 255, 0);
        if(ret > 0)
        {
            recvData[ret] = 0x00;
            printf(recvData);
        }
        char* buff = (char*)"\r\nHello, my friend.\r\n";
        send(client_socket, buff, strlen(buff), 0);
        closesocket(client_socket);
    }
    closesocket(listen_socket);
    WSACleanup();
    return 0;
}