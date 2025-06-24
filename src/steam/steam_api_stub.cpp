#define STEAMAPI DYNAMIC_EXPORT

#include <steam/steam_api_flat.h>

#include <cstdlib>

extern "C" {

bool SteamAPI_Init() { return false; }
HSteamPipe SteamAPI_GetHSteamPipe() { abort(); }
void SteamAPI_Shutdown() { abort(); }
void SteamAPI_ManualDispatch_Init() { abort(); }
void SteamAPI_ManualDispatch_FreeLastCallback(HSteamPipe SteamPipe) { abort(); }
bool SteamAPI_ManualDispatch_GetNextCallback(HSteamPipe SteamPipe, CallbackMsg_t *pCallbackMsg) { abort(); }
void SteamAPI_ManualDispatch_RunFrame(HSteamPipe SteamPipe) { abort(); }
ISteamApps *SteamAPI_SteamApps_v008() { abort(); }
int SteamAPI_ISteamApps_GetLaunchCommandLine(ISteamApps *pSelf, char *pBuffer, int BufferSize) { abort(); }
const char *SteamAPI_ISteamApps_GetLaunchQueryParam(ISteamApps *pSelf, const char *pKey) { abort(); }
ISteamFriends *SteamAPI_SteamFriends_v017() { abort(); }
const char *SteamAPI_ISteamFriends_GetPersonaName(ISteamFriends *pSelf) { abort(); }
bool SteamAPI_ISteamFriends_SetRichPresence(ISteamFriends *pSelf, const char *pKey, const char *pValue) { abort(); }
void SteamAPI_ISteamFriends_ClearRichPresence(ISteamFriends *pSelf) { abort(); }

// TClient
ISteamNetworkingMessages *SteamAPI_SteamNetworkingMessages_SteamAPI_v002() { abort(); }
EResult SteamAPI_ISteamNetworkingMessages_SendMessageToUser(ISteamNetworkingMessages *, const SteamNetworkingIdentity &, const void *, uint32_t, int, int) { abort(); }
int SteamAPI_ISteamNetworkingMessages_ReceiveMessagesOnChannel(ISteamNetworkingMessages *, int, SteamNetworkingMessage_t **, int) { abort(); }
bool SteamAPI_ISteamNetworkingMessages_AcceptSessionWithUser(ISteamNetworkingMessages *, const SteamNetworkingIdentity &) { abort(); }
bool SteamAPI_ISteamNetworkingMessages_CloseSessionWithUser(ISteamNetworkingMessages *, const SteamNetworkingIdentity &) { abort(); }
bool SteamAPI_ISteamNetworkingMessages_CloseChannelWithUser(ISteamNetworkingMessages *, const SteamNetworkingIdentity &, int) { abort(); }
ESteamNetworkingConnectionState SteamAPI_ISteamNetworkingMessages_GetSessionConnectionInfo(ISteamNetworkingMessages *, const SteamNetworkingIdentity &, SteamNetConnectionInfo_t *, SteamNetConnectionRealTimeStatus_t *) { abort(); }

ISteamMatchmaking *SteamAPI_SteamMatchmaking_v009() { abort(); }
SteamAPICall_t SteamAPI_ISteamMatchmaking_CreateLobby(ISteamMatchmaking *, int, int) { abort(); }
SteamAPICall_t SteamAPI_ISteamMatchmaking_JoinLobby(ISteamMatchmaking *, CSteamID) { abort(); }
bool SteamAPI_ISteamMatchmaking_LeaveLobby(ISteamMatchmaking *, CSteamID) { abort(); }

ISteamNetworkingUtils *SteamAPI_SteamNetworkingUtils_SteamAPI_v004() { abort(); }
void SteamAPI_ISteamNetworkingUtils_InitRelayNetworkAccess(ISteamNetworkingUtils *) { abort(); }

uint64_t SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(ISteamMatchmaking *self, uint64_t steamIDLobby, int iMember) { abort(); }
int SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(ISteamMatchmaking *self, uint64_t steamIDLobby) { abort(); }
uint64_t SteamAPI_ISteamMatchmaking_GetLobbyOwner(ISteamMatchmaking *self, uint64_t steamIDLobby) { abort(); }
bool SteamAPI_ISteamMatchmaking_SetLobbyData(ISteamMatchmaking *self, uint64_t steamIDLobby, const char *pchKey, const char *pchValue) { abort(); }
const char *SteamAPI_ISteamMatchmaking_GetLobbyData(ISteamMatchmaking *self, uint64_t steamIDLobby, const char *pchKey) { abort(); }

ISteamUser *SteamAPI_SteamUser_v021() { abort(); }
uint64_t SteamAPI_ISteamUser_GetSteamID(ISteamUser *self) { abort(); }

}
