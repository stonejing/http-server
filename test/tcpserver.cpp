#include <winsock2.h>
#include <stdio.h>
#include <iostream>

using std::cout;
using std::endl;

int main(void)
{
    WSADATA         wsaData;
    SOCKET          ListeningSocket;
    SOCKET          NewConnection;
    SOCKADDR_IN     ServerAddr;
    SOCKADDR_IN     ClientAddr;
    int             ClientAddrLen = sizeof(ClientAddr);
    int             Port = 8000;
    int             Ret;
    char            DataBuffer[1024];

    if((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        cout << "WSAStartup failed with error " << WSAGetLastError() << endl;
        return 0;
    }

    if((ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        cout << "socket failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    if(bind(ListeningSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        cout << "bind failed with error " << WSAGetLastError() << endl;
        closesocket(ListeningSocket);
        WSACleanup();
        return 0;
    }

    if(listen(ListeningSocket, 5) == SOCKET_ERROR)
    {
        cout << "Listen failed with error " << WSAGetLastError() << endl;
        closesocket(ListeningSocket);
        WSACleanup();
        return 0;
    }

    cout << "We are awaiting a connection on port " << Port << endl;

    if((NewConnection = accept(ListeningSocket, (SOCKADDR*)&ClientAddr, &ClientAddrLen)) == INVALID_SOCKET)
    {
        cout << "accept failed with error " << WSAGetLastError() << endl;
        closesocket(ListeningSocket);
        WSACleanup();
        return 0;
    }

    cout << "We successfully got a connection from " << inet_ntoa(ClientAddr.sin_addr) <<
            ":" << ntohs(ClientAddr.sin_port) << endl;

    closesocket(ListeningSocket);
    cout << "We are waiting to receive data..." << endl;

    if((Ret = recv(NewConnection, DataBuffer, sizeof(DataBuffer), 0)) == SOCKET_ERROR)
    {
        cout << "recv failed with error " << WSAGetLastError() << endl;
        closesocket(NewConnection);
        WSACleanup();
        return 0;
    }

    cout << "We successfully received " << Ret << " bytes." << endl;
    for(int i = 0; i < Ret; i++)
    {
        cout << DataBuffer[i];
    }
    cout << endl;
    cout << "We are now going to close the client connection." << endl;

    closesocket(NewConnection);
    WSACleanup();
}