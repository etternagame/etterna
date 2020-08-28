#include "Etterna/Globals/global.h"
#include "CodeDetector.h"
#include "Game.h"
#include "Etterna/Singletons/GameState.h"
#include "InputEventPlus.h"
#include "Etterna/Singletons/InputQueue.h"
#include "PlayerOptions.h"
#include "PlayerState.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ThemeManager.h"

const char* CodeNames[] = {
	"PrevSteps1",
	"PrevSteps2",
	"NextSteps1",
	"NextSteps2",
	"NextSort1",
	"NextSort2",
	"NextSort3",
	"NextSort4",
	"ModeMenu1",
	"ModeMenu2",
	"Mirror",
	"Backwards",
	"Left",
	"Right",
	"Shuffle",
	"SuperShuffle",
	"NextScrollSpeed",
	"PreviousScrollSpeed",
	"Reverse",
	"Mines",
	"Hidden",
	"CancelAll",
	"NextGroup",
	"PrevGroup",
	"SaveScreenshot1",
	"SaveScreenshot2",
	"CancelAllPlayerOptions",
	"CloseCurrentFolder",
};
XToString(Code);

static InputQueueCode g_CodeItems[NUM_Code];

bool
CodeDetector::EnteredCode(GameController controller, Code code)
{
	return g_CodeItems[code].EnteredCode(controller);
}

void
CodeDetector::RefreshCacheItems(std::string sClass)
{
	if (sClass == "")
		sClass = "CodeDetector";
	FOREACH_ENUM(Code, c)
	{
		auto& item = g_CodeItems[c];
		const auto sCodeName = CodeToString(c);
		const auto sButtonsNames = THEME->GetMetric(sClass, sCodeName);

		item.Load(sButtonsNames);
	}
}

bool
CodeDetector::EnteredCloseFolder(GameController controller)
{
	return EnteredCode(controller, CODE_CLOSE_CURRENT_FOLDER);
}

bool
CodeDetector::EnteredNextGroup(GameController controller)
{
	return EnteredCode(controller, CODE_NEXT_GROUP);
}

bool
CodeDetector::EnteredPrevGroup(GameController controller)
{
	return EnteredCode(controller, CODE_PREV_GROUP);
}

bool
CodeDetector::EnteredPrevSteps(GameController controller)
{
	return EnteredCode(controller, Code_PrevSteps1) ||
		   EnteredCode(controller, Code_PrevSteps2);
}

bool
CodeDetector::EnteredNextSteps(GameController controller)
{
	return EnteredCode(controller, Code_NextSteps1) ||
		   EnteredCode(controller, Code_NextSteps2);
}

bool
CodeDetector::EnteredNextSort(GameController controller)
{
	return EnteredCode(controller, CODE_NEXT_SORT1) ||
		   EnteredCode(controller, CODE_NEXT_SORT2) ||
		   EnteredCode(controller, CODE_NEXT_SORT3) ||
		   EnteredCode(controller, CODE_NEXT_SORT4);
}

bool
CodeDetector::EnteredModeMenu(GameController controller)
{
	return EnteredCode(controller, CODE_MODE_MENU1) ||
		   EnteredCode(controller, CODE_MODE_MENU2);
}

#define TOGGLE(v, a, b)                                                        \
	if ((v) != (a))                                                            \
		(v) = a;                                                               \
	else                                                                       \
		(v) = b;
#define FLOAT_TOGGLE(v)                                                        \
	if ((v) != 1.f)                                                            \
		(v) = 1.f;                                                             \
	else                                                                       \
		(v) = 0.f;
// XXX: Read the metrics file instead!
// Using this can give us unlisted scroll speeds on the Options screen.
// Zmey: done.
// AJ: thanks Zmey! :D
#define INCREMENT_SCROLL_SPEED(s)                                              \
	((s) == 0.5f)                                                              \
	  ? (s) = 0.75f                                                            \
	  : ((s) == 0.75f)                                                         \
		  ? (s) = 1.0f                                                         \
		  : ((s) == 1.0f)                                                      \
			  ? (s) = 1.5f                                                     \
			  : ((s) == 1.5f)                                                  \
				  ? (s) = 2.0f                                                 \
				  : ((s) == 2.0f)                                              \
					  ? (s) = 3.0f                                             \
					  : ((s) == 3.0f)                                          \
						  ? (s) = 4.0f                                         \
						  : ((s) == 4.0f)                                      \
							  ? (s) = 5.0f                                     \
							  : ((s) == 5.0f) ? (s) = 8.0f : (s) = 0.5f;
#define DECREMENT_SCROLL_SPEED(s)                                              \
	((s) == 0.75f)                                                             \
	  ? (s) = 0.5f                                                             \
	  : ((s) == 1.0f)                                                          \
		  ? (s) = 0.75f                                                        \
		  : ((s) == 1.5f)                                                      \
			  ? (s) = 1.0f                                                     \
			  : ((s) == 2.0f)                                                  \
				  ? (s) = 1.5f                                                 \
				  : ((s) == 3.0f)                                              \
					  ? (s) = 2.0f                                             \
					  : ((s) == 4.0f)                                          \
						  ? (s) = 3.0f                                         \
						  : ((s) == 5.0f)                                      \
							  ? (s) = 4.0f                                     \
							  : ((s) == 8.0f) ? (s) = 4.0f : (s) = 8.0f;

