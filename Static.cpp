#include "Static.h"
HWND mainHwnd;
std::vector<std::pair<std::string, std::string>> languageMap;
std::vector<std::pair<std::string, std::string>> settings;
uint8_t netPayload[NETPIPE_BUFFER_SIZE];
SteamCallbacks steamCallbacks;
std::string commandLine;
