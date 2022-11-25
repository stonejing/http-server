#include <winsock2.h>
#include <iostream>

using std::cout;
using std::endl;

int main(void)
{
    WSADATA         wsaData;
    SOCKET          s;
    SOCKADDR_IN     ServerAddr;
    int             Port = 8000;
    int             Ret;

    if((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        cout << "WSAStartup failed with error " << WSAGetLastError() << endl;
        return 0;
    }

    if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    {
        cout << "socket failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    cout << "We are trying to connect to " << inet_ntoa(ServerAddr.sin_addr)
        << ":" << htons(ServerAddr.sin_port) << endl;
    
    if(connect(s, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR)
    {
        cout << "connect failed with error " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 0;
    }

    cout << "Our connection succeeded." << endl;
    cout << "We will now try to send a hello message." << endl;

    if((Ret = send(s, "Hello", 5, 0)) == SOCKET_ERROR)
    {
        cout << "send failed with error " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 0;
    }

    cout << "We successfully sent " << Ret << " bytes" << endl;
    cout << "We are closing the connection" << endl;

    closesocket(s);

    WSACleanup();
}