#include "NetBackend.h"
#include "SteamProtocol.h"
#include "steam/steam_api.h"
#include <iostream>

bool NetBackend_Setup() {
	SteamNetworkingUtils()->InitRelayNetworkAccess();
	//SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(
	//	NetBackend_OnConnectionStatusChanged
	//);
	config.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)NetBackend_OnConnectionStatusChanged);
	sockets = SteamNetworkingSockets();
	if (!sockets) {
		SteamAPI_Shutdown();
		return false;
	}
	listenSocket = k_HSteamListenSocket_Invalid;
	for (int connId = 0; connId < MAX_CONN; connId++)
	{
		connections[connId] = k_HSteamNetConnection_Invalid;
	}
	return true;
}

// Small helper to find connId by connection handle
uint8_t NetBackend_FindConnIdByHandle(HSteamNetConnection hConn)
{
	if (hConn == k_HSteamNetConnection_Invalid) {
		return 0;
	}
	for (int i = 1; i < MAX_CONN; ++i)
	{
		if (connections[i] == hConn) {
			return (uint8_t)i;
		}
	}
	return 0;
}

// -----------------------------------------------------------------------------
// Connection table helpers (client + server shared)
// -----------------------------------------------------------------------------

HSteamNetConnection GetConn(uint8_t connId)
{
	if (connId == 0) {
		return k_HSteamNetConnection_Invalid;
	}
	return connections[connId];
}

void SetConn(uint8_t connId, HSteamNetConnection hConn)
{
	if (connId == 0) {
		return;
	}
	connections[connId] = hConn;
}

void ClearConn(uint8_t connId)
{
	if (connId == 0) {
		return;
	}
	connections[connId] = k_HSteamNetConnection_Invalid;
}

// Allocate a free connId slot for a new connection (server-side)
uint8_t NetBackend_AllocConnId(HSteamNetConnection hConn)
{
	for (int i = 1; i < MAX_CONN; ++i)
	{
		if (connections[i] == k_HSteamNetConnection_Invalid)
		{
			connections[i] = hConn;
			return (uint8_t)i;
		}
	}
	return 0;
}

// -----------------------------------------------------------------------------
// SERVER-SIDE: host via Steam (listen socket, callback, etc.)
// -----------------------------------------------------------------------------

bool NetBackend_StartHost()
{
	if (!sockets) {
		return false;
	}
	if (listenSocket != k_HSteamListenSocket_Invalid) {
		return true;
	}
	//std::string connectStr = std::format("steam-conn|{}", static_cast<unsigned long long>(SteamUser()->GetSteamID().ConvertToUint64()));
	//SteamFriends()->SetRichPresence("connect", connectStr.c_str());
	//listenSocket = sockets->CreateHostedDedicatedServerListenSocket(
	//	0,
	//	0,
	//	&config
	//);
	//listenSocket = sockets->CreateListenSocketP2P(
	//	27020,
	//	1,
	//	&config
	//);
	SteamNetworkingIPAddr serverLocalAddr;
	serverLocalAddr.Clear();
	serverLocalAddr.m_port = 27020;
	listenSocket = sockets->CreateListenSocketIP(serverLocalAddr, 1, &config);
	if (listenSocket == k_HSteamListenSocket_Invalid)
	{
		WriteHeader(NETPIPE_OP_HOST_ERR, 0, 0, NULL, TRUE);
		return false;
	}
	WriteHeader(NETPIPE_OP_HOST_OK, 0, 0, NULL, TRUE);
	std::cout << "Hosting\n";
	return true;
}

void NetBackend_StopHost()
{
	if (!sockets) {
		return;
	}
	if (listenSocket != k_HSteamListenSocket_Invalid)
	{
		sockets->CloseListenSocket(listenSocket);
	}
	listenSocket = k_HSteamListenSocket_Invalid;
	SteamFriends()->ClearRichPresence();
	// NOTE: we do NOT close all connections here automatically;
	// game / higher level can decide when to call NETPIPE_OP_CLOSE, etc.
}

