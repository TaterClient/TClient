#ifndef STEAM_STEAM_API_FLAT_H
#define STEAM_STEAM_API_FLAT_H

#include <base/dynamic.h>

#include <cstdint>
#include <cstring>

#ifndef STEAMAPI
#define STEAMAPI DYNAMIC_IMPORT
#endif

extern "C" {

typedef uint64_t CSteamId;
typedef int32_t HSteamPipe;
typedef int32_t HSteamUser;

struct CallbackMsg_t
{
	HSteamUser m_hSteamUser;
	int m_iCallback;
	unsigned char *m_pubParam;
	int m_cubParam;
};

struct GameRichPresenceJoinRequested_t
{
	enum
	{
		k_iCallback = 337
	};
	CSteamId m_steamIdFriend;
	char m_aRGCHConnect[256];
};

struct NewUrlLaunchParameters_t
{
	enum
	{
		k_iCallback = 1014
	};
	unsigned char m_EmptyStructDontUse;
};

struct ISteamApps;
struct ISteamFriends;

STEAMAPI bool SteamAPI_Init(); // Returns true on success.
STEAMAPI HSteamPipe SteamAPI_GetHSteamPipe();
STEAMAPI void SteamAPI_Shutdown();

STEAMAPI void SteamAPI_ManualDispatch_Init();
STEAMAPI void SteamAPI_ManualDispatch_FreeLastCallback(HSteamPipe SteamPipe);
STEAMAPI bool SteamAPI_ManualDispatch_GetNextCallback(HSteamPipe SteamPipe, CallbackMsg_t *pCallbackMsg);
STEAMAPI void SteamAPI_ManualDispatch_RunFrame(HSteamPipe SteamPipe);

STEAMAPI ISteamApps *SteamAPI_SteamApps_v008();
STEAMAPI int SteamAPI_ISteamApps_GetLaunchCommandLine(ISteamApps *pSelf, char *pBuffer, int BufferSize);
STEAMAPI const char *SteamAPI_ISteamApps_GetLaunchQueryParam(ISteamApps *pSelf, const char *pKey);

STEAMAPI ISteamFriends *SteamAPI_SteamFriends_v017();
STEAMAPI void SteamAPI_ISteamFriends_ClearRichPresence(ISteamFriends *pSelf);
STEAMAPI const char *SteamAPI_ISteamFriends_GetPersonaName(ISteamFriends *pSelf);
STEAMAPI bool SteamAPI_ISteamFriends_SetRichPresence(ISteamFriends *pSelf, const char *pKey, const char *pValue);

// TClientha

// Copyright (c) 2016 Facepunch Studios LTD
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS",
// WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// ============ Copyright Valve Corporation, All rights reserved. ============
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

typedef uint32_t HSteamListenSocket;
typedef uint32_t SteamNetworkingPOPID;

typedef uint64_t SteamAPICall_t;

struct ISteamNetworkingMessages;
struct ISteamMatchmaking;
struct ISteamNetworkingUtils;
struct ISteamUser;

typedef int64_t SteamNetworkingMicroseconds;
typedef uint32_t HSteamNetConnection;

enum EChatMemberStateChange
{
	// Specific to joining / leaving the chatroom
	k_EChatMemberStateChangeEntered = 0x0001, // This user has joined or is joining the chat room
	k_EChatMemberStateChangeLeft = 0x0002, // This user has left or is leaving the chat room
	k_EChatMemberStateChangeDisconnected = 0x0004, // User disconnected without leaving the chat first
	k_EChatMemberStateChangeKicked = 0x0008, // User kicked
	k_EChatMemberStateChangeBanned = 0x0010, // User kicked and banned
};

enum ELobbyType
{
	k_ELobbyTypePrivate = 0, // only way to join the lobby is to invite to someone else
	k_ELobbyTypeFriendsOnly = 1, // shows for friends or invitees, but not in lobby list
	k_ELobbyTypePublic = 2, // visible for friends and in lobby list
	k_ELobbyTypeInvisible = 3, // returned by search, but not visible to other friends
	k_ELobbyTypePrivateUnique = 4, // private, unique and does not delete when empty - only one of these may exist per unique keypair set
};
enum EUniverse
{
	k_EUniverseInvalid = 0,
	k_EUniversePublic = 1,
	k_EUniverseBeta = 2,
	k_EUniverseInternal = 3,
	k_EUniverseDev = 4,
	k_EUniverseMax
};

enum EAccountType
{
	k_EAccountTypeInvalid = 0,
	k_EAccountTypeIndividual = 1,
	k_EAccountTypeMultiseat = 2,
	k_EAccountTypeGameServer = 3,
	k_EAccountTypeAnonGameServer = 4,
	k_EAccountTypePending = 5,
	k_EAccountTypeContentServer = 6,
	k_EAccountTypeClan = 7,
	k_EAccountTypeChat = 8,
	k_EAccountTypeConsoleUser = 9,
	k_EAccountTypeAnonUser = 10,
	k_EAccountTypeMax
};
enum ESteamNetworkingIdentityType
{
	k_ESteamNetworkingIdentityType_Invalid = 0,
	k_ESteamNetworkingIdentityType_SteamID = 16,
	k_ESteamNetworkingIdentityType_XboxPairwiseID = 17,
	k_ESteamNetworkingIdentityType_SonyPSN = 18,
	k_ESteamNetworkingIdentityType_IPAddress = 1,
	k_ESteamNetworkingIdentityType_GenericString = 2,
	k_ESteamNetworkingIdentityType_GenericBytes = 3,
	k_ESteamNetworkingIdentityType_UnknownType = 4,
	k_ESteamNetworkingIdentityType__Force32bit = 0x7fffffff,
};

enum ESteamNetworkingFakeIPType
{
	k_ESteamNetworkingFakeIPType_Invalid,
	k_ESteamNetworkingFakeIPType_NotFake,
	k_ESteamNetworkingFakeIPType_GlobalIPv4,
	k_ESteamNetworkingFakeIPType_LocalIPv4,
	k_ESteamNetworkingFakeIPType__Force32Bit = 0x7fffffff
};
enum EResult
{
	k_EResultOK = 1,
	k_EResultFail = 2,
	k_EResultNoConnection = 3
};
/// High level connection status
enum ESteamNetworkingConnectionState
{

	k_ESteamNetworkingConnectionState_None = 0,
	k_ESteamNetworkingConnectionState_Connecting = 1,
	k_ESteamNetworkingConnectionState_FindingRoute = 2,
	k_ESteamNetworkingConnectionState_Connected = 3,
	k_ESteamNetworkingConnectionState_ClosedByPeer = 4,
	k_ESteamNetworkingConnectionState_ProblemDetectedLocally = 5,
	k_ESteamNetworkingConnectionState_FinWait = -1,
	k_ESteamNetworkingConnectionState_Linger = -2,
	k_ESteamNetworkingConnectionState_Dead = -3,
	k_ESteamNetworkingConnectionState__Force32Bit = 0x7fffffff
};
const unsigned int k_unSteamUserDefaultInstance = 1;
const unsigned int k_unSteamAccountInstanceMask = 0x000FFFFF;
enum EChatSteamIDInstanceFlags
{
	k_EChatAccountInstanceMask = 0x00000FFF,
	k_EChatInstanceFlagClan = (k_unSteamAccountInstanceMask + 1) >> 1,
	k_EChatInstanceFlagLobby = (k_unSteamAccountInstanceMask + 1) >> 2,
	k_EChatInstanceFlagMMSLobby = (k_unSteamAccountInstanceMask + 1) >> 3,
};

struct LobbyChatUpdate_t
{
	enum
	{
		k_iCallback = 506
	};
	uint64_t m_ulSteamIDLobby; // Lobby ID
	uint64_t m_ulSteamIDUserChanged; // user who's status in the lobby just changed - can be recipient
	uint64_t m_ulSteamIDMakingChange; // Chat member who made the change (different from SteamIDUserChange if kicking, muting, etc.)
					  // for example, if one user kicks another from the lobby, this will be set to the id of the user who initiated the kick
	uint32_t m_rgfChatMemberStateChange; // bitfield of EChatMemberStateChange values
};

struct LobbyCreated_t
{
	enum
	{
		k_iCallback = 513
	};
	EResult m_eResult;
	uint64_t m_ulSteamIDLobby;
};

struct LobbyEnter_t
{
	enum
	{
		k_iCallback = 504
	};
	uint64_t m_ulSteamIDLobby;
	uint32_t m_rgfChatPermissions;
	bool m_bLocked;
	EResult m_EChatRoomEnterResponse; // 0 == OK
};

struct LobbyKicked_t
{
	enum
	{
		k_iCallback = 512
	};
	uint64_t m_ulSteamIDLobby;
	uint64_t m_ulSteamIDAdmin;
	uint8_t m_bKickedDueToDisconnect;
};

// INTENTIONALLY DIFFERENT CAPITLIZATION (sane ID capitlization) TO NOT CONFLICT WITH DDNET TYPE TO AVOID MERGE ISSUES HOPEFULLY (probably not) -Tater
class CSteamID
{
public:
	CSteamID()
	{
		m_steamid.m_comp.m_unAccountID = 0;
		m_steamid.m_comp.m_EAccountType = k_EAccountTypeInvalid;
		m_steamid.m_comp.m_EUniverse = k_EUniverseInvalid;
		m_steamid.m_comp.m_unAccountInstance = 0;
	}
	CSteamID(uint64_t ulSteamID)
	{
		SetFromUint64(ulSteamID);
	}
	void SetFromUint64(uint64_t ulSteamID)
	{
		m_steamid.m_unAll64Bits = ulSteamID;
	}
	void Clear()
	{
		m_steamid.m_comp.m_unAccountID = 0;
		m_steamid.m_comp.m_EAccountType = k_EAccountTypeInvalid;
		m_steamid.m_comp.m_EUniverse = k_EUniverseInvalid;
		m_steamid.m_comp.m_unAccountInstance = 0;
	}
	uint64_t ConvertToUint64() const
	{
		return m_steamid.m_unAll64Bits;
	}
	uint64_t GetStaticAccountKey() const
	{
		return (uint64_t)((((uint64_t)m_steamid.m_comp.m_EUniverse) << 56) + ((uint64_t)m_steamid.m_comp.m_EAccountType << 52) + m_steamid.m_comp.m_unAccountID);
	}
	bool IsLobby() const
	{
		return (m_steamid.m_comp.m_EAccountType == k_EAccountTypeChat) && (m_steamid.m_comp.m_unAccountInstance & k_EChatInstanceFlagLobby);
	}
	void SetAccountID(uint32_t unAccountID) { m_steamid.m_comp.m_unAccountID = unAccountID; }
	void SetAccountInstance(uint32_t unInstance) { m_steamid.m_comp.m_unAccountInstance = unInstance; }
	uint32_t GetUnAccountInstance() const { return m_steamid.m_comp.m_unAccountInstance; }
	EAccountType GetEAccountType() const { return (EAccountType)m_steamid.m_comp.m_EAccountType; }
	EUniverse GetEUniverse() const { return m_steamid.m_comp.m_EUniverse; }
	void SetEUniverse(EUniverse eUniverse) { m_steamid.m_comp.m_EUniverse = eUniverse; }
	bool IsValid() const;

	inline bool operator==(const CSteamID &val) const { return m_steamid.m_unAll64Bits == val.m_steamid.m_unAll64Bits; }
	inline bool operator!=(const CSteamID &val) const { return !operator==(val); }
	inline bool operator<(const CSteamID &val) const { return m_steamid.m_unAll64Bits < val.m_steamid.m_unAll64Bits; }
	inline bool operator>(const CSteamID &val) const { return m_steamid.m_unAll64Bits > val.m_steamid.m_unAll64Bits; }

private:
	CSteamID(uint32_t);
	CSteamID(int32_t);
	union SteamID_t
	{
		struct SteamIDComponent_t
		{
			uint32_t m_unAccountID : 32;
			unsigned int m_unAccountInstance : 20;
			unsigned int m_EAccountType : 4;
			EUniverse m_EUniverse : 8;

		} m_comp;
		uint64_t m_unAll64Bits;
	} m_steamid;
};

inline bool CSteamID::IsValid() const
{
	if(m_steamid.m_comp.m_EAccountType <= k_EAccountTypeInvalid || m_steamid.m_comp.m_EAccountType >= k_EAccountTypeMax)
		return false;
	if(m_steamid.m_comp.m_EUniverse <= k_EUniverseInvalid || m_steamid.m_comp.m_EUniverse >= k_EUniverseMax)
		return false;
	if(m_steamid.m_comp.m_EAccountType == k_EAccountTypeIndividual)
	{
		if(m_steamid.m_comp.m_unAccountID == 0 || m_steamid.m_comp.m_unAccountInstance != k_unSteamUserDefaultInstance)
			return false;
	}
	if(m_steamid.m_comp.m_EAccountType == k_EAccountTypeClan)
	{
		if(m_steamid.m_comp.m_unAccountID == 0 || m_steamid.m_comp.m_unAccountInstance != 0)
			return false;
	}
	if(m_steamid.m_comp.m_EAccountType == k_EAccountTypeGameServer)
	{
		if(m_steamid.m_comp.m_unAccountID == 0)
			return false;
	}
	return true;
}
const int k_cchSteamNetworkingMaxConnectionCloseReason = 128;
const int k_cchSteamNetworkingMaxConnectionDescription = 128;

struct SteamNetConnectionRealTimeStatus_t
{
	ESteamNetworkingConnectionState m_eState;
	int m_nPing;
	float m_flConnectionQualityLocal;
	float m_flConnectionQualityRemote;
	float m_flOutPacketsPerSec;
	float m_flOutBytesPerSec;
	float m_flInPacketsPerSec;
	float m_flInBytesPerSec;
	int m_nSendRateBytesPerSecond;
	int m_cbPendingUnreliable;
	int m_cbPendingReliable;
	int m_cbSentUnackedReliable;
	SteamNetworkingMicroseconds m_usecQueueTime;
	uint32_t reserved[16];
};

struct SteamNetworkingIPAddr
{
	void Clear(); // Set everything to zero.  E.g. [::]:0
	bool IsIPv6AllZeros() const; // Return true if the IP is ::0.  (Doesn't check port.)
	void SetIPv6(const uint8_t *ipv6, uint16_t nPort); // Set IPv6 address.  IP is interpreted as bytes, so there are no endian issues.  (Same as inaddr_in6.)  The IP can be a mapped IPv4 address
	void SetIPv4(uint32_t nIP, uint16_t nPort); // Sets to IPv4 mapped address.  IP and port are in host byte order.
	bool IsIPv4() const; // Return true if IP is mapped IPv4
	uint32_t GetIPv4() const; // Returns IP in host byte order (e.g. aa.bb.cc.dd as 0xaabbccdd).  Returns 0 if IP is not mapped IPv4.
	void SetIPv6LocalHost(uint16_t nPort = 0); // Set to the IPv6 localhost address ::1, and the specified port.
	bool IsLocalHost() const; // Return true if this identity is localhost.  (Either IPv6 ::1, or IPv4 127.0.0.1)
	enum
	{
		k_cchMaxString = 48
	};

	struct IPv4MappedAddress
	{
		uint64_t m_8zeros;
		uint16_t m_0000;
		uint16_t m_ffff;
		uint8_t m_ip[4]; // NOTE: As bytes, i.e. network byte order
	};

	union
	{
		uint8_t m_ipv6[16];
		IPv4MappedAddress m_ipv4;
	};
	uint16_t m_port; // Host byte order
	bool operator==(const SteamNetworkingIPAddr &x) const;
	ESteamNetworkingFakeIPType GetFakeIPType() const;
	bool IsFakeIP() const { return GetFakeIPType() > k_ESteamNetworkingFakeIPType_NotFake; }
};

struct SteamNetworkingIdentity
{
	ESteamNetworkingIdentityType m_eType;
	void Clear();
	bool IsInvalid() const; // Return true if we are the invalid type.  Does not make any other validity checks (e.g. is SteamID actually valid)
	void SetSteamID(CSteamID steamID);
	CSteamID GetSteamID() const; // Return black CSteamID (!IsValid()) if identity is not a SteamID
	void SetSteamID64(uint64_t steamID); // Takes SteamID as raw 64-bit number
	uint64_t GetSteamID64() const; // Returns 0 if identity is not SteamID
	void SetIPAddr(const SteamNetworkingIPAddr &addr); // Set to specified IP:port
	const SteamNetworkingIPAddr *GetIPAddr() const; // returns null if we are not an IP address.
	void SetIPv4Addr(uint32_t nIPv4, uint16_t nPort); // Set to specified IPv4:port
	uint32_t GetIPv4() const; // returns 0 if we are not an IPv4 address.
	ESteamNetworkingFakeIPType GetFakeIPType() const;
	bool IsFakeIP() const { return GetFakeIPType() > k_ESteamNetworkingFakeIPType_NotFake; }
	void SetLocalHost(); // Set to localhost.  (We always use IPv6 ::1 for this, not 127.0.0.1)
	bool IsLocalHost() const; // Return true if this identity is localhost.
	bool SetGenericString(const char *pszString); // Returns false if invalid length
	const char *GetGenericString() const; // Returns nullptr if not generic string type
	bool SetGenericBytes(const void *data, size_t cbLen); // Returns false if invalid size.
	const uint8_t *GetGenericBytes(int &cbLen) const; // Returns null if not generic bytes type
	bool operator==(const SteamNetworkingIdentity &x) const;
	void ToString(char *buf, size_t cbBuf) const;
	bool ParseString(const char *pszStr);

	enum
	{
		k_cchMaxString = 128, // Max length of the buffer needed to hold any identity, formatted in string format by ToString
		k_cchMaxGenericString = 32, // Max length of the string for generic string identities.  Including terminating '\0'
		k_cchMaxXboxPairwiseID = 33, // Including terminating '\0'
		k_cbMaxGenericBytes = 32,
	};
	int m_cbSize;
	union
	{
		uint64_t m_steamID64;
		uint64_t m_PSNID;
		char m_szGenericString[k_cchMaxGenericString];
		char m_szXboxPairwiseID[k_cchMaxXboxPairwiseID];
		uint8_t m_genericBytes[k_cbMaxGenericBytes];
		char m_szUnknownRawString[k_cchMaxString];
		SteamNetworkingIPAddr m_ip;
		uint32_t m_reserved[32];
	};
};

const int k_nSteamNetworkingSend_Unreliable = 0;
const int k_nSteamNetworkingSend_NoNagle = 1;
const int k_nSteamNetworkingSend_UnreliableNoNagle = k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoNagle;
const int k_nSteamNetworkingSend_NoDelay = 4;
const int k_nSteamNetworkingSend_UnreliableNoDelay = k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoDelay | k_nSteamNetworkingSend_NoNagle;
const int k_nSteamNetworkingSend_Reliable = 8;
const int k_nSteamNetworkingSend_ReliableNoNagle = k_nSteamNetworkingSend_Reliable | k_nSteamNetworkingSend_NoNagle;

inline void SteamNetworkingIdentity::Clear() { memset(this, 0, sizeof(*this)); }
inline bool SteamNetworkingIdentity::IsInvalid() const { return m_eType == k_ESteamNetworkingIdentityType_Invalid; }
inline void SteamNetworkingIdentity::SetSteamID(CSteamID steamID) { SetSteamID64(steamID.ConvertToUint64()); }
inline CSteamID SteamNetworkingIdentity::GetSteamID() const { return CSteamID(GetSteamID64()); }
inline void SteamNetworkingIdentity::SetSteamID64(uint64_t steamID)
{
	m_eType = k_ESteamNetworkingIdentityType_SteamID;
	m_cbSize = sizeof(m_steamID64);
	m_steamID64 = steamID;
}
inline uint64_t SteamNetworkingIdentity::GetSteamID64() const { return m_eType == k_ESteamNetworkingIdentityType_SteamID ? m_steamID64 : 0; }
inline const SteamNetworkingIPAddr *SteamNetworkingIdentity::GetIPAddr() const { return m_eType == k_ESteamNetworkingIdentityType_IPAddress ? &m_ip : NULL; }
inline void SteamNetworkingIdentity::SetIPv4Addr(uint32_t nIPv4, uint16_t nPort)
{
	m_eType = k_ESteamNetworkingIdentityType_IPAddress;
	m_cbSize = (int)sizeof(m_ip);
	m_ip.SetIPv4(nIPv4, nPort);
}
inline uint32_t SteamNetworkingIdentity::GetIPv4() const { return m_eType == k_ESteamNetworkingIdentityType_IPAddress ? m_ip.GetIPv4() : 0; }
inline ESteamNetworkingFakeIPType SteamNetworkingIdentity::GetFakeIPType() const { return m_eType == k_ESteamNetworkingIdentityType_IPAddress ? m_ip.GetFakeIPType() : k_ESteamNetworkingFakeIPType_Invalid; }
inline void SteamNetworkingIdentity::SetLocalHost()
{
	m_eType = k_ESteamNetworkingIdentityType_IPAddress;
	m_cbSize = (int)sizeof(m_ip);
	m_ip.SetIPv6LocalHost();
}
inline bool SteamNetworkingIdentity::IsLocalHost() const { return m_eType == k_ESteamNetworkingIdentityType_IPAddress && m_ip.IsLocalHost(); }
inline bool SteamNetworkingIdentity::SetGenericString(const char *pszString)
{
	size_t l = strlen(pszString);
	if(l >= sizeof(m_szGenericString))
		return false;
	m_eType = k_ESteamNetworkingIdentityType_GenericString;
	m_cbSize = int(l + 1);
	memcpy(m_szGenericString, pszString, m_cbSize);
	return true;
}
inline const char *SteamNetworkingIdentity::GetGenericString() const { return m_eType == k_ESteamNetworkingIdentityType_GenericString ? m_szGenericString : NULL; }
inline bool SteamNetworkingIdentity::SetGenericBytes(const void *data, size_t cbLen)
{
	if(cbLen > sizeof(m_genericBytes))
		return false;
	m_eType = k_ESteamNetworkingIdentityType_GenericBytes;
	m_cbSize = int(cbLen);
	memcpy(m_genericBytes, data, m_cbSize);
	return true;
}
inline const uint8_t *SteamNetworkingIdentity::GetGenericBytes(int &cbLen) const
{
	if(m_eType != k_ESteamNetworkingIdentityType_GenericBytes)
		return NULL;
	cbLen = m_cbSize;
	return m_genericBytes;
}
inline bool SteamNetworkingIdentity::operator==(const SteamNetworkingIdentity &x) const { return m_eType == x.m_eType && m_cbSize == x.m_cbSize && memcmp(m_genericBytes, x.m_genericBytes, m_cbSize) == 0; }

struct SteamNetworkingMessage_t
{
	void *m_pData;
	int m_cbSize;
	HSteamNetConnection m_conn;
	SteamNetworkingIdentity m_identityPeer;
	int64_t m_nConnUserData;
	SteamNetworkingMicroseconds m_usecTimeReceived;
	int64_t m_nMessageNumber;
	void (*m_pfnFreeData)(SteamNetworkingMessage_t *pMsg);
	void (*m_pfnRelease)(SteamNetworkingMessage_t *pMsg);
	int m_nChannel;
	int m_nFlags;
	int64_t m_nUserData;
	uint16_t m_idxLane;
	uint16_t _pad1__;
	inline void Release();

protected:
	inline ~SteamNetworkingMessage_t() {}
};

struct SteamNetConnectionInfo_t
{
	SteamNetworkingIdentity m_identityRemote;
	int64_t m_nUserData;
	HSteamListenSocket m_hListenSocket;
	SteamNetworkingIPAddr m_addrRemote;
	uint16_t m__pad1;
	SteamNetworkingPOPID m_idPOPRemote;
	SteamNetworkingPOPID m_idPOPRelay;
	ESteamNetworkingConnectionState m_eState;
	int m_eEndReason;
	char m_szEndDebug[k_cchSteamNetworkingMaxConnectionCloseReason];
	char m_szConnectionDescription[k_cchSteamNetworkingMaxConnectionDescription];
	int m_nFlags;
	uint32_t reserved[63];
};
inline void SteamNetworkingMessage_t::Release() { (*m_pfnRelease)(this); }

struct SteamNetworkingMessagesSessionRequest_t
{
	enum
	{
		k_iCallback = 1250 + 1
	};
	SteamNetworkingIdentity m_identityRemote;
};

enum ESteamNetworkingConfigValue
{
	k_ESteamNetworkingConfig_Invalid = 0,
	k_ESteamNetworkingConfig_TimeoutInitial = 24,
	k_ESteamNetworkingConfig_TimeoutConnected = 25,
	k_ESteamNetworkingConfig_SendBufferSize = 9,
	k_ESteamNetworkingConfig_RecvBufferSize = 47,
	k_ESteamNetworkingConfig_RecvBufferMessages = 48,
	k_ESteamNetworkingConfig_RecvMaxMessageSize = 49,
	k_ESteamNetworkingConfig_RecvMaxSegmentsPerPacket = 50,
	k_ESteamNetworkingConfig_ConnectionUserData = 40,
	k_ESteamNetworkingConfig_SendRateMin = 10,
	k_ESteamNetworkingConfig_SendRateMax = 11,
	k_ESteamNetworkingConfig_NagleTime = 12,
	k_ESteamNetworkingConfig_IP_AllowWithoutAuth = 23,
	k_ESteamNetworkingConfig_IPLocalHost_AllowWithoutAuth = 52,
	k_ESteamNetworkingConfig_MTU_PacketSize = 32,
	k_ESteamNetworkingConfig_MTU_DataSize = 33,
	k_ESteamNetworkingConfig_Unencrypted = 34,
	k_ESteamNetworkingConfig_SymmetricConnect = 37,
	k_ESteamNetworkingConfig_LocalVirtualPort = 38,
	k_ESteamNetworkingConfig_DualWifi_Enable = 39,
	k_ESteamNetworkingConfig_EnableDiagnosticsUI = 46,
	k_ESteamNetworkingConfig_SendTimeSincePreviousPacket = 59,
	k_ESteamNetworkingConfig_FakePacketLoss_Send = 2,
	k_ESteamNetworkingConfig_FakePacketLoss_Recv = 3,
	k_ESteamNetworkingConfig_FakePacketLag_Send = 4,
	k_ESteamNetworkingConfig_FakePacketLag_Recv = 5,
	k_ESteamNetworkingConfig_FakePacketJitter_Send_Avg = 53,
	k_ESteamNetworkingConfig_FakePacketJitter_Send_Max = 54,
	k_ESteamNetworkingConfig_FakePacketJitter_Send_Pct = 55,
	k_ESteamNetworkingConfig_FakePacketJitter_Recv_Avg = 56,
	k_ESteamNetworkingConfig_FakePacketJitter_Recv_Max = 57,
	k_ESteamNetworkingConfig_FakePacketJitter_Recv_Pct = 58,
	k_ESteamNetworkingConfig_FakePacketReorder_Send = 6,
	k_ESteamNetworkingConfig_FakePacketReorder_Recv = 7,
	k_ESteamNetworkingConfig_FakePacketReorder_Time = 8,
	k_ESteamNetworkingConfig_FakePacketDup_Send = 26,
	k_ESteamNetworkingConfig_FakePacketDup_Recv = 27,
	k_ESteamNetworkingConfig_FakePacketDup_TimeMax = 28,
	k_ESteamNetworkingConfig_PacketTraceMaxBytes = 41,
	k_ESteamNetworkingConfig_FakeRateLimit_Send_Rate = 42,
	k_ESteamNetworkingConfig_FakeRateLimit_Send_Burst = 43,
	k_ESteamNetworkingConfig_FakeRateLimit_Recv_Rate = 44,
	k_ESteamNetworkingConfig_FakeRateLimit_Recv_Burst = 45,
	k_ESteamNetworkingConfig_OutOfOrderCorrectionWindowMicroseconds = 51,
	k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged = 201,
	k_ESteamNetworkingConfig_Callback_AuthStatusChanged = 202,
	k_ESteamNetworkingConfig_Callback_RelayNetworkStatusChanged = 203,
	k_ESteamNetworkingConfig_Callback_MessagesSessionRequest = 204,
	k_ESteamNetworkingConfig_Callback_MessagesSessionFailed = 205,
	k_ESteamNetworkingConfig_Callback_CreateConnectionSignaling = 206,
	k_ESteamNetworkingConfig_Callback_FakeIPResult = 207,
	k_ESteamNetworkingConfig_P2P_STUN_ServerList = 103,
	k_ESteamNetworkingConfig_P2P_Transport_ICE_Enable = 104,
	k_ESteamNetworkingConfig_P2P_Transport_ICE_Penalty = 105,
	k_ESteamNetworkingConfig_P2P_Transport_SDR_Penalty = 106,
	k_ESteamNetworkingConfig_P2P_TURN_ServerList = 107,
	k_ESteamNetworkingConfig_P2P_TURN_UserList = 108,
	k_ESteamNetworkingConfig_P2P_TURN_PassList = 109,
	// k_ESteamNetworkingConfig_P2P_Transport_LANBeacon_Penalty = 107,
	k_ESteamNetworkingConfig_P2P_Transport_ICE_Implementation = 110,
	k_ESteamNetworkingConfig_SDRClient_ConsecutitivePingTimeoutsFailInitial = 19,
	k_ESteamNetworkingConfig_SDRClient_ConsecutitivePingTimeoutsFail = 20,
	k_ESteamNetworkingConfig_SDRClient_MinPingsBeforePingAccurate = 21,
	k_ESteamNetworkingConfig_SDRClient_SingleSocket = 22,
	k_ESteamNetworkingConfig_SDRClient_ForceRelayCluster = 29,
	k_ESteamNetworkingConfig_SDRClient_DevTicket = 30,
	k_ESteamNetworkingConfig_SDRClient_ForceProxyAddr = 31,
	k_ESteamNetworkingConfig_SDRClient_FakeClusterPing = 36,
	k_ESteamNetworkingConfig_SDRClient_LimitPingProbesToNearestN = 60,
	k_ESteamNetworkingConfig_LogLevel_AckRTT = 13, 
	k_ESteamNetworkingConfig_LogLevel_PacketDecode = 14, 
	k_ESteamNetworkingConfig_LogLevel_Message = 15, 
	k_ESteamNetworkingConfig_LogLevel_PacketGaps = 16, 
	k_ESteamNetworkingConfig_LogLevel_P2PRendezvous = 17, 
	k_ESteamNetworkingConfig_LogLevel_SDRRelayPings = 18,
	k_ESteamNetworkingConfig_ECN = 999,
	k_ESteamNetworkingConfig_DELETED_EnumerateDevVars = 35,
	k_ESteamNetworkingConfigValue__Force32Bit = 0x7fffffff
};

enum ESteamNetworkingConfigDataType
{
	k_ESteamNetworkingConfig_Int32 = 1,
	k_ESteamNetworkingConfig_Int64 = 2,
	k_ESteamNetworkingConfig_Float = 3,
	k_ESteamNetworkingConfig_String = 4,
	k_ESteamNetworkingConfig_Ptr = 5,
	k_ESteamNetworkingConfigDataType__Force32Bit = 0x7fffffff
};
enum ESteamNetworkingConfigScope
{

	k_ESteamNetworkingConfig_Global = 1,
	k_ESteamNetworkingConfig_SocketsInterface = 2,
	k_ESteamNetworkingConfig_ListenSocket = 3,
	k_ESteamNetworkingConfig_Connection = 4,
	k_ESteamNetworkingConfigScope__Force32Bit = 0x7fffffff
};
const int k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Default = -1;
const int k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Disable = 0; 
const int k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Relay = 1; 
const int k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Private = 2; 
const int k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_Public = 4; 
const int k_nSteamNetworkingConfig_P2P_Transport_ICE_Enable_All = 0x7fffffff;

struct SteamNetworkingConfigValue_t
{
	/// Which option is being set
	ESteamNetworkingConfigValue m_eValue;

	/// Which field below did you fill in?
	ESteamNetworkingConfigDataType m_eDataType;

	/// Option value
	union
	{
		int32_t m_int32;
		int64_t m_int64;
		float m_float;
		const char *m_string; // Points to your '\0'-terminated buffer
		void *m_ptr;
	} m_val;

	//
	// Shortcut helpers to set the type and value in a single call
	//
	inline void SetInt32(ESteamNetworkingConfigValue eVal, int32_t data)
	{
		m_eValue = eVal;
		m_eDataType = k_ESteamNetworkingConfig_Int32;
		m_val.m_int32 = data;
	}
	inline void SetInt64(ESteamNetworkingConfigValue eVal, int64_t data)
	{
		m_eValue = eVal;
		m_eDataType = k_ESteamNetworkingConfig_Int64;
		m_val.m_int64 = data;
	}
	inline void SetFloat(ESteamNetworkingConfigValue eVal, float data)
	{
		m_eValue = eVal;
		m_eDataType = k_ESteamNetworkingConfig_Float;
		m_val.m_float = data;
	}
	inline void SetPtr(ESteamNetworkingConfigValue eVal, void *data)
	{
		m_eValue = eVal;
		m_eDataType = k_ESteamNetworkingConfig_Ptr;
		m_val.m_ptr = data;
	}
	inline void SetString(ESteamNetworkingConfigValue eVal, const char *data) // WARNING - Just saves your pointer.  Does NOT make a copy of the string
	{
		m_eValue = eVal;
		m_eDataType = k_ESteamNetworkingConfig_Ptr;
		m_val.m_string = data;
	}
};

enum ESteamNetworkingSocketsDebugOutputType
{
	k_ESteamNetworkingSocketsDebugOutputType_None = 0,
	k_ESteamNetworkingSocketsDebugOutputType_Bug = 1, // You used the API incorrectly, or an internal error happened
	k_ESteamNetworkingSocketsDebugOutputType_Error = 2, // Run-time error condition that isn't the result of a bug.  (E.g. we are offline, cannot bind a port, etc)
	k_ESteamNetworkingSocketsDebugOutputType_Important = 3, // Nothing is wrong, but this is an important notification
	k_ESteamNetworkingSocketsDebugOutputType_Warning = 4,
	k_ESteamNetworkingSocketsDebugOutputType_Msg = 5, // Recommended amount
	k_ESteamNetworkingSocketsDebugOutputType_Verbose = 6, // Quite a bit
	k_ESteamNetworkingSocketsDebugOutputType_Debug = 7, // Practically everything
	k_ESteamNetworkingSocketsDebugOutputType_Everything = 8, // Wall of text, detailed packet contents breakdown, etc

	k_ESteamNetworkingSocketsDebugOutputType__Force32Bit = 0x7fffffff
};

typedef void (*FSteamNetworkingSocketsDebugOutput)(ESteamNetworkingSocketsDebugOutputType nType, const char *pszMsg);


STEAMAPI ISteamNetworkingMessages *SteamAPI_SteamNetworkingMessages_SteamAPI_v002();
STEAMAPI EResult SteamAPI_ISteamNetworkingMessages_SendMessageToUser(ISteamNetworkingMessages *self, const SteamNetworkingIdentity &identityRemote, const void *pubData, uint32_t cubData, int nSendFlags, int nRemoteChannel);
STEAMAPI int SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel(ISteamNetworkingMessages *self, int nLocalChannel, SteamNetworkingMessage_t **ppOutMessages, int nMaxMessages);
STEAMAPI bool SteamAPI_ISteamNetworkingMessages_AcceptSessionWithUser(ISteamNetworkingMessages *self, const SteamNetworkingIdentity &identityRemote);
STEAMAPI bool SteamAPI_ISteamNetworkingMessages_CloseSessionWithUser(ISteamNetworkingMessages *self, const SteamNetworkingIdentity &identityRemote);
STEAMAPI bool SteamAPI_ISteamNetworkingMessages_CloseChannelWithUser(ISteamNetworkingMessages *self, const SteamNetworkingIdentity &identityRemote, int nLocalChannel);
STEAMAPI ESteamNetworkingConnectionState SteamAPI_ISteamNetworkingMessages_GetSessionConnectionInfo(ISteamNetworkingMessages *self, const SteamNetworkingIdentity &identityRemote, SteamNetConnectionInfo_t *pConnectionInfo, SteamNetConnectionRealTimeStatus_t *pQuickStatus);

STEAMAPI ISteamMatchmaking *SteamAPI_SteamMatchmaking_v009();
STEAMAPI SteamAPICall_t SteamAPI_ISteamMatchmaking_CreateLobby(ISteamMatchmaking *self, int eLobbyType, int cMaxMembers);
STEAMAPI SteamAPICall_t SteamAPI_ISteamMatchmaking_JoinLobby(ISteamMatchmaking *self, CSteamID steamIDLobby);
STEAMAPI bool SteamAPI_ISteamMatchmaking_LeaveLobby(ISteamMatchmaking *self, CSteamID steamIDLobby);

STEAMAPI ISteamNetworkingUtils *SteamAPI_SteamNetworkingUtils_SteamAPI_v004();
STEAMAPI void SteamAPI_ISteamNetworkingUtils_InitRelayNetworkAccess(ISteamNetworkingUtils *self);
STEAMAPI bool SteamAPI_ISteamNetworkingUtils_SetConfigValueStruct(ISteamNetworkingUtils *self, const SteamNetworkingConfigValue_t &opt, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj);
STEAMAPI void SteamAPI_ISteamNetworkingUtils_SetDebugOutputFunction(ISteamNetworkingUtils *self, ESteamNetworkingSocketsDebugOutputType eDetailLevel, FSteamNetworkingSocketsDebugOutput pfnFunc);

STEAMAPI uint64_t SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(ISteamMatchmaking *self, uint64_t steamIDLobby, int iMember);
STEAMAPI int SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(ISteamMatchmaking *self, uint64_t steamIDLobby);
STEAMAPI uint64_t SteamAPI_ISteamMatchmaking_GetLobbyOwner(ISteamMatchmaking *self, uint64_t steamIDLobby);
STEAMAPI bool SteamAPI_ISteamMatchmaking_SetLobbyData(ISteamMatchmaking *self, uint64_t steamIDLobby, const char *pchKey, const char *pchValue);
STEAMAPI const char *SteamAPI_ISteamMatchmaking_GetLobbyData(ISteamMatchmaking *self, uint64_t steamIDLobby, const char *pchKey);

STEAMAPI ISteamUser *SteamAPI_SteamUser_v021();
STEAMAPI uint64_t SteamAPI_ISteamUser_GetSteamID(ISteamUser *self);
}

#endif // STEAM_STEAM_API_FLAT_H
