#include "outlines.h"

#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/mapitems.h>

// The order of this is the order of priority for outlines
enum
{
	OUTLINE_NONE = 0,
	OUTLINE_UNFREEZE,
	OUTLINE_FREEZE,
	OUTLINE_TELE,
	OUTLINE_KILL,
	OUTLINE_SOLID,
};

enum class OutlineLayer
{
	GAME,
	FRONT,
	TELE
};

class COutLineLayer
{
private:
	CMapItemLayerTilemap *GetLayer(CGameClient *pThis) const
	{
		if(m_Type == OutlineLayer::GAME)
			return pThis->Layers()->GameLayer();
		if(m_Type == OutlineLayer::FRONT)
			return pThis->Layers()->FrontLayer();
		if(m_Type == OutlineLayer::TELE)
			return pThis->Layers()->TeleLayer();
		dbg_assert(false, "Invalid value for m_Type");
	}
	int GetLayerData(CGameClient *pThis) const
	{
		if(m_Type == OutlineLayer::GAME)
			return pThis->Layers()->GameLayer()->m_Data;
		if(m_Type == OutlineLayer::FRONT)
			return pThis->Layers()->FrontLayer()->m_Front;
		if(m_Type == OutlineLayer::TELE)
			return pThis->Layers()->TeleLayer()->m_Tele;
		dbg_assert(false, "Invalid value for m_Type");
	}

public:
	const OutlineLayer m_Type;
	void GetMeta(CGameClient *pThis, ivec2 &Size) const
	{
		Size = {0, 0};
		const auto *pLayer = GetLayer(pThis);
		if(!pLayer)
			return;
		const size_t TileSize = m_Type == OutlineLayer::TELE ? sizeof(CTeleTile) : sizeof(CTile);
		const int DataSize = pThis->Layers()->Map()->GetDataSize(GetLayerData(pThis));
		if(DataSize <= 0 || (size_t)DataSize < (size_t)pLayer->m_Width * (size_t)pLayer->m_Height * TileSize)
			return;
		Size = {pLayer->m_Width, pLayer->m_Height};
	}
	void SetData(CGameClient *pThis, int *pData, const ivec2 &Size) const
	{
		const auto *pLayer = GetLayer(pThis);
		const auto *pTiles = (CTile *)pThis->Layers()->Map()->GetData(GetLayerData(pThis));
		for(int y = 0; y < pLayer->m_Height; ++y)
		{
			for(int x = 0; x < pLayer->m_Width; ++x)
			{
				const int Index = y * pLayer->m_Width + x;
				const int IndexOut = y * Size.x + x;
				if(m_Type == OutlineLayer::TELE)
				{
					const auto &Tile = ((CTeleTile *)pTiles)[Index];
					if(Tile.m_Number != 0 && Tile.m_Type != 0)
						pData[IndexOut] = OUTLINE_TELE;
				}
				else
				{
					const auto Tile = pTiles[Index].m_Index;
					if(Tile == TILE_SOLID || Tile == TILE_NOHOOK)
						pData[IndexOut] = OUTLINE_SOLID;
					else if(Tile == TILE_FREEZE || Tile == TILE_DFREEZE || Tile == TILE_LFREEZE)
						pData[IndexOut] = OUTLINE_FREEZE;
					else if(Tile == TILE_UNFREEZE || Tile == TILE_DUNFREEZE || Tile == TILE_LUNFREEZE)
						pData[IndexOut] = OUTLINE_UNFREEZE;
					else if(Tile == TILE_DEATH)
						pData[IndexOut] = OUTLINE_KILL;
				}
			}
		}
	}
};

// The order of this determines order of priority into the one map (tele + freeze = tele)
static constexpr COutLineLayer OUTLINE_LAYERS[] = {{OutlineLayer::TELE}, {OutlineLayer::GAME}, {OutlineLayer::FRONT}};

void COutlines::OnMapLoad()
{
	if(m_pMapData)
	{
		delete[] m_pMapData;
		m_pMapData = nullptr;
	}

	// Find valid layers and size
	std::vector<const COutLineLayer *> vValidOutlineLayers;
	m_MapDataSize = {0, 0};
	for(const auto &Layer : OUTLINE_LAYERS)
	{
		ivec2 LayerSize;
		Layer.GetMeta(GameClient(), LayerSize);
		if(LayerSize.x <= 0 || LayerSize.y <= 0)
			continue;
		m_MapDataSize.x = std::max(m_MapDataSize.x, LayerSize.x);
		m_MapDataSize.y = std::max(m_MapDataSize.y, LayerSize.y);
		vValidOutlineLayers.push_back(&Layer);
	}
	if(m_MapDataSize.x <= 0 || m_MapDataSize.y <= 0)
		return;
	m_pMapData = new int[m_MapDataSize.x * m_MapDataSize.y](OUTLINE_NONE);

	// Do it
	for(const auto *pLayer : vValidOutlineLayers)
	{
		pLayer->SetData(GameClient(), m_pMapData, m_MapDataSize);
	}
}

