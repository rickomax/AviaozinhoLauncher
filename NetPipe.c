#include "NetPipe.h"

HANDLE netpipe_handle = INVALID_HANDLE_VALUE;
char   netpipe_buffer[NETPIPE_BUFFER_SIZE];
DWORD  netpipe_bytes_read;
DWORD  netpipe_bytes_written;

OVERLAPPED g_netov;
HANDLE g_netevent = NULL;

BOOL NetPipe_Create(void) {
    netpipe_handle = CreateNamedPipeA(
        NETPIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        NETPIPE_BUFFER_SIZE,
        NETPIPE_BUFFER_SIZE,
        0,
        NULL
    );
    if (netpipe_handle == INVALID_HANDLE_VALUE) return FALSE;
    g_netevent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ZeroMemory(&g_netov, sizeof(g_netov));
    g_netov.hEvent = g_netevent;
    return TRUE;
}

DWORD NetPipe_AvailableBytes(void)
{
    if (netpipe_handle == INVALID_HANDLE_VALUE) {
        return 0;
    }
    DWORD bytesAvailable = 0;
    if (!PeekNamedPipe(netpipe_handle, NULL, 0, NULL, &bytesAvailable, NULL)) {
        return 0;
    }
    return bytesAvailable;
}

BOOL NetPipe_ConnectToNew(void) {
    return ConnectNamedPipe(netpipe_handle, NULL);
}

BOOL NetPipe_ConnectToExisting(void)
{
    netpipe_handle = CreateFileA(
        NETPIPE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (netpipe_handle != INVALID_HANDLE_VALUE) {

        DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
        SetNamedPipeHandleState(netpipe_handle, &mode, NULL, NULL);
    }

    return netpipe_handle != INVALID_HANDLE_VALUE;
}

BOOL NetPipe_Write(const void* data, DWORD size) {
    const char* p = (const char*)data;
    DWORD total = 0;
    while (total < size) {
        DWORD written = 0;
        if (!WriteFile(netpipe_handle, p + total, size - total, &written, NULL)) {
            return 0;
        }
        if (written == 0) {
            return 0;
        }
        total += written;
    }
    netpipe_bytes_written = total;
    return 1;
}

BOOL NetPipe_Read(void* data, DWORD size) {
    char* p = (char*)data;
    DWORD total = 0;
    while (total < size) {
        DWORD got = 0;
        if (!ReadFile(netpipe_handle, p + total, size - total, &got, NULL)) {
            return 0;
        }
        if (got == 0) {
            return 0;
        }
        total += got;
    }
    netpipe_bytes_read = total;
    return 1;
}

void NetPipe_Close(void) {
    if (netpipe_handle != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(netpipe_handle);
        CloseHandle(netpipe_handle);
        netpipe_handle = INVALID_HANDLE_VALUE;
    }
}

void NetPipe_BeginConnect(void) {
    ResetEvent(g_netevent);
    BOOL ok = ConnectNamedPipe(netpipe_handle, &g_netov);
    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_CONNECTED) {
            SetEvent(g_netevent);
        }
    }
}

BOOL NetPipe_IsConnected(void) {
    if (WaitForSingleObject(g_netevent, 0) == WAIT_OBJECT_0) {
        return TRUE;
    }
    return FALSE;
}

