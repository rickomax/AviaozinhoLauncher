#include "Static.h"
HWND mainHwnd;

#if false
uint8_t netPayload[NETPIPE_BUFFER_SIZE];
#endif

std::vector<std::pair<std::string, std::string>> languageMap;
std::vector<std::pair<std::string, std::string>> settings;

SteamCallbacks steamCallbacks;

std::string commandLine;
uint64_t lobbyId;

std::unordered_map<uint64_t, ServerEntry> serverMap;
int expectedLobbies;
int pendingLobbies;
bool serverListInProgress;

int serverListRequestId;
int activeServerListRequestId;