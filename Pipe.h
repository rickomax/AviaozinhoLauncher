#pragma once

#include <windows.h>
#include <stdio.h>

#define PIPE_BUFFER_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif
	extern HANDLE pipe_handle;
	extern char pipe_buffer[PIPE_BUFFER_SIZE];
	extern DWORD pipe_bytes_read;
	extern DWORD pipe_bytes_written;

	BOOL  Pipe_Create(void);
	DWORD Pipe_AvailableBytes(void);
	BOOL  Pipe_ConnectToNew(void);
	BOOL  Pipe_ConnectToExisting(void);
	BOOL  Pipe_Write(const char* format, ...);
	BOOL  Pipe_Read(void);
	void Pipe_Close(void);
	BOOL Pipe_IsConnected(void);
	void Pipe_BeginConnect(void);
#ifdef __cplusplus
}
#endif