#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/generated/client_data.h>
#include <game/generated/protocol.h>
#include <game/localization.h>

#include <algorithm>
#include <deque>
#include <vector>

#include <game/client/components/tclient/bg_draw_file.h>

#include "bg_draw.h"

#define MAX_ITEMS_TO_LOAD 65536

static float cross(vec2 A, vec2 B)
{
	return A.x * B.y - A.y * B.x;
}

static bool line_intersects(vec2 A, vec2 B, vec2 C, vec2 D)
{
	const vec2 R = B - A;
	const vec2 S = D - C;
	const vec2 Ac = C - A;
	const float Denom = cross(R, S);
	const float Num1 = cross(Ac, S);
	const float Num2 = cross(Ac, R);
	if(Denom == 0.0f)
		return false;
	float T = Num1 / Denom;
	float U = Num2 / Denom;
	return (T >= 0.0f && T <= 1.0f) && (U >= 0.0f && U <= 1.0f);
}

class CBgDrawItem
{
private:
	CGameClient &m_This;

	int m_Drawing = true;
	int m_QuadContainerIndex = -1;
	int m_QuadCount = 0;

	vec2 m_BoundingBoxMin = vec2(0.0f, 0.0f);
	vec2 m_BoundingBoxMax = vec2(0.0f, 0.0f);
	CBgDrawItemData m_Data;

	void ExtendBoundingBox(vec2 Pos, float Radius)
	{
		const vec2 MinPos = Pos - vec2(Radius, Radius);
		const vec2 MaxPos = Pos + vec2(Radius, Radius);
		if(MinPos.x < m_BoundingBoxMin.x)
			m_BoundingBoxMin.x = MinPos.x;
		if(MinPos.y < m_BoundingBoxMin.y)
			m_BoundingBoxMin.y = MinPos.y;
		if(MaxPos.x > m_BoundingBoxMax.x)
			m_BoundingBoxMax.x = MaxPos.x;
		if(MaxPos.y > m_BoundingBoxMax.y)
			m_BoundingBoxMax.y = MaxPos.y;
	}
	void AddCircle(vec2 Pos, float Angle1, float Angle2, float Width, ColorRGBA Color)
	{
		const float Radius = Width / 2.0f;
		const int Segments = (int)(std::fabs(Angle2 - Angle1) / (pi * 2.0f) * 20.0f);
		const float SegmentAngle = (Angle2 - Angle1) / Segments;
		IGraphics::CFreeformItem FreeformItems[20];
		for(int i = 0; i < Segments; ++i)
		{
			const float A1 = Angle1 + (float)i * SegmentAngle;
			const float A2 = A1 + SegmentAngle;
			const vec2 P1 = Pos + direction(A1) * Radius;
			const vec2 P2 = Pos + direction(A2) * Radius;
			FreeformItems[i] = IGraphics::CFreeformItem(P1.x, P1.y, P2.x, P2.y, Pos.x, Pos.y, Pos.x, Pos.y);
		}
		m_This.Graphics()->SetColor(Color);
		m_This.Graphics()->QuadContainerAddQuads(m_QuadContainerIndex, FreeformItems, Segments);
		m_QuadCount += Segments;
	}
	void AddLine(vec2 Pos, vec2 LastPos, float Width, ColorRGBA Color)
	{
		const float Angle = angle(LastPos - Pos);
		const vec2 Offset = direction(Angle + pi / 2.0f) * (Width / 2.0f);
		const vec2 P1 = LastPos + Offset;
		const vec2 P2 = LastPos - Offset;
		const vec2 P3 = Pos + Offset;
		const vec2 P4 = Pos - Offset;
		IGraphics::CFreeformItem FreeformItem(P1.x, P1.y, P2.x, P2.y, P3.x, P3.y, P4.x, P4.y);
		m_This.Graphics()->SetColor(Color);
		m_This.Graphics()->QuadContainerAddQuads(m_QuadContainerIndex, &FreeformItem, 1);
		m_QuadCount += 1;
	}

	float CurrentWidth() const
	{
		return (float)g_Config.m_ClBgDrawWidth * m_This.m_Camera.m_Zoom;
	}
	ColorRGBA CurrentColor() const
	{
		return color_cast<ColorRGBA>(ColorHSLA(g_Config.m_ClBgDrawColor));
	}

public:
	bool m_Killed = false;
	float m_SecondsAge = 0.0f;

