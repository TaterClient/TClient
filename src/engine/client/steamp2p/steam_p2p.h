#ifndef STEAM_P2P_H
#define STEAM_P2P_H

#include "network_p2p.h"
#include <base/types.h>
#include <steam/steam_api_flat.h>

#include <chrono>
#include <deque>
#include <unordered_map>

constexpr int KEEP_ALIVE_INTERVAL_MS = 3000; // 3 Seconds
constexpr int PEER_TIMEOUT_MS = 15000; // 15 Seconds

struct CP2PPreInput
{
	int m_GameTick = 0;
	CNetObj_PlayerInput m_Input = {};
};

struct CPeerConnection
{
	uint64_t m_SteamID64;

	std::chrono::steady_clock::time_point m_LastReceived;
	std::chrono::steady_clock::time_point m_LastSent;

	int m_RTT; // ms
	int m_ClientId;
	uint32_t m_ServerCRC; // If we are not in the same server we don't need to send inputs

	CPeerConnection(uint64_t SteamID64) :
		m_SteamID64(SteamID64),
		m_LastReceived(std::chrono::steady_clock::now()),
		m_LastSent(std::chrono::steady_clock::now()),
		m_RTT(-1),
		m_ServerCRC(0),
		m_ClientId(0)
	{
	}

	bool IsTimedOut() const
	{
		auto Now = std::chrono::steady_clock::now();
		auto ElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(Now - m_LastReceived).count();
		return ElapsedMs > PEER_TIMEOUT_MS;
	}
	bool NeedsKeepAlive() const
	{
		auto Now = std::chrono::steady_clock::now();
		auto ElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(Now - m_LastSent).count();
		return ElapsedMs > KEEP_ALIVE_INTERVAL_MS;
	}
	bool IsLobbyOwner() const
	{
		uint64_t OwnerId = CSteamP2PManager::Instance().GetLobbyOwnerID();
		CSteamID OwnerSteamId = CSteamID{OwnerId};
		return m_SteamID64 == CSteamP2PManager::Instance().GetLobbyOwnerID() && OwnerSteamId.IsValid();
	}
};

class CSteamP2PManager
{
public:
	static CSteamP2PManager &Instance();

	bool Init(); // after SteamAPI_Init
	void Shutdown();
	void UpdateLocalServer(const NETADDR &Addr);

	// Lobby
	bool HostLobbyPrivate(int MaxMembers = 8);
	bool JoinLobbyByID(uint64_t LobbyId64);
	void LeaveLobby();
	uint64_t GetLobbyID() const { return m_LobbyID.ConvertToUint64(); }
	bool IsLobbyValid() const { return m_LobbyID.IsLobby(); }
	uint64_t GetLobbyOwnerID() const { return m_LobbyOwnerID.ConvertToUint64(); }
	bool IsLobbyOwner() const;

	std::unordered_map<uint64_t, CPeerConnection> &GetPeers() { return m_Peers; };

	void Update();
	void PollIngress();
	void SendInputs(const CNetObj_PlayerInput &PlayerInput, uint32_t GameTick, int ClientId);
	int GetPeerRTT(uint64_t SteamID64) const; // ms, -1 = not connected
	bool GetPeerInput(int ClientId, uint32_t GameTick, CNetObj_PlayerInput &OutInput) const;
	void UpdateLobbyOwner();

	// Steam Callbacks
	void OnLobbyCreated(const LobbyCreated_t &Callback);
	void OnLobbyEnter(const LobbyEnter_t &Callback);
	void OnLobbyChatUpdate(const LobbyChatUpdate_t &Callback);
	void OnLobbyKicked(const LobbyKicked_t &Callback);

	uint32_t m_LocalSrvCRC = 0;
	NETADDR m_LocalSrvAddr = {};
	bool m_Initalized = false;

	CP2PPreInput m_InputBuffer[MAX_CLIENTS][200]; // used with [ClientId][m_Gametick % 200]

private:
	CSteamP2PManager() = default;
	~CSteamP2PManager() = default;
	CSteamP2PManager(const CSteamP2PManager &) = delete;
	CSteamP2PManager &operator=(const CSteamP2PManager &) = delete;

	void UpdateLobbyMembers();
	void SendKeepAlives();
	void CheckTimeouts();

	void RemovePeer(uint64_t SteamID64);
	CPeerConnection *GetOrCreatePeer(uint64_t SteamID64);
	void SendMessageToPeer(uint64_t SteamID64, int SendFlags, const void *pData, size_t Size);
	void SendMessageToAll(int SendFlags, const void *pData, size_t Size);

	void ProcessPacket(const void *pData, size_t Size, uint64_t SenderID);

	ISteamMatchmaking *m_pSteamMatchmaking = nullptr;
	ISteamNetworkingMessages *m_pSteamMessages = nullptr;
	ISteamUser *m_pSteamUser = nullptr;

	SteamAPICall_t m_hPendingCreate = 0;
	SteamAPICall_t m_hPendingJoin = 0;
	CSteamID m_LobbyID = 0ULL;
	CSteamID m_LobbyOwnerID = 0ULL;

	std::unordered_map<uint64_t, CPeerConnection> m_Peers;
	std::chrono::steady_clock::time_point m_LastMemberUpdate;
};

#endif
