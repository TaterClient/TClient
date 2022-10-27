/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include "voting.h"
#include <game/client/components/sounds.h>
#include <game/client/render.h>
#include <game/generated/protocol.h>

#include <game/client/gameclient.h>

void CVoting::ConCallvote(IConsole::IResult *pResult, void *pUserData)
{
	CVoting *pSelf = (CVoting *)pUserData;
	pSelf->Callvote(pResult->GetString(0), pResult->GetString(1), pResult->NumArguments() > 2 ? pResult->GetString(2) : "");
}

void CVoting::ConVote(IConsole::IResult *pResult, void *pUserData)
{
	CVoting *pSelf = (CVoting *)pUserData;
	if(str_comp_nocase(pResult->GetString(0), "yes") == 0)
		pSelf->Vote(1);
	else if(str_comp_nocase(pResult->GetString(0), "no") == 0)
		pSelf->Vote(-1);
}

void CVoting::Callvote(const char *pType, const char *pValue, const char *pReason)
{
	CNetMsg_Cl_CallVote Msg = {0};
	Msg.m_pType = pType;
	Msg.m_pValue = pValue;
	Msg.m_pReason = pReason;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

void CVoting::CallvoteSpectate(int ClientID, const char *pReason, bool ForceVote)
{
	if(ForceVote)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "set_team %d -1", ClientID);
		Client()->Rcon(aBuf);
	}
	else
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", ClientID);
		Callvote("spectate", aBuf, pReason);
	}
}

void CVoting::CallvoteKick(int ClientID, const char *pReason, bool ForceVote)
{
	if(ForceVote)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "force_vote kick %d %s", ClientID, pReason);
		Client()->Rcon(aBuf);
	}
	else
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", ClientID);
		Callvote("kick", aBuf, pReason);
	}
}

void CVoting::CallvoteOption(int OptionID, const char *pReason, bool ForceVote)
{
	CVoteOptionClient *pOption = m_pFirst;
	while(pOption && OptionID >= 0)
	{
		if(OptionID == 0)
		{
			if(ForceVote)
			{
				char aBuf[128];
				str_copy(aBuf, "force_vote option \"");
				char *pDst = aBuf + str_length(aBuf);
				str_escape(&pDst, pOption->m_aDescription, aBuf + sizeof(aBuf));
				str_append(aBuf, "\" \"", sizeof(aBuf));
				pDst = aBuf + str_length(aBuf);
				str_escape(&pDst, pReason, aBuf + sizeof(aBuf));
				str_append(aBuf, "\"", sizeof(aBuf));
				Client()->Rcon(aBuf);
			}
			else
				Callvote("option", pOption->m_aDescription, pReason);
			break;
		}

		OptionID--;
		pOption = pOption->m_pNext;
	}
}

void CVoting::RemovevoteOption(int OptionID)
{
	CVoteOptionClient *pOption = m_pFirst;
	while(pOption && OptionID >= 0)
	{
		if(OptionID == 0)
		{
			char aBuf[128];
			str_copy(aBuf, "remove_vote \"");
			char *pDst = aBuf + str_length(aBuf);
			str_escape(&pDst, pOption->m_aDescription, aBuf + sizeof(aBuf));
			str_append(aBuf, "\"", sizeof(aBuf));
			Client()->Rcon(aBuf);
			break;
		}

		OptionID--;
		pOption = pOption->m_pNext;
	}
}

void CVoting::AddvoteOption(const char *pDescription, const char *pCommand)
{
	char aBuf[128];
	str_copy(aBuf, "add_vote \"");
	char *pDst = aBuf + str_length(aBuf);
	str_escape(&pDst, pDescription, aBuf + sizeof(aBuf));
	str_append(aBuf, "\" \"", sizeof(aBuf));
	pDst = aBuf + str_length(aBuf);
	str_escape(&pDst, pCommand, aBuf + sizeof(aBuf));
	str_append(aBuf, "\"", sizeof(aBuf));
	Client()->Rcon(aBuf);
}

