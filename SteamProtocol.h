#pragma once
#include <stdint.h>

#define NETPIPE_OP_OPEN       1
#define NETPIPE_OP_OPEN_OK    2 
#define NETPIPE_OP_OPEN_ERR   3

#define NETPIPE_OP_DATA       4 
#define NETPIPE_OP_CLOSE      5 
#define NETPIPE_OP_CLOSED     6 

#define NETPIPE_OP_HOST_ON    7
#define NETPIPE_OP_HOST_ERR   8
#define NETPIPE_OP_HOST_OK    9

#define NETPIPE_OP_HOST_OFF   10
#define NETPIPE_OP_NEWCONN	  11

#pragma pack(push,1)
typedef struct NetPipeHeader
{
	uint8_t  op;        // operation code (OPEN, DATA, CLOSE, etc.)
	uint8_t  connId;    // small local connection ID (1..255)
	uint16_t length;    // size of payload following this header
	uint8_t reliable;   // reliable?
} NetPipeHeader;
#pragma pack(pop)

static NetPipeHeader WriteHeader(uint8_t op, uint8_t connId, uint16_t length, const void* data, uint8_t reliable) {
	NetPipeHeader header;
	header.op = op;
	header.connId = connId;
	header.length = length;
	header.reliable = reliable;
	if (!NetPipe_Write(&header, sizeof(header))) {
		printf("Error writing header [%i]", op);
		exit(300);
	}
	if (header.length > 0 && data) {
		if (!NetPipe_Write(data, header.length)) {
			printf("Error writing data [%i]", length);
			exit(300);
		}
		printf("\x1b[32m[SERVER]>>>%.*s\n\x1b[0m", header.length, data);
	}
	//printf("Write data to client [%i](%i):%i\n", header.op, header.connId, header.length);
	return header;
}

static NetPipeHeader ReadHeader(void* payload) {
	NetPipeHeader header;
	if (!NetPipe_Read(&header, (DWORD)sizeof(header))) {
		printf("Error reading header");
		exit(300);
	}
	if (header.length > 0 && payload) {
		if (!NetPipe_Read(payload, header.length)) {
			printf("Error reading  data");
			exit(300);
		}
		printf("\x1b[32m[SERVER]<<<%.*s\n\x1b[0m", header.length, payload);
	}
	//printf("Got data from client [%i](%i):%i\n", header.op, header.connId, header.length);
	return header;
}