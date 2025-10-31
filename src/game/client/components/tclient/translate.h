#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_TRANSLATE_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_TRANSLATE_H

#include <game/client/component.h>
#include <game/client/components/chat.h>

#include <memory>
#include <optional>
#include <vector>

class CTranslate;

class ITranslateBackend
{
public:
	virtual ~ITranslateBackend() = default;
	virtual const char *EncodeTarget(const char *pTarget) const;
	virtual bool CompareTargets(const char *pA, const char *pB) const;
	virtual const char *Name() const = 0;
	virtual std::optional<bool> Update(CTranslateResponse &Out) = 0;
};

class CTranslate : public CComponent
{
	class CTranslateJob
	{
	public:
		std::unique_ptr<ITranslateBackend> m_pBackend = nullptr;
		// For chat translations
		CChat::CLine *m_pLine = nullptr;
		std::shared_ptr<CTranslateResponse> m_pTranslateResponse = nullptr;
	};
	std::vector<CTranslateJob> m_vJobs;

	static void ConTranslate(IConsole::IResult *pResult, void *pUserData);
	static void ConTranslateId(IConsole::IResult *pResult, void *pUserData);

public:
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnRender() override;

	void Translate(int Id, bool ShowProgress = true);
	void Translate(const char *pName, bool ShowProgress = true);
	void Translate(CChat::CLine &Line, bool ShowProgress = true);

	void AutoTranslate(CChat::CLine &Line);
};

#endif
