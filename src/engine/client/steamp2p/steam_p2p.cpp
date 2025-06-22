#include "steam_p2p.h"
#include <algorithm>
#include <base/system.h>
#include <engine/shared/config.h>
#include <unordered_set>

static constexpr int kInputChannel = 0;
static constexpr int kReliableChannel = 1;
static constexpr int kInputSendFlags = k_nSteamNetworkingSend_UnreliableNoDelay;
static constexpr int kReliableSendFlags = k_nSteamNetworkingSend_Reliable;

CSteamP2PManager &CSteamP2PManager::Instance()
{
	static CSteamP2PManager Instance;
	return Instance;
}

bool CSteamP2PManager::Init()
{
	m_pSteamMatchmaking = SteamAPI_SteamMatchmaking_v009();
	m_pSteamMessages = SteamAPI_SteamNetworkingMessages_SteamAPI_v002();
	m_pSteamUser = SteamAPI_SteamUser_v021();
	m_Initalized = m_pSteamMatchmaking && m_pSteamMessages && m_pSteamUser;
	return m_Initalized;
}

void CSteamP2PManager::Shutdown()
{
	LeaveLobby();
}

bool CSteamP2PManager::HostLobbyPrivate(int MaxMembers)
{
	if(!m_Initalized)
		return false;

	if(m_LobbyID.IsValid())
		LeaveLobby();

	m_hPendingCreate = SteamAPI_ISteamMatchmaking_CreateLobby(m_pSteamMatchmaking, k_ELobbyTypePrivate, MaxMembers);
	return true;
}

bool CSteamP2PManager::JoinLobbyByID(uint64_t LobbyId64)
{
	if(!m_Initalized)
		return false;

	if(m_LobbyID.IsValid())
		LeaveLobby();

	CSteamID Indentity{LobbyId64};
	m_hPendingJoin = SteamAPI_ISteamMatchmaking_JoinLobby(m_pSteamMatchmaking, Indentity);

	return true;
}

void CSteamP2PManager::LeaveLobby()
{
	if(m_LobbyID.IsValid() && m_pSteamMatchmaking)
	{
		SteamAPI_ISteamMatchmaking_LeaveLobby(m_pSteamMatchmaking, m_LobbyID);
		m_LobbyID.Clear();
	}
	for(const auto &[SteamID64, Peer] : m_Peers)
	{
		SteamNetworkingIdentity Indentity{};
		Indentity.SetSteamID64(SteamID64);
		SteamAPI_ISteamNetworkingMessages_CloseSessionWithUser(m_pSteamMessages, Indentity);
	}
	m_Peers.clear();
}

void CSteamP2PManager::SendInputs(const CNetObj_PlayerInput &PlayerInput, uint32_t GameTick, int ClientId)
{
	if(!m_LobbyID.IsValid() || !m_Initalized)
		return;

	CP2PPlayerInput P2PInput = ConvertToP2PInput(PlayerInput);

	CP2PInputMsg Msg;
	Msg.m_ServerAddr = m_LocalSrvAddr;
	Msg.m_GameTick = GameTick;
	Msg.m_Input = P2PInput;
	Msg.m_ClientId = ClientId;

	uint32_t ServerCRC = NetAddrCRC32(m_LocalSrvAddr);

	// only send to peers on the same server
	for(const auto &[SteamID64, Peer] : m_Peers)
	{
		if(Peer.m_ServerCRC == 0 || Peer.m_ServerCRC == ServerCRC)
		{
			SteamNetworkingIdentity Indentity{};
			Indentity.SetSteamID64(SteamID64);
			SteamAPI_ISteamNetworkingMessages_SendMessageToUser(m_pSteamMessages, Indentity, &Msg, sizeof(Msg), kInputSendFlags, kInputChannel);
		}
	}
}

void CSteamP2PManager::UpdateLobbyOwner()
{
	if(!m_Initalized || !m_LobbyID.IsValid())
		return;
	CSteamID NewOwnerID = SteamAPI_ISteamMatchmaking_GetLobbyOwner(m_pSteamMatchmaking, m_LobbyID.ConvertToUint64());
	if(NewOwnerID != m_LobbyOwnerID)
	{
		CSteamID OldOwnerID = m_LobbyOwnerID;
		m_LobbyOwnerID = OldOwnerID;
		dbg_msg("p2p", "lobby owner changed from (%llu) to (%llu)", OldOwnerID.ConvertToUint64(), NewOwnerID.ConvertToUint64());
	}
}

