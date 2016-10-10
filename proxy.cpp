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

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8080"


std::string replace(char *bufArr) {
   std::string buf(bufArr);
   buf.replace(buf.find("hacking"), sizeof("asdfgf") - 1, "asdfgf");
   return buf;
}
void sendToClient(SOCKET csock, std::string buf) {
   send(csock, buf.c_str(), buf.length(), 0);


   std::cout << buf << std::endl;
}

void proxy(SOCKET csock, char *bufArr, int length) {
   char *port ="80";
   std::string buf(bufArr);
   buf = buf.substr(0,length);
   int i= buf.find("Host: ")+strlen("Host: ");
   std::string address;
   while(buf.c_str()[i] !='\r'){     //
      address += buf.c_str()[i];
      i++;
   }
   
   
   std::cout<<address<<std::endl;

   struct addrinfo *result =NULL;
   struct addrinfo *ptr = NULL,hints;
   SOCKET ConnectSocket = INVALID_SOCKET;

   ZeroMemory(&hints,sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;

   int check = getaddrinfo(address.c_str(), port, &hints, &result);
   if (check != 0) {
      printf("getaddrinfo error2\n");
      WSACleanup();
      exit(1);
   }

   for(ptr = result; ptr != NULL; ptr = ptr->ai_next){//
      
      csock = socket(ptr->ai_family,ptr->ai_socktype,ptr->ai_protocol);
      if(csock == INVALID_SOCKET){
         printf("socket error2\n");
         WSACleanup();
         exit(1);
      }

      check = connect(csock,ptr->ai_addr, (int)ptr->ai_addrlen);
      if (check == SOCKET_ERROR) {
         printf("getaddrinfo error2\n");
         closesocket(csock);
         csock=INVALID_SOCKET;
         continue;
      }
      int recvbuflen = DEFAULT_BUFLEN;
      char recvbuf[DEFAULT_BUFLEN] = "";
      do {
         check = recv(csock, recvbuf, recvbuflen, 0);
         if (check >0) {
            sendToClient(csock, replace(recvbuf));
         }
         else if (check == 0) {
            wprintf(L"Bytes received: %d\n", check);
         }
         else {
            wprintf(L"Bytes received: %d\n", check);
         }
      } while (check >0);

      break;
   }
   freeaddrinfo(result);
}
   }

}
int _cdecl main() {
   WSADATA wsaData;
   int check;

   SOCKET ssock = INVALID_SOCKET;
   SOCKET csock = INVALID_SOCKET;

   struct addrinfo *result = NULL;
   struct addrinfo hints;

   char recvbuf[DEFAULT_BUFLEN];
   int recvbuflen = DEFAULT_BUFLEN;

   check = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (check != 0) {
      printf("WSAStartup error\n");
      return 1;
   }

   check = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
   if (check != 0) {
      printf("getaddrinfo error\n");
      return 1;
   }

   ssock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
   if (ssock == INVALID_SOCKET) {
      printf("SSocket error\n");
      closesocket(ssock);
      WSACleanup();
      return 1;

   }

   check = bind(ssock, result->ai_addr, (int)result->ai_addrlen);
   if (check == SOCKET_ERROR) {
      printf("Bind error\n");
      closesocket(ssock);
      WSACleanup();
      return 1;

   }
   while (1) {
      csock = accept(csock, NULL, NULL);
      if (csock == INVALID_SOCKET) {
         printf("accept error\n");
         closesocket(ssock);
         WSACleanup();
         return 1;

      }
      do {
         check = recv(csock, recvbuf, recvbuflen, NULL);
         if (check > 0) {
            std::thread(proxy, csock, recvbuf, check).detach();
         }
         else if (check == 0)
            printf("Connection closing...\n");
         else {
            printf("recv error: %d\n", WSAGetLastError());
            closesocket(csock);
            WSACleanup();
            return 1;
         }

      } while (check > 0);
   }
   closesocket(ssock);

   check = shutdown(csock, SD_SEND);
   if (check == SOCKET_ERROR) {
      printf("shutdown error\n");
      closesocket(ssock);
      WSACleanup();
      return 1;

   }
   closesocket(csock);
   WSACleanup();


}
