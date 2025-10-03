/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "emoticon.h"

#include "chat.h"

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

CEmoticon::CEmoticon()
{
	OnReset();
}

void CEmoticon::ConKeyEmoticon(IConsole::IResult *pResult, void *pUserData)
{
	CEmoticon *pSelf = (CEmoticon *)pUserData;
	if(!pSelf->GameClient()->m_Snap.m_SpecInfo.m_Active && pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(pSelf->GameClient()->m_BindWheel.IsActive())
			pSelf->m_Active = false;
		else
			pSelf->m_Active = pResult->GetInteger(0) != 0;
	}
}

void CEmoticon::ConEmote(IConsole::IResult *pResult, void *pUserData)
{
	((CEmoticon *)pUserData)->Emote(pResult->GetInteger(0));
}

void CEmoticon::OnConsoleInit()
{
	Console()->Register("+emote", "", CFGFLAG_CLIENT, ConKeyEmoticon, this, "Open emote selector");
	Console()->Register("emote", "i[emote-id]", CFGFLAG_CLIENT, ConEmote, this, "Use emote");
}

void CEmoticon::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedEmote = -1;
	m_SelectedEyeEmote = -1;
	m_TouchPressedOutside = false;
}

void CEmoticon::OnRelease()
{
	m_Active = false;
}

bool CEmoticon::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
	if(!m_Active)
		return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	m_SelectorMouse += vec2(x, y);
	return true;
}

bool CEmoticon::OnInput(const IInput::CEvent &Event)
{
	if(IsActive() && Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE)
	{
		OnRelease();
		return true;
	}
	return false;
}

