#ifndef NETWORK_P2P_H
#define NETWORK_P2P_H

#include <base/types.h>
#include <engine/shared/network.h>
#include <game/generated/protocol.h>
#include <engine/shared/protocol.h>

static constexpr uint8_t P2P_PROTOCOL_VERSION = 1;

enum EP2PPacketType : uint8_t
{
	P2P_PACKET_PEER_INFO,
	P2P_PACKET_KEEP_ALIVE,
	P2P_PACKET_INPUT,
	P2P_PACKET_MAX
};

#pragma pack(push, 1)
struct CP2PPacketHeader
{
	uint8_t m_Version;
	uint8_t m_Type; // EP2PPacketType
	uint16_t m_Size;
};
#pragma pack(pop)


struct CP2PPlayerInput
{
	int m_TargetX;
	int m_TargetY;
	uint8_t m_Jump;
	uint8_t m_Fire;
	uint8_t m_Hook;
	uint8_t m_WantedWeapon;
	uint8_t m_PrevWeapon;
	uint8_t m_NextWeapon;
	int8_t m_Direction;
};

struct CP2PInputMsg
{
	CP2PPacketHeader m_Header;
	NETADDR m_ServerAddr = {}; 
	uint32_t m_GameTick = 0;
	CP2PPlayerInput m_Input = {};
	int8_t m_ClientId = -1;

	CP2PInputMsg()
	{
		m_Header.m_Type = P2P_PACKET_INPUT;
		m_Header.m_Version = P2P_PROTOCOL_VERSION;
		m_Header.m_Size = sizeof(CP2PInputMsg);
	}
};

struct CP2PPeerInfoMsg
{
	CP2PPacketHeader m_Header;

	char m_aPlayerName[MAX_NAME_LENGTH] = {};

	CP2PPeerInfoMsg()
	{
		m_Header.m_Type = P2P_PACKET_PEER_INFO;
		m_Header.m_Version = P2P_PROTOCOL_VERSION;
		m_Header.m_Size = sizeof(CP2PPeerInfoMsg);
	}
};

struct CP2PKeepAliveMsg
{
	CP2PPacketHeader m_Header;
	NETADDR m_ServerAddr = {};

	CP2PKeepAliveMsg()
	{
		m_Header.m_Type = P2P_PACKET_KEEP_ALIVE;
		m_Header.m_Version = P2P_PROTOCOL_VERSION;
		m_Header.m_Size = sizeof(CP2PKeepAliveMsg);
	}
};


inline EP2PPacketType GetPacketType(const void *pData, size_t Size)
{
	if(Size < sizeof(CP2PPacketHeader))
		return P2P_PACKET_MAX;

	const CP2PPacketHeader *pHeader = static_cast<const CP2PPacketHeader *>(pData);
	if(pHeader->m_Type >= P2P_PACKET_MAX)
		return P2P_PACKET_MAX;

	return static_cast<EP2PPacketType>(pHeader->m_Type);
}


inline uint32_t NetAddrCRC32(const NETADDR &Addr)
{
	uint32_t crc = 0xFFFFFFFF;
	const uint8_t *data = reinterpret_cast<const uint8_t *>(&Addr);
	for(size_t i = 0; i < sizeof(NETADDR); ++i)
	{
		crc ^= data[i];
		for(int j = 0; j < 8; ++j)
		{
			crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
		}
	}
	return ~crc;
}

inline CP2PPlayerInput ConvertToP2PInput(const CNetObj_PlayerInput &NetInput)
{
	CP2PPlayerInput P2PInput;
	P2PInput.m_TargetX = NetInput.m_TargetX;
	P2PInput.m_TargetY = NetInput.m_TargetY;
	P2PInput.m_Jump = static_cast<uint8_t>(NetInput.m_Jump);
	P2PInput.m_Fire = static_cast<uint8_t>(NetInput.m_Fire);
	P2PInput.m_Hook = static_cast<uint8_t>(NetInput.m_Hook);
	P2PInput.m_WantedWeapon = static_cast<uint8_t>(NetInput.m_WantedWeapon);
	P2PInput.m_PrevWeapon = static_cast<uint8_t>(NetInput.m_PrevWeapon);
	P2PInput.m_NextWeapon = static_cast<uint8_t>(NetInput.m_NextWeapon);
	P2PInput.m_Direction = static_cast<int8_t>(NetInput.m_Direction);
	return P2PInput;
}

inline CNetObj_PlayerInput ConvertFromP2PInput(const CP2PPlayerInput &P2PInput)
{
	CNetObj_PlayerInput NetInput;
	NetInput.m_TargetX = P2PInput.m_TargetX;
	NetInput.m_TargetY = P2PInput.m_TargetY;
	NetInput.m_Jump = static_cast<int>(P2PInput.m_Jump);
	NetInput.m_Fire = static_cast<int>(P2PInput.m_Fire);
	NetInput.m_Hook = static_cast<int>(P2PInput.m_Hook);
	NetInput.m_WantedWeapon = static_cast<int>(P2PInput.m_WantedWeapon);
	NetInput.m_PrevWeapon = static_cast<int>(P2PInput.m_PrevWeapon);
	NetInput.m_NextWeapon = static_cast<int>(P2PInput.m_NextWeapon);
	NetInput.m_Direction = static_cast<int>(P2PInput.m_Direction);
	return NetInput;
}



#endif // NETWORK_P2P_H
