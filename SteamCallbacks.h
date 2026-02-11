#pragma once
#include "steam/steam_api.h"
#include "Pipe.h"

class SteamCallbacks
{
public:
    SteamCallbacks()
        : 
        m_cbJoinRequested(this, &SteamCallbacks::OnGameRichPresenceJoinRequested),
        m_cbLobbyCreated(this, &SteamCallbacks::OnLobbyCreated),
        m_LobbyMatchList(this, &SteamCallbacks::OnLobbyMatchList),
        m_LobbyDataUpdate(this, &SteamCallbacks::OnLobbyDataUpdate)
    {
    }

private:
    STEAM_CALLBACK(SteamCallbacks, OnGameRichPresenceJoinRequested, GameRichPresenceJoinRequested_t, m_cbJoinRequested);
    STEAM_CALLBACK(SteamCallbacks, OnLobbyCreated, LobbyCreated_t, m_cbLobbyCreated);
    STEAM_CALLBACK(SteamCallbacks, OnLobbyMatchList, LobbyMatchList_t, m_LobbyMatchList);
    STEAM_CALLBACK(SteamCallbacks, OnLobbyDataUpdate, LobbyDataUpdate_t, m_LobbyDataUpdate);
};
