#pragma once
#include <steam\isteamnetworkingsockets.h>
#include <string>
#include <vector>
#include "SteamCallbacks.h"
#include "NetBackend.h"
#include "NetPipe.h"
extern HWND mainHwnd;
extern std::vector<std::pair<std::string, std::string>> languageMap;
extern std::vector<std::pair<std::string, std::string>> settings;
extern uint8_t netPayload[NETPIPE_BUFFER_SIZE];
extern SteamCallbacks steamCallbacks;
extern std::string commandLine;
static void ShowError(const char* msg)
{
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}