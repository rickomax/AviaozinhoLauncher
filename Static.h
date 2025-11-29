#pragma once
#include <steam\isteamnetworkingsockets.h>
#include <string>
#include <vector>
#include "SteamCallbacks.h"
#include "NetBackend.h"
#include "NetPipe.h"
#define MAX_CONN 256
extern std::vector<std::pair<std::string, std::string>> languageMap;
extern std::vector<std::pair<std::string, std::string>> settings;
extern ISteamNetworkingSockets* sockets;
//extern SteamCallbacks steamCallbacks;
extern uint8_t netPayload[NETPIPE_BUFFER_SIZE];
extern HSteamNetConnection connections[MAX_CONN];
extern HSteamListenSocket listenSocket;
extern SteamNetworkingConfigValue_t config;