#ifndef GRAPH_DISPLAY_H
#define GRAPH_DISPLAY_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"

class StageStats;
class PlayerStageStats;
class GraphLine;
class GraphBody;
/** @brief A graph of the player's life over the course of Gameplay, used on
 * Evaluation. */
class GraphDisplay : public ActorFrame
{
  public:
	GraphDisplay();
	~GraphDisplay() override;
	GraphDisplay* Copy() const override;

	void Load(const std::string& sMetricsGroup);
	void Set(const StageStats& ss, const PlayerStageStats& s);

	void SetWithoutStageStats(const PlayerStageStats& pss,
							  const float fTotalStepSeconds);

	// Lua
	void PushSelf(lua_State* L) override;

  private:
	void UpdateVerts();

	vector<float> m_Values;

	RectF m_quadVertices;

	vector<Actor*> m_vpSongBoundaries;
	AutoActor m_sprBarely;
	AutoActor m_sprBacking;
	AutoActor m_sprSongBoundary;

	GraphLine* m_pGraphLine;
	GraphBody* m_pGraphBody;
};

#endif
