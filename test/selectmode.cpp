#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <iostream>

using std::cout;
using std::endl;

#define PORT 8000
#define DATA_BUFSIZE 8192

typedef struct _SOCKET_INFORMATION {
    CHAR                    Buffer[DATA_BUFSIZE];
    WSABUF                  Databuf;
    INT                     DatabufLen;
    SOCKET                  SocketLocal;
    SOCKET                  SocketRemote;
    DWORD                   BytesSEND;
    DWORD                   BytesRECV;
    INT                     Stage;
    LPSOCKET_INFORMATION    Next;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

BOOL CreateSocketInformation(SOCKET s);
void FreeSocketInformation(DWORD Index);

LPSOCKET_INFORMATION SocketArray[FD_SETSIZE];

int main(void)
{
    SOCKET          ListenSocket;
    SOCKET          AcceptSocket;
    SOCKADDR_IN     InternetAddr;
    WSADATA         wsaData;
    INT             Ret;
    FD_SET          WriteSet;
    FD_SET          ReadSet;
    DWORD           i;
    DWORD           Total;
    ULONG           NonBlock;
    DWORD           Flags;
    DWORD           SendBytes;
    DWORD           RecvBytes;

    LPSOCKET_INFORMATION Dummy = (LPSOCKET_INFORMATION)malloc(sizeof(LPSOCKET_INFORMATION));
    DWORD TotalSockets = 0;
    memset(Dummy, 0, sizeof(SOCKET_INFORMATION));
    Dummy->Next = NULL;

    if((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
    {
        cout << "WSAStartup() failed with error " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    } 
    else
    {
        cout << "WSAStartup() is fine!" << endl;
    }

    if((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
    {
        cout << "WSASocket() failed with error " << WSAGetLastError() << endl;
        return 0;
    }
    else
        cout << "WSASocket() is OK!" << endl;

    InternetAddr.sin_family = AF_INET;
    InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    InternetAddr.sin_port = htons(PORT);

    if(bind(ListenSocket, (PSOCKADDR)&InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
    {
        cout << "bind() failed with error " << WSAGetLastError() << endl;
        return 0;
    }
    else
        cout << "bind() is OK!" << endl;
    
    if(listen(ListenSocket, 5))
    {
        cout << "listen() failed with error " << WSAGetLastError() << endl;
        return 0;
    }
    else
        cout << "listen() is OK!" << endl;

    NonBlock = 1;
    if(ioctlsocket(ListenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
    {
        cout << "ioctlsocket() failed with error " << WSAGetLastError() << endl;
        return 0;
    }
    else
        cout << "ioctlsocket() is OK!" << endl;

    while(TRUE)
    {
        FD_ZERO(&ReadSet);
        FD_ZERO(&WriteSet);

        FD_SET(ListenSocket, &ReadSet);

        LPSOCKET_INFORMATION Temp = Dummy->Next;

        for(i = 0; i < TotalSockets; i++)
        {
            if(SocketArray[i]->BytesRECV > SocketArray[i]->BytesSEND)
                FD_SET(SocketArray[i]->SocketLocal, &WriteSet);
            else
                FD_SET(SocketArray[i]->SocketLocal, &ReadSet);
        }

        if(Total = select(88, &ReadSet, &WriteSet, NULL, NULL) == SOCKET_ERROR)
        {
            cout << "select() returned with error " << WSAGetLastError() << endl;
            return 0;
        }
        else
        {
            // cout << "select return " << Total << " connections." << endl;
        }

        if(FD_ISSET(ListenSocket, &ReadSet))
        {
            Total--;
            if((AcceptSocket = accept(ListenSocket, NULL, NULL)) != INVALID_SOCKET)
            {
                NonBlock = 1;
                if(ioctlsocket(AcceptSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
                {
                    cout << "ioctlsocket(FIONBIO) failed with error " << WSAGetLastError() << endl;
                    return 0;
                }
                else
                    cout << "ioctlsocket(FIONBIO) is OK!" << endl;
                
                if(CreateSocketInformation(AcceptSocket) == FALSE)
                {
                    cout << "CreateSocketInformation(AcceptSocket) failed!" << endl;
                    return 0;
                }
                else
                    cout << "CreateSocketInformation() is OK!" << endl;
            }
            else
            {
                if(WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    cout << "accept() failed with error" << WSAGetLastError() << endl;
                    return 0;
                }
                else
                    cout << "accept() is OK!" << endl;
            }
        }

        cout << "Total is " << Total << endl;

        for(i = 0; Total > 0 && i < TotalSockets; i++)
        {
            LPSOCKET_INFORMATION SocketInfo = SocketArray[i];

            if(FD_ISSET(SocketInfo->Socket, &ReadSet))
            {
                cout << "READ SET " << SocketInfo->Socket << endl;
                Total--;
                SocketInfo->Databuf.buf = SocketInfo->Buffer;
                SocketInfo->Databuf.len = DATA_BUFSIZE;

                Flags = 0;
                if(WSARecv(SocketInfo->Socket, &(SocketInfo->Databuf), 1, &RecvBytes, &Flags, NULL, NULL) == SOCKET_ERROR)
                {
                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        cout << "WSARecv() failed with error " << WSAGetLastError() << endl;
                        FreeSocketInformation(i);
                    }
                    else
                    {
                        cout << "WSARecv() is OK!" << endl;
                        for(int j = 0; j < RecvBytes; j++)
                        {
                            cout << SocketInfo->Databuf.buf[j];
                        }
                        cout << endl;
                    }
                    continue;
                }
                else
                {
                    SocketInfo->BytesRECV = RecvBytes;
                    if(RecvBytes == 0)
                    {
                        FreeSocketInformation(i);
                        continue;
                    }
                }
            }
            if(FD_ISSET(SocketInfo->Socket, &WriteSet))
            {
                Total--;
                cout << "WRITE SET " << SocketInfo->Socket << endl;
                SocketInfo->Databuf.buf = SocketInfo->Buffer + SocketInfo->BytesSEND;
                SocketInfo->Databuf.len = SocketInfo->BytesRECV - SocketInfo->BytesSEND;

                if(WSASend(SocketInfo->Socket, &(SocketInfo->Databuf), 1, &SendBytes, 0, NULL, NULL) == SOCKET_ERROR)
                {
                    if(WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        cout << "WSASend() failed with error " << WSAGetLastError() << endl;
                        FreeSocketInformation(i);
                    }
                    else
                        cout << "WSASend() is OK!" << endl;
                    continue;
                }
                else
                {
                    SocketInfo->BytesSEND += SendBytes;
                    if(SocketInfo->BytesSEND == SocketInfo->BytesRECV)
                    {
                        SocketInfo->BytesRECV = 0;
                        SocketInfo->BytesSEND = 0;
                    }
                }
            }
        }
    }
}

BOOL CreateSocketInformation(SOCKET s)
{
    LPSOCKET_INFORMATION SI;

    cout << "Accepted socket number" << s << endl;
    
    if((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
        cout << "GlobalAlloc() failed with error " << WSAGetLastError() << endl;
        return FALSE;
    }
    else
        cout << "GlobalAlloc() for SOCKET_INFORMATION is OK!" << endl;
    
    SI->Socket = s;
    SI->BytesSEND = 0;
    SI->BytesRECV = 0;

    SocketArray[TotalSockets] = SI;
    TotalSockets++;
    return TRUE;
}

void FreeSocketInformation(DWORD Index)
{
    LPSOCKET_INFORMATION SI = SocketArray[Index];
    DWORD i;

    closesocket(SI->Socket);
    cout << "Closing socket number " << SI->Socket << endl;
    GlobalFree(SI);

    // 在 QT 中，我可能使用的是链表的数据结构
    // 那么，这个 poll 有什么区别呢
    // linux select 快主要就是数据和 fd 索引对应
    // windows 上 select 绝对没有 linux 的快
    // windows 还是只能够使用 iocp 或者 ovverlap io 这种真正的异步 IO
    // 直接将数组末尾和当前的交换，不可以么？

    for(i = Index; i < TotalSockets; i++)
    {
        SocketArray[i] = SocketArray[i + 1];
    }

    TotalSockets--;
}