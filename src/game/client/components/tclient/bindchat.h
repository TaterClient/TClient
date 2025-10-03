#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_BINDCHAT_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_BINDCHAT_H

#include <engine/console.h>

#include <game/client/component.h>
#include <game/client/lineinput.h>

class IConfigManager;

enum
{
	BINDCHAT_MAX_NAME = 64,
	BINDCHAT_MAX_PARAMS = 64,
	BINDCHAT_MAX_HELP = 128,
	BINDCHAT_MAX_CMD = 1024,
	BINDCHAT_MAX_BINDS = 256,
};

class CBindChat : public CComponent
{
public:
	class CBind
	{
	public:
		bool m_IsEx = false;
		char m_aName[BINDCHAT_MAX_NAME];
		char m_aParams[BINDCHAT_MAX_PARAMS];
		char m_aHelp[BINDCHAT_MAX_HELP];
		char m_aCommand[BINDCHAT_MAX_CMD];
		CBind() = default;
		CBind(const char *pName, const char *pCommand);
		CBind(const char *pName, const char *pParams, const char *pHelp, const char *pCommand);
		bool CompContent(const CBind &Other) const;
	};
	class CBindDefault
	{
	public:
		const char *m_pTitle;
		CBind m_Bind;
		CLineInput m_LineInput;
	};
	static std::vector<std::pair<const char *, std::vector<CBindDefault>>> BIND_DEFAULTS;

private:
	static void ConAddBindchat(IConsole::IResult *pResult, void *pUserData);
	static void ConAddBindchatEx(IConsole::IResult *pResult, void *pUserData);
	static void ConBindchats(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindchat(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindchatAll(IConsole::IResult *pResult, void *pUserData);
	static void ConBindchatDefaults(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

	static void ExecuteBindExCallback(IConsole::IResult *pResult, void *pUserData);
	void ExecuteBind(const CBind &Bind, const char *pArgs);

public:
	std::vector<CBind> m_vBinds;

	CBindChat();
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;

	void AddBind(const char *pName, const char *pParams, const char *pHelp, const char *pCommand, bool IsEx);
	void AddBind(const CBind &Bind);

	bool RemoveBind(const char *pName);
	void RemoveAllBinds();

	CBind *GetBind(const char *pCommand);

	bool CheckBindChat(const char *pText);
	bool ChatDoBinds(const char *pText);
	bool ChatDoAutocomplete(bool ShiftPressed);
};

#endif
