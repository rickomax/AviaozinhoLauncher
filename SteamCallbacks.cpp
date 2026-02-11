#include "SteamCallbacks.h"
#include "Static.h"

#include <cstdio>
#include <cstdlib> 
#include <sstream>

void SteamCallbacks::OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* p)
{
    if (!p)
        return;

    commandLine = p->m_rgchConnect;
}

void SteamCallbacks::OnLobbyCreated(LobbyCreated_t* p)
{
    if (!p || p->m_eResult != k_EResultOK)
    {
        std::printf("CreateLobby failed: %d\n", p ? (int)p->m_eResult : -1);
        return;
    }

    lobbyId = p->m_ulSteamIDLobby;
    CSteamID lid(p->m_ulSteamIDLobby);

    const uint64_t hostId = SteamUser()->GetSteamID().ConvertToUint64();
    SteamMatchmaking()->SetLobbyData(lobbyId, "host", std::to_string(hostId).c_str());

    SteamMatchmaking()->SetLobbyJoinable(lid, true);
}

void SteamCallbacks::OnLobbyMatchList(LobbyMatchList_t* p)
{
    if (!p || !serverListInProgress)
        return;

    activeServerListRequestId = serverListRequestId;

    serverMap.clear();

    expectedLobbies = p->m_nLobbiesMatching;
    pendingLobbies = expectedLobbies;

    if (expectedLobbies <= 0)
    {
        serverListInProgress = false;
        Pipe_Write("{ \"servers\": [] }");
        return;
    }

    for (int i = 0; i < expectedLobbies; ++i)
    {
        CSteamID lid = SteamMatchmaking()->GetLobbyByIndex(i);
        uint64_t key = lid.ConvertToUint64();

        ServerEntry& e = serverMap[key];
        e.lobbyId = key;

        SteamMatchmaking()->RequestLobbyData(lid);
    }
}

void SteamCallbacks::OnLobbyDataUpdate(LobbyDataUpdate_t* p)
{
    if (!p || !p->m_bSuccess)
        return;

    if (!serverListInProgress)
        return;

    // Ignore callbacks from older list requests
    if (activeServerListRequestId != serverListRequestId)
        return;

    CSteamID lid(p->m_ulSteamIDLobby);
    const uint64_t key = lid.ConvertToUint64();

    auto it = serverMap.find(key);
    if (it == serverMap.end())
    {
        // Not in our current list (could be some unrelated update)
        return;
    }

    ServerEntry& entry = it->second;

    // Steam can fire LobbyDataUpdate more than once per lobby.
    // With a map, we still need to decrement pending only once per lobby,
    // so keep a per-entry flag.
    if (!entry.gotData)
    {
        entry.gotData = true;
        pendingLobbies--;
        if (pendingLobbies < 0)
            pendingLobbies = 0; // safety
    }

    const char* host = SteamMatchmaking()->GetLobbyData(lid, "host");     // host steamid string
    const char* map = SteamMatchmaking()->GetLobbyData(lid, "map");
    const char* cli = SteamMatchmaking()->GetLobbyData(lid, "clients");
    const char* maxc = SteamMatchmaking()->GetLobbyData(lid, "maxc");
    const char* name = SteamMatchmaking()->GetLobbyData(lid, "name");

    // Fill what we can. Don’t require host to exist.
    if (host && *host)
        entry.address = std::string("steam-conn|") + host;
    else
        entry.address.clear();

    entry.hostname = (name && *name) ? name : "UNNAMED";
    entry.map = map ? map : "";
    entry.clients = (cli && *cli) ? std::atoi(cli) : 0;
    entry.maxClients = (maxc && *maxc) ? std::atoi(maxc) : 0;

    // Populate the extra fields QSSM expects (keep them stable even if you don't use them yet)
    entry.lobbyId = key;

    // If you don't have any extra params/port, keep them empty/0.
    // (Don't guess net_hostport here unless you're explicitly storing it.)
    if (entry.parameters.empty())
        entry.parameters = "";

    if (entry.port < 0)
        entry.port = 0;

    if (pendingLobbies <= 0)
    {
        serverListInProgress = false;

        std::ostringstream json;
        json << "[";

        bool first = true;
        for (const auto& kv : serverMap)
        {
            const ServerEntry& s = kv.second;

            if (!first) json << ",";
            first = false;

            json << "{";

            json << "\"hostname\":\"" << JsonEscape(s.hostname) << "\",";
            
            json << "\"address\":\"" << JsonEscape(s.address) << "\",";

            json << "\"maxPlayers\":" << (s.maxClients > 0 ? s.maxClients : 0) << ",";

            json << "\"map\":\"" << JsonEscape(s.map) << "\",";

            json << "\"parameters\":\"" << JsonEscape(s.parameters) << "\",";

            json << "\"gameId\":" << 0 << ",";

            json << "\"port\":" << (s.port > 0 ? s.port : 0) << ",";

            char timebuf[20];
            std::time_t t = std::time(nullptr);
            std::tm tm;
            gmtime_s(&tm, &t);

            // Exactly 19 chars: YYYY-MM-DDTHH:MM:SS
            std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S", &tm);

            json << "\"timestamp\":\"" << timebuf << "\",";
            json << "\"lastQuery\":\"" << timebuf << "\",";

            const int playerCount = (s.clients > 0) ? s.clients : 0;
            json << "\"players\":[";
            for (int i = 0; i < playerCount; ++i)
            {
                if (i) json << ",";
                json << "{}";
            }
            json << "]";

            json << "}";
        }

        json << "]";
        Pipe_Write(json.str().c_str());
    }
}