void CEmoticon::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	static const auto QuadEaseInOut = [](float t) -> float {
		if(t == 0.0f)
			return 0.0f;
		if(t == 1.0f)
			return 1.0f;
		return (t < 0.5f) ? (2.0f * t * t) : (1.0f - std::pow(-2.0f * t + 2.0f, 2) / 2.0f);
	};
	static const auto PositiveMod = [](float x, float y) -> float {
		return std::fmod(x + y, y);
	};

	static const float s_InnerMouseLimitRadius = 40.0f;
	static const float s_InnerOuterMouseBoundaryRadius = 110.0f;
	static const float s_OuterMouseLimitRadius = 170.0f;
	static const float s_InnerItemRadius = 70.0f;
	static const float s_OuterItemRadius = 150.0f;
	static const float s_InnerCircleRadius = 100.0f;
	static const float s_OuterCircleRadius = 190.0f;

	const float AnimationTime = (float)g_Config.m_TcAnimateWheelTime / 1000.0f;
	const float ItemAnimationTime = AnimationTime / 2.0f;

	if(AnimationTime != 0.0f)
	{
		for(float &Time : m_aAnimationTimeEmotes)
			Time = std::max(0.0f, Time - Client()->RenderFrameTime());
		for(float &Time : m_aAnimationTimeEyeEmotes)
			Time = std::max(0.0f, Time - Client()->RenderFrameTime());
	}

	if(!m_Active)
	{
		if(m_TouchPressedOutside)
		{
			m_SelectedEmote = -1;
			m_SelectedEyeEmote = -1;
			m_TouchPressedOutside = false;
		}

		if(m_WasActive && m_SelectedEmote != -1)
			Emote(m_SelectedEmote);
		if(m_WasActive && m_SelectedEyeEmote != -1)
			EyeEmote(m_SelectedEyeEmote);
		m_WasActive = false;

		if(AnimationTime == 0.0f)
			return;

		m_AnimationTime -= Client()->RenderFrameTime() * 3.0f; // Close animation 3x faster
		if(m_AnimationTime <= 0.0f)
		{
			m_AnimationTime = 0.0f;
			return;
		}
	}
	else
	{
		if(AnimationTime != 0.0f)
		{
			m_AnimationTime += Client()->RenderFrameTime();
			if(m_AnimationTime > AnimationTime)
				m_AnimationTime = AnimationTime;
		}
		m_WasActive = true;
	}

	if(GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
	}

	const CUIRect Screen = *Ui()->Screen();

	const bool WasTouchPressed = m_TouchState.m_AnyPressed;
	Ui()->UpdateTouchState(m_TouchState);
	if(m_TouchState.m_AnyPressed)
	{
		const vec2 TouchPos = (m_TouchState.m_PrimaryPosition - vec2(0.5f, 0.5f)) * Screen.Size();
		const float TouchCenterDistance = length(TouchPos);
		if(TouchCenterDistance <= s_OuterMouseLimitRadius)
		{
			m_SelectorMouse = TouchPos;
		}
		else if(TouchCenterDistance > s_OuterCircleRadius)
		{
			m_TouchPressedOutside = true;
		}
	}
	else if(WasTouchPressed)
	{
		m_Active = false;
	}

	std::array<float, 5> aAnimationPhase;
	if(AnimationTime == 0.0f)
	{
		aAnimationPhase.fill(1.0f);
	}
	else
	{
		aAnimationPhase[0] = QuadEaseInOut(m_AnimationTime / AnimationTime);
		aAnimationPhase[1] = aAnimationPhase[0] * aAnimationPhase[0];
		aAnimationPhase[2] = aAnimationPhase[1] * aAnimationPhase[1];
		aAnimationPhase[3] = aAnimationPhase[2] * aAnimationPhase[2];
		aAnimationPhase[4] = aAnimationPhase[3] * aAnimationPhase[3];
	}

	if(length(m_SelectorMouse) > s_OuterMouseLimitRadius)
		m_SelectorMouse = normalize(m_SelectorMouse) * s_OuterMouseLimitRadius;

	const float SelectorAngle = angle(m_SelectorMouse);

	m_SelectedEmote = -1;
	m_SelectedEyeEmote = -1;
	if(length(m_SelectorMouse) > s_InnerOuterMouseBoundaryRadius)
		m_SelectedEmote = PositiveMod(std::round(SelectorAngle / (2.0f * pi) * NUM_EMOTICONS), NUM_EMOTICONS);
	else if(length(m_SelectorMouse) > s_InnerMouseLimitRadius)
		m_SelectedEyeEmote = PositiveMod(std::round(SelectorAngle / (2.0f * pi) * NUM_EMOTES), NUM_EMOTES);

	if(m_SelectedEmote != -1)
	{
		m_aAnimationTimeEmotes[m_SelectedEmote] += Client()->RenderFrameTime() * 2.0f; // To counteract earlier decrement
		if(m_aAnimationTimeEmotes[m_SelectedEmote] >= ItemAnimationTime)
			m_aAnimationTimeEmotes[m_SelectedEmote] = ItemAnimationTime;
	}
	if(m_SelectedEyeEmote != -1)
	{
		m_aAnimationTimeEyeEmotes[m_SelectedEyeEmote] += Client()->RenderFrameTime() * 2.0f; // To counteract earlier decrement
		if(m_aAnimationTimeEyeEmotes[m_SelectedEyeEmote] >= ItemAnimationTime)
			m_aAnimationTimeEyeEmotes[m_SelectedEyeEmote] = ItemAnimationTime;
	}

	const vec2 ScreenCenter = Screen.Center();

	Ui()->MapScreen();

	Graphics()->BlendNormal();

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f * aAnimationPhase[0]);
	Graphics()->DrawCircle(ScreenCenter.x, ScreenCenter.y, s_OuterCircleRadius * aAnimationPhase[0], 64);
	Graphics()->QuadsEnd();

	Graphics()->WrapClamp();
	for(int Emote = 0; Emote < NUM_EMOTICONS; Emote++)
	{
		float Angle = 2.0f * pi * Emote / NUM_EMOTICONS;
		if(Angle > pi)
			Angle -= 2.0f * pi;

		Graphics()->TextureSet(GameClient()->m_EmoticonsSkin.m_aSpriteEmoticons[Emote]);
		Graphics()->QuadsSetSubset(0, 0, 1, 1);
		Graphics()->QuadsBegin();
		const vec2 Nudge = direction(Angle) * s_OuterItemRadius * aAnimationPhase[1];
		const float Phase = ItemAnimationTime == 0.0f ? (Emote == m_SelectedEmote ? 1.0f : 0.0f) : QuadEaseInOut(m_aAnimationTimeEmotes[Emote] / ItemAnimationTime);
		const float Size = (50.0f + Phase * 30.0f) * aAnimationPhase[1];
		IGraphics::CQuadItem QuadItem(ScreenCenter.x + Nudge.x, ScreenCenter.y + Nudge.y, Size * aAnimationPhase[1], Size * aAnimationPhase[1]);
		Graphics()->QuadsDraw(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
	Graphics()->WrapNormal();

	if(GameClient()->m_GameInfo.m_AllowEyeWheel && g_Config.m_ClEyeWheel && GameClient()->m_aLocalIds[g_Config.m_ClDummy] >= 0)
	{
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.3f * aAnimationPhase[2]);
		Graphics()->DrawCircle(ScreenCenter.x, ScreenCenter.y, s_InnerCircleRadius * aAnimationPhase[2], 64);
		Graphics()->QuadsEnd();

		CTeeRenderInfo TeeInfo = GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_RenderInfo;

		for(int Emote = 0; Emote < NUM_EMOTES; Emote++)
		{
			float Angle = 2.0f * pi * Emote / NUM_EMOTES;
			if(Angle > pi)
				Angle -= 2.0f * pi;

			const vec2 Nudge = direction(Angle) * s_InnerItemRadius * aAnimationPhase[3];
			const float Phase = ItemAnimationTime == 0.0f ? (Emote == m_SelectedEyeEmote ? 1.0f : 0.0f) : QuadEaseInOut(m_aAnimationTimeEyeEmotes[Emote] / ItemAnimationTime);
			TeeInfo.m_Size = (48.0f + Phase * 18.0f) * aAnimationPhase[3];
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, Emote, vec2(-1.0f, 0.0f), ScreenCenter + Nudge, aAnimationPhase[3]);
		}

		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f * aAnimationPhase[4]);
		Graphics()->DrawCircle(ScreenCenter.x, ScreenCenter.y, 30.0f * aAnimationPhase[4], 64);
		Graphics()->QuadsEnd();
	}
	else
		m_SelectedEyeEmote = -1;

	RenderTools()->RenderCursor(ScreenCenter + m_SelectorMouse, 24.0f, aAnimationPhase[0]);
}

