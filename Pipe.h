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

	char Pipe_Create(void);
	DWORD Pipe_AvailableBytes(void);
	char Pipe_ConnectToNew(void);
	char Pipe_ConnectToExisting(void);
	char Pipe_Write(const char* format, ...);
	char Pipe_Read(void);
	void Pipe_Close(void);
#ifdef __cplusplus
}
#endif