bool CSteamP2PManager::IsLobbyOwner() const
{
	if(!m_Initalized || !m_LobbyID.IsValid())
		return false;
	return m_LobbyOwnerID == SteamAPI_ISteamUser_GetSteamID(m_pSteamUser);
}

void CSteamP2PManager::Update()
{
	if(!m_LobbyID.IsValid() || !m_Initalized)
		return;

	auto Now = std::chrono::steady_clock::now();
	auto ElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(Now - m_LastMemberUpdate).count();

	if(ElapsedMs > 3000)
	{
		UpdateLobbyMembers();
		UpdateLobbyOwner();
		m_LastMemberUpdate = Now;
	}
	PollIngress();
	SendKeepAlives();
	CheckTimeouts();
}

void CSteamP2PManager::PollIngress()
{
	if(!m_Initalized)
		return;

	SteamNetworkingMessage_t *Messages[32];

	for(int Channel = 0; Channel < 2; Channel++)
	{
		int NumMessages = SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel(m_pSteamMessages, Channel, Messages, 32);
		for(int i = 0; i < NumMessages; ++i)
		{
			auto *pMsg = Messages[i];

			if(pMsg->m_cbSize >= sizeof(CP2PPacketHeader))
			{
				uint64_t SenderID = pMsg->m_identityPeer.GetSteamID64();
				ProcessPacket(pMsg->m_pData, pMsg->m_cbSize, SenderID);
			}

			pMsg->Release();
		}
	}
}

void CSteamP2PManager::CheckTimeouts()
{
	std::vector<uint64_t> TimedOutPeers;

	for(const auto &[SteamID64, Peer] : m_Peers)
	{
		if(Peer.IsTimedOut())
		{
			TimedOutPeers.push_back(SteamID64);
		}
	}

	for(uint64_t SteamID64 : TimedOutPeers)
	{
		dbg_msg("p2p", "peer %llu timed out", SteamID64);
		RemovePeer(SteamID64);
	}
}

// If we haven't sent any inputs to the other players in a while send them something to let them know we are alive
// We don't just send another input because we might not be in a server, or not in the same server
void CSteamP2PManager::SendKeepAlives()
{
	if(!m_Initalized)
		return;

	for(auto &[SteamID64, Peer] : m_Peers)
	{
		if(Peer.NeedsKeepAlive())
		{
			CP2PKeepAliveMsg KeepAlive;
			KeepAlive.m_ServerAddr = m_LocalSrvAddr;
			SendMessageToPeer(SteamID64, kInputSendFlags, &KeepAlive, sizeof(KeepAlive));
			Peer.m_LastSent = std::chrono::steady_clock::now();
		}
	}
}

void CSteamP2PManager::SendMessageToPeer(uint64_t SteamID64, int SendFlags, const void *pData, size_t Size)
{
	if(!m_Initalized)
		return;

	SteamNetworkingIdentity Identity{};
	Identity.SetSteamID64(SteamID64);

	int Channel = kInputChannel;
	if(SendFlags == kReliableSendFlags)
		Channel = kReliableChannel;

	SteamAPI_ISteamNetworkingMessages_SendMessageToUser(m_pSteamMessages, Identity, pData, Size, SendFlags, Channel);
}

void CSteamP2PManager::SendMessageToAll(int SendFlags, const void *pData, size_t Size)
{
	for(const auto &[SteamID64, Peer] : m_Peers)
	{
		SendMessageToPeer(SteamID64, SendFlags, pData, Size);
	}
}

