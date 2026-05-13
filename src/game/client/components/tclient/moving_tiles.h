#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_MOVING_TILES_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_MOVING_TILES_H

#include <base/vmath.h>

#include <game/client/component.h>
#include <game/client/components/envelope_state.h>
#include <game/mapitems.h>

#include <vector>

class CQuad;
class CMapItemGroup;
class CMapItemLayerQuads;

enum class EMovingTileRenderType
{
	BELOW,
	ABOVE
};

enum class EQType
{
	NONE = -1,
	FREEZE,
	UNFREEZE,
	DEATH,
	STOPA,
	CFRM,
	HOOKABLE,
	UNHOOKABLE,
	NUM
};

static constexpr char ValidQuadNames[][30] =
	{
		"QFr",
		"QUnFr",
		"QDeath",
		"QStopa",
		"QCfrm",
		"QHook",
		"QUnHook"};

class CQuadData : CMapItemLayerQuads
{
public:
	CQuad *m_pQuad = nullptr;
	CMapItemGroup *m_pGroup = nullptr;
	CMapItemLayerQuads *m_pLayer = nullptr;
	EQType m_Type = EQType::NONE;
	vec2 m_Pos[5] = {vec2(0, 0)}; // 4 corners + center
	float m_Angle = 0.0f;
};

class CMovingTiles : public CComponent
{
	void Reset();
	std::vector<CQuadData> m_vQuads;

	bool m_RenderAbove; // hookables are rendered above tilemap, the rest below
	CEnvelopeState m_EnvEvaluator;

public:
	CMovingTiles(bool Above) :
		m_RenderAbove(Above)
	{
	}

	void OnStateChange(int NewState, int OldState) override;
	void OnMapLoad() override;
	void OnRender() override;

	int Sizeof() const override { return sizeof(*this); }
};

#endif // GAME_CLIENT_COMPONENTS_TCLIENT_MOVING_TILES_H