void COutlines::OnRender()
{
	if(!m_pMapData)
		return;
	if(GameClient()->m_MapLayersBackground.m_OnlineOnly && Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;
	if(!g_Config.m_ClOverlayEntities && g_Config.m_TcOutlineEntities)
		return;
	if(!g_Config.m_TcOutline)
		return;

	const float Scale = 32.0f;

	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	int StartY = (int)(ScreenY0 / Scale) - 1;
	int StartX = (int)(ScreenX0 / Scale) - 1;
	int EndY = (int)(ScreenY1 / Scale) + 1;
	int EndX = (int)(ScreenX1 / Scale) + 1;
	int MaxScale = 12;
	if(EndX - StartX > Graphics()->ScreenWidth() / MaxScale || EndY - StartY > Graphics()->ScreenHeight() / MaxScale)
	{
		int EdgeX = (EndX - StartX) - (Graphics()->ScreenWidth() / MaxScale);
		StartX += EdgeX / 2;
		EndX -= EdgeX / 2;
		int EdgeY = (EndY - StartY) - (Graphics()->ScreenHeight() / MaxScale);
		StartY += EdgeY / 2;
		EndY -= EdgeY / 2;
	}

	auto GetTile = [&](int x, int y) {
		x = std::clamp(x, 0, m_MapDataSize.x - 1);
		y = std::clamp(y, 0, m_MapDataSize.y - 1);
		return m_pMapData[y * m_MapDataSize.x + x];
	};

	Graphics()->TextureClear();
	Graphics()->QuadsBegin();

	for(int y = StartY; y < EndY; y++)
	{
		for(int x = StartX; x < EndX; x++)
		{
			const int Type = GetTile(x, y);
			if(Type == OUTLINE_NONE)
				continue;
			class COutlineConfig
			{
			public:
				const int &m_Enable;
				const int &m_Width;
				const unsigned int &m_Color;
			};
			const COutlineConfig Config = [&]() -> COutlineConfig {
				if(Type == OUTLINE_SOLID)
					return {g_Config.m_TcOutlineSolid, g_Config.m_TcOutlineWidthSolid, g_Config.m_TcOutlineColorSolid};
				if(Type == OUTLINE_FREEZE)
					return {g_Config.m_TcOutlineFreeze, g_Config.m_TcOutlineWidthFreeze, g_Config.m_TcOutlineColorFreeze};
				if(Type == OUTLINE_UNFREEZE)
					return {g_Config.m_TcOutlineUnfreeze, g_Config.m_TcOutlineWidthUnfreeze, g_Config.m_TcOutlineColorUnfreeze};
				if(Type == OUTLINE_KILL)
					return {g_Config.m_TcOutlineKill, g_Config.m_TcOutlineWidthKill, g_Config.m_TcOutlineColorKill};
				if(Type == OUTLINE_TELE)
					return {g_Config.m_TcOutlineTele, g_Config.m_TcOutlineWidthTele, g_Config.m_TcOutlineColorTele};
				dbg_assert(false, "Invalid value for Type at %d, %d/%d, %d", x, y, m_MapDataSize.x, m_MapDataSize.y);
			}();
			if(!Config.m_Enable || Config.m_Width <= 0)
				continue;
			// Find neighbours
			const bool aNeighbors[8] = {
				GetTile(x - 1, y - 1) >= Type,
				GetTile(x - 0, y - 1) >= Type,
				GetTile(x + 1, y - 1) >= Type,
				GetTile(x - 1, y + 0) >= Type,
				GetTile(x + 1, y + 0) >= Type,
				GetTile(x - 1, y + 1) >= Type,
				GetTile(x + 0, y + 1) >= Type,
				GetTile(x + 1, y + 1) >= Type,
			};
			// Figure out edges
			IGraphics::CQuadItem aQuads[8];
			int NumQuads = 0;
			// Lone corners first
			if(!aNeighbors[0] && aNeighbors[1] && aNeighbors[3])
				aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale, Config.m_Width, Config.m_Width);
			if(!aNeighbors[2] && aNeighbors[1] && aNeighbors[4])
				aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale + Scale - Config.m_Width, y * Scale, Config.m_Width, Config.m_Width);
			if(!aNeighbors[5] && aNeighbors[3] && aNeighbors[6])
				aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale + Scale - Config.m_Width, Config.m_Width, Config.m_Width);
			if(!aNeighbors[7] && aNeighbors[6] && aNeighbors[4])
				aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale + Scale - Config.m_Width, y * Scale + Scale - Config.m_Width, Config.m_Width, Config.m_Width);
			// Top
			if(!aNeighbors[1])
				aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale, Scale, Config.m_Width);
			// Bottom
			if(!aNeighbors[6])
				aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale + Scale - Config.m_Width, Scale, Config.m_Width);
			// Left
			if(!aNeighbors[3])
			{
				if(aNeighbors[1] && aNeighbors[6])
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale, Config.m_Width, Scale);
				else if(aNeighbors[6])
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale + Config.m_Width, Config.m_Width, Scale - Config.m_Width);
				else if(aNeighbors[1])
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale, Config.m_Width, Scale - Config.m_Width);
				else
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale, y * Scale + Config.m_Width, Config.m_Width, Scale - Config.m_Width * 2.0f);
			}
			// Right
			if(!aNeighbors[4])
			{
				if(aNeighbors[1] && aNeighbors[6])
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale + Scale - Config.m_Width, y * Scale, Config.m_Width, Scale);
				else if(aNeighbors[6])
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale + Scale - Config.m_Width, y * Scale + Config.m_Width, Config.m_Width, Scale - Config.m_Width);
				else if(aNeighbors[1])
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale + Scale - Config.m_Width, y * Scale, Config.m_Width, Scale - Config.m_Width);
				else
					aQuads[NumQuads++] = IGraphics::CQuadItem(x * Scale + Scale - Config.m_Width, y * Scale + Config.m_Width, Config.m_Width, Scale - Config.m_Width * 2.0f);
			}
			if(NumQuads <= 0)
				continue;
			Graphics()->SetColor(color_cast<ColorRGBA>(ColorHSLA(Config.m_Color, true)));
			Graphics()->QuadsDrawTL(aQuads, NumQuads);
		}
	}

	Graphics()->QuadsEnd();
}
