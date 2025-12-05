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

	BOOL NetPipe_Create(void);
	DWORD NetPipe_AvailableBytes(void);
	BOOL NetPipe_ConnectToNew(void);
	BOOL NetPipe_ConnectToExisting(void);
	BOOL NetPipe_Write(const void* data, DWORD size);
	BOOL NetPipe_Read(void* data, DWORD size);
	void NetPipe_Close(void);
	BOOL NetPipe_IsConnected(void);
	void NetPipe_BeginConnect(void);
#ifdef __cplusplus
}
#endif