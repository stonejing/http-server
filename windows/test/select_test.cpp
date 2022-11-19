// Description:
//
//    This sample illustrates how to develop a simple echo server Winsock
//    application using the select() API I/O model. This sample is
//    implemented as a console-style application and simply prints
//    messages when connections are established and removed from the server.
//    The application listens for TCP connections on port 5150 and accepts
//    them as they arrive. When this application receives data from a client,
//    it simply echos (this is why we call it an echo server) the data back in
//    it's original form until the client closes the connection.
//
//    Note: There are no command line options for this sample.
//
// Link to ws2_32.lib

#define FD_SETSIZE 1024

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#define PORT 5150
#define DATA_BUFSIZE 1024

typedef struct _SOCKET_INFORMATION {
	CHAR buffer[DATA_BUFSIZE];
	WSABUF data_buf;
	SOCKET local_socket;
	SOCKET remote_socket;
	DWORD bytes_send;
	DWORD bytes_recv;
} SOCKET_INFORMATION, * LPSOCKET_INFORMATION;

// Prototypes
BOOL Createsocket_information(SOCKET s);
void Freesocket_information(DWORD index);

// Global var
DWORD total_sockets = 0;
LPSOCKET_INFORMATION socket_array[FD_SETSIZE];

int main(int argc, char** argv)
{
	SOCKET listen_socket;
	SOCKET accept_socket;
	SOCKADDR_IN internet_addr;
	WSADATA wsadata;
	INT ret;
	FD_SET write_set;
	FD_SET read_set;
	DWORD i;
	DWORD total;
	ULONG nonblock;
	DWORD flags;
	DWORD send_bytes;
	DWORD recv_bytes;

	if ((ret = WSAStartup(0x0202, &wsadata)) != 0)
	{
		printf("WSAStartup() failed with error %d\n", ret);
		WSACleanup();
		return 1;
	}
	else
		printf("WSAStartup() is fine!\n");
	// Prepare a socket to listen for connections
	if ((listen_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	{
		printf("WSASocket() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("WSASocket() is OK!\n");
	
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
	// Change the socket mode on the listening socket from blocking to
	// non-block so the application will not block waiting for requests
	nonblock = 1;
	if (ioctlsocket(listen_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		return 1;
	}
	else
		printf("ioctlsocket() is OK!\n");
	
	while (TRUE)
	{
		// Prepare the Read and Write socket sets for network I/O notification
		FD_ZERO(&read_set);
		FD_ZERO(&write_set);
		// Always look for connection attempts
		FD_SET(listen_socket, &read_set);
		// Set Read and Write notification for each socket based on the
		// current state the buffer.  If there is data remaining in the
		// buffer then set the Write set otherwise the Read set
		for (i = 0; i < total_sockets; i++)
			if (socket_array[i]->bytes_recv > socket_array[i]->bytes_send)
				FD_SET(socket_array[i]->local_socket, &write_set);
			else
				FD_SET(socket_array[i]->local_socket, &read_set);
		if ((total = select(0, &read_set, &write_set, NULL, NULL)) == SOCKET_ERROR)
		{
			printf("select() returned with error %d\n", WSAGetLastError());
			return 1;
		}
		else
			printf("select() is OK!\n");
		// Check for arriving connections on the listening socket.
		if (FD_ISSET(listen_socket, &read_set))
		{
			total--;
			if ((accept_socket = accept(listen_socket, NULL, NULL)) != INVALID_SOCKET)
			{
				// Set the accepted socket to non-blocking mode so the server will
				// not get caught in a blocked condition on WSASends
				nonblock = 1;
				if (ioctlsocket(accept_socket, FIONBIO, &nonblock) == SOCKET_ERROR)
				{
					printf("ioctlsocket(FIONBIO) failed with error %d\n", WSAGetLastError());
					return 1;
				}
				else
					printf("ioctlsocket(FIONBIO) is OK!\n");
				if (Createsocket_information(accept_socket) == FALSE)
				{
					printf("Createsocket_information(AcceptSocket) failed!\n");
					return 1;
				}
				else
					printf("Createsocket_information() is OK!\n");
			}
			else
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					printf("accept() failed with error %d\n", WSAGetLastError());
					return 1;
				}
				else
					printf("accept() is fine!\n");
			}
		}
		// Check each socket for Read and Write notification until the number
		// of sockets in total is satisfied
		for (i = 0; total > 0 && i < total_sockets; i++)
		{
			LPSOCKET_INFORMATION socket_info = socket_array[i];
			// If the ReadSet is marked for this socket then this means data
			// is available to be read on the socket
			if (FD_ISSET(socket_info->local_socket, &read_set))
			{
				total--;
				socket_info->data_buf.buf = socket_info->buffer;
				socket_info->data_buf.len = DATA_BUFSIZE;
				flags = 0;

				if (WSARecv(socket_info->local_socket, &(socket_info->data_buf), 1, &recv_bytes, &flags, NULL, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						printf("WSARecv() failed with error %d\n", WSAGetLastError());
						Freesocket_information(i);
					}
					else
						printf("WSARecv() is OK!\n");
					continue;
				}
				else
				{
					socket_info->bytes_recv = recv_bytes;
					// If zero bytes are received, this indicates the peer closed the connection.
					if (recv_bytes == 0)
					{
						Freesocket_information(i);
						continue;
					}
				}
			}
			// If the write_set is marked on this socket then this means the internal
			// data buffers are available for more data
			if (FD_ISSET(socket_info->local_socket, &write_set))
			{
				total--;
				socket_info->data_buf.buf = socket_info->buffer + socket_info->bytes_send;
				socket_info->data_buf.len = socket_info->bytes_recv - socket_info->bytes_send;
				if (WSASend(socket_info->local_socket, &(socket_info->data_buf), 1, &send_bytes, 0, NULL, NULL) == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						printf("WSASend() failed with error %d\n", WSAGetLastError());
						Freesocket_information(i);
					}
					else
						printf("WSASend() is OK!\n");
					continue;
				}
				else
				{
					socket_info->bytes_send += send_bytes;
					if (socket_info->bytes_send == socket_info->bytes_recv)
					{
						socket_info->bytes_send = 0;
						socket_info->bytes_recv = 0;
					}
				}
			}
		}
	}
}

BOOL Createsocket_information(SOCKET local_socket)
{
	LPSOCKET_INFORMATION SI;
	printf("Accepted socket number %d\n", local_socket);
	if ((SI = (LPSOCKET_INFORMATION)GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
	{
		printf("GlobalAlloc() failed with error %d\n", GetLastError());
		return FALSE;
	}
	else
		printf("GlobalAlloc() for SOCKET_INFORMATION is OK!\n");
	// Prepare socket_info structure for use
	SI->local_socket = local_socket;
	SI->bytes_send = 0;
	SI->bytes_recv = 0;
	socket_array[total_sockets] = SI;
	total_sockets++;
	return(TRUE);
}

void Freesocket_information(DWORD Index)
{
	LPSOCKET_INFORMATION SI = socket_array[Index];
	DWORD i;
	closesocket(SI->local_socket);
	closesocket(SI->remote_socket);
	printf("Closing socket number %d\n", SI->local_socket);
	GlobalFree(SI);
	// Squash the socket array
	for (i = Index; i < total_sockets; i++)
	{
		socket_array[i] = socket_array[i + 1];
	}
	total_sockets--;
}