#include <engine/console.h>
#include <engine/external/remimu.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>

#include <optional>

#include "conditional.h"

static std::optional<bool> RegexMatch(const char *pString, const char *pRegex)
{
	RegexToken aTokens[512];
	int16_t TokenCount = 512;
	if(regex_parse(pRegex, aTokens, &TokenCount, 0))
		return std::nullopt;
	return regex_match(aTokens, pString, 0, 0, 0, 0) != -1;
}

int CConditional::ParseValue(char *pBuf, int Length)
{
	const char *pFirstSpace = nullptr;
	for(const char *p = pBuf; *p != '\0'; ++p)
	{
		if(*p == ' ')
		{
			pFirstSpace = p;
			break;
		}
	}
	if(pFirstSpace && *(pFirstSpace + 1) != '\0')
	{
		// Is a function, only function calls have spaces
		const int FuncLength = pFirstSpace - pBuf;
		char aParam[256];
		str_copy(aParam, pFirstSpace + 1);
		ParseString(aParam, sizeof(aParam));
		for(const auto &[Key, FFunc] : m_vFunctions)
			if(str_comp_nocase_num(pBuf, Key.c_str(), FuncLength) == 0)
				return FFunc(aParam, pBuf, Length);
	}
	else
	{
		// Is a variable
		if(m_pResult)
		{
			// Check for numerics
			int Index;
			if(str_toint(pBuf, &Index))
			{
				if(Index >= 0 && Index < m_pResult->NumArguments())
					return str_copy(pBuf, m_pResult->GetString(Index), Length);
				else
					return str_copy(pBuf, "", Length);
			}
		}
		for(const auto &[Key, FFunc] : m_vVariables)
			if(str_comp_nocase(pBuf, Key.c_str()) == 0)
				return FFunc(pBuf, Length);
	}
	return -1;
}

void CConditional::ParseString(char *pBuf, int Length)
{
	// May give incorrect values on buffer overflow
	if(!pBuf || Length <= 0)
		return;

	std::vector<std::pair<int, int>> vParsedRanges;
	const auto IsInParsedRegion = [&vParsedRanges](int Pos) {
		return std::any_of(vParsedRanges.begin(), vParsedRanges.end(), [Pos](const auto &ParsedRange) {
			return Pos >= ParsedRange.first && Pos < ParsedRange.second;
		});
	};

	int Len = strnlen(pBuf, Length);
	while(true)
	{
		int LastOpen = -1;
		int ClosePos = -1;

		// Find the innermost {...} not inside any parsed range
		for(int i = 0; i < Len; ++i)
		{
			if(pBuf[i] == '{' && !IsInParsedRegion(i))
			{
				LastOpen = i;
			}
			else if(pBuf[i] == '}' && LastOpen != -1 && !IsInParsedRegion(i))
			{
				ClosePos = i;
				break;
			}
		}

		if(LastOpen == -1 || ClosePos <= LastOpen)
			break;

		int ExprLen = ClosePos - LastOpen - 1;

		char aTemp[512];
		int CopyLen = std::min(ExprLen, (int)sizeof(aTemp) - 1);
		mem_copy(aTemp, pBuf + LastOpen + 1, CopyLen);
		aTemp[CopyLen] = '\0';

		int ResultLen = ParseValue(aTemp, sizeof(aTemp));
		if(ResultLen == -1)
		{
			ResultLen = CopyLen;
		}
		else
		{
			int TailLen = Len - ClosePos - 1;
			int NewLen = LastOpen + ResultLen + TailLen;

			if(NewLen >= Length)
			{
				ResultLen = Length - 1 - LastOpen - TailLen;
				if(ResultLen < 0)
					ResultLen = 0;
				NewLen = LastOpen + ResultLen + TailLen;
			}

			// Move tail forward/backward
			mem_move(pBuf + LastOpen + ResultLen, pBuf + ClosePos + 1, TailLen);
			pBuf[NewLen] = '\0';

			// Copy result
			mem_copy(pBuf + LastOpen, aTemp, ResultLen);
			Len = NewLen;

			// Shift existing ranges after ClosePos
			for(auto &ParsedRange : vParsedRanges)
			{
				if(ParsedRange.first > ClosePos)
				{
					int Delta = ResultLen - (ClosePos + 1 - LastOpen);
					ParsedRange.first += Delta;
					ParsedRange.second += Delta;
				}
			}
		}

		// Add this region to parsed ranges
		vParsedRanges.emplace_back(LastOpen, LastOpen + ResultLen);
	}
}

void CConditional::ConIfeq(IConsole::IResult *pResult, void *pUserData)
{
	CConditional *pThis = (CConditional *)pUserData;
	if(str_comp(pResult->GetString(0), pResult->GetString(1)) != 0)
		return;
	pThis->Console()->ExecuteLine(pResult->GetString(2));
}

