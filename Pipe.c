#include "Pipe.h"

HANDLE pipe_handle = INVALID_HANDLE_VALUE;
char pipe_buffer[PIPE_BUFFER_SIZE];
DWORD pipe_bytes_read;
DWORD pipe_bytes_written;

char Pipe_Create(void) {
    pipe_handle = CreateNamedPipe(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, 0, NULL
    );
    return pipe_handle != INVALID_HANDLE_VALUE;
}

char Pipe_ConnectToNew(void) {
    return ConnectNamedPipe(pipe_handle, NULL);
}

char Pipe_ConnectToExisting(void) {
    pipe_handle = CreateFile(
        PIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL
    );
    if (pipe_handle != INVALID_HANDLE_VALUE) {
        DWORD mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
        SetNamedPipeHandleState(pipe_handle, &mode, NULL, NULL);
    }
    return pipe_handle != INVALID_HANDLE_VALUE;
}

char Pipe_Write(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int newByteCount = vsnprintf(pipe_buffer, PIPE_BUFFER_SIZE, format, args);
    va_end(args);
    return WriteFile(pipe_handle, pipe_buffer, newByteCount, &pipe_bytes_written, NULL);
}

char Pipe_Read(void) {
    char result = ReadFile(pipe_handle, pipe_buffer, PIPE_BUFFER_SIZE - 1, &pipe_bytes_read, NULL);
    pipe_buffer[pipe_bytes_read] = '\0';
    return result;
}

void Pipe_Close(void) {
    CloseHandle(pipe_handle);
}