// from Pumpmania
void
CodeDetector::ChangeScrollSpeed(GameController controller, bool bIncrement)
{
// this doesn't compile, hence the #if 0 below.
// also I bet this code actually belongs in PlayerOptions.cpp
// on further inspection. -aj
// p.s. it's m_fScrollSpeed you'll want to mess with.
#if 0
	// opt = PlayerOptions
	// setup
	PlayerNumber pn = INPUTMAPPER->ControllerToPlayerNumber( controller );
	PlayerOptions po = GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred();

	/* what this code seems to be doing is:
	 * 1) getting the Speed line from the theme
	 * 2) throwing it into a vector
	 * 3) getting the current scroll speed (fallback on 1x)
	 * 4) loop through the entries until you find the current mod
	 * 5) check if it's increment/decrement, act accordingly
	 * 6) set mod and return.
	 * 7) "Current SpeedMod not found in Theme, revert to default"
	 * although I'd rather have it move to the next possible value at
	 * that point. If it's invalid, then revert to the default.
	 */

	OptionRowData row;
	OptionRowHandler hand;

	std::string sTitleOut;
	ScreenOptionsMaster::SetList( row, hand, "Speed", sTitleOut );

	vector<ModeChoice>& entries = hand.ListEntries;

	std::string sScrollSpeed = po.GetScrollSpeedAsString();
	if (sScrollSpeed.empty())
		sScrollSpeed = "1x";

	for ( vector<ModeChoice>::iterator it = entries.begin(); it != entries.end(); ++it )
	{
		ModeChoice& modeChoice = *it;
		if ( modeChoice.m_sModifiers == sScrollSpeed ) {
			if (bIncrement) {
				if ( &modeChoice == &entries.back() )
					po.FromString( entries.front().m_sModifiers );
				else
					po.FromString( (++it)->m_sModifiers );
			} else { // Decrement
				if ( &modeChoice == &entries.front() )
					po.FromString( entries.back().m_sModifiers );
				else
					po.FromString( (--it)->m_sModifiers );
			}
			return;
		}
	}
	// Current SpeedMod not found in Theme, revert to default:
	ModeChoice& defaultChoice = hand.Default;
	po.FromString(defaultChoice.m_sModifiers);
#endif
}

bool
CodeDetector::DetectAndAdjustMusicOptions(GameController controller)
{
	const auto pn = INPUTMAPPER->ControllerToPlayerNumber(controller);

	if (pn >= NUM_PlayerNumber) {
		Locator::getLogger()->warn("Invalid controller player number");
		return false;
	}

	for (int c = CODE_MIRROR; c <= CODE_CANCEL_ALL; c++) {
		const auto code = static_cast<Code>(c);

		auto po = GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred();

		if (EnteredCode(controller, code)) {
			switch (code) {
				case CODE_MIRROR:
					po.ToggleOneTurn(PlayerOptions::TURN_MIRROR);
					break;
				case CODE_BACKWARDS:
					po.ToggleOneTurn(PlayerOptions::TURN_BACKWARDS);
					break;
				case CODE_LEFT:
					po.ToggleOneTurn(PlayerOptions::TURN_LEFT);
					break;
				case CODE_RIGHT:
					po.ToggleOneTurn(PlayerOptions::TURN_RIGHT);
					break;
				case CODE_SHUFFLE:
					po.ToggleOneTurn(PlayerOptions::TURN_SHUFFLE);
					break;
				case CODE_SUPER_SHUFFLE:
					po.ToggleOneTurn(PlayerOptions::TURN_SUPER_SHUFFLE);
					break;
				case CODE_NEXT_SCROLL_SPEED:
					INCREMENT_SCROLL_SPEED(po.m_fScrollSpeed);
					break;
				case CODE_PREVIOUS_SCROLL_SPEED:
					DECREMENT_SCROLL_SPEED(po.m_fScrollSpeed);
					break;
				case CODE_REVERSE:
					po.NextScroll();
					break;
				case CODE_MINES:
					TOGGLE(po.m_bTransforms[PlayerOptions::TRANSFORM_NOMINES],
						   true,
						   false);
					break;
				case CODE_CANCEL_ALL:
					GAMESTATE->GetDefaultPlayerOptions(po);
					break;
				case CODE_HIDDEN:
					ZERO(po.m_fAppearances);
					po.m_fAppearances[PlayerOptions::APPEARANCE_HIDDEN] = 1;
					break;
				default:
					break;
			}

			GAMESTATE->m_pPlayerState->m_PlayerOptions.Assign(
			  ModsLevel_Preferred, po);

			return true; // don't check any more
		}
	}

	return false;
}