	const CBgDrawItemData &Data() const { return m_Data; }
	const vec2 &BoundingBoxMin() const { return m_BoundingBoxMin; }
	const vec2 &BoundingBoxMax() const { return m_BoundingBoxMax; }

	bool PenUp(const CBgDrawItemDataPoint &Point)
	{
		if(!m_Drawing)
			return false;
		m_Drawing = false;
		const vec2 LastPos = m_Data.empty() ? Point.Pos() : m_Data.back().Pos();
		const float Distance = distance(LastPos, Point.Pos());
		if(Distance < Point.w / 2.0f)
		{
			// Draw only a circle for a tiny segment
			AddCircle(m_Data.size() <= 1 ? Point.Pos() : LastPos, 0.0f, pi * 2.0f, Point.w, Point.Color());
		}
		else
		{
			const float Angle = angle(LastPos - Point.Pos());
			if(m_Data.size() <= 1)
			{
				// Start round cap
				AddCircle(LastPos, Angle + pi / 2.0f, Angle - pi / 2.0f, Point.w, Point.Color());
				// Join last position to position
				AddLine(LastPos, Point.Pos(), Point.w, Point.Color());
				// End round cap
				AddCircle(Point.Pos(), Angle + pi / 2.0f, Angle + pi * 1.5f, Point.w, Point.Color());
			}
			else
			{
				// Lazy round bevel
				AddCircle(LastPos, 0.0f, pi * 2.0f, Point.w, Point.Color());
				// Join from last position to end
				AddLine(LastPos, Point.Pos(), Point.w, Point.Color());
				// End round cap
				AddCircle(Point.Pos(), Angle + pi / 2.0f, Angle + pi * 1.5f, Point.w, Point.Color());
			}
		}
		m_Data.emplace_back(Point.Pos(), Point.w, Point.Color());
		return true;
	}
	bool PenUp(vec2 Pos)
	{
		return PenUp(CBgDrawItemDataPoint(Pos, CurrentWidth(), CurrentColor()));
	}
	bool MoveTo(const CBgDrawItemDataPoint &Point)
	{
		if(!m_Drawing)
			return false;
		if(m_Data.size() > BG_DRAW_MAX_POINTS_PER_ITEM)
			return PenUp(Point.Pos());
		const vec2 LastPos = m_Data.empty() ? Point.Pos() : m_Data.back().Pos();
		const float Distance = distance(LastPos, Point.Pos());
		// Don't draw short segments
		if(Distance < Point.w * 1.25f)
			return true;
		// Draw cap or bevel
		if(m_Data.size() <= 1)
		{
			// Start round cap
			const float Angle = angle(LastPos - Point.Pos());
			AddCircle(LastPos, Angle + pi / 2.0f, Angle - pi / 2.0f, Point.w, Point.Color());
		}
		else
		{
			// Lazy round bevel
			AddCircle(LastPos, 0.0f, pi * 2.0f, Point.w, Point.Color());
		}
		// Join last position to position
		AddLine(LastPos, Point.Pos(), Point.w, Point.Color());
		m_Data.emplace_back(Point);
		return true;
	}
	bool MoveTo(vec2 Pos)
	{
		return MoveTo(CBgDrawItemDataPoint(Pos, CurrentWidth(), CurrentColor()));
	}
	bool PointIntersect(const vec2 Pos, const float Radius) const
	{
		if(m_Data.empty())
			return true;
		if(m_Data.size() == 1)
			return distance(m_Data[0].Pos(), Pos) < m_Data[0].w + Radius;
		vec2 C, D = m_Data[0].Pos();
		for(auto It = std::next(m_Data.begin()); It != m_Data.end(); ++It)
		{
			const CBgDrawItemDataPoint &Point = *It;
			C = D;
			D = Point.Pos();
			vec2 Closest;
			if(!closest_point_on_line(C, D, Pos, Closest))
				continue;
			if(distance(Closest, Pos) < Radius + Point.w)
				return true;
		}
		return false;
	}
	bool LineIntersect(const vec2 A, const vec2 B) const
	{
		if(m_Data.empty())
			return true;
		if(m_Data.size() == 1)
		{
			vec2 P = m_Data[0].Pos();
			vec2 Closest;
			if(!closest_point_on_line(A, B, P, Closest))
				return false;
			return distance(Closest, P) < m_Data[0].w;
		}
		vec2 C, D = m_Data[0].Pos();
		for(auto It = std::next(m_Data.begin()); It != m_Data.end(); ++It)
		{
			const CBgDrawItemDataPoint &Point = *It;
			C = D;
			D = Point.Pos();
			if(line_intersects(A, B, C, D))
				return true;
		}
		return false;
	}
	void Render() const
	{
		m_This.Graphics()->RenderQuadContainer(m_QuadContainerIndex, m_QuadCount);
	}

