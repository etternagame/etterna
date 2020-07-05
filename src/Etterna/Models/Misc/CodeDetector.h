/* CodeDetector - Uses InputQueue to detect input of codes. */

#ifndef CODE_DETECTOR_H
#define CODE_DETECTOR_H

#include "GameInput.h"

enum Code
{
	Code_PrevSteps1,
	Code_PrevSteps2,
	Code_NextSteps1,
	Code_NextSteps2,
	CODE_NEXT_SORT1,
	CODE_NEXT_SORT2,
	CODE_NEXT_SORT3,
	CODE_NEXT_SORT4,
	CODE_MODE_MENU1,
	CODE_MODE_MENU2,
	CODE_MIRROR,
	CODE_BACKWARDS,
	CODE_LEFT,
	CODE_RIGHT,
	CODE_SHUFFLE,
	CODE_SUPER_SHUFFLE,
	CODE_NEXT_SCROLL_SPEED,
	CODE_PREVIOUS_SCROLL_SPEED,
	CODE_REVERSE,
	CODE_MINES,
	CODE_HIDDEN,
	CODE_CANCEL_ALL,
	CODE_NEXT_GROUP,
	CODE_PREV_GROUP,
	CODE_SAVE_SCREENSHOT1,
	CODE_SAVE_SCREENSHOT2,
	CODE_CANCEL_ALL_PLAYER_OPTIONS,
	CODE_CLOSE_CURRENT_FOLDER,
	NUM_Code // leave this at the end
};

class CodeDetector
{
  public:
	static void RefreshCacheItems(
	  std::string sClass =
		""); // call this before checking codes, but call infrequently
	static bool EnteredPrevSteps(GameController controller);
	static bool EnteredNextSteps(GameController controller);
	static bool EnteredNextSort(GameController controller);
	static bool EnteredModeMenu(GameController controller);
	static bool DetectAndAdjustMusicOptions(GameController controller);
	static bool EnteredCode(GameController controller, Code code);
	static bool EnteredPrevGroup(GameController controller);
	static bool EnteredNextGroup(GameController controller);
	static bool EnteredCloseFolder(GameController controller);

	// todo: move to PlayerOptions.h -aj
	void ChangeScrollSpeed(GameController controller, bool bIncrement);
};

#endif
