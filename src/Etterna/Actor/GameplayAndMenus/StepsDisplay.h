#ifndef StepsDisplay_H
#define StepsDisplay_H

#include "Etterna/Actor/Base/ActorFrame.h"
#include "Etterna/Actor/Base/AutoActor.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

class Steps;
class PlayerState;
/**
 * @brief A graphical representation of a Steps or a Trail.
 *
 * It has a difficulty number, meter, text, and an edit description. */
class StepsDisplay : public ActorFrame
{
  public:
	StepsDisplay();

	void Load(const std::string& sMetricsGroup,
			  const PlayerState* pPlayerState);

	StepsDisplay* Copy() const override;

	void SetFromGameState(PlayerNumber pn);
	void SetFromStepsTypeAndMeterAndDifficultyAndCourseType(StepsType st,
															int iMeter,
															Difficulty dc);
	void SetFromSteps(const Steps* pSteps);
	void Unset();
	int mypos = 0;
	// Lua
	void PushSelf(lua_State* L) override;

  private:
	struct SetParams
	{
		const Steps* pSteps;
		int iMeter;
		StepsType st; // pass because there may be a StepType icon
		Difficulty dc;
	};
	void SetInternal(const SetParams& params);

	std::string m_sMetricsGroup;

	AutoActor m_sprFrame;
	BitmapText m_textTicks; // 111100000
	BitmapText m_textMeter; // 3, 9
	/**
	 * @brief The description of the chart.
	 *
	 * This is meant to be separate from the author of the chart. */
	BitmapText m_textDescription;
	/** @brief The author of the chart. */
	BitmapText m_textAuthor;
	AutoActor m_sprStepsType;

	ThemeMetric<int> m_iNumTicks;
	ThemeMetric<int> m_iMaxTicks;
	ThemeMetric<bool> m_bShowTicks;
	ThemeMetric<bool> m_bShowMeter;
	ThemeMetric<bool> m_bShowDescription;
	ThemeMetric<bool> m_bShowCredit;
	ThemeMetric<bool> m_bShowStepsType;
	ThemeMetric<std::string> m_sZeroMeterString;
	ThemeMetric<std::string> m_sMeterFormatString;
};

#endif
