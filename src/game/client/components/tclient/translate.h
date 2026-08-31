#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_TRANSLATE_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_TRANSLATE_H

#include <game/client/component.h>
#include <game/client/components/chat.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>

class CTranslate;

class ITranslateBackend
{
public:
	explicit ITranslateBackend(const char *pName) :
		m_pName(pName) {}
	virtual ~ITranslateBackend() = default;
	virtual const char *EncodeTarget(const char *pTarget) const;
	virtual bool CompareTargets(const char *pA, const char *pB) const;
	// Set once at construction from the matching g_aTranslateBackends entry
	// -- backend classes don't hold their own copy of their display name.
	const char *Name() const { return m_pName; }
	virtual std::optional<bool> Update(CTranslateResponse &Out) = 0;

private:
	const char *m_pName;
};

struct STranslateLanguage
{
	const char *m_pName;
	const char *m_pCode;
};

// Single source of truth for every available translate backend: its config
// value, its display name (passed into the backend's constructor so
// ITranslateBackend::Name() never drifts from it), and the language list its
// settings-menu dropdown should offer. Different backends genuinely support
// different language sets (Google supports far more than a typical
// self-hosted LibreTranslate instance), so this is per-backend rather than
// one shared list.
struct STranslateBackendInfo
{
	const char *m_pValue;
	const char *m_pName;
	std::vector<STranslateLanguage> m_vLanguages;
	// Whether the settings menu should show the endpoint/API-key fields for
	// this backend -- data-driven so the menu never has to special-case a
	// backend by name.
	bool m_NeedsEndpointConfig;
};

extern const std::array<STranslateBackendInfo, 2> g_aTranslateBackends;

// Finds pCode's index in vLanguages, or whichever entry IS English (found by
// code, not a hardcoded position) if pCode is unset/unrecognised -- stays
// correct no matter how a language list is reordered or edited.
int TranslateLanguageIndex(const std::vector<STranslateLanguage> &vLanguages, const char *pCode);

class CTranslate : public CComponent
{
	class CTranslateJob
	{
	public:
		std::unique_ptr<ITranslateBackend> m_pBackend = nullptr;
		// For chat translations
		CChat::CLine *m_pLine = nullptr;
		std::shared_ptr<CTranslateResponse> m_pTranslateResponse = nullptr;
		// For outgoing translations (translating our own message before sending it)
		bool m_IsOutgoing = false;
		int m_OutgoingTeam = 0;
		char m_aOutgoingOriginalText[256] = "";
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

	// If outgoing translation applies to pText (enabled, non-empty, not a
	// "/"-prefixed command), queues a translate job that sends the result
	// (or the original text if translation fails) as a chat message, and
	// returns true. Returns false if the caller should send pText itself --
	// mirrors CTClient::ChatDoSpecId's shape so chat.cpp can chain it as
	// another "else if" instead of a nested block.
	bool ChatDoTranslateOutgoing(int Team, const char *pText);
};

#endif
