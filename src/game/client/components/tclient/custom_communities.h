#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_CUSTOM_COMMUNITIES_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_CUSTOM_COMMUNITIES_H

#include <engine/shared/console.h>
#include <engine/shared/http.h>

#include <game/client/component.h>

class CCustomCommunities : public CComponent
{
private:
	std::shared_ptr<CHttpRequest> m_pCustomCommunitiesDDNetInfoTask = nullptr;
	json_value *m_pCustomCommunitiesDDNetInfo = nullptr;
	void DownloadCustomCommunitiesDDNetInfo();
	void LoadCustomCommunitiesDDNetInfo();

public:
	void OnInit() override;
	void OnConsoleInit() override;
	void OnRender() override;
	int Sizeof() const override { return sizeof(*this); }
	~CCustomCommunities() override;
};

#endif
