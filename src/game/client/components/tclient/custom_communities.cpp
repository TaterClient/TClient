#include "custom_communities.h"

#include <base/log.h>

#include <engine/external/json-parser/json.h>

#include <engine/client/serverbrowser.h>

#include <engine/shared/config.h>

static constexpr char CUSTOM_COMMUNITIES_DDNET_INFO_FILE[] = "custom-communities-ddnet-info.json";

void CCustomCommunities::DownloadCustomCommunitiesDDNetInfo()
{
	log_info("a", "%s %p\n", g_Config.m_TcCustomCommunitiesUrl, m_pCustomCommunitiesDDNetInfoTask.get());
	if(g_Config.m_TcCustomCommunitiesUrl[0] == '\0')
	{
		LoadCustomCommunitiesDDNetInfo();
	}
	else
	{
		if(m_pCustomCommunitiesDDNetInfoTask != nullptr)
		{
			m_pCustomCommunitiesDDNetInfoTask->Abort();
			m_pCustomCommunitiesDDNetInfoTask = nullptr;
		}
		m_pCustomCommunitiesDDNetInfoTask = HttpGetFile(g_Config.m_TcCustomCommunitiesUrl, Storage(), CUSTOM_COMMUNITIES_DDNET_INFO_FILE, IStorage::TYPE_SAVE);
		m_pCustomCommunitiesDDNetInfoTask->Timeout(CTimeout{10000, 0, 500, 10});
		m_pCustomCommunitiesDDNetInfoTask->SkipByFileTime(false); // Always re-download.
		// Use ipv4 so we can know the ingame ip addresses of players before they join game servers
		m_pCustomCommunitiesDDNetInfoTask->IpResolve(IPRESOLVE::V4);
		Http()->Run(m_pCustomCommunitiesDDNetInfoTask);
	}
}

void CCustomCommunities::LoadCustomCommunitiesDDNetInfo()
{
	log_info("a", "hello");
	auto *pServerBrowser = dynamic_cast<CServerBrowser *>(ServerBrowser());
	dbg_assert(pServerBrowser, "pServerBrowser is nullptr");

	// Handle disabled case
	if(g_Config.m_TcCustomCommunitiesUrl[0] == '\0')
	{
		pServerBrowser->m_CustomCommunitiesFunction = nullptr;
		pServerBrowser->LoadDDNetServers();
		return;
	}
	log_info("a", "jello");

	// Read and parse file
	void *pBuf;
	unsigned Length;
	if(!Storage()->ReadFile(CUSTOM_COMMUNITIES_DDNET_INFO_FILE, IStorage::TYPE_SAVE, &pBuf, &Length))
	{
		return;
	}
	json_settings JsonSettings{};
	char aError[256];
	json_value *pCustomCommunitiesDDNetInfo = json_parse_ex(&JsonSettings, static_cast<json_char *>(pBuf), Length, aError);
	free(pBuf);
	if(pCustomCommunitiesDDNetInfo == nullptr)
	{
		log_error("customcommunities", "invalid info json: '%s'", aError);
		return;
	}
	else if(pCustomCommunitiesDDNetInfo->type != json_object)
	{
		log_error("customcommunities", "invalid info root");
		json_value_free(pCustomCommunitiesDDNetInfo);
		return;
	}
	log_info("a", "kello");

	// Apply new data
	if(m_pCustomCommunitiesDDNetInfo)
		json_value_free(m_pCustomCommunitiesDDNetInfo);
	m_pCustomCommunitiesDDNetInfo = pCustomCommunitiesDDNetInfo;
	pServerBrowser->m_CustomCommunitiesFunction = [&](std::vector<json_value *> &vCommunities) {
		const json_value &Communities = (*m_pCustomCommunitiesDDNetInfo)["communities"];
		if(Communities.type == json_array)
		{
			vCommunities.insert(
				vCommunities.end(),
				Communities.u.array.values,
				Communities.u.array.values + Communities.u.array.length);
		}
	};
	log_info("a", "feelo");
	pServerBrowser->LoadDDNetServers();
}

void CCustomCommunities::OnInit()
{
	DownloadCustomCommunitiesDDNetInfo();
}

void CCustomCommunities::OnConsoleInit()
{
	Console()->Chain("tc_custom_communities_url", [](IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData) {
		pfnCallback(pResult, pCallbackUserData);
		if(pResult->NumArguments() > 0)
			((CCustomCommunities *)pUserData)->DownloadCustomCommunitiesDDNetInfo();
	}, this);
}

void CCustomCommunities::OnRender()
{
	if(m_pCustomCommunitiesDDNetInfoTask)
	{
		log_info("a", "%d %d", m_pCustomCommunitiesDDNetInfoTask->State(), EHttpState::DONE);
		const auto State = m_pCustomCommunitiesDDNetInfoTask->State();
		if(State != EHttpState::RUNNING && State != EHttpState::QUEUED)
		{
			// TODO if(m_ServerBrowser.DDNetInfoSha256() == m_pDDNetInfoTask->ResultSha256())
			m_pCustomCommunitiesDDNetInfoTask = nullptr;
			LoadCustomCommunitiesDDNetInfo();
		}
	}
}

CCustomCommunities::~CCustomCommunities()
{
	m_pCustomCommunitiesDDNetInfoTask = nullptr;
	if(m_pCustomCommunitiesDDNetInfo)
		json_value_free(m_pCustomCommunitiesDDNetInfo);
}