	CBgDrawItem() = delete;
	CBgDrawItem(CGameClient &This, vec2 StartPos) :
		m_This(This), m_QuadContainerIndex(m_This.Graphics()->CreateQuadContainer()), m_BoundingBoxMin(StartPos), m_BoundingBoxMax(StartPos)
	{
		m_Data.emplace_back(StartPos, CurrentWidth(), CurrentColor());
	}
	CBgDrawItem(CGameClient &This, CBgDrawItemDataPoint StartPoint) :
		CBgDrawItem(This, StartPoint.Pos())
	{
		m_Data.clear();
		m_Data.push_back(StartPoint);
	}
	CBgDrawItem(CGameClient &This, const CBgDrawItemData &Data) :
		CBgDrawItem(This, Data[0])
	{
		if(Data.size() > 1)
			for(auto It = Data.begin() + 1; It != Data.end() - 1; ++It)
			{
				const CBgDrawItemDataPoint &Point = *It;
				MoveTo(Point);
			}
		PenUp(Data.back());
	}
	~CBgDrawItem()
	{
		m_This.Graphics()->DeleteQuadContainer(m_QuadContainerIndex);
	}
};

static void DoInput(CBgDraw &This, CBgDraw::InputMode Mode, int Value)
{
	CBgDraw::InputMode &Input = This.m_aInputData[g_Config.m_ClDummy];
	if(Value > 0)
		Input = Mode;
	else if(Input == Mode)
		Input = CBgDraw::InputMode::NONE;
}

void CBgDraw::ConBgDraw(IConsole::IResult *pResult, void *pUserData)
{
	DoInput(*(CBgDraw *)pUserData, InputMode::DRAW, pResult->GetInteger(0));
}

void CBgDraw::ConBgDrawErase(IConsole::IResult *pResult, void *pUserData)
{
	DoInput(*(CBgDraw *)pUserData, InputMode::ERASE, pResult->GetInteger(0));
}

void CBgDraw::ConBgDrawReset(IConsole::IResult *pResult, void *pUserData)
{
	CBgDraw *pThis = (CBgDraw *)pUserData;
	pThis->Reset();
}

void CBgDraw::ConBgDrawSave(IConsole::IResult *pResult, void *pUserData)
{
	CBgDraw *pThis = (CBgDraw *)pUserData;
	pThis->Save(pResult->GetString(0));
}

void CBgDraw::ConBgDrawLoad(IConsole::IResult *pResult, void *pUserData)
{
	CBgDraw *pThis = (CBgDraw *)pUserData;
	pThis->Load(pResult->GetString(0));
}

static IOHANDLE BgDrawOpenFile(CGameClient &This, const char *pFilename, int Flags)
{
	char aFilename[IO_MAX_PATH_LENGTH];
	if(pFilename && pFilename[0] != '\0')
	{
		if(str_endswith_nocase(pFilename, ".csv"))
			str_format(aFilename, sizeof(aFilename), "bgdraw/%s", pFilename);
		else
			str_format(aFilename, sizeof(aFilename), "bgdraw/%s.csv", pFilename);
	}
	else
	{
		SHA256_DIGEST Sha256 = This.Client()->GetCurrentMapSha256();
		char aSha256[SHA256_MAXSTRSIZE];
		sha256_str(Sha256, aSha256, sizeof(aSha256));
		str_format(aFilename, sizeof(aFilename), "bgdraw/%s_%s.csv", This.Client()->GetCurrentMap(), aSha256);
	}
	dbg_assert(Flags == IOFLAG_WRITE || Flags == IOFLAG_READ, "Flags must be either read or write");
	if(Flags == IOFLAG_WRITE)
	{
		// Create folder
		if(!This.Storage()->CreateFolder("bgdraw", IStorage::TYPE_SAVE))
			This.Echo(TCLocalize("Failed to create bgdraw folder", "bgdraw"));
	}
	IOHANDLE Handle = This.Storage()->OpenFile(aFilename, Flags, IStorage::TYPE_SAVE);
	char aMsg[IO_MAX_PATH_LENGTH + 32];
	if(Handle)
	{
		str_format(aMsg, sizeof(aMsg), TCLocalize("Opening %s for %s", "bgdraw"), aFilename, Flags == IOFLAG_WRITE ? TCLocalize("writing", "bgdraw") : TCLocalize("reading", "bgdraw"));
		dbg_msg("bgdraw", "Opening %s for %s", aFilename, Flags == IOFLAG_WRITE ? "writing" : "reading");
	}
	else
	{
		str_format(aMsg, sizeof(aMsg), TCLocalize("Failed to open %s for %s", "bgdraw"), aFilename, Flags == IOFLAG_WRITE ? TCLocalize("writing", "bgdraw") : TCLocalize("reading", "bgdraw"));
		dbg_msg("bgdraw", "Failed to open %s for %s", aFilename, Flags == IOFLAG_WRITE ? "writing" : "reading");
	}
	This.Echo(aMsg);
	return Handle;
}

