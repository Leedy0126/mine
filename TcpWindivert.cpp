#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "windivert.h"
#include <map>



using namespace std;
#define MAXBUF  0xFFFF


int __cdecl main(int argc, char **argv) {
	HANDLE handle;
	INT16 priority = 0;
	unsigned char packet[MAXBUF];
	UINT packet_len;
	WINDIVERT_ADDRESS recv_addr, send_addr;                  //recv,send 주소

	PWINDIVERT_IPHDR ip_header;

	PWINDIVERT_TCPHDR tcp_header;                                             //---

	UINT payload_len;

	UINT16 toProxy = 8080;
	UINT16 fromProxy = 80;

	struct formap {
		UINT32 ip;
		UINT16 port;

		bool operator<(const formap& m2) {
			return ip + port < m2.ip + m2.port;
		}

	} *Pformap;


	
	formap src;
	formap dst;

	map<formap, formap> m;

	handle = WinDivertOpen(argv[1], WINDIVERT_LAYER_NETWORK, priority, 0);

	if (handle == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_INVALID_PARAMETER)
		{
			fprintf(stderr, "error: filter syntax error\n");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "error: failed to open the WinDivert device (%d)\n",
			GetLastError());
		exit(EXIT_FAILURE);
	}

	while (TRUE)
	{
		if (!WinDivertRecv(handle, packet, sizeof(packet), &recv_addr,
			&packet_len)) {
			fprintf(stderr, "warning: failed to read packet\n");
			continue;
		}

	
		WinDivertHelperParsePacket(packet, packet_len, &ip_header, NULL, NULL, NULL, &tcp_header,  //charArr형의 packet을 구조체를 통해 구조체화
			NULL, NULL, &payload_len);


		//이게 나가는 아이피일때
		if(tcp_header->DstPort== toProxy){
			src.ip = ip_header->SrcAddr;
			src.port = tcp_header->SrcPort;
			dst.ip = ip_header->DstAddr;
			dst.port = tcp_header->DstPort;

			m.insert(pair<formap, formap>(src, dst));

			tcp_header->DstPort = toProxy;
			WinDivertHelperCalcChecksums(packet, packet_len, NULL);
			WinDivertSend(handle, packet, packet_len, &recv_addr, NULL);
		}else if (tcp_header->SrcPort = fromProxy) {
			formap recvFromPx;
			formap result;
			recvFromPx.ip = ip_header->DstAddr;
			recvFromPx.port = tcp_header->DstPort;

			result = m[recvFromPx];

			ip_header->DstAddr = result.ip;
			tcp_header->DstPort = result.port;

			WinDivertHelperCalcChecksums(packet, packet_len, NULL);
			WinDivertSend(handle, packet, packet_len, &recv_addr, NULL);

		}else{
			WinDivertSend(handle, packet, packet_len, &recv_addr, NULL);
		}
		
	
	
		
		



	}
}