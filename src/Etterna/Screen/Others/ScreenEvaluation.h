#ifndef SCREEN_EVALUATION_H
#define SCREEN_EVALUATION_H

#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Actor/Base/RollingNumbers.h"
#include "ScreenWithMenuElements.h"
#include "Etterna/Actor/Base/Sprite.h"
#include "Etterna/Models/Misc/ThemeMetric.h"

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
	void HandleScreenMessage(ScreenMessage SM) override;
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

/**
 * @file
 * @author Chris Danford (c) 2001-2004
 * @section LICENSE
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