bool CBgDraw::Save(const char *pFilename)
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return false;
	IOHANDLE Handle = BgDrawOpenFile(*GameClient(), pFilename, IOFLAG_WRITE);
	if(!Handle)
		return false;
	size_t Written = 0;
	bool Success = true;
	for(const CBgDrawItem &Item : *m_pvItems)
	{
		char aMsg[256];
		if(!BgDrawFile::Write(Handle, Item.Data()))
		{
			str_format(aMsg, sizeof(aMsg), TCLocalize("Writing item %zu failed", "bgdraw"), Written);
			GameClient()->Echo(aMsg);
			Success = false;
			break;
		}
		Written += 1;
	}
	char aMsg[256];
	str_format(aMsg, sizeof(aMsg), TCLocalize("Written %zu items", "bgdraw"), Written);
	GameClient()->Echo(aMsg);
	io_close(Handle);
	return Success;
}

bool CBgDraw::Load(const char *pFilename)
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return false;
	IOHANDLE Handle = BgDrawOpenFile(*GameClient(), pFilename, IOFLAG_READ);
	if(!Handle)
		return false;
	std::deque<CBgDrawItemData> Queue;
	size_t ItemsLoaded = 0;
	size_t ItemsDiscarded = 0;
	{
		CBgDrawItemData Data;
		while(BgDrawFile::Read(Handle, Data) && (ItemsLoaded++) < MAX_ITEMS_TO_LOAD)
		{
			if(Queue.size() > (size_t)g_Config.m_ClBgDrawMaxItems)
			{
				ItemsDiscarded += 1;
				Queue.pop_front();
			}
			Queue.push_back(Data);
		}
	}
	io_close(Handle);
	MakeSpaceFor(Queue.size());
	for(const CBgDrawItemData &Data : Queue)
		AddItem(*GameClient(), Data);
	char aInfo[256];
	if(ItemsDiscarded == 0)
		str_format(aInfo, sizeof(aInfo), TCLocalize("Loaded %zu items", "bgdraw"), ItemsLoaded);
	else
		str_format(aInfo, sizeof(aInfo), TCLocalize("Loaded %zu items (discarded %zu items)", "bgdraw"), ItemsLoaded - ItemsDiscarded, ItemsDiscarded);
	GameClient()->Echo(aInfo);

	return true;
}

template<typename... Args>
CBgDrawItem *CBgDraw::AddItem(Args &&... args)
{
	MakeSpaceFor(1);
	if(g_Config.m_ClBgDrawMaxItems == 0)
		return nullptr;
	m_pvItems->emplace_back(std::forward<Args>(args)...);
	return &m_pvItems->back();
}

void CBgDraw::MakeSpaceFor(size_t Count)
{
	if(g_Config.m_ClBgDrawMaxItems == 0 || Count >= (size_t)g_Config.m_ClBgDrawMaxItems)
	{
		m_pvItems->clear();
		return;
	}
	while(m_pvItems->size() + Count > (size_t)g_Config.m_ClBgDrawMaxItems)
	{
		// Prevent floating pointer
		for(std::optional<CBgDrawItem *> &ActiveItem : m_apActiveItems)
		{
			if(!ActiveItem.has_value())
				continue;
			if(ActiveItem.value() != &(*m_pvItems->begin()))
				continue;
			ActiveItem = std::nullopt;
		}
		m_pvItems->pop_front();
	}
}

