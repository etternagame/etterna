#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "ScreenSaveSync.h"
#include "Etterna/Models/Songs/Song.h"

static LocalizedString CHANGED_TIMING_OF("ScreenSaveSync",
										 "You have changed the timing of");
static LocalizedString WOULD_YOU_LIKE_TO_SAVE(
  "ScreenSaveSync",
  "Would you like to save these changes?");
static LocalizedString CHOOSING_NO_WILL_DISCARD(
  "ScreenSaveSync",
  "Choosing NO will discard your changes.");
static std::string
GetPromptText()
{
	std::string s;

	{
		vector<std::string> vs;
		AdjustSync::GetSyncChangeTextGlobal(vs);
		if (!vs.empty())
			s += join("\n", vs) + "\n\n";
	}

	{
		vector<std::string> vs;
		AdjustSync::GetSyncChangeTextSong(vs);
		if (!vs.empty()) {
			s += ssprintf(CHANGED_TIMING_OF.GetValue() + "\n"
														 "%s:\n"
														 "\n",
						  GAMESTATE->m_pCurSong->GetDisplayFullTitle().c_str());

			s += join("\n", vs) + "\n\n";
		}
	}

	s += WOULD_YOU_LIKE_TO_SAVE.GetValue() + "\n" +
		 CHOOSING_NO_WILL_DISCARD.GetValue();
	return s;
}

static void
SaveSyncChanges(void* pThrowAway)
{
	AdjustSync::SaveSyncChanges();
}

static void
RevertSyncChanges(void* pThrowAway)
{
	AdjustSync::RevertSyncChanges();
}

void
ScreenSaveSync::Init()
{
	ScreenPrompt::Init();

	ScreenPrompt::SetPromptSettings(GetPromptText(),
									PROMPT_YES_NO,
									ANSWER_YES,
									SaveSyncChanges,
									RevertSyncChanges,
									nullptr);
}

void
ScreenSaveSync::PromptSaveSync(const ScreenMessage& sm)
{
	ScreenPrompt::Prompt(sm,
						 GetPromptText(),
						 PROMPT_YES_NO,
						 ANSWER_YES,
						 SaveSyncChanges,
						 RevertSyncChanges,
						 nullptr);
}