void CConditional::ConIfneq(IConsole::IResult *pResult, void *pUserData)
{
	CConditional *pThis = (CConditional *)pUserData;
	if(str_comp(pResult->GetString(0), pResult->GetString(1)) == 0)
		return;
	pThis->Console()->ExecuteLine(pResult->GetString(2));
}

void CConditional::ConIfreq(IConsole::IResult *pResult, void *pUserData)
{
	CConditional *pThis = (CConditional *)pUserData;
	std::optional<bool> Result = RegexMatch(pResult->GetString(0), pResult->GetString(1));
	if(!Result.has_value())
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "conditional", "regex error");
		return;
	}
	if(!Result.value())
		return;
	pThis->Console()->ExecuteLine(pResult->GetString(2));
}

void CConditional::ConIfrneq(IConsole::IResult *pResult, void *pUserData)
{
	CConditional *pThis = (CConditional *)pUserData;
	std::optional<bool> Result = RegexMatch(pResult->GetString(0), pResult->GetString(1));
	if(!Result.has_value())
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "conditional", "regex error");
		return;
	}
	if(Result.value())
		return;
	pThis->Console()->ExecuteLine(pResult->GetString(2));
}

void CConditional::ConReturn(IConsole::IResult *pResult, void *pUserData)
{
	CConditional *pThis = (CConditional *)pUserData;
	pThis->Console()->m_Return = true;
}

static int UnitLengthSeconds(char Unit)
{
	switch(Unit)
	{
	case 's':
	case 'S': return 1;
	case 'm':
	case 'M': return 60;
	case 'h':
	case 'H': return 60 * 60;
	case 'd':
	case 'D': return 60 * 60 * 24;
	default: return -1;
	}
}

static int TimeFromStr(const char *pStr, char OutUnit)
{
	double Time = -1;
	char InUnit = OutUnit;
	std::sscanf(pStr, "%lf%c", &Time, &InUnit);
	if(Time < 0)
		return -1;
	int InUnitLength = UnitLengthSeconds(InUnit);
	if(InUnitLength < 0)
		return -1;
	int OutUnitLength = UnitLengthSeconds(OutUnit);
	if(OutUnitLength < 0)
		return -1;
	return std::round(Time * (float)InUnitLength / (float)OutUnitLength);
}

