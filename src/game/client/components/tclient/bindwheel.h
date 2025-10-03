#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_BINDWHEEL_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_BINDWHEEL_H

#include <base/str.h>

#include <engine/console.h>

#include <game/client/component.h>

#include <vector>

class IConfigManager;

enum
{
	BINDWHEEL_MAX_NAME = 64,
	BINDWHEEL_MAX_CMD = 1024,
	BINDWHEEL_MAX_BINDS = 64
};

class CBindWheel : public CComponent
{
	float m_AnimationTime = 0.0f;
	float m_aAnimationTimeItems[BINDWHEEL_MAX_BINDS] = {0};

	bool m_Active = false;
	bool m_WasActive = false;

	int m_SelectedBind;

	static void ConOpenBindwheel(IConsole::IResult *pResult, void *pUserData);
	static void ConAddBindwheelLegacy(IConsole::IResult *pResult, void *pUserData);
	static void ConAddBindwheel(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindwheel(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveAllBindwheelBinds(IConsole::IResult *pResult, void *pUserData);
	static void ConBindwheelExecuteHover(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

public:
	class CBind
	{
	public:
		char m_aName[BINDWHEEL_MAX_NAME] = "EMPTY";
		char m_aCommand[BINDWHEEL_MAX_CMD] = "";

		bool operator==(const CBind &Other) const
		{
			return str_comp(m_aName, Other.m_aName) == 0 && str_comp(m_aCommand, Other.m_aCommand) == 0;
		}
	};

	std::vector<CBind> m_vBinds;

	CBindWheel();
	int Sizeof() const override { return sizeof(*this); }

	void OnReset() override;
	void OnRender() override;
	void OnConsoleInit() override;
	void OnRelease() override;
	bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
	bool OnInput(const IInput::CEvent &Event) override;

	void AddBind(const char *Name, const char *Command);
	void RemoveBind(const char *Name, const char *Command);
	void RemoveBind(int Index);
	void RemoveAllBinds();

	void ExecuteHoveredBind();
	void ExecuteBind(int Bind);

	bool IsActive() const { return m_Active; }
};

#endif
