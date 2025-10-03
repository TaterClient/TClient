#include "player_indicator.h"

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>

static vec2 NormalizedDirection(vec2 Src, vec2 Dst)
{
	return normalize(vec2(Dst.x - Src.x, Dst.y - Src.y));
}

static float DistanceBetweenTwoPoints(vec2 Src, vec2 Dst)
{
	return std::sqrt(std::pow(Dst.x - Src.x, 2.0f) + std::pow(Dst.y - Src.y, 2.0f));
}

void CPlayerIndicator::OnRender()
{
	// Don't render if we can't find our own tee
	if(GameClient()->m_Snap.m_LocalClientId == -1 || !GameClient()->m_Snap.m_aCharacters[GameClient()->m_Snap.m_LocalClientId].m_Active)
		return;

	// Don't render if not race gamemode or in demo
	if(!GameClient()->m_GameInfo.m_Race || Client()->State() == IClient::STATE_DEMOPLAYBACK || !GameClient()->m_Camera.ZoomAllowed())
		return;

	vec2 Position = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderPos;

	if(g_Config.m_TcPlayerIndicator != 1)
		return;

	Graphics()->TextureClear();
	ColorRGBA Col = ColorRGBA(0.0f, 0.0f, 0.0f, 1.0f);
	if(!(GameClient()->m_Teams.Team(GameClient()->m_Snap.m_LocalClientId) == 0 && g_Config.m_TcIndicatorTeamOnly))
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(!GameClient()->m_Snap.m_apPlayerInfos[i] || i == GameClient()->m_Snap.m_LocalClientId)
				continue;

			CGameClient::CClientData OtherTee = GameClient()->m_aClients[i];
			CCharacterCore *pOtherCharacter = &GameClient()->m_aClients[i].m_Predicted;
			if(
				OtherTee.m_Team == GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_Team &&
				!OtherTee.m_Spec &&
				GameClient()->m_Snap.m_aCharacters[i].m_Active)
			{
				if(g_Config.m_TcPlayerIndicatorFreeze && !(OtherTee.m_FreezeEnd > 0 || OtherTee.m_DeepFrozen))
					continue;

				// Hide tees on our screen if the config is set to do so
				float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
				Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
				if(g_Config.m_TcIndicatorHideVisible && in_range(GameClient()->m_aClients[i].m_RenderPos.x, ScreenX0, ScreenX1) && in_range(GameClient()->m_aClients[i].m_RenderPos.y, ScreenY0, ScreenY1))
					continue;

				vec2 Norm = NormalizedDirection(GameClient()->m_aClients[i].m_RenderPos, GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_RenderPos) * (-1);

				float Offset = g_Config.m_TcIndicatorOffset;
				if(g_Config.m_TcIndicatorVariableDistance)
				{
					Offset = mix((float)g_Config.m_TcIndicatorOffset, (float)g_Config.m_TcIndicatorOffsetMax,
						std::min(DistanceBetweenTwoPoints(Position, OtherTee.m_RenderPos) / g_Config.m_TcIndicatorMaxDistance, 1.0f));
				}

				vec2 IndicatorPos(Norm.x * Offset + Position.x, Norm.y * Offset + Position.y);
				CTeeRenderInfo TeeInfo = OtherTee.m_RenderInfo;
				float Alpha = g_Config.m_TcIndicatorOpacity / 100.0f;
				if(OtherTee.m_FreezeEnd > 0 || OtherTee.m_DeepFrozen)
				{
					// check if player is frozen or is getting saved
					if(pOtherCharacter->m_IsInFreeze == 0)
					{
						// player is on the way to get free again
						Col = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_TcIndicatorSaved));
					}
					else
					{
						// player is frozen
						Col = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_TcIndicatorFreeze));
					}
					if(g_Config.m_TcIndicatorTees)
					{
						TeeInfo.m_ColorBody.r *= 0.4f;
						TeeInfo.m_ColorBody.g *= 0.4f;
						TeeInfo.m_ColorBody.b *= 0.4f;
						TeeInfo.m_ColorFeet.r *= 0.4f;
						TeeInfo.m_ColorFeet.g *= 0.4f;
						TeeInfo.m_ColorFeet.b *= 0.4f;
						Alpha *= 0.8f;
					}
				}
				else
				{
					Col = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_TcIndicatorAlive));
				}
				bool HideIfNotWar = false;
				ColorRGBA PrevCol = Col;
				if(g_Config.m_TcWarListIndicator)
				{
					HideIfNotWar = true;
					if(g_Config.m_TcWarListIndicatorAll)
					{
						if(GameClient()->m_WarList.GetAnyWar(i))
						{
							Col = GameClient()->m_WarList.GetPriorityColor(i);
							HideIfNotWar = false;
						}
					}
					if(g_Config.m_TcWarListIndicatorTeam)
					{
						if(GameClient()->m_WarList.GetWarData(i).m_WarGroupMatches[2])
						{
							Col = GameClient()->m_WarList.m_WarTypes[2]->m_Color;
							HideIfNotWar = false;
						}
					}
					if(g_Config.m_TcWarListIndicatorEnemy)
					{
						if(GameClient()->m_WarList.GetWarData(i).m_WarGroupMatches[1])
						{
							Col = GameClient()->m_WarList.m_WarTypes[1]->m_Color;
							HideIfNotWar = false;
						}
					}
				}

				if(HideIfNotWar)
					continue;
				if(!g_Config.m_TcWarListIndicatorColors)
					Col = PrevCol;

				Col.a = Alpha;

				TeeInfo.m_Size = g_Config.m_TcIndicatorRadius * 4.0f;

				if(g_Config.m_TcIndicatorTees)
				{
					RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, OtherTee.m_RenderCur.m_Emote, vec2(1.0f, 0.0f), IndicatorPos, Col.a);
				}
				else
				{
					Graphics()->QuadsBegin();
					Graphics()->SetColor(Col);
					Graphics()->DrawCircle(IndicatorPos.x, IndicatorPos.y, g_Config.m_TcIndicatorRadius, 16);
					Graphics()->QuadsEnd();
				}
			}
		}
	}

	// reset texture color
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}
