/* ComboGraph - A bar displaying the player's combo on Evaluation. */
#ifndef COMBO_GRAPH_H
#define COMBO_GRAPH_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class StageStats;
class PlayerStageStats;
class BitmapText;

class ComboGraph : public ActorFrame
{
  public:
	ComboGraph();
	void Load(const std::string& sMetricsGroup);
	void Set(const StageStats& s, const PlayerStageStats& pss);
	void SetWithoutStageStats(const PlayerStageStats& pss, const float fLastSecond);
	ComboGraph* Copy() const override;
	bool AutoLoadChildren() const override { return true; }

	// Commands
	void PushSelf(lua_State* L) override;

  private:
	ThemeMetric<float> BODY_WIDTH;
	ThemeMetric<float> BODY_HEIGHT;
	Actor* m_pBacking;
	Actor* m_pNormalCombo;
	Actor* m_pMaxCombo;
	BitmapText* m_pComboNumber;
};

#endif
