#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_SKINPROFILES_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_SKINPROFILES_H

#include <base/system.h>

#include <engine/console.h>
#include <engine/keys.h>
#include <engine/shared/protocol.h>

#include <game/client/component.h>

#include <vector>

class CProfile
{
public:
	int m_BodyColor;
	int m_FeetColor;
	int m_CountryFlag;
	int m_Emote;
	char m_SkinName[24];
	char m_Name[MAX_NAME_LENGTH];
	char m_Clan[MAX_CLAN_LENGTH];
	CProfile(int BodyColor, int FeetColor, int CountryFlag, int Emote, const char *pSkinName, const char *pName, const char *pClan);
};

class CSkinProfiles : public CComponent
{
	static void ConAddProfile(IConsole::IResult *pResult, void *pUserData);

	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

public:
	std::vector<CProfile> m_Profiles;
	void AddProfile(int BodyColor, int FeetColor, int CountryFlag, int Emote, const char *pSkinName, const char *pName, const char *pClan);
	void ApplyProfile(int Dummy, const CProfile &Profile);

	virtual int Sizeof() const override { return sizeof(*this); }

	virtual void OnConsoleInit() override;
};
#endif