void CVoting::Vote(int v)
{
	m_Voted = v;
	CNetMsg_Cl_Vote Msg = {v};
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

CVoting::CVoting()
{
	ClearOptions();

	m_Closetime = 0;
	m_aDescription[0] = 0;
	m_aReason[0] = 0;
	m_Yes = m_No = m_Pass = m_Total = 0;
	m_Voted = 0;
}

void CVoting::AddOption(const char *pDescription)
{
	CVoteOptionClient *pOption;
	if(m_pRecycleFirst)
	{
		pOption = m_pRecycleFirst;
		m_pRecycleFirst = m_pRecycleFirst->m_pNext;
		if(m_pRecycleFirst)
			m_pRecycleFirst->m_pPrev = 0;
		else
			m_pRecycleLast = 0;
	}
	else
		pOption = (CVoteOptionClient *)m_Heap.Allocate(sizeof(CVoteOptionClient));

	pOption->m_pNext = 0;
	pOption->m_pPrev = m_pLast;
	if(pOption->m_pPrev)
		pOption->m_pPrev->m_pNext = pOption;
	m_pLast = pOption;
	if(!m_pFirst)
		m_pFirst = pOption;

	str_copy(pOption->m_aDescription, pDescription);
	++m_NumVoteOptions;
}

void CVoting::ClearOptions()
{
	m_Heap.Reset();

	m_NumVoteOptions = 0;
	m_pFirst = 0;
	m_pLast = 0;

	m_pRecycleFirst = 0;
	m_pRecycleLast = 0;
}

void CVoting::OnReset()
{
	m_Closetime = 0;
	m_aDescription[0] = 0;
	m_aReason[0] = 0;
	m_Yes = m_No = m_Pass = m_Total = 0;
	m_Voted = 0;
}

void CVoting::OnConsoleInit()
{
	Console()->Register("callvote", "s[command] s[id] ?r[reason]", CFGFLAG_CLIENT, ConCallvote, this, "Call vote");
	Console()->Register("vote", "r['yes'|'no']", CFGFLAG_CLIENT, ConVote, this, "Vote yes/no");
}

void CVoting::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_VOTESET)
	{
		CNetMsg_Sv_VoteSet *pMsg = (CNetMsg_Sv_VoteSet *)pRawMsg;
		OnReset();
		if(pMsg->m_Timeout)
		{
			str_copy(m_aDescription, pMsg->m_pDescription);
			str_copy(m_aReason, pMsg->m_pReason);
			m_Closetime = time() + time_freq() * pMsg->m_Timeout;

			if(Client()->RconAuthed())
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s (%s)", m_aDescription, m_aReason);
				Client()->Notify("DDNet Vote", aBuf);
				m_pClient->m_Sounds.Play(CSounds::CHN_GUI, SOUND_CHAT_HIGHLIGHT, 0);
			}
		}
	}
	else if(MsgType == NETMSGTYPE_SV_VOTESTATUS)
	{
		CNetMsg_Sv_VoteStatus *pMsg = (CNetMsg_Sv_VoteStatus *)pRawMsg;
		m_Yes = pMsg->m_Yes;
		m_No = pMsg->m_No;
		m_Pass = pMsg->m_Pass;
		m_Total = pMsg->m_Total;
	}
	else if(MsgType == NETMSGTYPE_SV_VOTECLEAROPTIONS)
	{
		ClearOptions();
	}
	else if(MsgType == NETMSGTYPE_SV_VOTEOPTIONLISTADD)
	{
		CNetMsg_Sv_VoteOptionListAdd *pMsg = (CNetMsg_Sv_VoteOptionListAdd *)pRawMsg;
		int NumOptions = pMsg->m_NumOptions;
		for(int i = 0; i < NumOptions; ++i)
		{
			switch(i)
			{
			case 0: AddOption(pMsg->m_pDescription0); break;
			case 1: AddOption(pMsg->m_pDescription1); break;
			case 2: AddOption(pMsg->m_pDescription2); break;
			case 3: AddOption(pMsg->m_pDescription3); break;
			case 4: AddOption(pMsg->m_pDescription4); break;
			case 5: AddOption(pMsg->m_pDescription5); break;
			case 6: AddOption(pMsg->m_pDescription6); break;
			case 7: AddOption(pMsg->m_pDescription7); break;
			case 8: AddOption(pMsg->m_pDescription8); break;
			case 9: AddOption(pMsg->m_pDescription9); break;
			case 10: AddOption(pMsg->m_pDescription10); break;
			case 11: AddOption(pMsg->m_pDescription11); break;
			case 12: AddOption(pMsg->m_pDescription12); break;
			case 13: AddOption(pMsg->m_pDescription13); break;
			case 14: AddOption(pMsg->m_pDescription14);
			}
		}
	}
	else if(MsgType == NETMSGTYPE_SV_VOTEOPTIONADD)
	{
		CNetMsg_Sv_VoteOptionAdd *pMsg = (CNetMsg_Sv_VoteOptionAdd *)pRawMsg;
		AddOption(pMsg->m_pDescription);
	}
	else if(MsgType == NETMSGTYPE_SV_VOTEOPTIONREMOVE)
	{
		CNetMsg_Sv_VoteOptionRemove *pMsg = (CNetMsg_Sv_VoteOptionRemove *)pRawMsg;

		for(CVoteOptionClient *pOption = m_pFirst; pOption; pOption = pOption->m_pNext)
		{
			if(str_comp(pOption->m_aDescription, pMsg->m_pDescription) == 0)
			{
				// remove it from the list
				if(m_pFirst == pOption)
					m_pFirst = m_pFirst->m_pNext;
				if(m_pLast == pOption)
					m_pLast = m_pLast->m_pPrev;
				if(pOption->m_pPrev)
					pOption->m_pPrev->m_pNext = pOption->m_pNext;
				if(pOption->m_pNext)
					pOption->m_pNext->m_pPrev = pOption->m_pPrev;
				--m_NumVoteOptions;

				// add it to recycle list
				pOption->m_pNext = 0;
				pOption->m_pPrev = m_pRecycleLast;
				if(pOption->m_pPrev)
					pOption->m_pPrev->m_pNext = pOption;
				m_pRecycleLast = pOption;
				if(!m_pRecycleFirst)
					m_pRecycleLast = pOption;

				break;
			}
		}
	}
}

