#include "scripting.h"

#include "scripting/impl.h"

#include <base/log.h>
#include <base/str.h>

#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/component.h>
#include <game/client/gameclient.h>

class CScriptRunner : CComponentInterfaces
{
private:
	CScriptingCtx m_ScriptingCtx;
	const CServerInfo *GetServerInfo()
	{
		if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
		{
			return &Client()->ServerInfo();
		}
		else if(GameClient()->m_ConnectServerInfo)
		{
			return &*GameClient()->m_ConnectServerInfo;
		}
		return nullptr;
	}
	CScriptingCtx::Any State(const std::string &Str, const CScriptingCtx::Any &Arg)
	{
		if(Str == "game_mode")
		{
			return GameClient()->m_GameInfo.m_aGameType;
		}
		else if(Str == "game_mode_pvp")
		{
			return GameClient()->m_GameInfo.m_Pvp;
		}
		else if(Str == "game_mode_race")
		{
			return GameClient()->m_GameInfo.m_Race;
		}
		else if(Str == "eye_wheel_allowed")
		{
			return GameClient()->m_GameInfo.m_AllowEyeWheel;
		}
		else if(Str == "zoom_allowed")
		{
			return GameClient()->m_GameInfo.m_AllowZoom;
		}
		else if(Str == "dummy_allowed")
		{
			return Client()->DummyAllowed();
		}
		else if(Str == "dummy_connected")
		{
			return Client()->DummyConnected();
		}
		else if(Str == "rcon_authed")
		{
			return Client()->RconAuthed();
		}
		else if(Str == "team")
		{
			return GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_Team;
		}
		if(Str == "ddnet_team")
		{
			return GameClient()->m_Teams.Team(GameClient()->m_aLocalIds[g_Config.m_ClDummy]);
		}
		if(Str == "map")
		{
			if(Client()->State() == IClient::STATE_ONLINE || Client()->State() == IClient::STATE_DEMOPLAYBACK)
				return GameClient()->Map()->BaseName();
			else if(GameClient()->m_ConnectServerInfo)
				return GameClient()->m_ConnectServerInfo->m_aMap;
			else
				return nullptr;
		}
		else if(Str == "server_ip")
		{
			const NETADDR *pAddress = nullptr;
			if(Client()->State() == IClient::STATE_ONLINE)
				pAddress = &Client()->ServerAddress();
			else if(GameClient()->m_ConnectServerInfo)
				pAddress = &GameClient()->m_ConnectServerInfo->m_aAddresses[0];
			else
				return nullptr;
			char Addr[128];
			net_addr_str(pAddress, Addr, sizeof(Addr), true);
			return Addr;
		}
		else if(Str == "players_connected")
		{
			return GameClient()->m_Snap.m_NumPlayers;
		}
		else if(Str == "players_cap")
		{
			const CServerInfo *pServerInfo = GetServerInfo();
			if(!pServerInfo)
				return nullptr;
			return pServerInfo->m_MaxClients;
		}
		else if(Str == "server_name")
		{
			const CServerInfo *pServerInfo = GetServerInfo();
			if(!pServerInfo)
				return nullptr;
			return pServerInfo->m_aName;
		}
		else if(Str == "community")
		{
			const CServerInfo *pServerInfo = GetServerInfo();
			if(!pServerInfo)
				return nullptr;
			return pServerInfo->m_aCommunityId;
		}
		else if(Str == "location")
		{
			if(GameClient()->m_GameInfo.m_Race)
				return nullptr;
			float w = 100.0f, h = 100.0;
			float x = 50.0f, y = 50.0f;
			const CLayers *pLayers = GameClient()->m_MapLayersForeground.m_pLayers;
			const CMapItemLayerTilemap *pLayer = pLayers->GameLayer();
			if(pLayer)
			{
				w = (float)pLayer->m_Width * 30.0f;
				h = (float)pLayer->m_Height * 30.0f;
			}
			x = GameClient()->m_Camera.m_Center.x;
			y = GameClient()->m_Camera.m_Center.y;
			static const char *s_apLocations[] = {
				"NW", "N", "NE",
				"W", "C", "E",
				"SW", "S", "SE"};
			int i = std::clamp((int)(y / h * 3.0f), 0, 2) * 3 + std::clamp((int)(x / w * 3.0f), 0, 2);
			return s_apLocations[i];
		}
		else if(Str == "state")
		{
			const char *pState = nullptr;
			switch(Client()->State())
			{
			case IClient::EClientState::STATE_CONNECTING:
				pState = "connecting";
				break;
			case IClient::STATE_OFFLINE:
				pState = "offline";
				break;
			case IClient::STATE_LOADING:
				pState = "loading";
				break;
			case IClient::STATE_ONLINE:
				pState = "online";
				break;
			case IClient::STATE_DEMOPLAYBACK:
				pState = "demo";
				break;
			case IClient::STATE_QUITTING:
				pState = "quitting";
				break;
			case IClient::STATE_RESTARTING:
				pState = "restarting";
				break;
			}
			return pState;
		}
		else if(Str == "id")
		{
			if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
				return nullptr;
			if(!std::holds_alternative<std::string>(Arg))
				return nullptr;
			const std::string &Name = std::get<std::string>(Arg);
			for(const auto &Client : GameClient()->m_aClients)
			{
				if(!Client.m_Active)
					continue;
				if(str_comp(Name.c_str(), Client.m_aName) == 0)
					return Client.ClientId();
			}
			for(const auto &Client : GameClient()->m_aClients)
			{
				if(!Client.m_Active)
					continue;
				if(str_comp_nocase(Name.c_str(), Client.m_aName) == 0)
					return Client.ClientId();
			}
			return nullptr;
		}
		else if(Str == "name")
		{
			if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
				return nullptr;
			if(!std::holds_alternative<int>(Arg))
				return nullptr;
			int Id = std::get<int>(Arg);
			if(Id < 0 || Id > MAX_CLIENTS)
				return nullptr;
			if(!GameClient()->m_aClients[Id].m_Active)
				return nullptr;
			return GameClient()->m_aClients[Id].m_aName;
		}
		else if(Str == "clan")
		{
			if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
				return nullptr;
			if(!std::holds_alternative<int>(Arg))
				return nullptr;
			int Id = std::get<int>(Arg);
			if(Id < 0 || Id > MAX_CLIENTS)
				return nullptr;
			if(!GameClient()->m_aClients[Id].m_Active)
				return nullptr;
			return GameClient()->m_aClients[Id].m_aClan;
		}
		throw std::string("No state with name '") + Str + std::string("'");
	}

public:
	CScriptRunner(CGameClient *pClient)
	{
		OnInterfacesInit(pClient);
		m_ScriptingCtx.AddFunction("exec", [this](const std::string &Str) {
			log_info(SCRIPTING_IMPL "/exec", "%s", Str.c_str());
			Console()->ExecuteLine(Str.c_str(), IConsole::CLIENT_ID_UNSPECIFIED);
		});
		m_ScriptingCtx.AddFunction("echo", [this](const std::string &Str) {
			GameClient()->Echo(Str.c_str());
		});
		m_ScriptingCtx.AddFunction("state", [this](const std::string &Str, const CScriptingCtx::Any &Arg) {
			return State(Str, Arg);
		});
	}
	void Run(const char *pFilename, const char *pArgs)
	{
		m_ScriptingCtx.Run(Storage(), pFilename, pArgs);
	}
};

void CScripting::ConExecScript(IConsole::IResult *pResult, void *pUserData)
{
	CScripting *pThis = static_cast<CScripting *>(pUserData);
	pThis->ExecScript(pResult->GetString(0), pResult->GetString(1));
}

void CScripting::ExecScript(const char *pFilename, const char *pArgs)
{
	CScriptRunner Runner(GameClient());
	Runner.Run(pFilename, pArgs);
}

void CScripting::OnConsoleInit()
{
	Console()->Register(SCRIPTING_IMPL, "s[file] ?r[args]", CFGFLAG_CLIENT, ConExecScript, this, "Execute a " SCRIPTING_IMPL " script");
}