// This should be registered with SteamNetworkingSockets as the
// connection status changed callback.
static void NetBackend_OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
{
	if (!sockets)
		return;

	const SteamNetConnectionInfo_t& ci = info->m_info;
	HSteamNetConnection hConn = info->m_hConn;

	printf(
		"[Steam] Conn %u state=%d (old=%d) reason=%d '%s' listenSock=%u ours=%u\n",
		hConn,
		ci.m_eState,
		0,
		ci.m_eEndReason,
		ci.m_szEndDebug,
		ci.m_hListenSocket,
		listenSocket
	);

	switch (ci.m_eState)
	{
	case k_ESteamNetworkingConnectionState_Connecting:
	{
		// *** SOMENTE conexões vindas do nosso listen socket são aceitas ***
		if (ci.m_hListenSocket == listenSocket && listenSocket != k_HSteamListenSocket_Invalid)
		{
			uint8_t connId = NetBackend_AllocConnId(hConn);
			if (connId == 0)
			{
				sockets->CloseConnection(hConn, 0, "No conn slots", false);
				printf("[Steam] No conn slots\n");
				break;
			}

			EResult e = sockets->AcceptConnection(hConn);
			if (e != k_EResultOK)
			{
				sockets->CloseConnection(hConn, 0, "Connection error", false);
				printf("[Steam] AcceptConnection failed: %d\n", e);
				ClearConn(connId);
				break;
			}

			printf("[Steam] Accepted incoming connId=%u (hConn=%u)\n", connId, hConn);
		}
		else
		{
			// Conexão de saída (ConnectP2P / ConnectByIPAddress).
			// NÃO chamar AcceptConnection aqui.
			printf("[Steam] Outgoing connection is Connecting (hConn=%u)\n", hConn);
		}
		break;
	}

	case k_ESteamNetworkingConnectionState_Connected:
	{
		// Se for uma conexão de entrada (veio do nosso listen socket),
		// agora é a hora de avisar o jogo servidor com NETPIPE_OP_NEWCONN.
		if (ci.m_hListenSocket == listenSocket && listenSocket != k_HSteamListenSocket_Invalid)
		{
			uint8_t connId = NetBackend_FindConnIdByHandle(hConn);
			if (connId == 0)
			{
				// Só por segurança: se por algum motivo não achar, aloca aqui.
				connId = NetBackend_AllocConnId(hConn);
			}

			printf("[Steam] Incoming connection CONNECTED, connId=%u (hConn=%u)\n", connId, hConn);

			// Backend -> jogo servidor: "tem conexão nova com esse connId"
			WriteHeader(NETPIPE_OP_NEWCONN, connId, 0, NULL, TRUE);
		}
		else
		{
			// Conexão de saída no cliente – aqui você pode mandar um OPEN_OK pro jogo cliente se quiser.
			printf("[Steam] Outgoing connection %u is now CONNECTED\n", hConn);
			// Ex: se você quiser só avisar o cliente que o connect terminou:
			// uint8_t connId = NetBackend_FindConnIdByHandle(hConn);
			// if (connId) WriteHeader(NETPIPE_OP_OPEN_OK, connId, 0, NULL, TRUE);
		}
		break;
	}

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		uint8_t connId = NetBackend_FindConnIdByHandle(hConn);
		printf("[Steam] Closing connId=%u (hConn=%u)\n", connId, hConn);
		sockets->CloseConnection(hConn, 0, nullptr, false);
		if (connId != 0)
			ClearConn(connId);

		// Aqui provavelmente você já tem algum NETPIPE_OP_CLOSE_FROM_BACKEND
		// para avisar o jogo que esse connId morreu.
		// Ex: WriteHeader(NETPIPE_OP_CLOSE, connId, 0, NULL, TRUE);
		break;
	}

	default:
		break;
	}
}


// -----------------------------------------------------------------------------
// CLIENT-SIDE: game _ backend (OPEN / DATA / CLOSE)
// -----------------------------------------------------------------------------
void NetBackend_HandleCloseFromGame(const NetPipeHeader header)
{
	if (!sockets) {
		return;
	}
	HSteamNetConnection hConn = GetConn(header.connId);
	if (hConn == k_HSteamNetConnection_Invalid) {
		return;
	}
	sockets->CloseConnection(
		hConn,
		0,                 // reason code
		"closed by game",  // debug message to peer
		false              // do not linger; close immediately
	);
	ClearConn(header.connId);
}

