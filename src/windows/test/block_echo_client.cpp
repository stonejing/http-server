#include <WinSock2.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	WORD sockVersion = MAKEWORD(2, 2);
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		return 0;
	}
	SOCKET sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == SOCKET_ERROR) {
		printf("socket error\n");
		return 0;
	}
	struct sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(5150);
	servAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (connect(sClient, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		printf("socket error\n");
		closesocket(sClient);
		return 0;
	}
	char* buff = (char*)"\r\nHello, my friend\r\n";
	send(sClient, buff, strlen(buff), 0);
	char revData[255];
	int ret = recv(sClient, revData, 255, 0);
	if (ret > 0) {
		revData[ret] = 0x00;
		printf(revData);
	}
	closesocket(sClient);
	WSACleanup();
	return 0;
}