void CEmoticon::Emote(int Emoticon)
{
	CNetMsg_Cl_Emoticon Msg;
	Msg.m_Emoticon = Emoticon;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);

	if(g_Config.m_ClDummyCopyMoves)
	{
		CMsgPacker MsgDummy(NETMSGTYPE_CL_EMOTICON, false);
		MsgDummy.AddInt(Emoticon);
		Client()->SendMsg(!g_Config.m_ClDummy, &MsgDummy, MSGFLAG_VITAL);
	}
}

void CEmoticon::EyeEmote(int Emote)
{
	char aBuf[32];
	switch(Emote)
	{
	case EMOTE_NORMAL:
		str_format(aBuf, sizeof(aBuf), "/emote normal %d", g_Config.m_ClEyeDuration);
		break;
	case EMOTE_PAIN:
		str_format(aBuf, sizeof(aBuf), "/emote pain %d", g_Config.m_ClEyeDuration);
		break;
	case EMOTE_HAPPY:
		str_format(aBuf, sizeof(aBuf), "/emote happy %d", g_Config.m_ClEyeDuration);
		break;
	case EMOTE_SURPRISE:
		str_format(aBuf, sizeof(aBuf), "/emote surprise %d", g_Config.m_ClEyeDuration);
		break;
	case EMOTE_ANGRY:
		str_format(aBuf, sizeof(aBuf), "/emote angry %d", g_Config.m_ClEyeDuration);
		break;
	case EMOTE_BLINK:
		str_format(aBuf, sizeof(aBuf), "/emote blink %d", g_Config.m_ClEyeDuration);
		break;
	}
	GameClient()->m_Chat.SendChat(0, aBuf);
}
