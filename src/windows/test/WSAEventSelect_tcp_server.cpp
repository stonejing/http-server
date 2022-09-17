// Description:
//
//    This sample illustrates how to develop a simple echo server Winsock
//    application using the WSAEventSelect() I/O model. This sample is
//    implemented as a console-style application and simply prints
//    messages when connections are established and removed from the server.
//    The application listens for TCP connections on port 5150 and accepts them
//    as they arrive. When this application receives data from a client, it
//    simply echoes (this is why we call it an echo server) the data back in
//    it's original form until the client closes the connection.
//
//    Note: There are no command line options for this sample.
// Link to ws2_32.lib
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#define PORT 5150
#define DATA_BUFSIZE 8192

typedef struct _SOCKET_INFORMATION
{
    CHAR buffer[DATA_BUFSIZE];
    WSABUF data_buf;
    SOCKET socket;
    DWORD bytes_send;
    DWORD bytes_recv;
} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

BOOL Createsocket_information(SOCKET s);
void Freesocket_information(DWORD event);

DWORD event_total = 0;
WSAEVENT event_array[WSA_MAXIMUM_WAIT_EVENTS];
LPSOCKET_INFORMATION socket_array[WSA_MAXIMUM_WAIT_EVENTS];

int main(int argc, char **argv)

