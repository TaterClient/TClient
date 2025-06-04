#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_BINDCHAT_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_BINDCHAT_H

#include <game/client/component.h>

#include <engine/console.h>

class IConfigManager;

enum
{
	BINDCHAT_MAX_NAME = 64,
	BINDCHAT_MAX_CMD = 1024,
	BINDCHAT_MAX_BINDS = 256,
};

class CBindChat : public CComponent
{
	static void ConAddBindchat(IConsole::IResult *pResult, void *pUserData);
	static void ConBindchats(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindchat(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveBindchatAll(IConsole::IResult *pResult, void *pUserData);
	static void ConBindchatDefaults(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

	void ExecuteBind(int Bind, const char *pArgs);

public:
	class CBind
	{
	public:
		char m_aName[BINDCHAT_MAX_NAME];
		char m_aCommand[BINDCHAT_MAX_CMD];
		bool operator==(const CBind &Other) const
		{
			return str_comp(m_aName, Other.m_aName) == 0 && str_comp(m_aCommand, Other.m_aCommand) == 0;
		}
		CBind() = default;
		CBind(const char *pName, const char *pCommand)
		{
			str_copy(m_aName, pName);
			str_copy(m_aCommand, pCommand);
		}
	};
	class CBindDefault
	{
	public:
		const char *m_pTitle;
		CBind m_Bind;
		CBindDefault(const char *pTitle, const char *pName, const char *pCommand)
		{
			m_pTitle = pTitle;
			m_Bind = CBind(pName, pCommand);
		}
	};

	std::vector<CBind> m_vBinds;

	CBindChat();
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;

	void AddBind(const char *pName, const char *pCommand);
	void AddBind(const CBind &Bind);

	void RemoveBindCommand(const char *pCommand);
	void RemoveBind(const char *pName);
	void RemoveBind(int Index);
	void RemoveAllBinds();

	int GetBind(const char *pCommand);
	CBind *Get(int Index);

	bool CheckBindChat(const char *pText);
	bool ChatDoBinds(const char *pText);
	bool ChatDoAutocomplete(bool ShiftPressed);
};

static CBindChat::CBindDefault s_aDefaultBindChatKaomoji[] = {
	{"Shrug:", "!shrug", "say ¯\\_(ツ)_/¯"},
	{"Flip:", "!flip", "say (╯°□°)╯︵ ┻━┻"},
	{"Unflip:", "!unflip", "say ┬─┬ノ( º _ ºノ)"},
	{"Cute:", "!cute", "say ૮ ˶ᵔ ᵕ ᵔ˶ ა"},
	{"Lenny:", "!lenny", "say ( ͡° ͜ʖ ͡°)"},
};

static CBindChat::CBindDefault s_aDefaultBindChatWar[] = {
	// These don't get displayed in settings
	{nullptr, "!war", "war_name_index 1"},
	{nullptr, "!warclan", "war_clan_index 1"},
	{nullptr, "!team", "war_name_index 2"},
	{nullptr, "!teamclan", "war_clan_index 2"},
	{nullptr, "!delwar", "remove_war_name_index 1"},
	{nullptr, "!delwarclan", "remove_war_clan_index 1"},
	{nullptr, "!delteam", "remove_war_name_index 2"},
	{nullptr, "!delteamclan", "remove_war_clan_index 2"},
	{nullptr, "!name", "war_name"},
	{nullptr, "!clan", "war_clan"},
	{nullptr, "!delname", "remove_war_name"},
	{nullptr, "!delclan", "remove_war_clan"},
};

static CBindChat::CBindDefault s_aDefaultBindChatMod[] = {
	{"Mute ID:", "!mmute", "mod_rcon_mute"},
	{"Mute Name:", "!mmuten", "mod_rcon_mute_name"},
	{"Unmute Last:", "!munmutelast", "rcon unmute 0"},
	{"Kick ID:", "!mkick", "mod_rcon_kick"},
	{"Kick Name:", "!mkickn", "mod_rcon_kick_name"},
	{"Ban ID:", "!mban", "mod_rcon_ban"},
	{"Ban Name:", "!mbann", "mod_rcon_ban_name"},
	{"Unban Last:", "!munbanlast", "rcon unban 0"},
	{"Kill Ids:", "!mkill", "rcon mod_rcon_kill"},
	{"Kill Names:", "!mkilln", "rcon mod_rcon_kill_name"},
};

static CBindChat::CBindDefault s_aDefaultBindChatOther[] = {
	{"Translate:", "!translate", "translate"},
	{"Translate ID:", "!translateid", "translate_id"},
	{"Mute:", "!mute", "add_foe"},
	{"Unmute:", "!unmute", "remove_foe"},
};

#endif
