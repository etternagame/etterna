/* ModIconRow - Shows a row of ModIcons. */

#ifndef ModIconRow_H
#define ModIconRow_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "ModIcon.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
class PlayerOptions;
struct lua_State;

class ModIconRow : public ActorFrame
{
  public:
	ModIconRow();
	~ModIconRow() override;

	void Load(const std::string& sMetricsGroup, PlayerNumber pn);

	ModIconRow* Copy() const override;
	void SetFromGameState();

	void HandleMessage(const Message& msg) override;

	// Commands
	void PushSelf(lua_State* L) override;

  protected:
	std::string m_sMetricsGroup;
	PlayerNumber m_pn;

	ThemeMetric<float> SPACING_X;
	ThemeMetric<float> SPACING_Y;
	ThemeMetric<int> NUM_OPTION_ICONS;
	ThemeMetric<std::string> OPTION_ICON_METRICS_GROUP;

	vector<ModIcon*> m_vpModIcon;
};

#endif