{
    SOCKET listen_socket;
    SOCKET accept_socket;
    SOCKADDR_IN internet_addr;
    LPSOCKET_INFORMATION socket_info;
    DWORD event;
    WSANETWORKEVENTS network_events;
    WSADATA wsadata;
    DWORD flags;
    DWORD recv_bytes;
    DWORD send_bytes;
    if (WSAStartup(0x0202, &wsadata) != 0)
    {
        printf("WSAStartup() failed with error %d\n", WSAGetLastError());
        return 1;
    }
    else
        printf("WSAStartup() is OK!\n");
    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("socket() failed with error %d\n", WSAGetLastError());
        return 1;
    }
    else
        printf("socket() is OK!\n");
    if (Createsocket_information(listen_socket) == FALSE)
        printf("Createsocket_information() failed!\n");
    else
        printf("Createsocket_information() is OK lol!\n");

    if (WSAEventSelect(listen_socket, event_array[event_total - 1], FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
    {
        printf("WSAEventSelect() failed with error %d\n", WSAGetLastError());
        return 1;
    }
    else
        printf("WSAEventSelect() is pretty fine!\n");

    internet_addr.sin_family = AF_INET;
    internet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    internet_addr.sin_port = htons(PORT);

    if (bind(listen_socket, (PSOCKADDR)&internet_addr, sizeof(internet_addr)) == SOCKET_ERROR)
    {
        printf("bind() failed with error %d\n", WSAGetLastError());
        return 1;
    }
    else
        printf("bind() is OK!\n");
    if (listen(listen_socket, 5))
    {
        printf("listen() failed with error %d\n", WSAGetLastError());
        return 1;
    }
    else
        printf("listen() is OK!\n");
    while (TRUE)
    {
        // Wait for one of the sockets to receive I/O notification and
        if ((event = WSAWaitForMultipleEvents(event_total, event_array, FALSE, WSA_INFINITE, FALSE)) == WSA_WAIT_FAILED)
        {
            printf("WSAWaitForMultipleEvents() failed with error %d\n", WSAGetLastError());
            return 1;
        }
        else
            printf("WSAWaitForMultipleEvents() is pretty damn OK!\n");
        if (WSAEnumNetworkEvents(socket_array[event - WSA_WAIT_EVENT_0]->socket,
                                 event_array[event - WSA_WAIT_EVENT_0], &network_events) == SOCKET_ERROR)
        {
            printf("WSAEnumNetworkEvents() failed with error %d\n", WSAGetLastError());
            return 1;
        }
        else
            printf("WSAEnumNetworkEvents() should be fine!\n");
        if (network_events.lNetworkEvents & FD_ACCEPT)
        {
            if (network_events.iErrorCode[FD_ACCEPT_BIT] != 0)
            {
                printf("FD_ACCEPT failed with error %d\n", network_events.iErrorCode[FD_ACCEPT_BIT]);
                break;
            }
            if ((accept_socket = accept(socket_array[event - WSA_WAIT_EVENT_0]->socket, NULL, NULL)) == INVALID_SOCKET)
            {
                printf("accept() failed with error %d\n", WSAGetLastError());
                break;
            }
            else
                printf("accept() should be OK!\n");
            if (event_total > WSA_MAXIMUM_WAIT_EVENTS)
            {
                printf("Too many connections - closing socket...\n");
                closesocket(accept_socket);
                break;
            }
            Createsocket_information(accept_socket);
            if (WSAEventSelect(accept_socket, event_array[event_total - 1], FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
            {
                printf("WSAEventSelect() failed with error %d\n", WSAGetLastError());
                return 1;
            }
            else
                printf("WSAEventSelect() is OK!\n");
            printf("Socket %d got connected...\n", accept_socket);
        }
        // Try to read and write data to and from the data buffer if read and write events occur
        if (network_events.lNetworkEvents & FD_READ || network_events.lNetworkEvents & FD_WRITE)
        {
            if (network_events.lNetworkEvents & FD_READ && network_events.iErrorCode[FD_READ_BIT] != 0)
            {
                printf("FD_READ failed with error %d\n", network_events.iErrorCode[FD_READ_BIT]);
                break;
            }
            else
                printf("FD_READ is OK!\n");
            if (network_events.lNetworkEvents & FD_WRITE && network_events.iErrorCode[FD_WRITE_BIT] != 0)
            {
                printf("FD_WRITE failed with error %d\n", network_events.iErrorCode[FD_WRITE_BIT]);
                break;
            }
            else
                printf("FD_WRITE is OK!\n");
            socket_info = socket_array[event - WSA_WAIT_EVENT_0];
            // Read data only if the receive buffer is empty
            if (socket_info->bytes_recv == 0)
            {
                socket_info->data_buf.buf = socket_info->buffer;
                socket_info->data_buf.len = DATA_BUFSIZE;
                flags = 0;
                if (WSARecv(socket_info->socket, &(socket_info->data_buf), 1, &recv_bytes, &flags, NULL, NULL) == SOCKET_ERROR)
                {
                    if (WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        printf("WSARecv() failed with error %d\n", WSAGetLastError());
                        Freesocket_information(event - WSA_WAIT_EVENT_0);
                        return 1;
                    }
                }
                else
                {
                    printf("WSARecv() is working!\n");
                    socket_info->bytes_recv = recv_bytes;
                }
            }
            // Write buffer data if it is available
            if (socket_info->bytes_recv > socket_info->bytes_send)
            {
                socket_info->data_buf.buf = socket_info->buffer + socket_info->bytes_send;
                socket_info->data_buf.len = socket_info->bytes_recv - socket_info->bytes_send;
                if (WSASend(socket_info->socket, &(socket_info->data_buf), 1, &send_bytes, 0, NULL, NULL) == SOCKET_ERROR)
                {
                    if (WSAGetLastError() != WSAEWOULDBLOCK)
                    {
                        printf("WSASend() failed with error %d\n", WSAGetLastError());
                        Freesocket_information(event - WSA_WAIT_EVENT_0);
                        return 1;
                    }
                    // A WSAEWOULDBLOCK error has occurred. An FD_WRITE event will be posted
                    // when more buffer space becomes available
                }
                else
                {
                    printf("WSASend() is fine! Thank you...\n");
                    socket_info->bytes_send += send_bytes;
                    if (socket_info->bytes_send == socket_info->bytes_recv)
                    {
                        socket_info->bytes_send = 0;
                        socket_info->bytes_recv = 0;
                    }
                }
            }
        }
        if (network_events.lNetworkEvents & FD_CLOSE)
        {
            if (network_events.iErrorCode[FD_CLOSE_BIT] != 0)
            {
                printf("FD_CLOSE failed with error %d\n", network_events.iErrorCode[FD_CLOSE_BIT]);
                break;
            }
            else
                printf("FD_CLOSE is OK!\n");
            printf("Closing socket information %d\n", socket_array[event - WSA_WAIT_EVENT_0]->socket);
            Freesocket_information(event - WSA_WAIT_EVENT_0);
        }
    }
    return 0;
}

BOOL Createsocket_information(SOCKET s)
{
    LPSOCKET_INFORMATION SI;
    if ((event_array[event_total] = WSACreateEvent()) == WSA_INVALID_EVENT)
    {
        printf("WSACreateEvent() failed with error %d\n", WSAGetLastError());
        return FALSE;
    }
    else
        printf("WSACreateEvent() is OK!\n");
    if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
    {
        printf("GlobalAlloc() failed with error %d\n", GetLastError());
        return FALSE;
    }
    else
        printf("GlobalAlloc() for LPSOCKET_INFORMATION is OK!\n");
    // Prepare socket_info structure for use
    SI->socket = s;
    SI->bytes_send = 0;
    SI->bytes_recv = 0;
    socket_array[event_total] = SI;
    event_total++;
    return (TRUE);
}

void Freesocket_information(DWORD Event)
{
    LPSOCKET_INFORMATION SI = socket_array[Event];
    DWORD i;
    closesocket(SI->socket);
    GlobalFree(SI);
    if (WSACloseEvent(event_array[Event]) == TRUE)
        printf("WSACloseEvent() is OK!\n\n");
    else
        printf("WSACloseEvent() failed miserably!\n\n");
    // Squash the socket and event arrays
    for (i = Event; i < event_total; i++)
    {
        event_array[i] = event_array[i + 1];
        socket_array[i] = socket_array[i + 1];
    }
    event_total--;
}