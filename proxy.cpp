#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <thread>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8080"

std::string replace(char *bufArr) {
	std::string buf(bufArr);
	buf.replace(buf.find("hello"), sizeof("asdfg") - 1, "asdfg");
	return buf;
}
void sendToClient(SOCKET ConnectSocket, std::string buf) {
	send(ConnectSocket, buf.c_str(), buf.length(), 0);


	std::cout << buf << std::endl;
}

void proxy(SOCKET ClientSocket, char *bufArr, int length) {
	std::string buf(bufArr);
	buf = buf.substr(0, length);
	int i = buf.find("Host: ") + strlen("Host: ");
	std::string address;
	while (buf.c_str()[i] != '\r') {
		address += buf.c_str()[i];
		i++;
	}
	char *port = "80";
	if (address.find(":") != std::string::npos) {
		address = address.substr(0, address.find(":"));
		port = "443";
	}

	std::cout << address << std::endl;



	struct addrinfo *result = NULL,*ptr = NULL,hints;
	SOCKET ConnectSocket = INVALID_SOCKET;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve server address and port
	int iResult = getaddrinfo(address.c_str(), port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		exit(1);
	}

	// connect to address
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create socket for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			exit(1);
		}

		// Connect to Server
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		iResult = send(ConnectSocket, buf.c_str(), buf.length(), 0);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		int recvbuflen = DEFAULT_BUFLEN;
		char recvbuf[DEFAULT_BUFLEN] = "";
		do {
			iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
			if (iResult >0) {
				sendToClient(ClientSocket, replace(recvbuf));
			}
			else if (iResult == 0) {
				wprintf(L"Bytes received: %d\n", iResult);
			}
			else {
				wprintf(L"Bytes received: %d\n", iResult);
			}
		} while (iResult >0);

		break;
	}
	freeaddrinfo(result);
}

int __cdecl main(void)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ServerSocket = INVALID_SOCKET;      //클라이언트로부터 받아드릴 소켓
	SOCKET ClientSocket = INVALID_SOCKET;      //클라이언트로 접속 한 소켓

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));      //hints 라는 구조체를 0으로 다 채움
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ServerSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ServerSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}

	while (true) {

		// Accept a client socket
		ClientSocket = accept(ServerSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ServerSocket);
			WSACleanup();
			return 1;
		}
		// Receive until the peer shuts down the connection
		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);

			if (iResult > 0) {
				std::thread(proxy, ClientSocket, recvbuf, iResult).detach();
			}
			else if (iResult == 0)
				printf("Connection closing...\n");
			else {
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}

		} while (iResult > 0);
	}
	// No longer need server socket
	closesocket(ServerSocket);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}