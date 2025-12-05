#include "SteamCallbacks.h"
#include "Static.h"

void SteamCallbacks::OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* p)
{
    commandLine = p->m_rgchConnect;
}