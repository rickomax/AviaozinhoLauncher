#pragma once

#include <windows.h>
#include <stdio.h>

#define NETPIPE_BUFFER_SIZE 65535

#ifdef __cplusplus
extern "C" {
#endif
	extern HANDLE netpipe_handle;
	extern char netpipe_buffer[NETPIPE_BUFFER_SIZE];
	extern DWORD netpipe_bytes_read;
	extern DWORD netpipe_bytes_written;

	char NetPipe_Create(void);
	DWORD NetPipe_AvailableBytes(void);
	char NetPipe_ConnectToNew(void);
	char NetPipe_ConnectToExisting(void);
	char NetPipe_Write(const void* data, DWORD size);
	char NetPipe_Read(void* data, DWORD size);
	void NetPipe_Close(void);
#ifdef __cplusplus
}
#endif