void CVoting::OnRender()
{
    if(IsVoting())
	{
		bool kickvote = str_startswith(m_aDescription, "Kick ") != 0 ? true : false;
		bool specvote = str_startswith(m_aDescription, "Pause ") != 0 ? true : false;
		bool mapvote = !kickvote && !specvote;

		if(g_Config.m_ClVoteAuto)
		{
			if(m_Voted)
			{
				return;
			}

			// check if timer is up
			if(!(g_Config.m_ClVoteAutoTime == 0))
			{
				if(SecondsLeft() > g_Config.m_ClVoteAutoTime)
				{
					return;
				}
			}

			if(!mapvote)
			{
				// find the player who got voted
				CGameClient::CClientData *player = nullptr;
				const char *p, *t;
				const char *pLookFor = "'";
				if((p = str_find(m_aDescription, pLookFor)))
				{
					char aName[MAX_NAME_LENGTH];
					p += str_length(pLookFor);
					t = str_rchr(p, '\'');

					if(t <= p)
						return;
					str_utf8_truncate(aName, sizeof(aName), p, t - p);

					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						if(!m_pClient->m_aStats[i].IsActive())
							continue;

						if(str_comp(m_pClient->m_aClients[i].m_aName, aName) == 0)
						{
							player = &m_pClient->m_aClients[i];
						}
					}
				}

				// check if player exists
				if(!player)
					return;

				// check if player is a friend
				bool isFriend = player->m_Friend;
				bool SameTeam = m_pClient->m_Teams.Team(m_pClient->m_Snap.m_LocalClientID) == player->m_Team;

				// vote if friend is checked and player is a friend
				if(!m_Voted && g_Config.m_ClVoteAutoFriends && isFriend)
				{
					int voice = g_Config.m_ClVoteAutoFriends == 1 ? 1 : -1;
					Vote(voice);
					return;
				}

				// vote if same team is checked and player is on the same team
				if(!m_Voted && g_Config.m_ClVoteAutoTeam && SameTeam)
				{
					int voice = g_Config.m_ClVoteAutoTeam == 1 ? 1 : -1;
					Vote(voice);
					return;
				}
			}

			// its not a player vote.. here is only default vote options

			// vote default if no other vote was made and default is checked
			if(!m_Voted && g_Config.m_ClVoteAutoDefault)
			{
				dbg_msg("voting", "voting default");
				int voice = g_Config.m_ClVoteAutoDefault == 1 ? 1 : -1;
				Vote(voice);
				return;
			}
		}
	}
}

void CVoting::RenderBars(CUIRect Bars, bool Text)
{
	Bars.Draw(ColorRGBA(0.8f, 0.8f, 0.8f, 0.5f), IGraphics::CORNER_ALL, Bars.h / 3);

	CUIRect Splitter = Bars;
	Splitter.x = Splitter.x + Splitter.w / 2;
	Splitter.w = Splitter.h / 2.0f;
	Splitter.x -= Splitter.w / 2;
	Splitter.Draw(ColorRGBA(0.4f, 0.4f, 0.4f, 0.5f), IGraphics::CORNER_ALL, Splitter.h / 4);

	if(m_Total)
	{
		CUIRect PassArea = Bars;
		if(m_Yes)
		{
			CUIRect YesArea = Bars;
			YesArea.w *= m_Yes / (float)m_Total;
			YesArea.Draw(ColorRGBA(0.2f, 0.9f, 0.2f, 0.85f), IGraphics::CORNER_ALL, Bars.h / 3);

			if(Text)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "%d", m_Yes);
				UI()->DoLabel(&YesArea, aBuf, Bars.h * 0.75f, TEXTALIGN_CENTER);
			}

			PassArea.x += YesArea.w;
			PassArea.w -= YesArea.w;
		}

		if(m_No)
		{
			CUIRect NoArea = Bars;
			NoArea.w *= m_No / (float)m_Total;
			NoArea.x = (Bars.x + Bars.w) - NoArea.w;
			NoArea.Draw(ColorRGBA(0.9f, 0.2f, 0.2f, 0.85f), IGraphics::CORNER_ALL, Bars.h / 3);

			if(Text)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "%d", m_No);
				UI()->DoLabel(&NoArea, aBuf, Bars.h * 0.75f, TEXTALIGN_CENTER);
			}

			PassArea.w -= NoArea.w;
		}

		if(Text && m_Pass)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "%d", m_Pass);
			UI()->DoLabel(&PassArea, aBuf, Bars.h * 0.75f, TEXTALIGN_CENTER);
		}
	}
}
