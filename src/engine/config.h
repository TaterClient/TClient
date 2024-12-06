/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include "kernel.h"

enum EConfigDomain
{
	CFGDOMAIN_NONE = 0,
	CFGDOMAIN_TCLIENT,
	CFGDOMAIN_MAX,
};

class IConfigManager : public IInterface
{
	MACRO_INTERFACE("config")
public:
	typedef void (*SAVECALLBACKFUNC)(IConfigManager *pConfig, void *pUserData);
	typedef void (*POSSIBLECFGFUNC)(const struct SConfigVariable *, void *pUserData);

	virtual void Init() = 0;
	virtual void Reset(const char *pScriptName) = 0;
	virtual void ResetGameSettings() = 0;
	virtual void SetReadOnly(const char *pScriptName, bool ReadOnly) = 0;
	virtual bool Save() = 0;
	virtual class CConfig *Values() = 0;

	virtual void RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData, EConfigDomain pClient = EConfigDomain::CFGDOMAIN_NONE) = 0;

	virtual void WriteLine(const char *pLine, EConfigDomain pClient = EConfigDomain::CFGDOMAIN_NONE) = 0;

	virtual void StoreUnknownCommand(const char *pCommand) = 0;

	virtual void PossibleConfigVariables(const char *pStr, int FlagMask, POSSIBLECFGFUNC pfnCallback, void *pUserData) = 0;
};

extern IConfigManager *CreateConfigManager();

#endif
