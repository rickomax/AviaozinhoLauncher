#pragma once
#include <stdint.h>

#define GNS_FAIL 0
#define GNS_SUCCESS 1

#define GNS_SENDTO 2
#define GNS_RECVFROM 3

#define GNS_INIT 4
#define GNS_SHUTDOWN 5

#define GNS_ERR_WRITE 100
#define GNS_ERR_READ 101

#pragma pack(push,1)
typedef struct NetPipeHeader
{
	uint8_t  op;
	int32_t  socket;
	uint64_t steamId;
	int32_t length;
} NetPipeHeader;
#pragma pack(pop)

static NetPipeHeader WriteHeader(uint8_t op, int32_t socket, uint64_t steamId, const void* data, int32_t length) {
	NetPipeHeader header;
	header.op = op;
	header.socket = socket;
	header.steamId = steamId;
	header.length = length;
	if (!NetPipe_Write(&header, sizeof(header))) {
#ifdef _DEBUGX
		printf("Error writing header [%i]", op);
#endif
		exit(GNS_ERR_WRITE);
	}
	if (header.length > 0 && data) {
		if (!NetPipe_Write(data, header.length)) {
#ifdef _DEBUGX
			printf("Error writing data [%i]", length);
#endif
			exit(GNS_ERR_WRITE);
		}
#ifdef _DEBUGX
		printf("\x1b[33m[CLIENT]>>>%.*s\n\x1b[0m", header.length, data);
#endif
	}
	return header;
}

static NetPipeHeader ReadHeader(void* payload) {
	NetPipeHeader header;
	if (!NetPipe_Read(&header, (DWORD)sizeof(header))) {
#ifdef _DEBUGX
		printf("Error reading header");
#endif
		exit(GNS_ERR_READ);
	}
	if (header.length > 0 && payload) {
		if (!NetPipe_Read(payload, header.length)) {
#ifdef _DEBUGX
			printf("Error reading  data");
#endif
			exit(GNS_ERR_READ);
		}
#ifdef _DEBUGX
		printf("\x1b[33m[CLIENT]<<<%.*s\n\x1b[0m", header.length, payload);
#endif
	}
	return header;
}