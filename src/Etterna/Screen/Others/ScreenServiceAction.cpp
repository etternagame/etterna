#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/NoteLoaders/NotesLoaderSSC.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
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
CopyEdits(const RString& sFromProfileDir,
		  const RString& sToProfileDir,
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
		RString sFromDir = sFromProfileDir + EDIT_STEPS_SUBDIR;
		RString sToDir = sToProfileDir + EDIT_STEPS_SUBDIR;

		vector<RString> vsFiles;
		GetDirListing(sFromDir + "*.edit", vsFiles, false, false);
		FOREACH_CONST(RString, vsFiles, i)
		{
			if (DoesFileExist(sToDir + *i))
				iNumOverwritten++;
			bool bSuccess = FileCopy(sFromDir + *i, sToDir + *i);
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
static LocalizedString IGNORED_STR("ScreenServiceAction", "%d ignored");
static LocalizedString FAILED_STR("ScreenServiceAction", "%d failed");
static LocalizedString DELETED("ScreenServiceAction", "%d deleted");

static RString
CopyEdits(const RString& sFromProfileDir,
		  const RString& sToProfileDir,
		  const RString& sDisplayDir)
{
	int iNumSucceeded = 0;
	int iNumOverwritten = 0;
	int iNumIgnored = 0;
	int iNumErrored = 0;

	CopyEdits(sFromProfileDir,
			  sToProfileDir,
			  iNumSucceeded,
			  iNumOverwritten,
			  iNumIgnored,
			  iNumErrored);

	vector<RString> vs;
	vs.push_back(sDisplayDir);
	vs.push_back(ssprintf(COPIED.GetValue(), iNumSucceeded) + ", " +
				 ssprintf(OVERWRITTEN.GetValue(), iNumOverwritten));
	if (iNumIgnored)
		vs.push_back(ssprintf(IGNORED_STR.GetValue(), iNumIgnored));
	if (iNumErrored)
		vs.push_back(ssprintf(FAILED_STR.GetValue(), iNumErrored));
	return join("\n", vs);
}

static void
SyncFiles(const RString& sFromDir,
		  const RString& sToDir,
		  const RString& sMask,
		  int& iNumAdded,
		  int& iNumDeleted,
		  int& iNumOverwritten,
		  int& iNumFailed)
{
	vector<RString> vsFilesSource;
	GetDirListing(sFromDir + sMask, vsFilesSource, false, false);

	vector<RString> vsFilesDest;
	GetDirListing(sToDir + sMask, vsFilesDest, false, false);

	vector<RString> vsToDelete;
	GetAsNotInBs(vsFilesDest, vsFilesSource, vsToDelete);

	for (unsigned i = 0; i < vsToDelete.size(); ++i) {
		RString sFile = sToDir + vsToDelete[i];
		LOG->Trace("Delete \"%s\"", sFile.c_str());

		if (FILEMAN->Remove(sFile))
			++iNumDeleted;
		else
			++iNumFailed;
	}

	for (unsigned i = 0; i < vsFilesSource.size(); ++i) {
		RString sFileFrom = sFromDir + vsFilesSource[i];
		RString sFileTo = sToDir + vsFilesSource[i];
		LOG->Trace("Copy \"%s\"", sFileFrom.c_str());
		bool bOverwrite = DoesFileExist(sFileTo);
		bool bSuccess = FileCopy(sFileFrom, sFileTo);
		if (bSuccess) {
			if (bOverwrite)
				++iNumOverwritten;
			else
				++iNumAdded;
		} else
			++iNumFailed;
	}
	FILEMAN->FlushDirCache(sToDir);
}

static void
SyncEdits(const RString& sFromDir,
		  const RString& sToDir,
		  int& iNumAdded,
		  int& iNumDeleted,
		  int& iNumOverwritten,
		  int& iNumFailed)
{
	iNumAdded = 0;
	iNumDeleted = 0;
	iNumOverwritten = 0;
	iNumFailed = 0;

	SyncFiles(sFromDir + EDIT_STEPS_SUBDIR,
			  sToDir + EDIT_STEPS_SUBDIR,
			  "*.edit",
			  iNumAdded,
			  iNumDeleted,
			  iNumOverwritten,
			  iNumFailed);
}

static LocalizedString COPIED_FROM_CARD("ScreenServiceAction",
										"Copied from P%d card:");

static LocalizedString PREFERENCES_RESET("ScreenServiceAction",
										 "Preferences reset.");
static RString
ResetPreferences()
{
	StepMania::ResetPreferences();
	return PREFERENCES_RESET.GetValue();
}

REGISTER_SCREEN_CLASS(ScreenServiceAction);
void
ScreenServiceAction::BeginScreen()
{
	RString sActions = THEME->GetMetric(m_sName, "Actions");
	vector<RString> vsActions;
	split(sActions, ",", vsActions);

	vector<RString> vsResults;
	FOREACH(RString, vsActions, s)
	{
		RString (*pfn)() = NULL;

		if (*s == "ResetPreferences")
			pfn = ResetPreferences;

		ASSERT_M(pfn != NULL, *s);

		RString sResult = pfn();
		vsResults.push_back(sResult);
	}

	ScreenPrompt::SetPromptSettings(join("\n", vsResults), PROMPT_OK);

	ScreenPrompt::BeginScreen();
}

/*
 * (c) 2001-2005 Chris Danford
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
