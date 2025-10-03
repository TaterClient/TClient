#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_MOD_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_MOD_H

#include <engine/shared/console.h>
#include <engine/shared/http.h>

#include <game/client/component.h>

class CMod : public CComponent
{
public:
	int m_ModWeaponActiveId = -1;
	float m_ModWeaponActiveTimeLeft;
	void ModWeapon(int Id);
	void OnFire(bool Pressed);

	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnRender() override;
	void OnStateChange(int OldState, int NewState) override;
};

#endif