void NetBackend_HandleOpen(const NetPipeHeader header)
{
	if (!sockets)
	{
		WriteHeader(NETPIPE_OP_OPEN_ERR, header.connId, 0, NULL, TRUE);
		return;
	}
	//const char* netPayloadStr = reinterpret_cast<const char*>(netPayload);
	//std::string host(netPayloadStr, netPayloadStr + header.length);
	//unsigned long long steamid64 = std::stoull(host);
	//if (steamid64 == 0)
	//{
	//	WriteHeader(NETPIPE_OP_OPEN_ERR, header.connId, 0, NULL, TRUE);
	//	return;
	//}
	//SteamNetworkingIdentity identity{};
	//identity.Clear();
	//identity.SetSteamID64((uint64)steamid64);
	//HSteamNetConnection hConn = sockets->ConnectP2P(
	//	identity,
	//	27020,
	//	1,
	//	&config
	//);
	const char address[] = "127.0.0.1:27020";
	SteamNetworkingIPAddr ipAddr;
	ipAddr.Clear();
	ipAddr.ParseString(address);
	HSteamNetConnection hConn = sockets->ConnectByIPAddress(ipAddr, 1, &config);
	if (hConn == k_HSteamNetConnection_Invalid)
	{
		WriteHeader(NETPIPE_OP_OPEN_ERR, header.connId, 0, NULL, TRUE);
		return;
	}
	SetConn(header.connId, hConn);
	WriteHeader(NETPIPE_OP_OPEN_OK, header.connId, 0, NULL, TRUE);
}

void NetBackend_HandleDataFromGame(const NetPipeHeader header)
{
	if (!sockets) {
		return;
	}
	HSteamNetConnection hConn = GetConn(header.connId);
	if (hConn == k_HSteamNetConnection_Invalid) {
		return;
	}
	EResult result = sockets->SendMessageToConnection(
		hConn,
		netPayload,
		header.length,
		header.reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable,
		nullptr
	);
	if (result != k_EResultOK) {
		printf("Error sending peer message to (%i)\n", header.connId);
	}
}

// -----------------------------------------------------------------------------
// PIPE PUMP: game _ backend commands
// -----------------------------------------------------------------------------

void NetBackend_PumpPipe(void)
{
	sockets->RunCallbacks();
	for (;;)
	{
		if (NetPipe_AvailableBytes() < sizeof(NetPipeHeader)) {
			break;
		}
		NetPipeHeader header = ReadHeader(netPayload);
		switch (header.op)
		{
		case NETPIPE_OP_OPEN:
			NetBackend_HandleOpen(header);
			break;
		case NETPIPE_OP_DATA:
			NetBackend_HandleDataFromGame(header);
			break;
		case NETPIPE_OP_CLOSE:
			NetBackend_HandleCloseFromGame(header);
			break;
		case NETPIPE_OP_HOST_ON:
			NetBackend_StartHost();
			break;
		}
	}
}

// -----------------------------------------------------------------------------
// STEAM POLLING: backend _ game (DATA / CLOSE)
// -----------------------------------------------------------------------------
void NetBackend_PollSteamMessages()
{
	if (!sockets) {
		return;
	}
	for (int connId = 0; connId < MAX_CONN; connId++)
	{
		HSteamNetConnection hConn = GetConn((uint8_t)connId);
		if (hConn == k_HSteamNetConnection_Invalid) {
			continue;
		}
		for (;;)
		{
			ISteamNetworkingMessage* msg;
			int count = sockets->ReceiveMessagesOnConnection(hConn, &msg, 1);
			if (count <= 0) {
				break;
			}
			WriteHeader(NETPIPE_OP_DATA, (uint8_t)connId, msg->GetSize(), msg->GetData(), msg->m_nFlags & k_nSteamNetworkingSend_Reliable);
			msg->Release();
		}
	}
}
