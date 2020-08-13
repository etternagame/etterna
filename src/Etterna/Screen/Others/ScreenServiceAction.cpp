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

static LocalizedString EDITS_CLEARED("ScreenServiceAction",
									 "%d edits cleared, %d errors.");
static LocalizedString STATS_NOT_SAVED(
  "ScreenServiceAction",
  "Stats not saved - No memory cards ready.");
static LocalizedString STATS_NOT_LOADED(
  "ScreenServiceAction",
  "Stats not loaded - No memory cards ready.");
static LocalizedString THERE_IS_NO_PROFILE(
  "ScreenServiceAction",
  "There is no machine profile on P%d card.");
static LocalizedString PROFILE_CORRUPT(
  "ScreenServiceAction",
  "The profile on P%d card contains corrupt or tampered data.");

static void
CopyEdits(const std::string& sFromProfileDir,
		  const std::string& sToProfileDir,
		  int& iNumSucceeded,
		  int& iNumOverwritten,
		  int& iNumIgnored,
		  int& iNumErrored)
{
	iNumSucceeded = 0;
	iNumOverwritten = 0;
	iNumIgnored = 0;
	iNumErrored = 0;

	{
		std::string sFromDir = sFromProfileDir + EDIT_STEPS_SUBDIR;
		std::string sToDir = sToProfileDir + EDIT_STEPS_SUBDIR;

		vector<std::string> vsFiles;
		GetDirListing(sFromDir + "*.edit", vsFiles, false, false);
		for (auto& i : vsFiles) {
			if (DoesFileExist(sToDir + i))
				iNumOverwritten++;
			bool bSuccess = FileCopy(sFromDir + i, sToDir + i);
			if (bSuccess)
				iNumSucceeded++;
			else
				iNumErrored++;
		}
	}
}

static LocalizedString EDITS_NOT_COPIED(
  "ScreenServiceAction",
  "Edits not copied - No memory cards ready.");
static LocalizedString COPIED_TO_CARD("ScreenServiceAction",
									  "Copied to P%d card:");
static LocalizedString COPIED("ScreenServiceAction", "%d copied");
static LocalizedString OVERWRITTEN("ScreenServiceAction", "%d overwritten");
static LocalizedString ADDED("ScreenServiceAction", "%d added");
static LocalizedString IGNORED("ScreenServiceAction", "%d ignored");
static LocalizedString FAILED("ScreenServiceAction", "%d failed");
static LocalizedString DELETED("ScreenServiceAction", "%d deleted");

static LocalizedString COPIED_FROM_CARD("ScreenServiceAction",
										"Copied from P%d card:");

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
	vector<std::string> vsActions;
	split(sActions, ",", vsActions);

	vector<std::string> vsResults;
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
