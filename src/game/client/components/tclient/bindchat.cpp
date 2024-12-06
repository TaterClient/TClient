#include <engine/shared/config.h>
#include <game/client/gameclient.h>

#include "../chat.h"
#include "../emoticon.h"

#include "bindchat.h"

CBindChat::CBindChat()
{
	OnReset();
}

void CBindChat::ConAddBindchat(IConsole::IResult *pResult, void *pUserData)
{
	const char *aName = pResult->GetString(0);
	const char *aCommand = pResult->GetString(1);

	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	pThis->AddBind(aName, aCommand);
}

void CBindChat::ConRemoveBindchat(IConsole::IResult *pResult, void *pUserData)
{
	const char *aName = pResult->GetString(0);
	const char *aCommand = pResult->GetString(1);

	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	pThis->RemoveBind(aName, aCommand);
}

void CBindChat::ConRemoveAllBindchatBinds(IConsole::IResult *pResult, void *pUserData)
{
	CBindChat *pThis = static_cast<CBindChat *>(pUserData);
	pThis->RemoveAllBinds();
}

void CBindChat::AddBind(const char *pName, const char *pCommand)
{
	if((pName[0] == '\0' && pCommand[0] == '\0') || m_vBinds.size() >= BINDCHAT_MAX_BINDS)
		return;

	CBind Bind;
	str_copy(Bind.m_aName, pName);
	str_copy(Bind.m_aCommand, pCommand);
	m_vBinds.push_back(Bind);
}

void CBindChat::RemoveBind(const char *pName, const char *pCommand)
{
	CBind Bind;
	str_copy(Bind.m_aName, pName);
	str_copy(Bind.m_aCommand, pCommand);
	auto it = std::find(m_vBinds.begin(), m_vBinds.end(), Bind);
	if(it != m_vBinds.end())
		m_vBinds.erase(it);
}

void CBindChat::RemoveBind(int Index)
{
	if(Index >= static_cast<int>(m_vBinds.size()) || Index < 0)
		return;
	auto Pos = m_vBinds.begin() + Index;
	m_vBinds.erase(Pos);
}

void CBindChat::RemoveAllBinds()
{
	m_vBinds.clear();
}

void CBindChat::OnConsoleInit()
{
	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterTCallback(ConfigSaveCallback, this);

	Console()->Register("add_bindchat", "s[namfe] r[command]", CFGFLAG_CLIENT, ConAddBindchat, this, "Add a bind to the bindchat");
	Console()->Register("remove_bindchat", "s[name] r[command]", CFGFLAG_CLIENT, ConRemoveBindchat, this, "Remove a bind from the bindchat");
	Console()->Register("delete_all_bindchat_binds", "", CFGFLAG_CLIENT, ConRemoveAllBindchatBinds, this, "Removes all bindchat binds");
}

void CBindChat::ExecuteBind(int Bind, const char *pArgs)
{
	char aBuf[BINDCHAT_MAX_CMD] = "";
	str_append(aBuf, m_vBinds[Bind].m_aCommand);
	if(pArgs)
	{
		str_append(aBuf, " ");
		str_append(aBuf, pArgs);
	}
	Console()->ExecuteLine(aBuf);
}

bool CBindChat::ChatDoBinds(const char *pText)
{
	CChat &Chat = GameClient()->m_Chat;
	const char *pSpace = str_find(pText, " ");
	size_t SpaceIndex = pSpace ? pSpace - pText : strlen(pText);
	for (const CBind &Bind : m_vBinds)
	{
		if(str_comp_nocase_num(pText, Bind.m_aName, SpaceIndex) == 0)
		{
			ExecuteBind(&Bind - &m_vBinds[0], pSpace ? pSpace + 1 : nullptr);
			// Add to history (see CChat::SendChatQueued)
			const int Length = str_length(pText);
			CChat::CHistoryEntry *pEntry = Chat.m_History.Allocate(sizeof(CChat::CHistoryEntry) + Length);
			pEntry->m_Team = 0; // All
			str_copy(pEntry->m_aText, pText, Length + 1);
			return true;
		}
	}
	return false;
}

bool CBindChat::ChatDoAutocomplete(bool ShiftPressed) {
	CChat &Chat = GameClient()->m_Chat;

	if(*Chat.m_aCompletionBuffer == '\0')
		return false;

	const CBind *pCompletionBind = nullptr;

	if(ShiftPressed && Chat.m_CompletionUsed)
		Chat.m_CompletionChosen--;
	else if(!ShiftPressed)
		Chat.m_CompletionChosen++;
	Chat.m_CompletionChosen = (Chat.m_CompletionChosen + m_vBinds.size()) % m_vBinds.size();

	Chat.m_CompletionUsed = true;
	for(const CBind &Bind : m_vBinds)
	{
		if(str_startswith_nocase(Bind.m_aName, Chat.m_aCompletionBuffer))
		{
			pCompletionBind = &Bind;
			Chat.m_CompletionChosen = &Bind - &m_vBinds[0];
			break;
		}
	}

	// insert the command
	if(pCompletionBind)
	{
		char aBuf[CChat::MAX_LINE_LENGTH];
		// add part before the name
		str_truncate(aBuf, sizeof(aBuf), Chat.m_Input.GetString(), Chat.m_PlaceholderOffset);

		// add the command
		str_append(aBuf, pCompletionBind->m_aName);

		// add separator
		// TODO: figure out if the command would accept an extra param
		// char commandBuf[128];
		// str_next_token(pCompletionBind->m_aCommand, " ", commandBuf, sizeof(commandBuf));
		// CCommandInfo *pInfo = m_pClient->Console()->GetCommandInfo(commandBuf, CFGFLAG_CLIENT, false);
		// if(pInfo && pInfo->m_pParams != '\0')
		const char pSeperator[] = " ";
		str_append(aBuf, pSeperator);

		// add part after the name
		str_append(aBuf, Chat.m_Input.GetString() + Chat.m_PlaceholderOffset + Chat.m_PlaceholderLength);

		Chat.m_PlaceholderLength = sizeof(pSeperator) + str_length(pCompletionBind->m_aName) + 1;
		Chat.m_Input.Set(aBuf);
		Chat.m_Input.SetCursorOffset(Chat.m_PlaceholderOffset + Chat.m_PlaceholderLength);
	}

	return pCompletionBind != nullptr;
}

void CBindChat::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CBindChat *pThis = (CBindChat *)pUserData;

	for(CBind &Bind : pThis->m_vBinds)
	{
		char aBuf[BINDCHAT_MAX_CMD * 2] = "";
		char *pEnd = aBuf + sizeof(aBuf);
		char *pDst;
		str_append(aBuf, "add_bindchat \"");
		// Escape name
		pDst = aBuf + str_length(aBuf);
		str_escape(&pDst, Bind.m_aName, pEnd);
		str_append(aBuf, "\" \"");
		// Escape command
		pDst = aBuf + str_length(aBuf);
		str_escape(&pDst, Bind.m_aCommand, pEnd);
		str_append(aBuf, "\"");
		pConfigManager->WriteLine(aBuf);
	}
}
