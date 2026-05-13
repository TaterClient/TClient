#include "mumble.h"

#include <base/log.h>
#include <base/str.h>

#include <engine/client.h>
#include <engine/external/mumble.h>

#include <game/client/gameclient.h>

#define MUMBLE_2D_SCALE 128.0f

void CMumble::MakeContext()
{
	mumble_destroy_context(&m_pContext);
	m_pContext = mumble_create_context_args("DDNet", "DDraceNetwork (DDNet) is an actively maintained version of DDRace, a Teeworlds modification with a unique cooperative gameplay.");
	if(!m_pContext)
	{
		log_error("mumble", "Failed to create context");
		return;
	}
	m_LastClientId = -1;
	m_LastTeam = 0;
	m_LastAddr = {};
	log_info("mumble", "Mumble inited");
}

CMumble::CMumble()
{
	MakeContext();
}

CMumble::~CMumble()
{
	mumble_destroy_context(&m_pContext);
}

void CMumble::OnRender()
{
	if(!m_pContext)
		return;
	if(Client()->State() != IClient::STATE_ONLINE)
	{
		auto *pLm = mumble_get_linked_mem(m_pContext);
		pLm->context_len = 0;
		pLm->identity[0] = L'\0';
		m_LastClientId = -1;
		m_LastTeam = -1;
		return;
	}
	if(mumble_relink_needed(m_pContext))
	{
		MakeContext();
		if(!m_pContext)
			return;
	}
	const int ClientId = GameClient()->m_Snap.m_LocalClientId;
	const int Team = GameClient()->IsTeamPlay() ? GameClient()->m_aClients[ClientId].m_Team : GameClient()->m_Teams.Team(ClientId);
	const NETADDR &NetAddr = Client()->ServerAddress();
	if(Team != m_LastTeam || NetAddr != m_LastAddr)
	{
		char aContext[64];
		net_addr_str(&NetAddr, aContext, sizeof(aContext), true);
		str_append(aContext, " ");
		str_format(aContext + str_length(aContext), sizeof(aContext) - str_length(aContext), "%d", Team);
		if(!mumble_set_context(m_pContext, aContext))
			log_error("mumble", "Failed to set context");
		m_LastTeam = Team;
		m_LastAddr = NetAddr;
	}
	if(ClientId != m_LastClientId)
	{
		char aIdentity[16];
		str_format(aIdentity, sizeof(aIdentity), "%d", ClientId);
		if(!mumble_set_identity(m_pContext, aIdentity))
			log_error("mumble", "Failed to set identity");
		m_LastClientId = ClientId;
	}
	const auto &Pos = GameClient()->m_aClients[ClientId].m_RenderPos;
	mumble_2d_update(m_pContext, Pos.x / MUMBLE_2D_SCALE, Pos.y / MUMBLE_2D_SCALE);
}

void CMumble::OnConsoleInit()
{
	const auto pfnFunc = [](IConsole::IResult *pResult, void *pUserData) { ((CMumble *)pUserData)->MakeContext(); };
	Console()->Register("mumble_reconnect", "", CFGFLAG_CLIENT, pfnFunc, this, "Reconnect to Mumble");
}
