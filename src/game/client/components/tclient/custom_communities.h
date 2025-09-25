#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_CUSTOM_COMMUNITIES_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_CUSTOM_COMMUNITIES_H

#include <game/client/component.h>

#include <engine/shared/console.h>
#include <engine/shared/http.h>

class CCustomCommunities : public CComponent
{
private:
	std::shared_ptr<CHttpRequest> m_pCustomCommunitiesDDNetInfoTask = nullptr;
	json_value *m_pCustomCommunitiesDDNetInfo = nullptr;
	void StartLoadCustomCommunitiesDDNetInfo();
	void LoadCustomCommunitiesDDNetInfo();
	void CustomCommunitiesFunction(std::vector<json_value *> &vCommunities);

public:
	void OnConsoleInit() override;
	void OnRender() override;
	int Sizeof() const override { return sizeof(*this); }
	~CCustomCommunities() override;
};

#endif
