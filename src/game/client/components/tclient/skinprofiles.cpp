#include "skinprofiles.h"

#include <engine/config.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/client/gameclient.h>

static void EscapeParam(char *pDst, const char *pSrc, int Size)
{
	str_escape(&pDst, pSrc, pDst + Size);
}

CProfile::CProfile(int BodyColor, int FeetColor, int CountryFlag, int Emote, const char *pSkinName, const char *pName, const char *pClan)
{
	m_BodyColor = BodyColor;
	m_FeetColor = FeetColor;
	m_CountryFlag = CountryFlag;
	m_Emote = Emote;
	str_copy(m_SkinName, pSkinName);
	str_copy(m_Name, pName);
	str_copy(m_Clan, pClan);
}

void CSkinProfiles::OnConsoleInit()
{
	IConfigManager *pConfigManager = Kernel()->RequestInterface<IConfigManager>();
	if(pConfigManager)
		pConfigManager->RegisterCallback(ConfigSaveCallback, this, ConfigDomain::TCLIENTPROFILES);

	Console()->Register("add_profile", "i[body] i[feet] i[flag] i[emote] s[skin] s[name] s[clan]", CFGFLAG_CLIENT, ConAddProfile, this, "Add a profile");
}

void CSkinProfiles::ConAddProfile(IConsole::IResult *pResult, void *pUserData)
{
	CSkinProfiles *pSelf = (CSkinProfiles *)pUserData;
	pSelf->AddProfile(pResult->GetInteger(0), pResult->GetInteger(1), pResult->GetInteger(2), pResult->GetInteger(3), pResult->GetString(4), pResult->GetString(5), pResult->GetString(6));
}

void CSkinProfiles::AddProfile(int BodyColor, int FeetColor, int CountryFlag, int Emote, const char *pSkinName, const char *pName, const char *pClan)
{
	CProfile Profile = CProfile(BodyColor, FeetColor, CountryFlag, Emote, pSkinName, pName, pClan);
	m_Profiles.push_back(Profile);
}

void CSkinProfiles::ApplyProfile(int Dummy, const CProfile &Profile)
{
	if(g_Config.m_TcProfileSkin && strlen(Profile.m_SkinName) != 0)
		str_copy(Dummy ? g_Config.m_ClDummySkin : g_Config.m_ClPlayerSkin, Profile.m_SkinName);
	if(g_Config.m_TcProfileColors && Profile.m_BodyColor != -1 && Profile.m_FeetColor != -1)
	{
		(Dummy ? g_Config.m_ClDummyColorBody : g_Config.m_ClPlayerColorBody) = Profile.m_BodyColor;
		(Dummy ? g_Config.m_ClDummyColorFeet : g_Config.m_ClPlayerColorFeet) = Profile.m_FeetColor;
	}
	if(g_Config.m_TcProfileEmote && Profile.m_Emote != -1)
		(Dummy ? g_Config.m_ClDummyDefaultEyes : g_Config.m_ClPlayerDefaultEyes) = Profile.m_Emote;
	if(g_Config.m_TcProfileName && strlen(Profile.m_Name) != 0)
		str_copy(Dummy ? g_Config.m_ClDummyName : g_Config.m_PlayerName, Profile.m_Name); // TODO m_ClPlayerName
	if(g_Config.m_TcProfileClan && (strlen(Profile.m_Clan) != 0 || g_Config.m_TcProfileOverwriteClanWithEmpty))
		str_copy(Dummy ? g_Config.m_ClDummyClan : g_Config.m_PlayerClan, Profile.m_Clan); // TODO m_ClPlayerClan
	if(g_Config.m_TcProfileFlag && Profile.m_CountryFlag != -2)
		(Dummy ? g_Config.m_ClDummyCountry : g_Config.m_PlayerCountry) = Profile.m_CountryFlag;
	GameClient()->m_Skins.m_SkinList.ForceRefresh(); // Prevent segfault
	if(Dummy)
		GameClient()->SendDummyInfo(false);
	else
		GameClient()->SendInfo(false);
}

void CSkinProfiles::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CSkinProfiles *pThis = (CSkinProfiles *)pUserData;
	char aBuf[256];
	char aBufTemp[128];
	char aEscapeBuf[256];
	for(const CProfile &Profile : pThis->m_Profiles)
	{
		str_copy(aBuf, "add_profile ", sizeof(aBuf));

		str_format(aBufTemp, sizeof(aBufTemp), "%d ", Profile.m_BodyColor);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		str_format(aBufTemp, sizeof(aBufTemp), "%d ", Profile.m_FeetColor);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		str_format(aBufTemp, sizeof(aBufTemp), "%d ", Profile.m_CountryFlag);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		str_format(aBufTemp, sizeof(aBufTemp), "%d ", Profile.m_Emote);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		EscapeParam(aEscapeBuf, Profile.m_SkinName, sizeof(aEscapeBuf));
		str_format(aBufTemp, sizeof(aBufTemp), "\"%s\" ", aEscapeBuf);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		EscapeParam(aEscapeBuf, Profile.m_Name, sizeof(aEscapeBuf));
		str_format(aBufTemp, sizeof(aBufTemp), "\"%s\" ", aEscapeBuf);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		EscapeParam(aEscapeBuf, Profile.m_Clan, sizeof(aEscapeBuf));
		str_format(aBufTemp, sizeof(aBufTemp), "\"%s\"", aEscapeBuf);
		str_append(aBuf, aBufTemp, sizeof(aBuf));

		pConfigManager->WriteLine(aBuf, ConfigDomain::TCLIENTPROFILES);
	}
}