void CConditional::OnConsoleInit()
{
	m_vVariables.emplace_back("l", [&](char *pOut, int Length) {
		return str_copy(pOut, "{", Length);
	});
	m_vVariables.emplace_back("r", [&](char *pOut, int Length) {
		return str_copy(pOut, "}", Length);
	});
	m_vVariables.emplace_back("game_mode", [&](char *pOut, int Length) {
		return str_copy(pOut, GameClient()->m_GameInfo.m_aGameType, Length);
	});
	m_vVariables.emplace_back("game_mode_pvp", [&](char *pOut, int Length) {
		return str_copy(pOut, GameClient()->m_GameInfo.m_Pvp ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("game_mode_race", [&](char *pOut, int Length) {
		return str_copy(pOut, GameClient()->m_GameInfo.m_Race ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("eye_wheel_allowed", [&](char *pOut, int Length) {
		return str_copy(pOut, GameClient()->m_GameInfo.m_AllowEyeWheel ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("zoom_allowed", [&](char *pOut, int Length) {
		return str_copy(pOut, GameClient()->m_GameInfo.m_AllowZoom ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("dummy_allowed", [&](char *pOut, int Length) {
		return str_copy(pOut, Client()->DummyAllowed() ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("dummy_connected", [&](char *pOut, int Length) {
		return str_copy(pOut, Client()->DummyConnected() ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("rcon_authed", [&](char *pOut, int Length) {
		return str_copy(pOut, Client()->RconAuthed() ? "1" : "0", Length);
	});
	m_vVariables.emplace_back("team", [&](char *pOut, int Length) {
		return str_format(pOut, Length, "%d", GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_Team);
	});
	m_vVariables.emplace_back("ddnet_team", [&](char *pOut, int Length) {
		return str_format(pOut, Length, "%d", GameClient()->m_Teams.Team(GameClient()->m_aLocalIds[g_Config.m_ClDummy]));
	});
	m_vVariables.emplace_back("map", [&](char *pOut, int Length) {
		return str_copy(pOut, Client()->GetCurrentMap(), Length);
	});
	m_vVariables.emplace_back("server_ip", [&](char *pOut, int Length) {
		net_addr_str(&Client()->ServerAddress(), pOut, Length, true);
		return str_length(pOut);
	});
	m_vVariables.emplace_back("players_connected", [&](char *pOut, int Length) {
		return str_format(pOut, Length, "%d", GameClient()->m_Snap.m_NumPlayers);
	});
	m_vVariables.emplace_back("players_cap", [&](char *pOut, int Length) {
		CServerInfo CurrentServerInfo;
		Client()->GetServerInfo(&CurrentServerInfo);
		return str_format(pOut, Length, "%d", CurrentServerInfo.m_MaxClients);
	});
	m_vVariables.emplace_back("server_name", [&](char *pOut, int Length) {
		CServerInfo CurrentServerInfo;
		Client()->GetServerInfo(&CurrentServerInfo);
		return str_copy(pOut, CurrentServerInfo.m_aName, Length);
	});
	m_vVariables.emplace_back("community", [&](char *pOut, int Length) {
		CServerInfo CurrentServerInfo;
		Client()->GetServerInfo(&CurrentServerInfo);
		return str_copy(pOut, CurrentServerInfo.m_aCommunityId, Length);
	});
	m_vVariables.emplace_back("location", [&](char *pOut, int Length) {
		if(GameClient()->m_GameInfo.m_Race)
			return str_copy(pOut, "Forbidden", Length);
		float w = 100.0f, h = 100.0f;
		float x = 50.0f, y = 50.0f;
		{
			const CLayers *pLayers = GameClient()->m_MapLayersForeground.m_pLayers;
			const CMapItemLayerTilemap *pLayer = pLayers->GameLayer();
			if(pLayer)
			{
				w = (float)pLayer->m_Width * 30.0f;
				h = (float)pLayer->m_Height * 30.0f;
			}
		}
		{
			x = GameClient()->m_Camera.m_Center.x;
			y = GameClient()->m_Camera.m_Center.y;
		}
		static const char *s_apLocations[] = {
			"NW", "N", "NE",
			"W", "C", "E",
			"SW", "S", "SE"};
		dbg_msg("conditional", "%f %f / %f %f\n", x, y, w, h);
		int i = std::clamp((int)(y / h * 3.0f), 0, 2) * 3 + std::clamp((int)(x / w * 3.0f), 0, 2);
		return str_copy(pOut, s_apLocations[i], Length);
	});

	m_vFunctions.emplace_back("id", [&](const char *pParam, char *pOut, int Length) {
		if(Client()->State() != CClient::STATE_ONLINE && Client()->State() != CClient::STATE_DEMOPLAYBACK)
			return str_copy(pOut, "Not connected", Length);
		for(const auto &Player : GameClient()->m_aClients)
		{
			if(!Player.m_Active)
				continue;
			if(str_comp(Player.m_aName, pParam))
				continue;
			return str_format(pOut, Length, "%d", Player.ClientId());
		}
		return str_copy(pOut, "Invalid Name", Length);
	});
	m_vFunctions.emplace_back("name", [&](const char *pParam, char *pOut, int Length) {
		if(Client()->State() != CClient::STATE_ONLINE && Client()->State() != CClient::STATE_DEMOPLAYBACK)
			return str_copy(pOut, "Not connected", Length);
		int ClientId;
		if(!str_toint(pParam, &ClientId))
			return str_copy(pOut, "Invalid ID", Length);
		if(ClientId < 0 || ClientId >= (int)std::size(GameClient()->m_aClients))
			return str_copy(pOut, "ID out of range", Length);
		const auto &Player = GameClient()->m_aClients[ClientId];
		if(!Player.m_Active)
			return str_copy(pOut, "ID not connected", Length);
		return str_copy(pOut, Player.m_aName, Length);
	});
	m_vFunctions.emplace_back("seconds", [&](const char *pParam, char *pOut, int Length) {
		return str_format(pOut, Length, "%d", TimeFromStr(pParam, 's'));
	});
	m_vFunctions.emplace_back("minutes", [&](const char *pParam, char *pOut, int Length) {
		return str_format(pOut, Length, "%d", TimeFromStr(pParam, 'm'));
	});
	m_vFunctions.emplace_back("hours", [&](const char *pParam, char *pOut, int Length) {
		return str_format(pOut, Length, "%d", TimeFromStr(pParam, 'h'));
	});
	m_vFunctions.emplace_back("days", [&](const char *pParam, char *pOut, int Length) {
		return str_format(pOut, Length, "%d", TimeFromStr(pParam, 'd'));
	});

	Console()->Register("ifeq", "s[a] s[b] r[command]", CFGFLAG_CLIENT, ConIfeq, this, "Comapre 2 values, if equal run the command");
	Console()->Register("ifneq", "s[a] s[b] r[command]", CFGFLAG_CLIENT, ConIfneq, this, "Comapre 2 values, if not equal run the command");
	Console()->Register("ifreq", "s[a] s[b] r[command]", CFGFLAG_CLIENT, ConIfreq, this, "Comapre 2 values, if a matches the regex b run the command");
	Console()->Register("ifrneq", "s[a] s[b] r[command]", CFGFLAG_CLIENT, ConIfrneq, this, "Comapre 2 values, if a doesnt match the regex b run the command");
	Console()->Register("return", "", CFGFLAG_CLIENT, ConReturn, this, "Stop executing the current script, does nothing in other contexts");

	Console()->m_FConditionalCompose = [this](char *pBuf, int Length) {
		ParseString(pBuf, Length);
	};
}
