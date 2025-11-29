#include "Static.h"
std::vector<std::pair<std::string, std::string>> languageMap;
std::vector<std::pair<std::string, std::string>> settings;
ISteamNetworkingSockets* sockets;
//SteamCallbacks steamCallbacks;
uint8_t netPayload[NETPIPE_BUFFER_SIZE];
HSteamNetConnection connections[MAX_CONN];
HSteamListenSocket listenSocket = k_HSteamListenSocket_Invalid;
SteamNetworkingConfigValue_t config;