void CSteamP2PManager::ProcessPacket(const void *pData, size_t Size, uint64_t SenderID)
{
	if(Size < sizeof(CP2PPacketHeader))
		return;

	const CP2PPacketHeader *pHeader = static_cast<const CP2PPacketHeader *>(pData);

	if(pHeader->m_Version != P2P_PROTOCOL_VERSION)
		return;

	if(pHeader->m_Size != Size)
		return;

	CPeerConnection *pPeer = GetOrCreatePeer(SenderID);
	if(!pPeer)
		return;

	pPeer->m_LastReceived = std::chrono::steady_clock::now();

	switch(pHeader->m_Type)
	{
	case P2P_PACKET_INPUT:
	{
		if(Size != sizeof(CP2PInputMsg))
			return;

		const CP2PInputMsg *pInput = static_cast<const CP2PInputMsg *>(pData);
		uint32_t ServerCRC = NetAddrCRC32(pInput->m_ServerAddr);

		if(ServerCRC != m_LocalSrvCRC)
			return;

		// TODO: Scary, add helper or something for this
		int BufferIndex = std::clamp(pInput->m_GameTick % 200, 0u, 199u);
		int ClientId = std::clamp(static_cast<int>(pInput->m_ClientId), 0, static_cast<int>(MAX_CLIENTS - 1));

		m_InputBuffer[ClientId][BufferIndex].m_GameTick = pInput->m_GameTick;
		m_InputBuffer[ClientId][BufferIndex].m_Input = ConvertFromP2PInput(pInput->m_Input); 
		break;
	}
	case P2P_PACKET_KEEP_ALIVE:
	{
		if(Size != sizeof(CP2PKeepAliveMsg))
			return;

		const CP2PKeepAliveMsg *pKeepAlive = static_cast<const CP2PKeepAliveMsg *>(pData);

		uint32_t ServerCRC = NetAddrCRC32(pKeepAlive->m_ServerAddr);
		pPeer->m_ServerCRC = ServerCRC;
		break;
	}
	case P2P_PACKET_PEER_INFO:
	{
		if(Size != sizeof(CP2PPeerInfoMsg))
			return;

		const CP2PPeerInfoMsg *pInfo = static_cast<const CP2PPeerInfoMsg *>(pData);
		char aPlayerName[MAX_NAME_LENGTH];
		str_copy(aPlayerName, pInfo->m_aPlayerName, sizeof(aPlayerName));
		break;
	}
	default:
		break;
	}
}

void CSteamP2PManager::UpdateLobbyMembers()
{
	if(!m_Initalized || !m_LobbyID.IsValid())
		return;

	int NumMembers = SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(m_pSteamMatchmaking, m_LobbyID.ConvertToUint64());

	std::unordered_set<uint64_t> CurrentMembers;

	for(int i = 0; i < NumMembers; ++i)
	{
		CSteamID MemberID = SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(
			m_pSteamMatchmaking, m_LobbyID.ConvertToUint64(), i);

		if(MemberID.IsValid())
		{
			uint64_t SteamID64 = MemberID.ConvertToUint64();
			CurrentMembers.insert(SteamID64);

			if(SteamID64 != SteamAPI_ISteamUser_GetSteamID(m_pSteamUser))
			{
				GetOrCreatePeer(SteamID64);
			}
		}
	}
	std::vector<uint64_t> ToRemove;
	for(const auto &[SteamID64, Peer] : m_Peers)
	{
		if(CurrentMembers.find(SteamID64) == CurrentMembers.end())
		{
			ToRemove.push_back(SteamID64);
		}
	}

	for(uint64_t SteamID64 : ToRemove)
	{
		dbg_msg("p2p", "peer %llu left lobby", SteamID64);
		RemovePeer(SteamID64);
	}
}

CPeerConnection *CSteamP2PManager::GetOrCreatePeer(uint64_t SteamID64)
{
	auto It = m_Peers.find(SteamID64);
	if(It != m_Peers.end())
		return &It->second;

	auto [NewIt, Inserted] = m_Peers.emplace(SteamID64, CPeerConnection(SteamID64));

	// Send our info to the new peer
	// TODO: Fill out the InfoMsg data with our tee name
	CP2PPeerInfoMsg InfoMsg;

	str_copy(InfoMsg.m_aPlayerName, "Player", sizeof(InfoMsg.m_aPlayerName));

	SendMessageToPeer(SteamID64, kReliableSendFlags, &InfoMsg, sizeof(InfoMsg));

	return &NewIt->second;
}

