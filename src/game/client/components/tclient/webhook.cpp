#include "webhook.h"

#include <base/log.h>
#include <engine/client.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include <engine/shared/http.h>
#include <game/client/gameclient.h>

#include <optional>

constexpr const float BUFFER_MAX_TIME = 5.0f; // seconds
constexpr const float BUFFER_MIN_TIME = 0.25f; // seconds
constexpr const float BUFFER_MIN_TIME_AUTHED = 0.1f; // seconds

CWebhook::CWebhook()
{
	m_Buffer.reserve(BUFFER_MAX_SIZE);
}

CWebhook::~CWebhook()
{
	for(const auto &Request : m_vpHttpRequests)
		Request->Abort();
	m_vpHttpRequests.clear();
}

void CWebhook::OnConsoleInit()
{
	Console()->Register(
		"webhook_command", "r[command]", CFGFLAG_CLIENT, [](IConsole::IResult *pResult, void *pUserData) {
			auto &This = *(CWebhook *)pUserData;
			if(g_Config.m_ClWebhookUrl[0] == '\0')
			{
				log_error("webhook", "tc_webhook_url not set");
				return;
			}
			This.ConsoleLine(-1, pResult->GetString(0));
		},
		this, "Send a command to the webhook");
}

void CWebhook::ConsoleLine(int Type, const char *pLine)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return; // This is so sure to break that I'm just disabling it before the problems will begin
	if(Client()->State() == IClient::STATE_QUITTING)
		return; // There's no time to get the response so just stop now
	if(g_Config.m_ClWebhookUrl[0] == '\0')
		return;
	m_TimeSinceLastRequest = 0.0f;
	if(m_Buffer.empty())
		m_TimeSinceBufferStart = 0.0f;
	m_Buffer += std::to_string(Type) + '\n' + std::string(pLine) + '\n';
	if(m_Buffer.size() >= BUFFER_MAX_SIZE)
		FlushBuffer();
}

void CWebhook::OnRender()
{
	if(!m_vpHttpRequests.empty())
	{
		std::erase_if(m_vpHttpRequests, [&](const std::shared_ptr<CHttpRequest> &Request) {
			if(Request->State() == EHttpState::QUEUED || Request->State() == EHttpState::RUNNING)
				return false;
			if(Request->State() == EHttpState::DONE && Request->StatusCode() == 200)
			{
				std::optional<int> Type;
				const char *pLine;
				auto DoLine = [&]() {
					if(Type.has_value())
					{
						if(!pLine)
							log_error("webhook", "Got empty line");
						else if(Type == 0)
							Console()->ExecuteLine(pLine);
						else if(Type == 1)
							Client()->Rcon(pLine);
						else
							log_error("webhook", "Got invalid type");
						Type = std::nullopt;
					}
					else
					{
						Type = str_toint(pLine); // On fail returns 0
					}
				};
				char *pResult;
				size_t Size;
				Request->Result((unsigned char **)&pResult, &Size);
				if(pResult && Size > 0)
				{
					pLine = pResult;
					for(size_t i = 0; i < Size && pResult[i] != '\0'; ++i)
					{
						if(pResult[i] == '\n')
						{
							pResult[i] = '\0';
							DoLine();
							pLine = &pResult[i + 1];
						}
					}
				}
			}
			return true;
		});
	}
	if(g_Config.m_ClWebhookUrl[0] == '\0')
		return;
	m_TimeSinceLastRequest += Client()->RenderFrameTime();
	m_TimeSinceBufferStart += Client()->RenderFrameTime();
	if(m_TimeSinceLastRequest >= (Client()->RconAuthed() ? BUFFER_MIN_TIME_AUTHED : BUFFER_MIN_TIME))
		FlushBuffer();
	if(m_TimeSinceBufferStart >= BUFFER_MAX_TIME)
		FlushBuffer();
}

void CWebhook::FlushBuffer()
{
	m_TimeSinceLastRequest = 0.0f;
	m_TimeSinceBufferStart = 0.0f;
	if(m_Buffer.empty())
		return;
	if(m_vpHttpRequests.size() > BUFFER_MAX_TIME / BUFFER_MIN_TIME)
	{
		log_error("webhook", "Server is overloaded with too many requests");
		return;
	}
	auto pGet = std::make_shared<CHttpRequest>(g_Config.m_ClWebhookUrl);
	pGet->LogProgress(HTTPLOG::FAILURE);
	pGet->FailOnErrorStatus(true);
	pGet->Timeout(CTimeout{1000, 0, 500, 10});
	pGet->Post((const unsigned char *)m_Buffer.c_str(), m_Buffer.size());
	pGet->MaxResponseSize(BUFFER_MAX_SIZE);
	Http()->Run(pGet);
	m_vpHttpRequests.emplace_back(pGet);
	m_Buffer.clear();
}
