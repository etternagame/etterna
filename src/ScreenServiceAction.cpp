#include "global.h"
#include "ScreenServiceAction.h"
#include "ThemeManager.h"
#include "Bookkeeper.h"
#include "Profile.h"
#include "ProfileManager.h"
#include "ScreenManager.h"
#include "RageFileManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "Song.h"
#include "MemoryCardManager.h"
#include "GameState.h"
#include "PlayerState.h"
#include "LocalizedString.h"
#include "StepMania.h"
#include "NotesLoaderSSC.h"

static LocalizedString BOOKKEEPING_DATA_CLEARED( "ScreenServiceAction", "Bookkeeping data cleared." );
static RString ClearBookkeepingData()
{
	BOOKKEEPER->ClearAll();
	BOOKKEEPER->WriteToDisk();
	return BOOKKEEPING_DATA_CLEARED.GetValue();
}

static LocalizedString MACHINE_STATS_CLEARED( "ScreenServiceAction", "Machine stats cleared." );
static LocalizedString MACHINE_EDITS_CLEARED( "ScreenServiceAction", "%d edits cleared, %d errors." );

static PlayerNumber GetFirstReadyMemoryCard()
{
	FOREACH_PlayerNumber( pn )
	{
		if( MEMCARDMAN->GetCardState(pn) != MemoryCardState_Ready )
			continue;	// skip

		if( !MEMCARDMAN->IsMounted(pn) )
			MEMCARDMAN->MountCard(pn);
		return pn;
	}

	return PLAYER_INVALID;
}

static LocalizedString MEMORY_CARD_EDITS_NOT_CLEARED	( "ScreenServiceAction", "Edits not cleared - No memory cards ready." );
static LocalizedString EDITS_CLEARED			( "ScreenServiceAction", "%d edits cleared, %d errors." );
static RString ClearMemoryCardEdits()
{
	PlayerNumber pn = GetFirstReadyMemoryCard();
	if( pn == PLAYER_INVALID )
		return MEMORY_CARD_EDITS_NOT_CLEARED.GetValue();

	int iNumAttempted = 0;
	int iNumSuccessful = 0;
	
	if( !MEMCARDMAN->IsMounted(pn) )
		MEMCARDMAN->MountCard(pn);

	RString sDir = MEM_CARD_MOUNT_POINT[pn] + (RString)PREFSMAN->m_sMemoryCardProfileSubdir + "/";
	vector<RString> vsEditFiles;
	GetDirListing( sDir+EDIT_STEPS_SUBDIR+"*.edit", vsEditFiles, false, true );
	FOREACH_CONST( RString, vsEditFiles, i )
	{
		iNumAttempted++;
		bool bSuccess = FILEMAN->Remove( *i );
		if( bSuccess )
			iNumSuccessful++;
	}

	MEMCARDMAN->UnmountCard(pn);

	return ssprintf(EDITS_CLEARED.GetValue(),iNumSuccessful,iNumAttempted-iNumSuccessful);
}


static LocalizedString STATS_NOT_SAVED			( "ScreenServiceAction", "Stats not saved - No memory cards ready." );
static LocalizedString MACHINE_STATS_SAVED		( "ScreenServiceAction", "Machine stats saved to P%d card." );
static LocalizedString ERROR_SAVING_MACHINE_STATS	( "ScreenServiceAction", "Error saving machine stats to P%d card." );
static LocalizedString STATS_NOT_LOADED		( "ScreenServiceAction", "Stats not loaded - No memory cards ready." );
static LocalizedString MACHINE_STATS_LOADED	( "ScreenServiceAction", "Machine stats loaded from P%d card." );
static LocalizedString THERE_IS_NO_PROFILE	( "ScreenServiceAction", "There is no machine profile on P%d card." );
static LocalizedString PROFILE_CORRUPT		( "ScreenServiceAction", "The profile on P%d card contains corrupt or tampered data." );

static void CopyEdits( const RString &sFromProfileDir, const RString &sToProfileDir, int &iNumSucceeded, int &iNumOverwritten, int &iNumIgnored, int &iNumErrored )
{
	iNumSucceeded = 0;
	iNumOverwritten = 0;
	iNumIgnored = 0;
	iNumErrored = 0;

	{
		RString sFromDir = sFromProfileDir + EDIT_STEPS_SUBDIR;
		RString sToDir = sToProfileDir + EDIT_STEPS_SUBDIR;

		vector<RString> vsFiles;
		GetDirListing( sFromDir+"*.edit", vsFiles, false, false );
		FOREACH_CONST( RString, vsFiles, i )
		{
			if( DoesFileExist(sToDir+*i) )
				iNumOverwritten++;
			bool bSuccess = FileCopy( sFromDir+*i, sToDir+*i );
			if( bSuccess )
				iNumSucceeded++;
			else
				iNumErrored++;
		}
	}
}