void CSteamP2PManager::RemovePeer(uint64_t SteamID64)
{
	auto It = m_Peers.find(SteamID64);
	if(It == m_Peers.end())
		return;

	SteamNetworkingIdentity Identity{};
	Identity.SetSteamID64(SteamID64);
	SteamAPI_ISteamNetworkingMessages_CloseSessionWithUser(m_pSteamMessages, Identity);

	m_Peers.erase(It);
}

void CSteamP2PManager::OnLobbyCreated(const LobbyCreated_t &Callback)
{
	if(Callback.m_eResult == k_EResultOK)
	{
		m_LobbyID = CSteamID{Callback.m_ulSteamIDLobby};
		dbg_msg("p2p", "lobby created (%llu)", Callback.m_ulSteamIDLobby);
	}
	else
		dbg_msg("p2p", "lobby create failed (%d)", Callback.m_eResult);

	m_hPendingCreate = 0;
}

void CSteamP2PManager::OnLobbyKicked(const LobbyKicked_t &Callback)
{
	LeaveLobby();
}

void CSteamP2PManager::OnLobbyEnter(const LobbyEnter_t &Callback)
{
	if(Callback.m_EChatRoomEnterResponse == k_EResultOK)
	{
		m_LobbyID = CSteamID{Callback.m_ulSteamIDLobby};
		dbg_msg("p2p", "entered lobby (%llu)",
			(unsigned long long)Callback.m_ulSteamIDLobby);

		const int Count = SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(m_pSteamMatchmaking, m_LobbyID.ConvertToUint64());
		for(int i = 0; i < Count; ++i)
		{
			CSteamID Member = SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(m_pSteamMatchmaking, m_LobbyID.ConvertToUint64(), i);
			// skip ourself
			if(Member != SteamAPI_ISteamUser_GetSteamID(m_pSteamUser))
				GetOrCreatePeer(Member.ConvertToUint64());
		}
	}
	else
		dbg_msg("p2p", "join lobby failed (%d)", Callback.m_EChatRoomEnterResponse);
	m_hPendingJoin = 0;
}

void CSteamP2PManager::OnLobbyChatUpdate(const LobbyChatUpdate_t &Callback)
{
	const uint64_t SteamID64 = Callback.m_ulSteamIDUserChanged;
	const uint32_t Mask = Callback.m_rgfChatMemberStateChange;

	enum
	{
		ENTERED = k_EChatMemberStateChangeEntered,
		LEFT = k_EChatMemberStateChangeLeft,
		DISCONNECTED = k_EChatMemberStateChangeDisconnected,
		KICKED = k_EChatMemberStateChangeKicked,
		BANNED = k_EChatMemberStateChangeBanned,
	};

	if(Mask & (LEFT | DISCONNECTED | KICKED | BANNED))
	{
		dbg_msg("p2p", "member left: (%llu)", SteamID64);
		RemovePeer(SteamID64);
	}
	else if(Mask & ENTERED)
	{
		dbg_msg("p2p", "member joined: (%llu)", SteamID64);
		GetOrCreatePeer(SteamID64);
	}
}

void CSteamP2PManager::UpdateLocalServer(const NETADDR &Addr)
{
	m_LocalSrvCRC = NetAddrCRC32(Addr);
	m_LocalSrvAddr = Addr;
}

int CSteamP2PManager::GetPeerRTT(uint64_t SteamID64) const
{
	if(!m_Initalized)
		return -1;

	SteamNetConnectionInfo_t Info{};
	SteamNetConnectionRealTimeStatus_t Status{};

	SteamNetworkingIdentity Identity{};
	Identity.SetSteamID64(SteamID64);

	if(SteamAPI_ISteamNetworkingMessages_GetSessionConnectionInfo(m_pSteamMessages, Identity, &Info, &Status) 
		== k_ESteamNetworkingConnectionState_Connected)
	{
		return Status.m_nPing; // RTT in milliseconds
	}
	return -1;
}

bool CSteamP2PManager::GetPeerInput(int ClientId, uint32_t GameTick, CNetObj_PlayerInput &OutInput) const
{
	if(!m_Initalized || !m_LobbyID.IsValid())
		return false;

	int Tick = m_InputBuffer[ClientId][GameTick % 200].m_GameTick;

	if(Tick != GameTick)
		return false;

	OutInput = m_InputBuffer[ClientId][GameTick % 200].m_Input;
	return true;
}