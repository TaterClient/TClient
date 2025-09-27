#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>

#include "trails.h"

bool CTrails::ShouldPredictPlayer(int ClientId)
{
	if(!GameClient()->Predict())
		return false;
	CCharacter *pChar = GameClient()->m_PredictedWorld.GetCharacterById(ClientId);
	if(GameClient()->Predict() && (ClientId == GameClient()->m_Snap.m_LocalClientId || (GameClient()->AntiPingPlayers() && !GameClient()->IsOtherTeam(ClientId))) && pChar)
		return true;
	return false;
}

void CTrails::ClearAllHistory()
{
	for(int i = 0; i < MAX_CLIENTS; ++i)
		ClearHistory(i);
}
void CTrails::ClearHistory(int ClientId)
{
	for(int i = 0; i < 200; ++i)
		m_History[ClientId][i] = {{}, -1};
	m_HistoryValid[ClientId] = false;
}
void CTrails::OnReset()
{
	ClearAllHistory();
}

void CTrails::OnRender()
{
	if(!g_Config.m_TcTeeTrail)
		return;

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	if(!GameClient()->m_Snap.m_pGameInfoObj)
		return;

	Graphics()->TextureClear();

	for(int ClientId = 0; ClientId < MAX_CLIENTS; ClientId++)
	{
		const bool Local = GameClient()->m_Snap.m_LocalClientId == ClientId;

		const bool ZoomAllowed = GameClient()->m_Camera.ZoomAllowed();
		if(!g_Config.m_TcTeeTrailOthers && !Local)
			continue;

		if(!Local && !ZoomAllowed)
			continue;

		if(!GameClient()->m_Snap.m_aCharacters[ClientId].m_Active)
		{
			if(m_HistoryValid[ClientId])
				ClearHistory(ClientId);
			continue;
		}
		else
			m_HistoryValid[ClientId] = true;

		CTeeRenderInfo TeeInfo = GameClient()->m_aClients[ClientId].m_RenderInfo;

		const bool PredictPlayer = ShouldPredictPlayer(ClientId);
		int StartTick;
		const int GameTick = Client()->GameTick(g_Config.m_ClDummy);
		const int PredTick = Client()->PredGameTick(g_Config.m_ClDummy);
		float IntraTick;
		if(PredictPlayer)
		{
			StartTick = PredTick;
			IntraTick = Client()->PredIntraGameTick(g_Config.m_ClDummy);
			if(g_Config.m_TcRemoveAnti)
			{
				StartTick = GameClient()->m_SmoothTick[g_Config.m_ClDummy];
				IntraTick = GameClient()->m_SmoothIntraTick[g_Config.m_ClDummy];
			}
			if(g_Config.m_TcUnpredOthersInFreeze && !Local && Client()->m_IsLocalFrozen)
			{
				StartTick = GameTick;
			}
		}
		else
		{
			StartTick = GameTick;
			IntraTick = Client()->IntraGameTick(g_Config.m_ClDummy);
		}

		const vec2 CurServerPos = vec2(GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur.m_X, GameClient()->m_Snap.m_aCharacters[ClientId].m_Cur.m_Y);
		const vec2 PrevServerPos = vec2(GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev.m_X, GameClient()->m_Snap.m_aCharacters[ClientId].m_Prev.m_Y);
		m_History[ClientId][GameTick % 200] = {
			mix(PrevServerPos, CurServerPos, IntraTick),
			GameTick,
		};

		// // NOTE: this is kind of a hack to fix 25tps. This fixes flickering when using the speed mode
		// m_History[ClientId][(GameTick + 1) % 200] = m_History[ClientId][GameTick % 200];
		// m_History[ClientId][(GameTick + 2) % 200] = m_History[ClientId][GameTick % 200];

		IGraphics::CLineItem LineItem;
		bool LineMode = g_Config.m_TcTeeTrailWidth == 0;

		float Alpha = g_Config.m_TcTeeTrailAlpha / 100.0f;
		// Taken from players.cpp
		if(ClientId == -2)
			Alpha *= g_Config.m_ClRaceGhostAlpha / 100.0f;
		else if(ClientId < 0 || GameClient()->IsOtherTeam(ClientId))
			Alpha *= g_Config.m_ClShowOthersAlpha / 100.0f;

		int TrailLength = g_Config.m_TcTeeTrailLength;
		float Width = g_Config.m_TcTeeTrailWidth;

		static std::vector<CTrailPart> s_Trail;
		s_Trail.clear();

		// TODO: figure out why this is required
		if(!PredictPlayer)
			TrailLength += 2;
		bool TrailFull = false;
		// Fill trail list with initial positions
		for(int i = 0; i < TrailLength; i++)
		{
			CTrailPart Part;
			int PosTick = StartTick - i;
			if(PredictPlayer)
			{
				if(GameClient()->m_aClients[ClientId].m_aPredTick[PosTick % 200] != PosTick)
					continue;
				Part.m_Pos = GameClient()->m_aClients[ClientId].m_aPredPos[PosTick % 200];
				if(i == TrailLength - 1)
					TrailFull = true;
			}
			else
			{
				if(m_History[ClientId][PosTick % 200].m_Tick != PosTick)
					continue;
				Part.m_Pos = m_History[ClientId][PosTick % 200].m_Pos;
				if(i == TrailLength - 2 || i == TrailLength - 3)
					TrailFull = true;
			}
			Part.m_UnmovedPos = Part.m_Pos;
			Part.m_Tick = PosTick;
			s_Trail.push_back(Part);
		}

		// Trim the ends if intratick is too big
		// this was not trivial to figure out
		int TrimTicks = (int)IntraTick;
		for(int i = 0; i < TrimTicks; i++)
			if((int)s_Trail.size() > 0)
				s_Trail.pop_back();

		// Stuff breaks if we have less than 3 points because we cannot calculate an angle between segments to preserve constant width
		// TODO: Pad the list with generated entries in the same direction as before
		if((int)s_Trail.size() < 3)
			continue;

		if(PredictPlayer)
			s_Trail.at(0).m_Pos = GameClient()->m_aClients[ClientId].m_RenderPos;
		else
			s_Trail.at(0).m_Pos = mix(PrevServerPos, CurServerPos, IntraTick);

		if(TrailFull)
			s_Trail.at(s_Trail.size() - 1).m_Pos = mix(s_Trail.at(s_Trail.size() - 1).m_Pos, s_Trail.at(s_Trail.size() - 2).m_Pos, std::fmod(IntraTick, 1.0f));

		// Set progress
		for(int i = 0; i < (int)s_Trail.size(); i++)
		{
			float Size = float(s_Trail.size() - 1 + TrimTicks);
			CTrailPart &Part = s_Trail.at(i);
			if(i == 0)
				Part.m_Progress = 0.0f;
			else if(i == (int)s_Trail.size() - 1)
				Part.m_Progress = 1.0f;
			else
				Part.m_Progress = ((float)i + IntraTick - 1.0f) / (Size - 1.0f);

			switch(g_Config.m_TcTeeTrailColorMode)
			{
			case COLORMODE_SOLID:
				Part.m_Col = color_cast<ColorRGBA>(ColorHSLA(g_Config.m_TcTeeTrailColor));
				break;
			case COLORMODE_TEE:
				if(TeeInfo.m_CustomColoredSkin)
					Part.m_Col = TeeInfo.m_ColorBody;
				else
					Part.m_Col = TeeInfo.m_BloodColor;
				break;
			case COLORMODE_RAINBOW:
			{
				float Cycle = (1.0f / TrailLength) * 0.5f;
				float Hue = std::fmod(((Part.m_Tick + 6361 * ClientId) % 1000000) * Cycle, 1.0f);
				Part.m_Col = color_cast<ColorRGBA>(ColorHSLA(Hue, 1.0f, 0.5f));
				break;
			}
			case COLORMODE_SPEED:
			{
				float Speed = 0.0f;
				if(s_Trail.size() > 3)
				{
					if(i < 2)
						Speed = distance(s_Trail.at(i + 2).m_UnmovedPos, Part.m_UnmovedPos) / std::abs(s_Trail.at(i + 2).m_Tick - Part.m_Tick);
					else
						Speed = distance(Part.m_UnmovedPos, s_Trail.at(i - 2).m_UnmovedPos) / std::abs(Part.m_Tick - s_Trail.at(i - 2).m_Tick);
				}
				Part.m_Col = color_cast<ColorRGBA>(ColorHSLA(65280 * ((int)(Speed * Speed / 12.5f) + 1)).UnclampLighting(ColorHSLA::DARKEST_LGT));
				break;
			}
			default:
				dbg_assert(false, "Invalid value for g_Config.m_TcTeeTrailColorMode");
				dbg_break();
			}

			Part.m_Col.a = Alpha;
			if(g_Config.m_TcTeeTrailFade)
				Part.m_Col.a *= 1.0 - Part.m_Progress;

			Part.m_Width = Width;
			if(g_Config.m_TcTeeTrailTaper)
				Part.m_Width = Width * (1.0 - Part.m_Progress);
		}

		// Remove duplicate elements (those with same Pos)
		auto NewEnd = std::unique(s_Trail.begin(), s_Trail.end());
		s_Trail.erase(NewEnd, s_Trail.end());

		if((int)s_Trail.size() < 3)
			continue;

		// Calculate the widths
		for(int i = 0; i < (int)s_Trail.size(); i++)
		{
			CTrailPart &Part = s_Trail.at(i);
			vec2 PrevPos;
			vec2 Pos = s_Trail.at(i).m_Pos;
			vec2 NextPos;

			if(i == 0)
			{
				vec2 Direction = normalize(s_Trail.at(i + 1).m_Pos - Pos);
				PrevPos = Pos - Direction;
			}
			else
				PrevPos = s_Trail.at(i - 1).m_Pos;

			if(i == (int)s_Trail.size() - 1)
			{
				vec2 Direction = normalize(Pos - s_Trail.at(i - 1).m_Pos);
				NextPos = Pos + Direction;
			}
			else
				NextPos = s_Trail.at(i + 1).m_Pos;

			vec2 NextDirection = normalize(NextPos - Pos);
			vec2 PrevDirection = normalize(Pos - PrevPos);

			vec2 Normal = vec2(-PrevDirection.y, PrevDirection.x);
			Part.m_Normal = Normal;
			vec2 Tanget = normalize(NextDirection + PrevDirection);
			if(Tanget == vec2(0.0f, 0.0f))
				Tanget = Normal;

			vec2 PerpVec = vec2(-Tanget.y, Tanget.x);
			Width = Part.m_Width;
			float ScaledWidth = Width / dot(Normal, PerpVec);
			float TopScaled = ScaledWidth;
			float BotScaled = ScaledWidth;
			if(dot(PrevDirection, Tanget) > 0.0f)
				TopScaled = std::min(Width * 3.0f, TopScaled);
			else
				BotScaled = std::min(Width * 3.0f, BotScaled);

			vec2 Top = Pos + PerpVec * TopScaled;
			vec2 Bot = Pos - PerpVec * BotScaled;
			Part.m_Top = Top;
			Part.m_Bot = Bot;

			// Bevel Cap
			if(dot(PrevDirection, NextDirection) < -0.25f)
			{
				Top = Pos + Tanget * Width;
				Bot = Pos - Tanget * Width;

				float Det = PrevDirection.x * NextDirection.y - PrevDirection.y * NextDirection.x;
				if(Det >= 0.0f)
				{
					Part.m_Top = Top;
					Part.m_Bot = Bot;
					if(i > 0)
						s_Trail.at(i).m_Flip = true;
				}
				else // <-Left Direction
				{
					Part.m_Top = Bot;
					Part.m_Bot = Top;
					if(i > 0)
						s_Trail.at(i).m_Flip = true;
				}
			}
		}

		if(LineMode)
			Graphics()->LinesBegin();
		else
			Graphics()->QuadsBegin();

		// Draw the trail
		for(int i = 0; i < (int)s_Trail.size() - 1; i++)
		{
			const CTrailPart &Part = s_Trail.at(i);
			const CTrailPart &NextPart = s_Trail.at(i + 1);
			if(distance(Part.m_Pos, NextPart.m_Pos) > 120.0f)
				continue;

			if(LineMode)
			{
				Graphics()->SetColor(Part.m_Col);
				LineItem = IGraphics::CLineItem(Part.m_Pos.x, Part.m_Pos.y, NextPart.m_Pos.x, NextPart.m_Pos.y);
				Graphics()->LinesDraw(&LineItem, 1);
			}
			else
			{
				vec2 Top, Bot;
				if(Part.m_Flip)
				{
					Top = Part.m_Bot;
					Bot = Part.m_Top;
				}
				else
				{
					Top = Part.m_Top;
					Bot = Part.m_Bot;
				}

				Graphics()->SetColor4(NextPart.m_Col, NextPart.m_Col, Part.m_Col, Part.m_Col);
				// IGraphics::CFreeformItem FreeformItem(Top, Bot, NextPart.m_Top, NextPart.m_Bot);
				IGraphics::CFreeformItem FreeformItem(NextPart.m_Top, NextPart.m_Bot, Top, Bot);

				Graphics()->QuadsDrawFreeform(&FreeformItem, 1);
			}
		}
		if(LineMode)
			Graphics()->LinesEnd();
		else
			Graphics()->QuadsEnd();
	}
}
