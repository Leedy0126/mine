#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "windivert.h"




using namespace std;
#define MAXBUF  0xFFFF



static void PacketIpInit(PWINDIVERT_IPHDR packet)
{
	memset(packet, 0, sizeof(WINDIVERT_IPHDR));
	packet->Version = 4;
	packet->HdrLength = sizeof(WINDIVERT_IPHDR) / sizeof(UINT32);   //헤더는 원래 길이값에 8을나눈값
	packet->Id = ntohs(0xDEAD);
	packet->TTL = 64;
}


int __cdecl main(int argc, char **argv) {
	HANDLE handle;
	INT16 priority = 0;
	unsigned char packet[MAXBUF];
	UINT packet_len;
	WINDIVERT_ADDRESS recv_addr, send_addr;                  //recv,send 주소
	PWINDIVERT_IPHDR ip_header;
	PWINDIVERT_UDPHDR udp_header;
	UINT payload_len;
//	UINT8 *destination;
//	UINT8 *target;
	char *trg = "10.100.111.219";
	char *vct = "10.100.111.71";
	UINT32 *trg_dst = (UINT32 *)malloc(sizeof(UINT32));
	UINT32 *vct_dst = (UINT32 *)malloc(sizeof(UINT32));
	ZeroMemory(trg_dst, sizeof(UINT32));
	ZeroMemory(vct_dst, sizeof(UINT32));

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
			&packet_len)){
			fprintf(stderr, "warning: failed to read packet\n");
			continue;
		}
		WinDivertHelperParsePacket(packet, packet_len, &ip_header, NULL, NULL, NULL, NULL,  //charArr형의 packet을 구조체를 통해 구조체화
			&udp_header, NULL, &payload_len);
		
		if (ip_header != NULL)
		{
			//	ip_header->DstAddr=
			//destination = (UINT8*)&ip_header->DstAddr;
			//  얘랑비교 &ip_header->DstAddr
			//destination[3] = 219;
			//printf("Destination: %d %d\n",dst_addr,target);
			
			WinDivertHelperParseIPv4Address(trg, trg_dst);
			WinDivertHelperParseIPv4Address(vct, vct_dst);
	
			//if (ip_header->DstAddr == *vct_dst) {
			//	ip_header->DstAddr = *trg_dst;
			//	printf("dst is changed \n");
			//}

			if (ip_header->DstAddr == inet_addr(vct)) ip_header->DstAddr = inet_addr(trg);
			
			
			UINT8 *src_addr = (UINT8 *)&ip_header->SrcAddr;
			UINT8 *dst_addr = (UINT8 *)&ip_header->DstAddr;
			printf("ip.SrcAddr=%u.%u.%u.%u ip.DstAddr=%u.%u.%u.%u ",
				src_addr[0], src_addr[1], src_addr[2], src_addr[3],
				dst_addr[0], dst_addr[1], dst_addr[2], dst_addr[3]);

			printf("\n\n\n\n");
			WinDivertHelperCalcChecksums(packet, packet_len, NULL);
			WinDivertSend(handle, packet, packet_len, &recv_addr, NULL);
		}
	}
	putchar('\n');
}
