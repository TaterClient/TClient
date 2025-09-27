#include "custom_communities.h"

#include <base/log.h>

#include <engine/external/json-parser/json.h>

#include <engine/client/serverbrowser.h>

#include <engine/shared/config.h>

static constexpr char CUSTOM_COMMUNITIES_DDNET_INFO_FILE[] = "custom-communities-ddnet-info.json";

void CCustomCommunities::StartLoadCustomCommunitiesDDNetInfo()
{
	if(m_pCustomCommunitiesDDNetInfoTask != nullptr)
	{
		m_pCustomCommunitiesDDNetInfoTask->Abort();
	}
	if(g_Config.m_TcCustomCommunitiesUrl[0] != '\0')
	{
		m_pCustomCommunitiesDDNetInfoTask = HttpGetFile(g_Config.m_TcCustomCommunitiesUrl, Storage(), CUSTOM_COMMUNITIES_DDNET_INFO_FILE, IStorage::TYPE_SAVE);
		m_pCustomCommunitiesDDNetInfoTask->Timeout(CTimeout{10000, 0, 500, 10});
		m_pCustomCommunitiesDDNetInfoTask->SkipByFileTime(false); // Always re-download.
		// Use ipv4 so we can know the ingame ip addresses of players before they join game servers
		m_pCustomCommunitiesDDNetInfoTask->IpResolve(IPRESOLVE::V4);
	}
	LoadCustomCommunitiesDDNetInfo();
}

void CCustomCommunities::LoadCustomCommunitiesDDNetInfo()
{
	void *pBuf;
	unsigned Length;
	if(!Storage()->ReadFile(CUSTOM_COMMUNITIES_DDNET_INFO_FILE, IStorage::TYPE_SAVE, &pBuf, &Length))
	{
		return;
	}

	if(m_pCustomCommunitiesDDNetInfo)
		json_value_free(m_pCustomCommunitiesDDNetInfo);
	json_settings JsonSettings{};
	char aError[256];
	m_pCustomCommunitiesDDNetInfo = json_parse_ex(&JsonSettings, static_cast<json_char *>(pBuf), Length, aError);
	free(pBuf);
	if(m_pCustomCommunitiesDDNetInfo == nullptr)
	{
		log_error("customcommunities", "invalid info json: '%s'", aError);
		return;
	}
	else if(m_pCustomCommunitiesDDNetInfo->type != json_object)
	{
		log_error("customcommunities", "invalid info root");
		json_value_free(m_pCustomCommunitiesDDNetInfo);
		m_pCustomCommunitiesDDNetInfo = nullptr;
		return;
	}

	auto *pServerBrowser = dynamic_cast<CServerBrowser *>(ServerBrowser());
	dbg_assert(pServerBrowser, "pServerBrowser is nullptr");
	pServerBrowser->m_CustomCommunitiesFunction = [&](std::vector<json_value *> &vCommunities) { CustomCommunitiesFunction(vCommunities); };
	pServerBrowser->LoadDDNetServers();
}

void CCustomCommunities::CustomCommunitiesFunction(std::vector<json_value *> &vCommunities)
{
	if(m_pCustomCommunitiesDDNetInfo && g_Config.m_TcCustomCommunitiesUrl[0] != '\0')
	{
		const json_value &Communities = (*m_pCustomCommunitiesDDNetInfo)["communities"];
		if(Communities.type == json_array)
		{
			vCommunities.insert(
				vCommunities.end(),
				Communities.u.array.values,
				Communities.u.array.values + Communities.u.array.length);
		}
	}
}

void CCustomCommunities::OnConsoleInit()
{
	Console()->Chain("tc_custom_communities_url", [](IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData) {
		pfnCallback(pResult, pCallbackUserData);
		if(pResult->NumArguments() > 0)
			((CCustomCommunities *)pUserData)->StartLoadCustomCommunitiesDDNetInfo();
	}, this);
}

void CCustomCommunities::OnRender()
{
	if(m_pCustomCommunitiesDDNetInfoTask)
	{
		if(m_pCustomCommunitiesDDNetInfoTask->State() == EHttpState::DONE)
		{
			// TODO if(m_ServerBrowser.DDNetInfoSha256() == m_pDDNetInfoTask->ResultSha256())
			LoadCustomCommunitiesDDNetInfo();
			m_pCustomCommunitiesDDNetInfoTask = nullptr;
		}
	}
}

CCustomCommunities::~CCustomCommunities()
{
	m_pCustomCommunitiesDDNetInfoTask = nullptr;
	if(m_pCustomCommunitiesDDNetInfo)
		json_value_free(m_pCustomCommunitiesDDNetInfo);
}
