#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_MUMBLE_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_MUMBLE_H

#include <base/types.h>

#include <engine/shared/console.h>

#include <game/client/component.h>

struct MumbleContext;

class CMumble : public CComponent
{
private:
	int m_LastClientId = -1;
	int m_LastTeam = 0;
	NETADDR m_LastAddr = {};
	MumbleContext *m_pContext = nullptr;

public:
	void MakeContext();
	CMumble();
	~CMumble() override;
	int Sizeof() const override { return sizeof(*this); }
	void OnRender() override;
	void OnConsoleInit() override;
};

#endif