void CBgDraw::OnConsoleInit()
{
	Console()->Register("+bg_draw", "", CFGFLAG_CLIENT, ConBgDraw, this, "Draw on the in game background");
	Console()->Register("+bg_draw_erase", "", CFGFLAG_CLIENT, ConBgDrawErase, this, "Erase items on the in game background");
	Console()->Register("bg_draw_reset", "", CFGFLAG_CLIENT, ConBgDrawReset, this, "Reset all drawings on the background");
	Console()->Register("bg_draw_save", "?r[filename]", CFGFLAG_CLIENT, ConBgDrawSave, this, "Save drawings to a given csv file (defaults to map name)");
	Console()->Register("bg_draw_load", "?r[filename]", CFGFLAG_CLIENT, ConBgDrawLoad, this, "Load drawings from a given csv file (defaults to map name)");
}

void CBgDraw::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	Graphics()->TextureClear();
	GameClient()->RenderTools()->MapScreenToGroup(GameClient()->m_Camera.m_Center.x, GameClient()->m_Camera.m_Center.y, GameClient()->Layers()->GameGroup(), GameClient()->m_Camera.m_Zoom);

	float Delta = Client()->RenderFrameTime();

	for(int Dummy = 0; Dummy < NUM_DUMMIES; ++Dummy)
	{
		// Handle updating active item
		const InputMode Input = m_aInputData[Dummy];
		std::optional<CBgDrawItem *> &ActiveItem = m_apActiveItems[Dummy];
		vec2 Pos;
		if(GameClient()->m_Snap.m_SpecInfo.m_Active && Dummy == g_Config.m_ClDummy)
			Pos = GameClient()->m_Camera.m_Center;
		else
			Pos = (GameClient()->m_Controls.m_aTargetPos[Dummy] - GameClient()->m_Camera.m_Center) * GameClient()->m_Camera.m_Zoom + GameClient()->m_Camera.m_Center;
		if(Input == InputMode::DRAW)
		{
			if(ActiveItem.has_value())
				(*ActiveItem)->MoveTo(Pos);
			else
				ActiveItem = AddItem(*GameClient(), Pos);
		}
		else if(ActiveItem.has_value())
		{
			(*ActiveItem)->PenUp(Pos);
			ActiveItem = std::nullopt;
		}
		std::optional<vec2> &LastPos = m_aLastPos[Dummy];
		if(Input == InputMode::ERASE)
		{
			if(LastPos.has_value())
			{
				for(CBgDrawItem &Item : *m_pvItems)
					if(Item.LineIntersect(*LastPos, Pos))
						Item.m_Killed = true;
			}
			else
			{
				for(CBgDrawItem &Item : *m_pvItems)
					if(Item.PointIntersect(Pos, 2.0f))
						Item.m_Killed = true;
			}
		}
		else
		{
			LastPos = std::nullopt;
		}
	}
	// Remove extra items
	MakeSpaceFor(0);
	// Update age of items, delete old items, render items
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	for(CBgDrawItem &Item : *m_pvItems)
	{
		// If this item is currently active
		if(std::any_of(std::begin(m_apActiveItems), std::end(m_apActiveItems), [&](const std::optional<CBgDrawItem *> &ActiveItem) {
			   return ActiveItem.value_or(nullptr) == &Item;
		   }))
		{
			Item.m_SecondsAge = 0.0f;
		}
		else
		{
			Item.m_SecondsAge += Delta;
			if(g_Config.m_ClBgDrawFadeTime > 0 && Item.m_SecondsAge > (float)g_Config.m_ClBgDrawFadeTime)
				Item.m_Killed = true;
		}
		const bool InRangeX = Item.BoundingBoxMin().x < ScreenX1 || Item.BoundingBoxMax().x > ScreenX0;
		const bool InRangeY = Item.BoundingBoxMin().y < ScreenY1 || Item.BoundingBoxMax().y > ScreenY0;
		if(InRangeX && InRangeY)
			Item.Render();
	}
	// Remove killed items
	m_pvItems->remove_if([&](CBgDrawItem &Item) { return Item.m_Killed; });
	Graphics()->SetColor(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
}

void CBgDraw::Reset()
{
	m_aInputData.fill(InputMode::NONE);
	m_aLastPos.fill(std::nullopt);
	m_apActiveItems.fill(std::nullopt);
	m_pvItems->clear();
}

void CBgDraw::OnMapLoad()
{
	Reset();
}

void CBgDraw::OnShutdown()
{
	Reset();
}

CBgDraw::CBgDraw() :
	m_pvItems(new std::list<CBgDrawItem>())
{
}

CBgDraw::~CBgDraw()
{
	delete m_pvItems;
}
