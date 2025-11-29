#pragma once
#include <array>
#include <vector>
#include <string>
#include <format>
#include "NetPipe.h"
#include "Static.h"
bool NetBackend_Setup();
void NetBackend_OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
void NetBackend_PumpPipe();
void NetBackend_PollSteamMessages();
