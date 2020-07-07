#ifndef SCREEN_EVALUATION_H
#define SCREEN_EVALUATION_H

#include "RageUtil/Sound/RageSound.h"
#include "ScreenWithMenuElements.h"

/**
 * @brief How many songs are shown at the end?
 *
 * In the traditional summary, only the last three stages are shown.
 * If any extra stages are passed, those get shown as well. */
const int MAX_SONGS_TO_SHOW = 5;
/** @brief The different judgment lines shown. */

/** @brief Shows the player their score after gameplay has ended. */
class ScreenEvaluation : public ScreenWithMenuElements
{
  public:
	ScreenEvaluation();
	~ScreenEvaluation() override;
	void Init() override;
	bool Input(const InputEventPlus& input) override;
	void HandleScreenMessage(const ScreenMessage& SM) override;
	ScreenType GetScreenType() const override { return evaluation; }
	bool MenuBack(const InputEventPlus& input) override;
	bool MenuStart(const InputEventPlus& input) override;
	void PushSelf(lua_State* L) override;
	StageStats* GetStageStats() { return m_pStageStats; }

  protected:
	void HandleMenuStart();

	StageStats* m_pStageStats;

	RageSound m_soundStart; // sound played if the player passes or fails

	/** @brief Did a player save a screenshot of their score? */
	bool m_bSavedScreenshot;
};

#endif
