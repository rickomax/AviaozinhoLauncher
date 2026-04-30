#include "SteamCallbacks.h"
#include "Static.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <format>
#include <ctime>

void SteamCallbacks::OnGameRichPresenceJoinRequested(GameRichPresenceJoinRequested_t* p)
{
    if (!p)
        return;

    commandLine = p->m_rgchConnect;
}

void SteamCallbacks::OnGameLobbyJoinRequested(GameLobbyJoinRequested_t* p)
{
    if (!p)
        return;

    pendingJoinLobbyId = p->m_steamIDLobby;

    if (lobbyId != 0 && pendingJoinLobbyId.ConvertToUint64() == lobbyId)
        return;

    SteamMatchmaking()->JoinLobby(pendingJoinLobbyId);
}

void SteamCallbacks::OnLobbyCreated(LobbyCreated_t* p)
{
    if (!p || p->m_eResult != k_EResultOK)
    {
        std::printf("CreateLobby failed: %d\n", p ? (int)p->m_eResult : -1);
        return;
    }

    lobbyId = p->m_ulSteamIDLobby;
    CSteamID steamLobbyId(p->m_ulSteamIDLobby);

    const uint64_t hostId = SteamUser()->GetSteamID().ConvertToUint64();

    SteamMatchmaking()->SetLobbyData(steamLobbyId, "host", std::to_string(hostId).c_str());
    SteamMatchmaking()->SetLobbyJoinable(steamLobbyId, true);

    std::string rp = std::format("+toggleconsole +connect steam-conn|{}", hostId);
    SteamFriends()->SetRichPresence("connect", rp.c_str());
    SteamFriends()->SetRichPresence("status", "In match");
    SteamFriends()->SetRichPresence("steam_display", "#Status_InMatch");

    hostingLobby = true;
}

void SteamCallbacks::OnLobbyEnter(LobbyEnter_t* p)
{
    if (!p)
        return;

    CSteamID lid(p->m_ulSteamIDLobby);

    if (hostingLobby && lobbyId != 0 && lid.ConvertToUint64() == lobbyId)
    {
        hostingLobby = false;
        pendingJoinLobbyId = CSteamID();
        return;
    }

    const CSteamID self = SteamUser()->GetSteamID();
    const uint64_t selfId = self.ConvertToUint64();

    const char* host = SteamMatchmaking()->GetLobbyData(lid, "host");
    if (!host || !*host)
        return;

    const uint64_t hostId = std::strtoull(host, nullptr, 10);
    if (hostId == 0 || hostId == selfId)
        return;

    commandLine = std::format("+toggleconsole +connect steam-conn|{}", hostId);
    pendingJoinLobbyId = CSteamID();
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

    if (activeServerListRequestId != serverListRequestId)
        return;

    CSteamID lid(p->m_ulSteamIDLobby);
    const uint64_t key = lid.ConvertToUint64();

    auto it = serverMap.find(key);
    if (it == serverMap.end())
        return;

    ServerEntry& entry = it->second;

    if (!entry.gotData)
    {
        entry.gotData = true;
        pendingLobbies--;
        if (pendingLobbies < 0)
            pendingLobbies = 0;
    }

    const char* host = SteamMatchmaking()->GetLobbyData(lid, "host");
    const char* map = SteamMatchmaking()->GetLobbyData(lid, "map");
    const char* cli = SteamMatchmaking()->GetLobbyData(lid, "clients");
    const char* maxc = SteamMatchmaking()->GetLobbyData(lid, "maxc");
    const char* name = SteamMatchmaking()->GetLobbyData(lid, "name");

    if (host && *host)
        entry.address = std::string("steam-conn|") + host;
    else
        entry.address.clear();

    entry.hostname = (name && *name) ? name : "UNNAMED";
    entry.map = map ? map : "";
    entry.clients = (cli && *cli) ? std::atoi(cli) : 0;
    entry.maxClients = (maxc && *maxc) ? std::atoi(maxc) : 0;
    entry.lobbyId = key;

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