static LocalizedString EDITS_NOT_COPIED		( "ScreenServiceAction", "Edits not copied - No memory cards ready." );
static LocalizedString COPIED_TO_CARD		( "ScreenServiceAction", "Copied to P%d card:" );
static LocalizedString COPIED			( "ScreenServiceAction", "%d copied" );
static LocalizedString OVERWRITTEN		( "ScreenServiceAction", "%d overwritten" );
static LocalizedString ADDED			( "ScreenServiceAction", "%d added" );
static LocalizedString IGNORED			( "ScreenServiceAction", "%d ignored" );
static LocalizedString FAILED			( "ScreenServiceAction", "%d failed" );
static LocalizedString DELETED			( "ScreenServiceAction", "%d deleted" );

static RString CopyEdits( const RString &sFromProfileDir, const RString &sToProfileDir, const RString &sDisplayDir )
{
	int iNumSucceeded = 0;
	int iNumOverwritten = 0;
	int iNumIgnored = 0;
	int iNumErrored = 0;

	CopyEdits( sFromProfileDir, sToProfileDir, iNumSucceeded, iNumOverwritten, iNumIgnored, iNumErrored );

	vector<RString> vs;
	vs.push_back( sDisplayDir );
	vs.push_back( ssprintf( COPIED.GetValue(), iNumSucceeded ) + ", " + ssprintf( OVERWRITTEN.GetValue(), iNumOverwritten ) );
	if( iNumIgnored )
		vs.push_back( ssprintf( IGNORED.GetValue(), iNumIgnored ) );
	if( iNumErrored )
		vs.push_back( ssprintf( FAILED.GetValue(), iNumErrored ) );
	return join( "\n", vs );
}

static void SyncFiles( const RString &sFromDir, const RString &sToDir, const RString &sMask, int &iNumAdded, int &iNumDeleted, int &iNumOverwritten, int &iNumFailed )
{
	vector<RString> vsFilesSource;
	GetDirListing( sFromDir+sMask, vsFilesSource, false, false );

	vector<RString> vsFilesDest;
	GetDirListing( sToDir+sMask, vsFilesDest, false, false );

	vector<RString> vsToDelete;
	GetAsNotInBs( vsFilesDest, vsFilesSource, vsToDelete );

	for( unsigned i = 0; i < vsToDelete.size(); ++i )
	{
		RString sFile = sToDir + vsToDelete[i];
		LOG->Trace( "Delete \"%s\"", sFile.c_str() );

		if( FILEMAN->Remove(sFile) )
			++iNumDeleted;
		else
			++iNumFailed;
	}

	for( unsigned i = 0; i < vsFilesSource.size(); ++i )
	{
		RString sFileFrom = sFromDir + vsFilesSource[i];
		RString sFileTo = sToDir + vsFilesSource[i];
		LOG->Trace( "Copy \"%s\"", sFileFrom.c_str() );
		bool bOverwrite = DoesFileExist( sFileTo );
		bool bSuccess = FileCopy( sFileFrom, sFileTo );
		if( bSuccess )
		{
			if( bOverwrite )
				++iNumOverwritten;
			else
				++iNumAdded;
		}
		else
			++iNumFailed;
	}
	FILEMAN->FlushDirCache( sToDir );
}

static void SyncEdits( const RString &sFromDir, const RString &sToDir, int &iNumAdded, int &iNumDeleted, int &iNumOverwritten, int &iNumFailed )
{
	iNumAdded = 0;
	iNumDeleted = 0;
	iNumOverwritten = 0;
	iNumFailed = 0;

	SyncFiles( sFromDir + EDIT_STEPS_SUBDIR, sToDir + EDIT_STEPS_SUBDIR, "*.edit", iNumAdded, iNumDeleted, iNumOverwritten, iNumFailed );
}

static LocalizedString COPIED_FROM_CARD( "ScreenServiceAction", "Copied from P%d card:" );

static LocalizedString PREFERENCES_RESET( "ScreenServiceAction", "Preferences reset." );
static RString ResetPreferences()
{
	StepMania::ResetPreferences();
	return PREFERENCES_RESET.GetValue();
}



REGISTER_SCREEN_CLASS( ScreenServiceAction );
void ScreenServiceAction::BeginScreen()
{
	RString sActions = THEME->GetMetric(m_sName,"Actions");
	vector<RString> vsActions;
	split( sActions, ",", vsActions );

	vector<RString> vsResults;
	FOREACH( RString, vsActions, s )
	{
		RString (*pfn)() = NULL;

		if(	 *s == "ClearBookkeepingData" )			pfn = ClearBookkeepingData;
		else if( *s == "ClearMemoryCardEdits" )			pfn = ClearMemoryCardEdits;
		else if( *s == "ResetPreferences" )			pfn = ResetPreferences;
		
		ASSERT_M( pfn != NULL, *s );
		
		RString sResult = pfn();
		vsResults.push_back( sResult );
	}

	ScreenPrompt::SetPromptSettings( join( "\n", vsResults ), PROMPT_OK );

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
