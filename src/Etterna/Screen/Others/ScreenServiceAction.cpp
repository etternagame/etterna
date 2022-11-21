#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenServiceAction.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Globals/StepMania.h"
#include "Etterna/Singletons/ThemeManager.h"

static LocalizedString COPIED("ScreenServiceAction", "%d copied");
static LocalizedString OVERWRITTEN("ScreenServiceAction", "%d overwritten");
static LocalizedString ADDED("ScreenServiceAction", "%d added");
static LocalizedString IGNORED("ScreenServiceAction", "%d ignored");
static LocalizedString FAILED("ScreenServiceAction", "%d failed");
static LocalizedString DELETED("ScreenServiceAction", "%d deleted");

static LocalizedString PREFERENCES_RESET("ScreenServiceAction",
										 "Preferences reset.");
static std::string
ResetPreferences()
{
	StepMania::ResetPreferences();
	return PREFERENCES_RESET.GetValue();
}

REGISTER_SCREEN_CLASS(ScreenServiceAction);
void
ScreenServiceAction::BeginScreen()
{
	std::string sActions = THEME->GetMetric(m_sName, "Actions");
	std::vector<std::string> vsActions;
	split(sActions, ",", vsActions);

	std::vector<std::string> vsResults;
	for (auto& s : vsActions) {
		std::string (*pfn)() = nullptr;

		if (s == "ResetPreferences")
			pfn = ResetPreferences;

		ASSERT_M(pfn != nullptr, s);

		std::string sResult = pfn();
		vsResults.push_back(sResult);
	}

	ScreenPrompt::SetPromptSettings(join("\n", vsResults), PROMPT_OK);

	ScreenPrompt::BeginScreen();
}
