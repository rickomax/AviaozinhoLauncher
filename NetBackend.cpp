#include "NetBackend.h"
#include "SteamProtocol.h"
#include "steam/steam_api.h"
#include <iostream>

bool initialized = false;


void gnsMessagesSessionFailed(SteamNetworkingMessagesSessionFailed_t* pInfo) {
    const SteamNetConnectionInfo_t& info = pInfo->m_info;
    printf("Session failed with peer %llu: State=%d, EndReason=%d\n",
        info.m_identityRemote.GetSteamID64(), info.m_eState, info.m_eEndReason);
}

void gnsMessagesSessionRequest(SteamNetworkingMessagesSessionRequest_t* pInfo) {
    const SteamNetworkingIdentity& remote = pInfo->m_identityRemote;
    bool accepted = SteamNetworkingMessages()->AcceptSessionWithUser(remote);
    if (!accepted) {
        printf("Failed to accept session from %llu\n", remote.GetSteamID64());
    }
    else {
        printf("Accepted session from %llu\n", remote.GetSteamID64());
    }
}

void gns_init(const NetPipeHeader header) {
    if (initialized) {
        return;
    }
    SteamNetworkingUtils()->InitRelayNetworkAccess();
    SteamNetworkingUtils()->SetGlobalCallback_MessagesSessionFailed(gnsMessagesSessionFailed);
    SteamNetworkingUtils()->SetGlobalCallback_MessagesSessionRequest(gnsMessagesSessionRequest);
    initialized = true;
}

void gns_shutdown(const NetPipeHeader header) {
    if (initialized) {
        initialized = false;
    }
}

void gns_sendto(const NetPipeHeader header) {
    void* buf = netPayload;
    int len = header.length;
    int s = header.socket;
    uint64_t steamId = header.steamId;
    SteamNetworkingIdentity idRemote{};
    idRemote.SetSteamID64(steamId);
    EResult result = SteamNetworkingMessages()->SendMessageToUser(
        idRemote,
        buf,
        (uint32)len,
        k_nSteamNetworkingSend_Unreliable,
        s
    );
    if (result != k_EResultOK) {
        printf("SendMessageToUser failed: EResult=%d\n", result);
        WriteHeader(GNS_FAIL, s, steamId, nullptr, 0);
        return;
    }
    WriteHeader(GNS_SUCCESS, s, steamId, nullptr, 0);
}

void gns_recvfrom(const NetPipeHeader header) {
    // int len = header.length;
    int s = header.socket;
    ISteamNetworkingMessage* pMsg = nullptr;
    int count = SteamNetworkingMessages()->ReceiveMessagesOnChannel(s, &pMsg, 1);
    if (count <= 0) {
        WriteHeader(GNS_SUCCESS, s, 0, nullptr, 0);
        return;
    }
    int copyLen = (int)pMsg->m_cbSize;
    //if (copyLen > len) {
    //    copyLen = len;
    //}
    uint64_t steamId = pMsg->m_identityPeer.GetSteamID64();
    WriteHeader(GNS_SUCCESS, s, steamId, pMsg->m_pData, copyLen);
    pMsg->Release();
}

void gns_pumppipe(void) {
    SteamNetworkingSockets()->RunCallbacks();
    for (;;)
    {
        if (NetPipe_AvailableBytes() < sizeof(NetPipeHeader)) {
            break;
        }
        NetPipeHeader header = ReadHeader(netPayload);
        switch (header.op)
        {
        case GNS_INIT:
            gns_init(header);
            break;
        case GNS_SENDTO:
            gns_sendto(header);
            break;
        case GNS_RECVFROM:
            gns_recvfrom(header);
            break;
        case GNS_SHUTDOWN:
            gns_shutdown(header);
            break;
        }
    }
}