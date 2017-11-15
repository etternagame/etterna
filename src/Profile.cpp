#include "global.h"
#include "Profile.h"
#include "RageUtil.h"
#include "PrefsManager.h"
#include "XmlFile.h"
#include "IniFile.h"
#include "GameManager.h"
#include "GameState.h"
#include "RageLog.h"
#include "Song.h"
#include "SongManager.h"
#include "Steps.h"
#include "ThemeManager.h"
#include "CryptManager.h"
#include "ProfileManager.h"
#include "RageFile.h"
#include "RageFileDriverDeflate.h"
#include "RageFileManager.h"
#include "LuaManager.h"
#include "XmlFile.h"
#include "XmlFileUtil.h"
#include "Foreach.h"
#include "Game.h"
#include "CharacterManager.h"
#include "Character.h"
#include "MinaCalc.h"
#include "NoteData.h"
#include "ScoreManager.h"
#include <algorithm>
#include "ScreenManager.h"

const RString STATS_XML            = "Stats.xml";
const RString STATS_XML_GZ         = "Stats.xml.gz";
const RString ETT_XML			   = "Etterna.xml";
const RString ETT_XML_GZ		   = "Etterna.xml.gz";
/** @brief The filename for where one can edit their personal profile information. */
const RString EDITABLE_INI         = "Editable.ini";
/** @brief A tiny file containing the type and list priority. */
const RString TYPE_INI             = "Type.ini";
/** @brief The filename containing the signature for STATS_XML's signature. */
const RString DONT_SHARE_SIG       = "DontShare.sig";
const RString PUBLIC_KEY_FILE      = "public.key";
const RString SCREENSHOTS_SUBDIR   = "Screenshots/";
const RString EDIT_STEPS_SUBDIR    = "Edits/";
//const RString UPLOAD_SUBDIR         = "Upload/";
const RString RIVAL_SUBDIR         = "Rivals/";

ThemeMetric<bool> SHOW_COIN_DATA( "Profile", "ShowCoinData" );
static Preference<bool> g_bProfileDataCompress( "ProfileDataCompress", false );
#define GUID_SIZE_BYTES 8

#define MAX_EDITABLE_INI_SIZE_BYTES			2*1024		// 2KB
#define MAX_PLAYER_STATS_XML_SIZE_BYTES	\
	400 /* Songs */						\
	* 5 /* Steps per Song */			\
	* 5 /* HighScores per Steps */		\
	* 1024 /* size in bytes of a HighScores XNode */

const int DEFAULT_WEIGHT_POUNDS	= 120;
const float DEFAULT_BIRTH_YEAR= 1995;

#if defined(_MSC_VER)
#pragma warning (disable : 4706) // assignment within conditional expression
#endif

static const char* ProfileTypeNames[] = {
	"Guest",
	"Normal",
	"Test",
};
XToString(ProfileType);
StringToX(ProfileType);
LuaXType(ProfileType);


int Profile::HighScoresForASong::GetNumTimesPlayed() const
{
	int iCount = 0;
	FOREACHM_CONST( StepsID, HighScoresForASteps, m_StepsHighScores, i )
	{
		iCount += i->second.hsl.GetNumTimesPlayed();
	}
	return iCount;
}

void Profile::InitEditableData()
{
	m_sDisplayName = "";
	m_sCharacterID = "";
	m_sLastUsedHighScoreName = "";
}

void Profile::ClearStats()
{
	// don't reset the Guid
	RString sGuid = m_sGuid;
	InitAll();
	m_sGuid = sGuid;
}

RString Profile::MakeGuid()
{
	RString s;
	s.reserve( GUID_SIZE_BYTES*2 );
	unsigned char buf[GUID_SIZE_BYTES];
	CryptManager::GetRandomBytes( buf, GUID_SIZE_BYTES );
	for( unsigned i=0; i<GUID_SIZE_BYTES; i++ )
		s += ssprintf( "%02x", buf[i] );
	return s;
}

void Profile::InitGeneralData()
{
	m_sGuid = MakeGuid();

	m_SortOrder = SortOrder_Invalid;
	m_LastDifficulty = Difficulty_Invalid;
	m_LastStepsType = StepsType_Invalid;
	m_lastSong.Unset();
	m_iCurrentCombo = 0;
	m_iTotalSessions = 0;
	m_iTotalSessionSeconds = 0;
	m_iTotalGameplaySeconds = 0;
	m_iTotalDancePoints = 0;
	m_iNumExtraStagesPassed = 0;
	m_iNumExtraStagesFailed = 0;
	m_iNumToasties = 0;
	m_sLastPlayedMachineGuid = "";
	m_LastPlayedDate.Init();
	m_iTotalTapsAndHolds = 0;
	m_iTotalJumps = 0;
	m_iTotalHolds = 0;
	m_iTotalRolls = 0;
	m_iTotalMines = 0;
	m_iTotalHands = 0;
	m_iTotalLifts = 0;
	m_fPlayerRating = 0.f;

	FOREACH_ENUM( PlayMode, i )
		m_iNumSongsPlayedByPlayMode[i] = 0;
	m_iNumSongsPlayedByStyle.clear();
	FOREACH_ENUM( Difficulty, i )
		m_iNumSongsPlayedByDifficulty[i] = 0;
	for( int i=0; i<MAX_METER+1; i++ )
		m_iNumSongsPlayedByMeter[i] = 0;
	m_iNumTotalSongsPlayed = 0;
	ZERO( m_iNumStagesPassedByPlayMode );
	ZERO( m_iNumStagesPassedByGrade );
	ZERO( m_fPlayerSkillsets );

	m_UserTable.Unset();
}

void Profile::InitSongScores()
{
	m_SongHighScores.clear();
}

void Profile::InitCategoryScores()
{
	FOREACH_ENUM( StepsType,st )
		FOREACH_ENUM( RankingCategory,rc )
			m_CategoryHighScores[st][rc].Init();
}

void Profile::InitScreenshotData()
{
	m_vScreenshots.clear();
}

RString Profile::GetDisplayNameOrHighScoreName() const
{
	if( !m_sDisplayName.empty() )
		return m_sDisplayName;
	else if( !m_sLastUsedHighScoreName.empty() )
		return m_sLastUsedHighScoreName;
	else
		return RString();
}

Character *Profile::GetCharacter() const
{
	vector<Character*> vpCharacters;
	CHARMAN->GetCharacters( vpCharacters );
	FOREACH_CONST( Character*, vpCharacters, c )
	{
		if( (*c)->m_sCharacterID.CompareNoCase(m_sCharacterID)==0 )
			return *c;
	}
	return CHARMAN->GetDefaultCharacter();
}

void Profile::SetCharacter(const RString &sCharacterID)
{
	if(CHARMAN->GetCharacterFromID(sCharacterID))
		m_sCharacterID = sCharacterID;
}

int Profile::GetTotalNumSongsPassed() const
{
	int iTotal = 0;
	FOREACH_ENUM( PlayMode, i )
		iTotal += m_iNumStagesPassedByPlayMode[i];
	return iTotal;
}

int Profile::GetTotalStepsWithTopGrade( StepsType st, Difficulty d, Grade g ) const
{
	int iCount = 0;

	FOREACH_CONST( Song*, SONGMAN->GetAllSongs(), pSong )
	{
		FOREACH_CONST( Steps*, (*pSong)->GetAllSteps(), pSteps )
		{
			if( (*pSteps)->m_StepsType != st )
				continue;	// skip

			if( (*pSteps)->GetDifficulty() != d )
				continue;	// skip

			const HighScoreList &hsl = GetStepsHighScoreList( *pSong, *pSteps );
			if( hsl.vHighScores.empty() )
				continue;	// skip

			if( hsl.vHighScores[0].GetGrade() == g )
				iCount++;
		}
	}

	return iCount;
}

float Profile::GetSongsPossible( StepsType st, Difficulty dc ) const
{
	int iTotalSteps = 0;

	// add steps high scores
	const vector<Song*> &vSongs = SONGMAN->GetAllSongs();
	for( unsigned i=0; i<vSongs.size(); i++ )
	{
		Song* pSong = vSongs[i];

		vector<Steps*> vSteps = pSong->GetAllSteps();
		for( unsigned j=0; j<vSteps.size(); j++ )
		{
			Steps* pSteps = vSteps[j];
			
			if( pSteps->m_StepsType != st )
				continue;	// skip

			if( pSteps->GetDifficulty() != dc )
				continue;	// skip

			iTotalSteps++;
		}
	}

	return static_cast<float>(iTotalSteps);
}

float Profile::GetSongsActual( StepsType st, Difficulty dc ) const
{
	CHECKPOINT_M( ssprintf("Profile::GetSongsActual(%d,%d)",st,dc) );

	float fTotalPercents = 0;

	// add steps high scores
	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{
		const SongID &id = i->first;
		Song* pSong = id.ToSong();

		CHECKPOINT_M( ssprintf("Profile::GetSongsActual: %p", pSong) );

		// If the Song isn't loaded on the current machine, then we can't 
		// get radar values to compute dance points.
		if( pSong == NULL )
			continue;

		CHECKPOINT_M( ssprintf("Profile::GetSongsActual: song %s", pSong->GetSongDir().c_str()) );
		const HighScoresForASong &hsfas = i->second;

		FOREACHM_CONST( StepsID, HighScoresForASteps, hsfas.m_StepsHighScores, j )
		{
			const StepsID &sid = j->first;
			Steps* pSteps = sid.ToSteps( pSong, true );
			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: song %p, steps %p", pSong, pSteps) );

			// If the Steps isn't loaded on the current machine, then we can't 
			// get radar values to compute dance points.
			if( pSteps == NULL )
				continue;

			if( pSteps->m_StepsType != st )
				continue;

			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: n %s = %p", sid.ToString().c_str(), pSteps) );
			if( pSteps->GetDifficulty() != dc )
			{
				continue;	// skip
			}
			
			CHECKPOINT_M( ssprintf("Profile::GetSongsActual: difficulty %s is correct", DifficultyToString(dc).c_str()));

			const HighScoresForASteps& h = j->second;
			const HighScoreList& hsl = h.hsl;

			fTotalPercents += hsl.GetTopScore().GetPercentDP();
		}
	}

	return fTotalPercents;
}

float Profile::GetSongsPercentComplete( StepsType st, Difficulty dc ) const
{
	return GetSongsActual(st,dc) / GetSongsPossible(st,dc);
}

int Profile::GetSongNumTimesPlayed( const Song* pSong ) const
{
	SongID songID;
	songID.FromSong( pSong );
	return GetSongNumTimesPlayed( songID );
}

int Profile::GetSongNumTimesPlayed( const SongID& songID ) const
{
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return 0;

	int iTotalNumTimesPlayed = 0;
	FOREACHM_CONST( StepsID, HighScoresForASteps, hsSong->m_StepsHighScores, j )
	{
		const HighScoresForASteps &hsSteps = j->second;

		iTotalNumTimesPlayed += hsSteps.hsl.GetNumTimesPlayed();
	}
	return iTotalNumTimesPlayed;
}

/*
 * Get the profile default modifiers.  Return true if set, in which case sModifiersOut
 * will be set.  Return false if no modifier string is set, in which case the theme
 * defaults should be used.  Note that the null string means "no modifiers are active",
 * which is distinct from no modifier string being set at all.
 *
 * In practice, we get the default modifiers from the theme the first time a game
 * is played, and from the profile every time thereafter.
 */
bool Profile::GetDefaultModifiers( const Game* pGameType, RString &sModifiersOut ) const
{
	map<RString,RString>::const_iterator it;
	it = m_sDefaultModifiers.find( pGameType->m_szName );
	if( it == m_sDefaultModifiers.end() )
		return false;
	sModifiersOut = it->second;
	return true;
}

void Profile::SetDefaultModifiers( const Game* pGameType, const RString &sModifiers )
{
	if( sModifiers == "" )
		m_sDefaultModifiers.erase( pGameType->m_szName );
	else
		m_sDefaultModifiers[pGameType->m_szName] = sModifiers;
}

Song *Profile::GetMostPopularSong() const
{
	int iMaxNumTimesPlayed = 0;
	SongID id;
	FOREACHM_CONST( SongID, HighScoresForASong, m_SongHighScores, i )
	{
		int iNumTimesPlayed = i->second.GetNumTimesPlayed();
		if(i->first.ToSong() != NULL && iNumTimesPlayed > iMaxNumTimesPlayed)
		{
			id = i->first;
			iMaxNumTimesPlayed = iNumTimesPlayed;
		}
	}

	return id.ToSong();
}

// Steps high scores
void Profile::AddStepsHighScore( const Song* pSong, const Steps* pSteps, HighScore hs, int &iIndexOut )
{
	GetStepsHighScoreList(pSong,pSteps).AddHighScore( hs, iIndexOut, false );
}

const HighScoreList& Profile::GetStepsHighScoreList( const Song* pSong, const Steps* pSteps ) const
{
	return ((Profile*)this)->GetStepsHighScoreList(pSong,pSteps);
}

HighScoreList& Profile::GetStepsHighScoreList( const Song* pSong, const Steps* pSteps )
{
	SongID songID;
	songID.FromSong( pSong );

	StepsID stepsID;
	stepsID.FromSteps( pSteps );

	HighScoresForASong &hsSong = m_SongHighScores[songID];	// operator[] inserts into map
	HighScoresForASteps &hsSteps = hsSong.m_StepsHighScores[stepsID];	// operator[] inserts into map

	return hsSteps.hsl;
}

int Profile::GetStepsNumTimesPlayed( const Song* pSong, const Steps* pSteps ) const
{
	return GetStepsHighScoreList(pSong,pSteps).GetNumTimesPlayed();
}

DateTime Profile::GetSongLastPlayedDateTime( const Song* pSong ) const
{
	SongID id;
	id.FromSong( pSong );
	std::map<SongID,HighScoresForASong>::const_iterator iter = m_SongHighScores.find( id );

	// don't call this unless has been played once
	ASSERT( iter != m_SongHighScores.end() );
	ASSERT( !iter->second.m_StepsHighScores.empty() );

	DateTime dtLatest;	// starts out zeroed
	FOREACHM_CONST( StepsID, HighScoresForASteps, iter->second.m_StepsHighScores, i )
	{
		const HighScoreList &hsl = i->second.hsl;
		if( hsl.GetNumTimesPlayed() == 0 )
			continue;
		if( dtLatest < hsl.GetLastPlayed() )
			dtLatest = hsl.GetLastPlayed();
	}
	return dtLatest;
}

bool Profile::HasPassedSteps( const Song* pSong, const Steps* pSteps ) const
{
	const HighScoreList &hsl = GetStepsHighScoreList( pSong, pSteps );
	Grade grade = hsl.GetTopScore().GetGrade();
	switch( grade )
	{
	case Grade_Failed:
	case Grade_NoData:
		return false;
	default:
		return true;
	}
}

bool Profile::HasPassedAnyStepsInSong( const Song* pSong ) const
{
	FOREACH_CONST( Steps*, pSong->GetAllSteps(), steps )
	{
		if( HasPassedSteps( pSong, *steps ) )
			return true;
	}
	return false;
}

void Profile::IncrementStepsPlayCount( const Song* pSong, const Steps* pSteps )
{
	DateTime now = DateTime::GetNowDate();
	GetStepsHighScoreList(pSong,pSteps).IncrementPlayCount( now );
}

void Profile::GetGrades( const Song* pSong, StepsType st, int iCounts[NUM_Grade] ) const
{
	SongID songID;
	songID.FromSong( pSong );

	memset( iCounts, 0, sizeof(int)*NUM_Grade );
	const HighScoresForASong *hsSong = GetHighScoresForASong( songID );
	if( hsSong == NULL )
		return;

	FOREACH_ENUM( Grade,g)
	{
		FOREACHM_CONST( StepsID, HighScoresForASteps, hsSong->m_StepsHighScores, it )
		{
			const StepsID &id = it->first;
			if( !id.MatchesStepsType(st) )
				continue;

			const HighScoresForASteps &hsSteps = it->second;
			if( hsSteps.hsl.GetTopScore().GetGrade() == g )
				iCounts[g]++;
		}
	}
}


void Profile::GetAllUsedHighScoreNames(std::set<RString>& names)
{
#define GET_NAMES_FROM_MAP(main_member, main_key_type, main_value_type, sub_member, sub_key_type, sub_value_type) \
	for(std::map<main_key_type, main_value_type>::iterator main_entry= \
				main_member.begin(); main_entry != main_member.end(); ++main_entry) \
	{ \
		for(std::map<sub_key_type, sub_value_type>::iterator sub_entry= \
					main_entry->second.sub_member.begin(); \
				sub_entry != main_entry->second.sub_member.end(); ++sub_entry) \
		{ \
			for(vector<HighScore>::iterator high_score= \
						sub_entry->second.hsl.vHighScores.begin(); \
					high_score != sub_entry->second.hsl.vHighScores.end(); \
					++high_score) \
			{ \
				if(high_score->GetName().size() > 0) \
				{ \
					names.insert(high_score->GetName()); \
				} \
			} \
		} \
	}
	GET_NAMES_FROM_MAP(m_SongHighScores, SongID, HighScoresForASong, m_StepsHighScores, StepsID, HighScoresForASteps);
#undef GET_NAMES_FROM_MAP
}

// MergeScoresFromOtherProfile has three intended use cases:
// 1.  Restoring scores to the machine profile that were deleted because the
//   songs were not loaded.
// 2.  Migrating a profile from an older version of Stepmania, and adding its
//   scores to the machine profile.
// 3.  Merging two profiles that were separate together.
// In case 1, the various total numbers are still correct, so they should be
//   skipped.  This is why the skip_totals arg exists.
// -Kyz
void Profile::MergeScoresFromOtherProfile(Profile* other, bool skip_totals,
	RString const& from_dir, RString const& to_dir)
{
	if(!skip_totals)
	{
#define MERGE_FIELD(field_name) field_name+= other->field_name;
		MERGE_FIELD(m_iTotalSessions);
		MERGE_FIELD(m_iTotalSessionSeconds);
		MERGE_FIELD(m_iTotalGameplaySeconds);
		MERGE_FIELD(m_iTotalDancePoints);
		MERGE_FIELD(m_iNumExtraStagesPassed);
		MERGE_FIELD(m_iNumExtraStagesFailed);
		MERGE_FIELD(m_iNumToasties);
		MERGE_FIELD(m_iTotalTapsAndHolds);
		MERGE_FIELD(m_iTotalJumps);
		MERGE_FIELD(m_iTotalHolds);
		MERGE_FIELD(m_iTotalRolls);
		MERGE_FIELD(m_iTotalMines);
		MERGE_FIELD(m_iTotalHands);
		MERGE_FIELD(m_iTotalLifts);
		FOREACH_ENUM(PlayMode, i)
		{
			MERGE_FIELD(m_iNumSongsPlayedByPlayMode[i]);
			MERGE_FIELD(m_iNumStagesPassedByPlayMode[i]);
		}
		FOREACH_ENUM(Difficulty, i)
		{
			MERGE_FIELD(m_iNumSongsPlayedByDifficulty[i]);
		}
		for(int i= 0; i < MAX_METER; ++i)
		{
			MERGE_FIELD(m_iNumSongsPlayedByMeter[i]);
		}
		MERGE_FIELD(m_iNumTotalSongsPlayed);
		FOREACH_ENUM(Grade, i)
		{
			MERGE_FIELD(m_iNumStagesPassedByGrade[i]);
		}
#undef MERGE_FIELD
	}
#define MERGE_SCORES_IN_MEMBER(main_member, main_key_type, main_value_type, sub_member, sub_key_type, sub_value_type) \
	for(std::map<main_key_type, main_value_type>::iterator main_entry= \
				other->main_member.begin(); main_entry != other->main_member.end(); \
			++main_entry) \
	{ \
		std::map<main_key_type, main_value_type>::iterator this_entry= \
			main_member.find(main_entry->first); \
		if(this_entry == main_member.end()) \
		{ \
			main_member[main_entry->first]= main_entry->second; \
		} \
		else \
		{ \
			for(std::map<sub_key_type, sub_value_type>::iterator sub_entry= \
						main_entry->second.sub_member.begin(); \
					sub_entry != main_entry->second.sub_member.end(); ++sub_entry) \
			{ \
				std::map<sub_key_type, sub_value_type>::iterator this_sub= \
					this_entry->second.sub_member.find(sub_entry->first); \
				if(this_sub == this_entry->second.sub_member.end()) \
				{ \
					this_entry->second.sub_member[sub_entry->first]= sub_entry->second; \
				} \
				else \
				{ \
					this_sub->second.hsl.MergeFromOtherHSL(sub_entry->second.hsl, false); \
				} \
			} \
		} \
	}
	MERGE_SCORES_IN_MEMBER(m_SongHighScores, SongID, HighScoresForASong, m_StepsHighScores, StepsID, HighScoresForASteps);
#undef MERGE_SCORES_IN_MEMBER
	// I think the machine profile should not have screenshots merged into it
	// because the intended use case is someone whose profile scores were
	// deleted off the machine by mishap, or a profile being migrated from an
	// older version of Stepmania.  Either way, the screenshots should stay
	// with the profile they came from.
	// In the case where two local profiles are being merged together, the user
	// is probably planning to delete the old profile after the merge, so we
	// want to copy the screenshots over. -Kyz
	// The old screenshot count is stored so we know where to start in the
	// list when copying the screenshot images.
	size_t old_count = m_vScreenshots.size();
	m_vScreenshots.insert(m_vScreenshots.end(),
		other->m_vScreenshots.begin(), other->m_vScreenshots.end());
	for (size_t sid = old_count; sid < m_vScreenshots.size(); ++sid)
	{
		RString old_path = from_dir + "Screenshots/" + m_vScreenshots[sid].sFileName;
		RString new_path = to_dir + "Screenshots/" + m_vScreenshots[sid].sFileName;
		// Only move the old screenshot over if it exists and won't stomp an
		// existing screenshot.
		if (FILEMAN->DoesFileExist(old_path) && (!FILEMAN->DoesFileExist(new_path)))
		{
			FILEMAN->Move(old_path, new_path);
		}
	}
	// The screenshots are kept sorted by date for ease of use, and
	// duplicates are removed because they come from the user mistakenly
	// merging a second time. -Kyz
	std::sort(m_vScreenshots.begin(), m_vScreenshots.end());
	vector<Screenshot>::iterator unique_end =
		std::unique(m_vScreenshots.begin(), m_vScreenshots.end());
	m_vScreenshots.erase(unique_end, m_vScreenshots.end());
}

void Profile::swap(Profile& other)
{
	// Type is skipped because this is meant to be used only on matching types,
	// to move profiles after the priorities have been assigned. -Kyz
	// A bit of a misnomer, since it actually works on any type that has its
	// own swap function, which includes the standard containers.
#define SWAP_STR_MEMBER(member_name) member_name.swap(other.member_name)
#define SWAP_GENERAL(member_name) std::swap(member_name, other.member_name)
#define SWAP_ARRAY(member_name, size) \
	for(int i= 0; i < size; ++i) { \
		std::swap(member_name[i], other.member_name[i]); } \
	SWAP_GENERAL(m_ListPriority);
	SWAP_STR_MEMBER(m_sDisplayName);
	SWAP_STR_MEMBER(m_sCharacterID);
	SWAP_STR_MEMBER(m_sLastUsedHighScoreName);
	SWAP_STR_MEMBER(m_sGuid);
	SWAP_GENERAL(m_iCurrentCombo);
	SWAP_GENERAL(m_iTotalSessions);
	SWAP_GENERAL(m_iTotalSessionSeconds);
	SWAP_GENERAL(m_iTotalGameplaySeconds);
	SWAP_GENERAL(m_iTotalDancePoints);
	SWAP_GENERAL(m_iNumExtraStagesPassed);
	SWAP_GENERAL(m_iNumExtraStagesFailed);
	SWAP_GENERAL(m_iNumToasties);
	SWAP_GENERAL(m_iTotalTapsAndHolds);
	SWAP_GENERAL(m_iTotalJumps);
	SWAP_GENERAL(m_iTotalHolds);
	SWAP_GENERAL(m_iTotalRolls);
	SWAP_GENERAL(m_iTotalMines);
	SWAP_GENERAL(m_iTotalHands);
	SWAP_GENERAL(m_iTotalLifts);
	SWAP_GENERAL(m_bNewProfile);
	SWAP_STR_MEMBER(m_sLastPlayedMachineGuid);
	SWAP_GENERAL(m_LastPlayedDate);
	SWAP_ARRAY(m_iNumSongsPlayedByPlayMode, NUM_PlayMode);
	SWAP_STR_MEMBER(m_iNumSongsPlayedByStyle);
	SWAP_ARRAY(m_iNumSongsPlayedByDifficulty, NUM_Difficulty);
	SWAP_ARRAY(m_iNumSongsPlayedByMeter, MAX_METER+1);
	SWAP_GENERAL(m_iNumTotalSongsPlayed);
	SWAP_ARRAY(m_iNumStagesPassedByPlayMode, NUM_PlayMode);
	SWAP_ARRAY(m_iNumStagesPassedByGrade, NUM_Grade);
	SWAP_GENERAL(m_UserTable);
	SWAP_STR_MEMBER(m_SongHighScores);
	for(int st= 0; st < NUM_StepsType; ++st)
	{
		SWAP_ARRAY(m_CategoryHighScores[st], NUM_RankingCategory);
	}
	SWAP_STR_MEMBER(m_vScreenshots);
#undef SWAP_STR_MEMBER
#undef SWAP_GENERAL
#undef SWAP_ARRAY
}

// Category high scores
void Profile::AddCategoryHighScore( StepsType st, RankingCategory rc, HighScore hs, int &iIndexOut )
{
	m_CategoryHighScores[st][rc].AddHighScore( hs, iIndexOut, false );
}

const HighScoreList& Profile::GetCategoryHighScoreList( StepsType st, RankingCategory rc ) const
{
	return ((Profile *)this)->m_CategoryHighScores[st][rc];
}

HighScoreList& Profile::GetCategoryHighScoreList( StepsType st, RankingCategory rc )
{
	return m_CategoryHighScores[st][rc];
}

int Profile::GetCategoryNumTimesPlayed( StepsType st ) const
{
	int iNumTimesPlayed = 0;
	FOREACH_ENUM( RankingCategory,rc )
		iNumTimesPlayed += m_CategoryHighScores[st][rc].GetNumTimesPlayed();
	return iNumTimesPlayed;
}

void Profile::IncrementCategoryPlayCount( StepsType st, RankingCategory rc )
{
	DateTime now = DateTime::GetNowDate();
	m_CategoryHighScores[st][rc].IncrementPlayCount( now );
}


// Loading and saving
#define WARN_PARSER	ShowWarningOrTrace( __FILE__, __LINE__, "Error parsing file.", true )
#define WARN_AND_RETURN { WARN_PARSER; return; }
#define WARN_AND_CONTINUE { WARN_PARSER; continue; }
#define WARN_AND_BREAK { WARN_PARSER; break; }
#define WARN_M(m)	ShowWarningOrTrace( __FILE__, __LINE__, RString("Error parsing file: ")+(m), true )
#define WARN_AND_RETURN_M(m) { WARN_M(m); return; }
#define WARN_AND_CONTINUE_M(m) { WARN_M(m); continue; }
#define WARN_AND_BREAK_M(m) { WARN_M(m); break; }
#define LOAD_NODE(X)	{ \
	const XNode* X = xml->GetChild(#X); \
	if( X==NULL ) LOG->Warn("Failed to read section " #X); \
	else Load##X##FromNode(X); }

void Profile::LoadCustomFunction( const RString &sDir )
{
	/* Get the theme's custom load function:
	 *   [Profile]
	 *   CustomLoadFunction=function(profile, profileDir) ... end
	 */
	Lua *L = LUA->Get();
	LuaReference customLoadFunc = THEME->GetMetricR("Profile", "CustomLoadFunction");
	customLoadFunc.PushSelf(L);
	ASSERT_M(!lua_isnil(L, -1), "CustomLoadFunction not defined");

	// Pass profile and profile directory as arguments
	this->PushSelf(L);
	LuaHelpers::Push(L, sDir);

	// Run it
	RString Error= "Error running CustomLoadFunction: ";
	LuaHelpers::RunScriptOnStack(L, Error, 2, 0, true);

	LUA->Release(L);
}

void Profile::HandleStatsPrefixChange(RString dir, bool require_signature)
{
	// Temp variables to preserve stuff across the reload.
	// Some stuff intentionally left out because the original reason for the
	// stats prefix was to allow scores from different game types to coexist.
	RString display_name= m_sDisplayName;
	RString character_id= m_sCharacterID;
	RString last_high_score_name= m_sLastUsedHighScoreName;
	ProfileType type= m_Type;
	int priority= m_ListPriority;
	RString guid= m_sGuid;
	map<RString, RString> default_mods= m_sDefaultModifiers;
	SortOrder sort_order= m_SortOrder;
	Difficulty last_diff= m_LastDifficulty;
	StepsType last_stepstype= m_LastStepsType;
	SongID last_song= m_lastSong;
	int total_sessions= m_iTotalSessions;
	int total_session_seconds= m_iTotalSessionSeconds;
	int total_gameplay_seconds= m_iTotalGameplaySeconds;
	LuaTable user_table= m_UserTable;
	bool need_to_create_file= false;
	if(IsAFile(dir + PROFILEMAN->GetStatsPrefix() + STATS_XML))
	{
		LoadAllFromDir(dir, require_signature, NULL);
	}
	else
	{
		ClearStats();
		need_to_create_file= true;
	}
	m_sDisplayName= display_name;
	m_sCharacterID= character_id;
	m_sLastUsedHighScoreName= last_high_score_name;
	m_Type= type;
	m_ListPriority= priority;
	m_sGuid= guid;
	m_sDefaultModifiers= default_mods;
	m_SortOrder= sort_order;
	m_LastDifficulty= last_diff;
	m_LastStepsType= last_stepstype;
	m_lastSong= last_song;
	m_iTotalSessions= total_sessions;
	m_iTotalSessionSeconds= total_session_seconds;
	m_iTotalGameplaySeconds= total_gameplay_seconds;
	m_UserTable= user_table;
	if(need_to_create_file)
	{
		SaveAllToDir(dir, require_signature);
	}
}

ProfileLoadResult Profile::LoadAllFromDir( const RString &sDir, bool bRequireSignature, LoadingWindow* ld)
{
	LOG->Trace( "Profile::LoadAllFromDir( %s )", sDir.c_str() );
	ASSERT( sDir.Right(1) == "/" );

	InitAll();

	LoadTypeFromDir(sDir);
	// Not critical if this fails
	LoadEditableDataFromDir( sDir );

	ProfileLoadResult ret= LoadEttFromDir(sDir);
	if (ret != ProfileLoadResult_Success) {
		ret = LoadStatsFromDir(sDir, bRequireSignature);

		if (ret != ProfileLoadResult_Success)
			return ret;

		IsEtternaProfile = true;
		ImportScoresToEtterna();
	}

	CalculateStatsFromScores(ld);
	return ProfileLoadResult_Success;
}

ProfileLoadResult Profile::LoadStatsFromDir(RString dir, bool require_signature)
{
	dir += PROFILEMAN->GetStatsPrefix();
	profiledir = dir;
	// Check for the existance of stats.xml
	RString fn = dir + STATS_XML;
	bool compressed = false;
	if(!IsAFile(fn))
	{
		// Check for the existance of stats.xml.gz
		fn = dir + STATS_XML_GZ;
		compressed = true;
		if(!IsAFile(fn))
		{
			return ProfileLoadResult_FailedNoProfile;
		}
	}

	int iError;
	unique_ptr<RageFileBasic> pFile( FILEMAN->Open(fn, RageFile::READ, iError) );
	if(pFile.get() == NULL)
	{
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return ProfileLoadResult_FailedTampered;
	}

	if(compressed)
	{
		RString sError;
		uint32_t iCRC32;
		RageFileObjInflate *pInflate = GunzipFile(pFile.release(), sError, &iCRC32);
		if(pInflate == NULL)
		{
			LOG->Trace("Error opening %s: %s", fn.c_str(), sError.c_str());
			return ProfileLoadResult_FailedTampered;
		}

		pFile.reset(pInflate);
	}

	if(require_signature)
	{ 
		RString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
		RString sDontShareFile = dir + DONT_SHARE_SIG;

		LOG->Trace("Verifying don't share signature \"%s\" against \"%s\"", sDontShareFile.c_str(), sStatsXmlSigFile.c_str());
		// verify the stats.xml signature with the "don't share" file
		if(!CryptManager::VerifyFileWithFile(sStatsXmlSigFile, sDontShareFile))
		{
			LuaHelpers::ReportScriptErrorFmt("The don't share check for '%s' failed.  Data will be ignored.", sStatsXmlSigFile.c_str());
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace("Done.");

		// verify stats.xml
		LOG->Trace("Verifying stats.xml signature");
		if(!CryptManager::VerifyFileWithFile(fn, sStatsXmlSigFile))
		{
			LuaHelpers::ReportScriptErrorFmt("The signature check for '%s' failed.  Data will be ignored.", fn.c_str());
			return ProfileLoadResult_FailedTampered;
		}
		LOG->Trace("Done.");
	}

	LOG->Trace("Loading %s", fn.c_str());
	XNode xml;
	if(!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return ProfileLoadResult_FailedTampered;
	LOG->Trace("Done.");

	return LoadStatsXmlFromNode(&xml);
}


ProfileLoadResult Profile::LoadEttFromDir(RString dir) {
	dir += PROFILEMAN->GetStatsPrefix();
	profiledir = dir;
	IsEtternaProfile = true;
	RString fn = dir + ETT_XML;
	bool compressed = false;
	if (!IsAFile(fn)) {
		fn = dir + STATS_XML_GZ;
		compressed = true;
		if (!IsAFile(fn))
			return ProfileLoadResult_FailedNoProfile;
	}

	int iError;
	unique_ptr<RageFileBasic> pFile(FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL) {
		LOG->Trace("Error opening %s: %s", fn.c_str(), strerror(iError));
		return ProfileLoadResult_FailedTampered;
	}

	if (compressed) {
		RString sError;
		uint32_t iCRC32;
		RageFileObjInflate *pInflate = GunzipFile(pFile.release(), sError, &iCRC32);
		if (pInflate == NULL) {
			LOG->Trace("Error opening %s: %s", fn.c_str(), sError.c_str());
			return ProfileLoadResult_FailedTampered;
		}
		pFile.reset(pInflate);
	}

	LOG->Trace("Loading %s", fn.c_str());
	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return ProfileLoadResult_FailedTampered;
	LOG->Trace("Done.");

	return LoadEttXmlFromNode(&xml);
}


void Profile::LoadTypeFromDir(const RString &dir)
{
	m_Type= ProfileType_Normal;
	m_ListPriority= 0;
	RString fn= dir + TYPE_INI;
	if(FILEMAN->DoesFileExist(fn))
	{
		IniFile ini;
		if(ini.ReadFile(fn))
		{
			XNode const* data= ini.GetChild("ListPosition");
			if(data != NULL)
			{
				RString type_str;
				if(data->GetAttrValue("Type", type_str))
				{
					m_Type= StringToProfileType(type_str);
					if(m_Type >= NUM_ProfileType)
					{
						m_Type= ProfileType_Normal;
					}
				}
				data->GetAttrValue("Priority", m_ListPriority);
			}
		}
	}
}

ProfileLoadResult Profile::LoadStatsXmlFromNode( const XNode *xml, bool bIgnoreEditable )
{
	/* The placeholder stats.xml file has an <html> tag. Don't load it,
	 * but don't warn about it. */
	if( xml->GetName() == "html" )
		return ProfileLoadResult_FailedNoProfile;

	if( xml->GetName() != "Stats" )
	{
		WARN_M( xml->GetName() );
		return ProfileLoadResult_FailedTampered;
	}

	// These are loaded from Editable, so we usually want to ignore them here.
	RString sName = m_sDisplayName;
	RString sCharacterID = m_sCharacterID;
	RString sLastUsedHighScoreName = m_sLastUsedHighScoreName;

	LOAD_NODE( GeneralData );
	LOAD_NODE( SongScores );
	LOAD_NODE( CategoryScores );
	LOAD_NODE( ScreenshotData );

	if( bIgnoreEditable )
	{
		m_sDisplayName = sName;
		m_sCharacterID = sCharacterID;
		m_sLastUsedHighScoreName = sLastUsedHighScoreName;
	}

	return ProfileLoadResult_Success;
}

ProfileLoadResult Profile::LoadEttXmlFromNode(const XNode *xml) {
	/* The placeholder stats.xml file has an <html> tag. Don't load it,
	* but don't warn about it. */
	if (xml->GetName() == "html")
		return ProfileLoadResult_FailedNoProfile;

	if (xml->GetName() != "Stats")
	{
		WARN_M(xml->GetName());
		return ProfileLoadResult_FailedTampered;
	}

	const XNode* gen = xml->GetChild("GeneralData");
	if(gen)
		LoadEttGeneralDataFromNode(gen);

	const XNode* favs = xml->GetChild("Favorites");
	if(favs)
		LoadFavoritesFromNode(favs);

	const XNode* pmir = xml->GetChild("PermaMirror");
	if (pmir)
		LoadPermaMirrorFromNode(pmir);

	const XNode* goals = xml->GetChild("ScoreGoals");
	if(goals)
		LoadScoreGoalsFromNode(goals);
	
	const XNode* play = xml->GetChild("Playlists");
	if (play)
		LoadPlaylistsFromNode(play);
	
	const XNode* scores = xml->GetChild("PlayerScores");
	if (scores)
		LoadEttScoresFromNode(scores);

	return ProfileLoadResult_Success;
}

void Profile::CalculateStatsFromScores(LoadingWindow* ld) {
	LOG->Trace("Calculating stats from scores");
	vector<HighScore*> all = SCOREMAN->GetAllScores();
	float TotalGameplaySeconds = 0.f;
	m_iTotalTapsAndHolds = 0;
	m_iTotalHolds = 0;
	m_iTotalMines = 0;

	for (size_t i = 0; i < all.size(); ++i) {
		HighScore* hs = all[i];
		TotalGameplaySeconds += hs->GetSurvivalSeconds();
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W1);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W2);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W3);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W4);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W5);
		m_iTotalMines += hs->GetTapNoteScore(TNS_HitMine);
		hs->GetHoldNoteScore(HNS_Held);
	}

	m_iNumTotalSongsPlayed = all.size();
	m_iTotalDancePoints = m_iTotalTapsAndHolds * 2;
	m_iTotalGameplaySeconds = static_cast<int>(TotalGameplaySeconds);

	SCOREMAN->RecalculateSSRs(ld);
	SCOREMAN->CalcPlayerRating(m_fPlayerRating, m_fPlayerSkillsets);
	//SCOREMAN->RatingOverTime();
}

void Profile::CalculateStatsFromScores() {
	LOG->Trace("Calculating stats from scores");
	vector<HighScore*> all = SCOREMAN->GetAllScores();
	float TotalGameplaySeconds = 0.f;
	m_iTotalTapsAndHolds = 0;
	m_iTotalHolds = 0;
	m_iTotalMines = 0;

	for (size_t i = 0; i < all.size(); ++i) {
		HighScore* hs = all[i];
		TotalGameplaySeconds += hs->GetSurvivalSeconds();
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W1);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W2);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W3);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W4);
		m_iTotalTapsAndHolds += hs->GetTapNoteScore(TNS_W5);
		m_iTotalMines += hs->GetTapNoteScore(TNS_HitMine);
		m_iTotalTapsAndHolds += hs->GetHoldNoteScore(HNS_Held);
	}

	m_iNumTotalSongsPlayed = all.size();
	m_iTotalDancePoints = m_iTotalTapsAndHolds * 2;
	m_iTotalGameplaySeconds = static_cast<int>(TotalGameplaySeconds);

	SCOREMAN->RecalculateSSRs(NULL);
	SCOREMAN->CalcPlayerRating(m_fPlayerRating, m_fPlayerSkillsets);
}

bool Profile::SaveAllToDir( const RString &sDir, bool bSignData ) const
{
	m_LastPlayedDate = DateTime::GetNowDate();

	SaveTypeToDir(sDir);
	// Save editable.ini
	SaveEditableDataToDir( sDir );
	
	bool bSaved = SaveEttXmlToDir(sDir);
	SaveStatsWebPageToDir( sDir );
	
	// Empty directories if none exist.
	FILEMAN->CreateDir( sDir + SCREENSHOTS_SUBDIR );
	
	return bSaved;
}

XNode *Profile::SaveStatsXmlCreateNode() const
{
	XNode *xml = new XNode( "Stats" );

	xml->AppendChild( SaveGeneralDataCreateNode() );
	xml->AppendChild( SaveSongScoresCreateNode() );
	xml->AppendChild( SaveCategoryScoresCreateNode() );
	xml->AppendChild( SaveScreenshotDataCreateNode() );

	return xml;
}

XNode *Profile::SaveEttXmlCreateNode() const
{
	XNode *xml = new XNode("Stats");
	xml->AppendChild(SaveEttGeneralDataCreateNode());

	if(!FavoritedCharts.empty())
		xml->AppendChild(SaveFavoritesCreateNode());

	if (!PermaMirrorCharts.empty())
		xml->AppendChild(SavePermaMirrorCreateNode());
	
	if (!SONGMAN->allplaylists.empty())
		xml->AppendChild(SavePlaylistsCreateNode());
	
	if(!goalmap.empty())
		xml->AppendChild(SaveScoreGoalsCreateNode());
	
	xml->AppendChild(SaveEttScoresCreateNode());
	return xml;
}

bool Profile::SaveStatsXmlToDir( RString sDir, bool bSignData ) const
{
	LOG->Trace( "SaveStatsXmlToDir: %s", sDir.c_str() );
	unique_ptr<XNode> xml( SaveStatsXmlCreateNode() );

	sDir += PROFILEMAN->GetStatsPrefix();
	// Save stats.xml
	RString fn = sDir + (g_bProfileDataCompress? STATS_XML_GZ:STATS_XML);

	{
		RString sError;
		RageFile f;
		if( !f.Open(fn, RageFile::WRITE) )
		{
			LuaHelpers::ReportScriptErrorFmt( "Couldn't open %s for writing: %s", fn.c_str(), f.GetError().c_str() );
			return false;
		}

		if( g_bProfileDataCompress )
		{
			RageFileObjGzip gzip( &f );
			gzip.Start();
			if( !XmlFileUtil::SaveToFile( xml.get(), gzip, "", false ) )
				return false;

			if( gzip.Finish() == -1 )
				return false;

			/* After successfully saving STATS_XML_GZ, remove any stray STATS_XML. */
			if( FILEMAN->IsAFile(sDir + STATS_XML) )
				FILEMAN->Remove( sDir + STATS_XML );
		}
		else
		{
			if( !XmlFileUtil::SaveToFile( xml.get(), f, "", false ) )
				return false;

			/* After successfully saving STATS_XML, remove any stray STATS_XML_GZ. */
			if( FILEMAN->IsAFile(sDir + STATS_XML_GZ) )
				FILEMAN->Remove( sDir + STATS_XML_GZ );
		}
	}

	if( bSignData )
	{
		RString sStatsXmlSigFile = fn+SIGNATURE_APPEND;
		CryptManager::SignFileToFile(fn, sStatsXmlSigFile);

		// Save the "don't share" file
		RString sDontShareFile = sDir + DONT_SHARE_SIG;
		CryptManager::SignFileToFile(sStatsXmlSigFile, sDontShareFile);
	}

	return true;
}

bool Profile::SaveEttXmlToDir(RString sDir) const {
	LOG->Trace("Saving Etterna Profile to: %s", sDir.c_str());
	unique_ptr<XNode> xml(SaveEttXmlCreateNode());
	sDir += PROFILEMAN->GetStatsPrefix();
	// Save Etterna.xml
	RString fn = sDir + ETT_XML;
	{
		RString sError;
		RageFile f;
		if (!f.Open(fn, RageFile::WRITE))
		{
			LuaHelpers::ReportScriptErrorFmt("Couldn't open %s for writing: %s", fn.c_str(), f.GetError().c_str());
			return false;
		}

		if (g_bProfileDataCompress)
		{
			RageFileObjGzip gzip(&f);
			gzip.Start();
			if (!XmlFileUtil::SaveToFile(xml.get(), gzip, "", false))
				return false;

			if (gzip.Finish() == -1)
				return false;

			/* After successfully saving STATS_XML_GZ, remove any stray STATS_XML. */
			if (FILEMAN->IsAFile(sDir + STATS_XML))
				FILEMAN->Remove(sDir + STATS_XML);
		}
		else
		{
			if (!XmlFileUtil::SaveToFile(xml.get(), f, "", false))
				return false;

			/* After successfully saving STATS_XML, remove any stray STATS_XML_GZ. */
			if (FILEMAN->IsAFile(sDir + STATS_XML_GZ))
				FILEMAN->Remove(sDir + STATS_XML_GZ);
		}
	}

	return true;
}

void Profile::SaveTypeToDir(const RString &dir) const
{
	IniFile ini;
	ini.SetValue("ListPosition", "Type", ProfileTypeToString(m_Type));
	ini.SetValue("ListPosition", "Priority", m_ListPriority);
	ini.WriteFile(dir + TYPE_INI);
}

void Profile::SaveEditableDataToDir( const RString &sDir ) const
{
	IniFile ini;

	ini.SetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.SetValue( "Editable", "CharacterID",			m_sCharacterID );
	ini.SetValue( "Editable", "LastUsedHighScoreName",		m_sLastUsedHighScoreName );

	ini.WriteFile( sDir + EDITABLE_INI );
}

XNode* Profile::SaveGeneralDataCreateNode() const
{
	XNode* pGeneralDataNode = new XNode( "GeneralData" );

	// TRICKY: These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	pGeneralDataNode->AppendChild( "DisplayName",			GetDisplayNameOrHighScoreName() );
	pGeneralDataNode->AppendChild( "CharacterID",			m_sCharacterID );
	pGeneralDataNode->AppendChild( "LastUsedHighScoreName",		m_sLastUsedHighScoreName );
	pGeneralDataNode->AppendChild( "Guid",				m_sGuid );
	pGeneralDataNode->AppendChild( "SortOrder",			SortOrderToString(m_SortOrder) );
	pGeneralDataNode->AppendChild( "LastDifficulty",		DifficultyToString(m_LastDifficulty) );
	if( m_LastStepsType != StepsType_Invalid )
		pGeneralDataNode->AppendChild( "LastStepsType",			GAMEMAN->GetStepsTypeInfo(m_LastStepsType).szName );
	pGeneralDataNode->AppendChild( m_lastSong.CreateNode() );
	pGeneralDataNode->AppendChild( "CurrentCombo", m_iCurrentCombo );
	pGeneralDataNode->AppendChild( "TotalSessions",			m_iTotalSessions );
	pGeneralDataNode->AppendChild( "TotalSessionSeconds",		m_iTotalSessionSeconds );
	pGeneralDataNode->AppendChild( "TotalGameplaySeconds",		m_iTotalGameplaySeconds );
	pGeneralDataNode->AppendChild( "LastPlayedMachineGuid",		m_sLastPlayedMachineGuid );
	pGeneralDataNode->AppendChild( "LastPlayedDate",		m_LastPlayedDate.GetString() );
	pGeneralDataNode->AppendChild( "TotalDancePoints",		m_iTotalDancePoints );
	pGeneralDataNode->AppendChild( "NumExtraStagesPassed",		m_iNumExtraStagesPassed );
	pGeneralDataNode->AppendChild( "NumExtraStagesFailed",		m_iNumExtraStagesFailed );
	pGeneralDataNode->AppendChild( "NumToasties",			m_iNumToasties );
	pGeneralDataNode->AppendChild( "TotalTapsAndHolds",		m_iTotalTapsAndHolds );
	pGeneralDataNode->AppendChild( "TotalJumps",			m_iTotalJumps );
	pGeneralDataNode->AppendChild( "TotalHolds",			m_iTotalHolds );
	pGeneralDataNode->AppendChild( "TotalRolls",			m_iTotalRolls );
	pGeneralDataNode->AppendChild( "TotalMines",			m_iTotalMines );
	pGeneralDataNode->AppendChild( "TotalHands",			m_iTotalHands );
	pGeneralDataNode->AppendChild( "TotalLifts",			m_iTotalLifts );
	pGeneralDataNode->AppendChild( "PlayerRating",			m_fPlayerRating);

	// Keep declared variables in a very local scope so they aren't 
	// accidentally used where they're not intended.  There's a lot of
	// copying and pasting in this code.

	{
		XNode* pDefaultModifiers = pGeneralDataNode->AppendChild("DefaultModifiers");
		FOREACHM_CONST( RString, RString, m_sDefaultModifiers, it )
			pDefaultModifiers->AppendChild( it->first, it->second );
	}

	{
		XNode* pFavorites = pGeneralDataNode->AppendChild("Favorites");
		FOREACHS_CONST(string, FavoritedCharts, it)
			pFavorites->AppendChild(*it);
	}

	{
		XNode* pPlayerSkillsets = pGeneralDataNode->AppendChild("PlayerSkillsets");
		FOREACH_ENUM(Skillset, ss)
			pPlayerSkillsets->AppendChild(SkillsetToString(ss), m_fPlayerSkillsets[ss]);
	}

	{
		XNode* pNumSongsPlayedByPlayMode = pGeneralDataNode->AppendChild("NumSongsPlayedByPlayMode");
		FOREACH_ENUM( PlayMode, pm )
		{
			// Don't save unplayed PlayModes.
			if( !m_iNumSongsPlayedByPlayMode[pm] )
				continue;
			pNumSongsPlayedByPlayMode->AppendChild( PlayModeToString(pm), m_iNumSongsPlayedByPlayMode[pm] );
		}
	}

	{
		XNode* pNumSongsPlayedByStyle = pGeneralDataNode->AppendChild("NumSongsPlayedByStyle");
		FOREACHM_CONST( StyleID, int, m_iNumSongsPlayedByStyle, iter )
		{
			const StyleID &s = iter->first;
			int iNumPlays = iter->second;

			XNode *pStyleNode = s.CreateNode();
			pStyleNode->AppendAttr(XNode::TEXT_ATTRIBUTE, iNumPlays );

			pNumSongsPlayedByStyle->AppendChild( pStyleNode );
		}
	}

	{
		XNode* pNumSongsPlayedByDifficulty = pGeneralDataNode->AppendChild("NumSongsPlayedByDifficulty");
		FOREACH_ENUM( Difficulty, dc )
		{
			if( !m_iNumSongsPlayedByDifficulty[dc] )
				continue;
			pNumSongsPlayedByDifficulty->AppendChild( DifficultyToString(dc), m_iNumSongsPlayedByDifficulty[dc] );
		}
	}

	{
		XNode* pNumSongsPlayedByMeter = pGeneralDataNode->AppendChild("NumSongsPlayedByMeter");
		for( int i=0; i<MAX_METER+1; i++ )
		{
			if( !m_iNumSongsPlayedByMeter[i] )
				continue;
			pNumSongsPlayedByMeter->AppendChild( ssprintf("Meter%d",i), m_iNumSongsPlayedByMeter[i] );
		}
	}

	pGeneralDataNode->AppendChild( "NumTotalSongsPlayed", m_iNumTotalSongsPlayed );

	{
		XNode* pNumStagesPassedByPlayMode = pGeneralDataNode->AppendChild("NumStagesPassedByPlayMode");
		FOREACH_ENUM( PlayMode, pm )
		{
			// Don't save unplayed PlayModes.
			if( !m_iNumStagesPassedByPlayMode[pm] )
				continue;
			pNumStagesPassedByPlayMode->AppendChild( PlayModeToString(pm), m_iNumStagesPassedByPlayMode[pm] );
		}
	}

	{
		XNode* pNumStagesPassedByGrade = pGeneralDataNode->AppendChild("NumStagesPassedByGrade");
		FOREACH_ENUM( Grade, g )
		{
			if( !m_iNumStagesPassedByGrade[g] )
				continue;
			pNumStagesPassedByGrade->AppendChild( GradeToString(g), m_iNumStagesPassedByGrade[g] );
		}
	}

	// Load Lua UserTable from profile
	if( m_UserTable.IsSet() )
	{
		Lua *L = LUA->Get();
		m_UserTable.PushSelf( L );
		XNode* pUserTable = XmlFileUtil::XNodeFromTable( L );
		LUA->Release( L );

		// XXX: XNodeFromTable returns a root node with the name "Layer".
		pUserTable->m_sName = "UserTable";
		pGeneralDataNode->AppendChild( pUserTable );
	}

	return pGeneralDataNode;
}

XNode* Profile::SaveFavoritesCreateNode() const {
	CHECKPOINT_M("Saving the favorites node.");

	XNode* favs = new XNode("Favorites");
	FOREACHS_CONST(string, FavoritedCharts, it)
		favs->AppendChild(*it);
	return favs;
}

XNode* Profile::SavePermaMirrorCreateNode() const {
	CHECKPOINT_M("Saving the permamirror node.");

	XNode* pmir = new XNode("PermaMirror");
	FOREACHS_CONST(string, PermaMirrorCharts, it)
		pmir->AppendChild(*it);
	return pmir;
}

XNode* GoalsForChart::CreateNode() const {
	XNode* cg = new XNode("GoalsForChart");

	if (!goals.empty()) {
		cg->AppendAttr("Key", goals[0].chartkey);
		FOREACH_CONST(ScoreGoal, goals, sg)
			cg->AppendChild(sg->CreateNode());
	}
	return cg;
}

XNode* Profile::SaveScoreGoalsCreateNode() const {
	CHECKPOINT_M("Saving the scoregoals node.");

	XNode* goals = new XNode("ScoreGoals");
	FOREACHUM_CONST(string, GoalsForChart, goalmap, i) {
		const GoalsForChart& cg = i->second;
		goals->AppendChild(cg.CreateNode());
	}
	return goals;
}

XNode* Profile::SavePlaylistsCreateNode() const {
	CHECKPOINT_M("Saving the playlists node.");

	XNode* playlists = new XNode("Playlists");
	auto& pls = SONGMAN->allplaylists;
	FOREACHM(string, Playlist, pls, i)
		if(i->first != "" && i->first != "Favorites")
			playlists->AppendChild(i->second.CreateNode());
	return playlists;
}

void Profile::LoadFavoritesFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the favorites node.");

	FOREACH_CONST_Child(pNode, ck)
		FavoritedCharts.emplace(SONGMAN->ReconcileBustedKeys(ck->GetName()));

	SONGMAN->SetFavoritedStatus(FavoritedCharts);
	SONGMAN->MakePlaylistFromFavorites(FavoritedCharts);
}

void Profile::LoadPermaMirrorFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the permamirror node.");

	FOREACH_CONST_Child(pNode, ck)
		PermaMirrorCharts.emplace(SONGMAN->ReconcileBustedKeys(ck->GetName()));

	SONGMAN->SetPermaMirroredStatus(PermaMirrorCharts);
}

void GoalsForChart::LoadFromNode(const XNode *pNode) {
	FOREACH_CONST_Child(pNode, sg) {
		ScoreGoal doot;
		doot.LoadFromNode(sg);
		Add(doot);
	}
}

void Profile::LoadScoreGoalsFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the scoregoals node.");

	RString ck;
	FOREACH_CONST_Child(pNode, chgoals) {
		chgoals->GetAttrValue("Key", ck);
		ck = SONGMAN->ReconcileBustedKeys(ck);
		goalmap[ck].LoadFromNode(chgoals);
	}
	SONGMAN->SetHasGoal(goalmap);
}

void Profile::LoadPlaylistsFromNode(const XNode *pNode) {
	CHECKPOINT_M("Loading the playlists node.");

	auto& pls = SONGMAN->allplaylists;
	FOREACH_CONST_Child(pNode, pl) {
		Playlist tmp;
		tmp.LoadFromNode(pl);
		pls.emplace(tmp.name, tmp);
		SONGMAN->activeplaylist = tmp.name;
	}	
}


XNode* Profile::SaveEttGeneralDataCreateNode() const {
	CHECKPOINT_M("Saving the general node.");

	XNode* pGeneralDataNode = new XNode("GeneralData");

	// TRICKY: These are write-only elements that are normally never read again.
	// This data is required by other apps (like internet ranking), but is 
	// redundant to the game app.
	pGeneralDataNode->AppendChild("DisplayName", GetDisplayNameOrHighScoreName());
	pGeneralDataNode->AppendChild("CharacterID", m_sCharacterID);
	pGeneralDataNode->AppendChild("Guid", m_sGuid);
	pGeneralDataNode->AppendChild("SortOrder", SortOrderToString(m_SortOrder));
	pGeneralDataNode->AppendChild("LastDifficulty", DifficultyToString(Difficulty_Invalid));
	if (m_LastStepsType != StepsType_Invalid)
		pGeneralDataNode->AppendChild("LastStepsType", GAMEMAN->GetStepsTypeInfo(m_LastStepsType).szName);
	pGeneralDataNode->AppendChild(m_lastSong.CreateNode());
	pGeneralDataNode->AppendChild("TotalSessions", m_iTotalSessions);
	pGeneralDataNode->AppendChild("TotalSessionSeconds", m_iTotalSessionSeconds);
	pGeneralDataNode->AppendChild("TotalGameplaySeconds", m_iTotalGameplaySeconds);
	pGeneralDataNode->AppendChild("LastPlayedMachineGuid", m_sLastPlayedMachineGuid);
	pGeneralDataNode->AppendChild("LastPlayedDate", m_LastPlayedDate.GetString());
	pGeneralDataNode->AppendChild("TotalDancePoints", m_iTotalDancePoints);
	pGeneralDataNode->AppendChild("NumToasties", m_iNumToasties);
	pGeneralDataNode->AppendChild("TotalTapsAndHolds", m_iTotalTapsAndHolds);
	pGeneralDataNode->AppendChild("TotalJumps", m_iTotalJumps);
	pGeneralDataNode->AppendChild("TotalHolds", m_iTotalHolds);
	pGeneralDataNode->AppendChild("TotalRolls", m_iTotalRolls);
	pGeneralDataNode->AppendChild("TotalMines", m_iTotalMines);
	pGeneralDataNode->AppendChild("TotalHands", m_iTotalHands);
	pGeneralDataNode->AppendChild("TotalLifts", m_iTotalLifts);
	pGeneralDataNode->AppendChild("PlayerRating", m_fPlayerRating);

	// apparently this got ripped out in the course of streamlining things -mina
	GAMESTATE->SaveCurrentSettingsToProfile(PLAYER_1);

	// Keep declared variables in a very local scope so they aren't 
	// accidentally used where they're not intended.  There's a lot of
	// copying and pasting in this code.

	{
		XNode* pDefaultModifiers = pGeneralDataNode->AppendChild("DefaultModifiers");
		FOREACHM_CONST(RString, RString, m_sDefaultModifiers, it)
			pDefaultModifiers->AppendChild(it->first, it->second);
	}

	{
		XNode* pPlayerSkillsets = pGeneralDataNode->AppendChild("PlayerSkillsets");
		FOREACH_ENUM(Skillset, ss)
			pPlayerSkillsets->AppendChild(SkillsetToString(ss), m_fPlayerSkillsets[ss]);
	}

	pGeneralDataNode->AppendChild("NumTotalSongsPlayed", m_iNumTotalSongsPlayed);

	{
		XNode* pNumStagesPassedByPlayMode = pGeneralDataNode->AppendChild("NumStagesPassedByPlayMode");
		FOREACH_ENUM(PlayMode, pm)
		{
			// Don't save unplayed PlayModes.
			if (!m_iNumStagesPassedByPlayMode[pm])
				continue;
			pNumStagesPassedByPlayMode->AppendChild(PlayModeToString(pm), m_iNumStagesPassedByPlayMode[pm]);
		}
	}

	// Load Lua UserTable from profile
	if (m_UserTable.IsSet())
	{
		Lua *L = LUA->Get();
		m_UserTable.PushSelf(L);
		XNode* pUserTable = XmlFileUtil::XNodeFromTable(L);
		LUA->Release(L);

		// XXX: XNodeFromTable returns a root node with the name "Layer".
		pUserTable->m_sName = "UserTable";
		pGeneralDataNode->AppendChild(pUserTable);
	}

	return pGeneralDataNode;
}

ProfileLoadResult Profile::LoadEditableDataFromDir( const RString &sDir )
{
	RString fn = sDir + EDITABLE_INI;

	// Don't load unreasonably large editable.xml files.
	int iBytes = FILEMAN->GetFileSizeInBytes( fn );
	if( iBytes > MAX_EDITABLE_INI_SIZE_BYTES )
	{
		LuaHelpers::ReportScriptErrorFmt( "The file '%s' is unreasonably large. It won't be loaded.", fn.c_str() );
		return ProfileLoadResult_FailedTampered;
	}

	if( !IsAFile(fn) )
		return ProfileLoadResult_FailedNoProfile;

	IniFile ini;
	ini.ReadFile( fn );

	ini.GetValue( "Editable", "DisplayName",			m_sDisplayName );
	ini.GetValue( "Editable", "CharacterID",			m_sCharacterID );
	ini.GetValue( "Editable", "LastUsedHighScoreName",		m_sLastUsedHighScoreName );

	// This is data that the user can change, so we have to validate it.
	wstring wstr = RStringToWstring(m_sDisplayName);
	if( wstr.size() > PROFILE_MAX_DISPLAY_NAME_LENGTH )
		wstr = wstr.substr(0, PROFILE_MAX_DISPLAY_NAME_LENGTH);
	m_sDisplayName = WStringToRString(wstr);
	// TODO: strip invalid chars?

	return ProfileLoadResult_Success;
}

void Profile::LoadGeneralDataFromNode( const XNode* pNode )
{
	ASSERT( pNode->GetName() == "GeneralData" );

	RString s;
	const XNode* pTemp;

	pNode->GetChildValue( "DisplayName",				m_sDisplayName );
	pNode->GetChildValue( "CharacterID",				m_sCharacterID );
	pNode->GetChildValue( "LastUsedHighScoreName",			m_sLastUsedHighScoreName );
	pNode->GetChildValue( "Guid",					m_sGuid );
	pNode->GetChildValue( "SortOrder",				s );	m_SortOrder = StringToSortOrder( s );
	pNode->GetChildValue( "LastDifficulty",				s );	m_LastDifficulty = StringToDifficulty( s );
	pNode->GetChildValue( "LastStepsType",				s );	m_LastStepsType = GAMEMAN->StringToStepsType( s );
	pTemp = pNode->GetChild( "Song" );				if( pTemp ) m_lastSong.LoadFromNode( pTemp );
	pNode->GetChildValue( "CurrentCombo", m_iCurrentCombo );
	pNode->GetChildValue( "TotalSessions",				m_iTotalSessions );
	pNode->GetChildValue( "TotalSessionSeconds",			m_iTotalSessionSeconds );
	pNode->GetChildValue( "TotalGameplaySeconds",			m_iTotalGameplaySeconds );
	pNode->GetChildValue( "LastPlayedMachineGuid",			m_sLastPlayedMachineGuid );
	pNode->GetChildValue( "LastPlayedDate",				s ); m_LastPlayedDate.FromString( s );
	pNode->GetChildValue( "TotalDancePoints",			m_iTotalDancePoints );
	pNode->GetChildValue( "NumExtraStagesPassed",			m_iNumExtraStagesPassed );
	pNode->GetChildValue( "NumExtraStagesFailed",			m_iNumExtraStagesFailed );
	pNode->GetChildValue( "NumToasties",				m_iNumToasties );
	pNode->GetChildValue( "TotalTapsAndHolds",			m_iTotalTapsAndHolds );
	pNode->GetChildValue( "TotalJumps",				m_iTotalJumps );
	pNode->GetChildValue( "TotalHolds",				m_iTotalHolds );
	pNode->GetChildValue( "TotalRolls",				m_iTotalRolls );
	pNode->GetChildValue( "TotalMines",				m_iTotalMines );
	pNode->GetChildValue( "TotalHands",				m_iTotalHands );
	pNode->GetChildValue( "TotalLifts",				m_iTotalLifts );
	pNode->GetChildValue( "PlayerRating",			m_fPlayerRating);

	{
		const XNode* pDefaultModifiers = pNode->GetChild("DefaultModifiers");
		if( pDefaultModifiers )
		{
			FOREACH_CONST_Child( pDefaultModifiers, game_type )
			{
				game_type->GetTextValue( m_sDefaultModifiers[game_type->GetName()] );
			}
		}
	}

	{
		const XNode* pFavorites = pNode->GetChild("Favorites");
		if (pFavorites) {
			FOREACH_CONST_Child(pFavorites, ck) {
				RString tmp = ck->GetName();				// handle duplicated entries caused by an oversight - mina
				bool duplicated = false;
				FOREACHS(string, FavoritedCharts, chartkey)
					if (*chartkey == tmp)
						duplicated = true;
				if (!duplicated)
					FavoritedCharts.emplace(tmp);
			}
			SONGMAN->SetFavoritedStatus(FavoritedCharts);
		}
	}

	{
		const XNode* pPlayerSkillsets = pNode->GetChild("PlayerSkillsets");
		if (pPlayerSkillsets) {
			FOREACH_ENUM(Skillset, ss)
				pPlayerSkillsets->GetChildValue(SkillsetToString(ss), m_fPlayerSkillsets[ss]);
		}
	}

	{
		const XNode* pNumSongsPlayedByPlayMode = pNode->GetChild("NumSongsPlayedByPlayMode");
		if( pNumSongsPlayedByPlayMode )
			FOREACH_ENUM( PlayMode, pm )
				pNumSongsPlayedByPlayMode->GetChildValue( PlayModeToString(pm), m_iNumSongsPlayedByPlayMode[pm] );
	}

	{
		const XNode* pNumSongsPlayedByStyle = pNode->GetChild("NumSongsPlayedByStyle");
		if( pNumSongsPlayedByStyle )
		{
			FOREACH_CONST_Child( pNumSongsPlayedByStyle, style )
			{
				if( style->GetName() != "Style" )
					continue;

				StyleID sID;
				sID.LoadFromNode( style );

				if( !sID.IsValid() )
					WARN_AND_CONTINUE;

				style->GetTextValue( m_iNumSongsPlayedByStyle[sID] );
			}
		}
	}

	{
		const XNode* pNumSongsPlayedByDifficulty = pNode->GetChild("NumSongsPlayedByDifficulty");
		if( pNumSongsPlayedByDifficulty )
			FOREACH_ENUM( Difficulty, dc )
				pNumSongsPlayedByDifficulty->GetChildValue( DifficultyToString(dc), m_iNumSongsPlayedByDifficulty[dc] );
	}

	{
		const XNode* pNumSongsPlayedByMeter = pNode->GetChild("NumSongsPlayedByMeter");
		if( pNumSongsPlayedByMeter )
			for( int i=0; i<MAX_METER+1; i++ )
				pNumSongsPlayedByMeter->GetChildValue( ssprintf("Meter%d",i), m_iNumSongsPlayedByMeter[i] );
	}

	pNode->GetChildValue("NumTotalSongsPlayed", m_iNumTotalSongsPlayed );

	{
		const XNode* pNumStagesPassedByGrade = pNode->GetChild("NumStagesPassedByGrade");
		if( pNumStagesPassedByGrade )
			FOREACH_ENUM( Grade, g )
				pNumStagesPassedByGrade->GetChildValue( GradeToString(g), m_iNumStagesPassedByGrade[g] );
	}

	{
		const XNode* pNumStagesPassedByPlayMode = pNode->GetChild("NumStagesPassedByPlayMode");
		if( pNumStagesPassedByPlayMode )
			FOREACH_ENUM( PlayMode, pm )
				pNumStagesPassedByPlayMode->GetChildValue( PlayModeToString(pm), m_iNumStagesPassedByPlayMode[pm] );

	}

	const XNode *pUserTable = pNode->GetChild("UserTable");

	Lua *L = LUA->Get();

	// If we have custom data, load it. Otherwise, make a blank table.
	if (pUserTable)
		LuaHelpers::CreateTableFromXNode(L, pUserTable);
	else
		lua_newtable(L);

	m_UserTable.SetFromStack(L);
	LUA->Release(L);

}


void Profile::LoadEttGeneralDataFromNode(const XNode* pNode) {
	CHECKPOINT_M("Loading the general node.");
	ASSERT(pNode->GetName() == "GeneralData");

	RString s;
	const XNode* pTemp;

	pNode->GetChildValue("DisplayName", m_sDisplayName);
	pNode->GetChildValue("CharacterID", m_sCharacterID);
	pNode->GetChildValue("LastUsedHighScoreName", m_sLastUsedHighScoreName);
	pNode->GetChildValue("Guid", m_sGuid);
	pNode->GetChildValue("SortOrder", s);	m_SortOrder = StringToSortOrder(s);
	pNode->GetChildValue("LastDifficulty", s);	m_LastDifficulty = StringToDifficulty(s);
	pNode->GetChildValue("LastStepsType", s);	m_LastStepsType = GAMEMAN->StringToStepsType(s);
	pTemp = pNode->GetChild("Song");				if (pTemp) m_lastSong.LoadFromNode(pTemp);
	pNode->GetChildValue("CurrentCombo", m_iCurrentCombo);
	pNode->GetChildValue("TotalSessions", m_iTotalSessions);
	pNode->GetChildValue("TotalSessionSeconds", m_iTotalSessionSeconds);
	pNode->GetChildValue("TotalGameplaySeconds", m_iTotalGameplaySeconds);
	pNode->GetChildValue("LastPlayedDate", s); m_LastPlayedDate.FromString(s);
	pNode->GetChildValue("TotalDancePoints", m_iTotalDancePoints);
	pNode->GetChildValue("NumToasties", m_iNumToasties);
	pNode->GetChildValue("TotalTapsAndHolds", m_iTotalTapsAndHolds);
	pNode->GetChildValue("TotalJumps", m_iTotalJumps);
	pNode->GetChildValue("TotalHolds", m_iTotalHolds);
	pNode->GetChildValue("TotalRolls", m_iTotalRolls);
	pNode->GetChildValue("TotalMines", m_iTotalMines);
	pNode->GetChildValue("TotalHands", m_iTotalHands);
	pNode->GetChildValue("TotalLifts", m_iTotalLifts);
	pNode->GetChildValue("PlayerRating", m_fPlayerRating);

	{
		const XNode* pDefaultModifiers = pNode->GetChild("DefaultModifiers");
		if (pDefaultModifiers)
		{
			FOREACH_CONST_Child(pDefaultModifiers, game_type)
			{
				game_type->GetTextValue(m_sDefaultModifiers[game_type->GetName()]);
			}
		}
	}

	{
		const XNode* pPlayerSkillsets = pNode->GetChild("PlayerSkillsets");
		if (pPlayerSkillsets) {
			FOREACH_ENUM(Skillset, ss)
				pPlayerSkillsets->GetChildValue(SkillsetToString(ss), m_fPlayerSkillsets[ss]);
		}
	}

	const XNode *pUserTable = pNode->GetChild("UserTable");

	Lua *L = LUA->Get();

	// If we have custom data, load it. Otherwise, make a blank table.
	if (pUserTable)
		LuaHelpers::CreateTableFromXNode(L, pUserTable);
	else
		lua_newtable(L);

	m_UserTable.SetFromStack(L);
	LUA->Release(L);

}

void Profile::AddStepTotals( int iTotalTapsAndHolds, int iTotalJumps, int iTotalHolds, int iTotalRolls, int iTotalMines, int iTotalHands, int iTotalLifts)
{
	m_iTotalTapsAndHolds += iTotalTapsAndHolds;
	m_iTotalJumps += iTotalJumps;
	m_iTotalHolds += iTotalHolds;
	m_iTotalRolls += iTotalRolls;
	m_iTotalMines += iTotalMines;
	m_iTotalHands += iTotalHands;
	m_iTotalLifts += iTotalLifts;
}


XNode* Profile::SaveSongScoresCreateNode() const
{
	CHECKPOINT_M("Getting the node to save song scores.");

	const Profile* pProfile = this;
	ASSERT(pProfile != NULL);

	XNode* pNode = new XNode("SongScores");

	FOREACHM_CONST(SongID, HighScoresForASong, m_SongHighScores, i)
	{
		const SongID &songID = i->first;
		const HighScoresForASong &hsSong = i->second;

		// skip songs that have never been played
		if (pProfile->GetSongNumTimesPlayed(songID) == 0)
			continue;

		XNode* pSongNode = pNode->AppendChild(songID.CreateNode());

		int jCheck2 = hsSong.m_StepsHighScores.size();
		int jCheck1 = 0;
		FOREACHM_CONST(StepsID, HighScoresForASteps, hsSong.m_StepsHighScores, j)
		{
			jCheck1++;
			ASSERT(jCheck1 <= jCheck2);
			const StepsID &stepsID = j->first;
			const HighScoresForASteps &hsSteps = j->second;

			const HighScoreList &hsl = hsSteps.hsl;

			// skip steps that have never been played
			if (hsl.GetNumTimesPlayed() == 0)
				continue;

			XNode* pStepsNode = pSongNode->AppendChild(stepsID.CreateNode());

			pStepsNode->AppendChild(hsl.CreateNode());
		}
	}

	return pNode;
}

void Profile::RemoveFromFavorites(const string& ck) {
	FavoritedCharts.erase(ck);
}

void Profile::RemoveFromPermaMirror(const string& ck) {
	PermaMirrorCharts.erase(ck);
}

void Profile::LoadSongScoresFromNode(const XNode* pSongScores)
{
	CHECKPOINT_M("Loading the node that contains song scores.");

	ASSERT(pSongScores->GetName() == "SongScores");

	FOREACH_CONST_Child(pSongScores, pSong)
	{
		if (pSong->GetName() != "Song")
			continue;

		SongID songID;
		songID.LoadFromNode(pSong);
		// Allow invalid songs so that scores aren't deleted for people that use
		// AdditionalSongsFolders and change it frequently. -Kyz
		//if( !songID.IsValid() )
		//	continue;

		FOREACH_CONST_Child(pSong, pSteps)
		{
			if (pSteps->GetName() != "Steps")
				continue;

			StepsID stepsID;
			stepsID.LoadFromNode(pSteps);
			if (!stepsID.IsValid())
				WARN_AND_CONTINUE;

			const XNode *pHighScoreListNode = pSteps->GetChild("HighScoreList");
			if (pHighScoreListNode == NULL)
				WARN_AND_CONTINUE;

			HighScoreList &hsl = m_SongHighScores[songID].m_StepsHighScores[stepsID].hsl;
			hsl.LoadFromNode(pHighScoreListNode);
		}
	}
}

void Profile::LoadStatsXmlForConversion() {
	string dir = profiledir;
	RString fn = dir + STATS_XML;
	bool compressed = false;
	if (!IsAFile(fn)) {
		fn = dir + STATS_XML_GZ;
		compressed = true;
		if (!IsAFile(fn)) {
			return ;
		}
	}

	int iError;
	unique_ptr<RageFileBasic> pFile(FILEMAN->Open(fn, RageFile::READ, iError));
	if (pFile.get() == NULL)
		return;

	if (compressed) {
		RString sError;
		uint32_t iCRC32;
		RageFileObjInflate *pInflate = GunzipFile(pFile.release(), sError, &iCRC32);
		if (pInflate == NULL)
			return;

		pFile.reset(pInflate);
	}

	XNode xml;
	if (!XmlFileUtil::LoadFromFileShowErrors(xml, *pFile.get()))
		return;

	XNode* scores = xml.GetChild("SongScores");
	LoadSongScoresFromNode(scores);
}

void Profile::ImportScoresToEtterna() {
	// load stats.xml
	if(IsEtternaProfile)
		LoadStatsXmlForConversion();

	int loaded = 0;
	int notloaded = 0;
	LOG->Trace("Beginning import of stats.xml scores");

	const vector<Song*>& songs = SONGMAN->GetAllSongs();

	string ck;
	FOREACHM(SongID, HighScoresForASong, m_SongHighScores, i) {
		SongID id = i->first;
		HighScoresForASong& hsfas = i->second;
		FOREACHM(StepsID, HighScoresForASteps, hsfas.m_StepsHighScores, j) {
			StepsID sid = j->first;
			
			if (sid.GetStepsType() != StepsType_dance_single)
				continue;

			if (!id.IsValid()) {
				string sdir = id.ToString();
				string sdir2;

				if (sdir.substr(0, 15) == "AdditionalSongs") {
					sdir2 = id.ToString().substr(10);
				} else if (sdir.substr(0, 5) == "Songs") {
					sdir2 = "Additional" + id.ToString();
				}
				Song* imean = new Song;
				imean->SetSongDir(sdir2);
				id = SongID();
				id.FromSong(imean);
			}

			if (id.IsValid() && sid.IsValid()) {
				vector<HighScore>& hsv = j->second.hsl.vHighScores;

				Song* song = id.ToSong();
				Steps* steps = sid.ToSteps(song, true);

				if (!song) {
					LOG->Trace("Unloaded song %s, skipping score import", id.ToString().c_str());
					continue;
				}

				if (!steps) {
					++notloaded;
					LOG->Trace("Null steps for %s at %s, skipping score import", song->GetDisplayMainTitle().c_str(), song->m_sGroupName.c_str());
					continue;
				}


				ck = steps->GetChartKey();
				for (size_t i = 0; i < hsv.size(); ++i) {
					HighScore hs = hsv[i];
					// ignore historic key and just load from here since the hashing function was changed anyway
					hs.SetChartKey(ck);
					SCOREMAN->ImportScore(hs);
					++loaded;
				}
				continue;
			}

			// if we still haven't correlated a score to a key, match by song title and number of notes
			// score import is meant to be a qol and pre-existing scores need not undergo strict filtering -mina
			LOG->Trace("Attempting to match score for %s by dir name", id.ToString().c_str());
			if (!id.IsValid()) {
				string unloaded = id.ToString();
				unloaded = unloaded.substr(0, unloaded.size() - 1);
				int jjq = unloaded.find_last_of("/");
				unloaded = unloaded.substr(jjq + 1, unloaded.size() - jjq);

				for (size_t i = 0; i < songs.size(); ++i) {
					SongID matchid;
					matchid.FromSong(songs[i]);

					string matchstring = matchid.ToString();
					matchstring = matchstring.substr(0, matchstring.size() - 1);
					jjq = matchstring.find_last_of("/");
					matchstring = matchstring.substr(jjq + 1, matchstring.size() - jjq);

					if (unloaded == matchstring) {
						id = matchid;
						break;
					}
				}

				Song* song = id.ToSong();

				if (!song) {
					continue;
				}

				vector<HighScore>& hsv = j->second.hsl.vHighScores;
				string title = song->GetDisplayMainTitle();

				for (size_t i = 0; i < hsv.size(); ++i) {
					HighScore tmp = hsv[i];
					for (size_t i = 0; i < songs.size(); ++i) {
						if (songs[i]->GetDisplayMainTitle() == title) {
							vector<Steps*> demsteps = songs[i]->GetAllSteps();
							bool matched = false;
							for (size_t j = 0; j < demsteps.size(); ++j) {
								Steps* steps = demsteps[j];
								int notes = steps->GetRadarValues()[RadarCategory_TapsAndHolds];
								int snotes = 0;

								snotes += tmp.GetTapNoteScore(TNS_Miss);
								snotes += tmp.GetTapNoteScore(TNS_W1);
								snotes += tmp.GetTapNoteScore(TNS_W2);
								snotes += tmp.GetTapNoteScore(TNS_W3);
								snotes += tmp.GetTapNoteScore(TNS_W4);
								snotes += tmp.GetTapNoteScore(TNS_W5);

								if (notes == snotes) {
									LOG->Trace("Matched based on taps count");
									matched = true;
									break;
								}

								notes = steps->GetRadarValues()[RadarCategory_Notes];

								if (notes == snotes) {
									LOG->Trace("Matched based on notes count");
									matched = true;
									break;
								}

								if (matched) {
									ck = steps->GetChartKey();
									loaded++;
									SCOREMAN->ImportScore(tmp);
								}
							}
						}
					}
				}
			}

			if (!id.IsValid()) {
				++notloaded;
				LOG->Trace("Unloaded song %s, skipping score import", id.ToString().c_str());
			}
		}
	}

	if (IsEtternaProfile)
		m_SongHighScores.clear();

	LOG->Trace("Finished import of %i scores to etterna profile. %i scores were not imported.", loaded, notloaded);
	SCREENMAN->SystemMessage("Finished import of %i scores. %i scores were not imported. See log for details.");
	CalculateStatsFromScores();

	PROFILEMAN->SaveProfile(PLAYER_1);
}

XNode* Profile::SaveEttScoresCreateNode() const {
	CHECKPOINT_M("Saving the player scores node.");

	const Profile* pProfile = this;
	ASSERT(pProfile != NULL);

	SCOREMAN->SetAllTopScores();
	XNode* pNode = SCOREMAN->CreateNode();
	return pNode;
}

void Profile::LoadEttScoresFromNode(const XNode* pSongScores) {
	CHECKPOINT_M("Loading the player scores node.");
	SCOREMAN->LoadFromNode(pSongScores);
}

// more future goalman stuff
void Profile::CreateGoal(const string& ck) {
	ScoreGoal goal;
	goal.timeassigned = DateTime::GetNowDateTime();
	goal.rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	goalmap[ck].Add(goal);
}

XNode* ScoreGoal::CreateNode() const {
	XNode* pNode = new XNode("ScoreGoal");

	pNode->AppendChild("Rate", rate);
	pNode->AppendChild("Percent", percent);
	pNode->AppendChild("Priority", priority);
	pNode->AppendChild("Achieved", achieved);
	pNode->AppendChild("TimeAssigned", timeassigned.GetString());
	if (achieved) {
		pNode->AppendChild("TimeAchieved", timeachieved.GetString());
		pNode->AppendChild("ScoreKey", scorekey);
	}
		
	pNode->AppendChild("Comment", comment);

	return pNode;
}

void ScoreGoal::LoadFromNode(const XNode *pNode) {
	ASSERT(pNode->GetName() == "ScoreGoal");

	RString s;

	pNode->GetChildValue("Rate", rate);
	pNode->GetChildValue("Percent", percent);
	if (percent > 1.f)	// goddamnit why didnt i think this through originally
		percent /= 100.f;
	pNode->GetChildValue("Priority", priority);
	pNode->GetChildValue("Achieved", achieved);
	pNode->GetChildValue("TimeAssigned", s); timeassigned.FromString(s);
	if (achieved) {
		pNode->GetChildValue("TimeAchieved", s); timeachieved.FromString(s);
		pNode->GetChildValue("ScoreKey", s); scorekey;
	}
		
	pNode->GetChildValue("Comment", comment);
}

HighScore* ScoreGoal::GetPBUpTo() {
	return SCOREMAN->GetChartPBUpTo(chartkey, rate);
}

void ScoreGoal::CheckVacuity() {
	HighScore* pb = SCOREMAN->GetChartPBAt(chartkey, rate);

	if (pb && pb->GetWifeScore() >= percent)
		vacuous = true;
	else
		vacuous = false;
}

// aaa too lazy to write comparators rn -mina
ScoreGoal& Profile::GetLowestGoalForRate(const string& ck, float rate) {
	auto& sgv = goalmap[ck].Get();
	float lowest = 100.f;
	int lowestidx = 0;
	for (size_t i = 0; i < sgv.size(); ++i) {
		ScoreGoal& tmp = sgv[i];
		if (tmp.rate == rate) {
			if (tmp.percent > lowest) {
				lowest = tmp.percent;
				lowestidx = i;
			}
		}
	}
	return sgv[lowestidx];
}

void Profile::SetAnyAchievedGoals(const string& ck, float& rate, const HighScore& pscore) {
	CHECKPOINT_M("Scanning for any goals that may have been accomplished.");

	if (!HasGoal(ck))
		return;

	auto& sgv = goalmap[ck].Get();
	for (size_t i = 0; i < sgv.size(); ++i) {
		ScoreGoal& tmp = sgv[i];
		if (lround(tmp.rate * 10000.f) == lround(rate * 10000.f) && !tmp.achieved &&tmp.percent < pscore.GetWifeScore()) {
			tmp.achieved = true;
			tmp.timeachieved = pscore.GetDateTime();
			tmp.scorekey = pscore.GetScoreKey();
		}
	}
}

void Profile::DeleteGoal(const string& ck, DateTime assigned) {
	auto& sgv = goalmap.at(ck).Get();
	for (size_t i = 0; i < sgv.size(); ++i) {
		if (sgv[i].timeassigned == assigned)
			sgv.erase(sgv.begin() + i);
	}
}

XNode* Profile::SaveCategoryScoresCreateNode() const
{
	CHECKPOINT_M("Getting the node that saves category scores.");

	const Profile* pProfile = this;
	ASSERT( pProfile != NULL );

	XNode* pNode = new XNode( "CategoryScores" );

	FOREACH_ENUM( StepsType,st )
	{
		// skip steps types that have never been played
		if( pProfile->GetCategoryNumTimesPlayed( st ) == 0 )
			continue;

		XNode* pStepsTypeNode = pNode->AppendChild( "StepsType" );
		pStepsTypeNode->AppendAttr( "Type", GAMEMAN->GetStepsTypeInfo(st).szName );

		FOREACH_ENUM( RankingCategory,rc )
		{
			// skip steps types/categories that have never been played
			if( pProfile->GetCategoryHighScoreList(st,rc).GetNumTimesPlayed() == 0 )
				continue;

			XNode* pRankingCategoryNode = pStepsTypeNode->AppendChild( "RankingCategory" );
			pRankingCategoryNode->AppendAttr( "Type", RankingCategoryToString(rc) );

			const HighScoreList &hsl = pProfile->GetCategoryHighScoreList( (StepsType)st, (RankingCategory)rc );

			pRankingCategoryNode->AppendChild( hsl.CreateNode() );
		}
	}

	return pNode;
}

void Profile::LoadCategoryScoresFromNode( const XNode* pCategoryScores )
{
	CHECKPOINT_M("Loading the node that contains category scores.");

	ASSERT( pCategoryScores->GetName() == "CategoryScores" );

	FOREACH_CONST_Child( pCategoryScores, pStepsType )
	{
		if( pStepsType->GetName() != "StepsType" )
			continue;

		RString str;
		if( !pStepsType->GetAttrValue( "Type", str ) )
			WARN_AND_CONTINUE;
		StepsType st = GAMEMAN->StringToStepsType( str );
		if( st == StepsType_Invalid )
			WARN_AND_CONTINUE_M( str );

		FOREACH_CONST_Child( pStepsType, pRadarCategory )
		{
			if( pRadarCategory->GetName() != "RankingCategory" )
				continue;

			if( !pRadarCategory->GetAttrValue( "Type", str ) )
				WARN_AND_CONTINUE;
			RankingCategory rc = StringToRankingCategory( str );
			if( rc == RankingCategory_Invalid )
				WARN_AND_CONTINUE_M( str );

			const XNode *pHighScoreListNode = pRadarCategory->GetChild("HighScoreList");
			if( pHighScoreListNode == NULL )
				WARN_AND_CONTINUE;
			
			HighScoreList &hsl = this->GetCategoryHighScoreList( st, rc );
			hsl.LoadFromNode( pHighScoreListNode );
		}
	}
}

void Profile::SaveStatsWebPageToDir( const RString &sDir) const
{
	ASSERT( PROFILEMAN != NULL );
}

void Profile::SaveMachinePublicKeyToDir( const RString &sDir ) const
{
	if( PREFSMAN->m_bSignProfileData && IsAFile(CRYPTMAN->GetPublicKeyFileName()) )
		FileCopy( CRYPTMAN->GetPublicKeyFileName(), sDir+PUBLIC_KEY_FILE );
}

void Profile::AddScreenshot( const Screenshot &screenshot )
{
	m_vScreenshots.push_back( screenshot );
}

void Profile::LoadScreenshotDataFromNode( const XNode* pScreenshotData )
{
	CHECKPOINT_M("Loading the node containing screenshot data.");

	ASSERT( pScreenshotData->GetName() == "ScreenshotData" );
	FOREACH_CONST_Child( pScreenshotData, pScreenshot )
	{
		if( pScreenshot->GetName() != "Screenshot" )
			WARN_AND_CONTINUE_M( pScreenshot->GetName() );

		Screenshot ss;
		ss.LoadFromNode( pScreenshot );

		m_vScreenshots.push_back( ss );
	}
}

XNode* Profile::SaveScreenshotDataCreateNode() const
{
	CHECKPOINT_M("Getting the node containing screenshot data.");

	const Profile* pProfile = this;
	ASSERT( pProfile != NULL );

	XNode* pNode = new XNode( "ScreenshotData" );

	FOREACH_CONST( Screenshot, m_vScreenshots, ss )
	{
		pNode->AppendChild( ss->CreateNode() );
	}

	return pNode;
}

const Profile::HighScoresForASong *Profile::GetHighScoresForASong( const SongID& songID ) const
{
	map<SongID,HighScoresForASong>::const_iterator it;
	it = m_SongHighScores.find( songID );
	if( it == m_SongHighScores.end() )
		return NULL;
	return &it->second;
}

void Profile::MoveBackupToDir( const RString &sFromDir, const RString &sToDir )
{
	if( FILEMAN->IsAFile(sFromDir + STATS_XML) &&
		FILEMAN->IsAFile(sFromDir+STATS_XML+SIGNATURE_APPEND) )
	{
		FILEMAN->Move( sFromDir+STATS_XML,					sToDir+STATS_XML );
		FILEMAN->Move( sFromDir+STATS_XML+SIGNATURE_APPEND,	sToDir+STATS_XML+SIGNATURE_APPEND );
	}
	else if( FILEMAN->IsAFile(sFromDir + STATS_XML_GZ) &&
		FILEMAN->IsAFile(sFromDir+STATS_XML_GZ+SIGNATURE_APPEND) )
	{
		FILEMAN->Move( sFromDir+STATS_XML_GZ,					sToDir+STATS_XML );
		FILEMAN->Move( sFromDir+STATS_XML_GZ+SIGNATURE_APPEND,	sToDir+STATS_XML+SIGNATURE_APPEND );
	}

	if( FILEMAN->IsAFile(sFromDir + EDITABLE_INI) )
		FILEMAN->Move( sFromDir+EDITABLE_INI,				sToDir+EDITABLE_INI );
	if( FILEMAN->IsAFile(sFromDir + DONT_SHARE_SIG) )
		FILEMAN->Move( sFromDir+DONT_SHARE_SIG,				sToDir+DONT_SHARE_SIG );
}

RString Profile::MakeUniqueFileNameNoExtension( const RString &sDir, const RString &sFileNameBeginning )
{
	FILEMAN->FlushDirCache( sDir );
	// Find a file name for the screenshot
	vector<RString> files;
	GetDirListing( sDir + sFileNameBeginning+"*", files, false, false );
	sort( files.begin(), files.end() );

	int iIndex = 0;

	for( int i = files.size()-1; i >= 0; --i )
	{
		static Regex re( "^" + sFileNameBeginning + "([0-9]{5})\\....$" );
		vector<RString> matches;
		if( !re.Compare( files[i], matches ) )
			continue;

		ASSERT( matches.size() == 1 );
		iIndex = StringToInt( matches[0] )+1;
		break;
	}

	return MakeFileNameNoExtension( sFileNameBeginning, iIndex );
}

RString Profile::MakeFileNameNoExtension( const RString &sFileNameBeginning, int iIndex )
{
	return sFileNameBeginning + ssprintf( "%05d", iIndex );
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the Profile. */ 
class LunaProfile : public Luna<Profile>
{
public:
	static int AddScreenshot(T* p, lua_State *L)
	{
		HighScore* hs = Luna<HighScore>::check(L, 1);
		RString filename = SArg(2);
		Screenshot screenshot;
		screenshot.sFileName = filename;
		screenshot.sMD5 = BinaryToHex(CRYPTMAN->GetMD5ForFile(filename));
		screenshot.highScore = *hs;
		p->AddScreenshot(screenshot);
		COMMON_RETURN_SELF;
	}
	DEFINE_METHOD(GetType, m_Type);
	DEFINE_METHOD(GetPriority, m_ListPriority);

	static int GetDisplayName(T* p, lua_State *L) { lua_pushstring(L, p->m_sDisplayName); return 1; }
	static int SetDisplayName(T* p, lua_State *L)
	{
		p->m_sDisplayName = SArg(1);
		COMMON_RETURN_SELF;
	}
	static int GetLastUsedHighScoreName(T* p, lua_State *L) { lua_pushstring(L, p->m_sLastUsedHighScoreName); return 1; }
	static int SetLastUsedHighScoreName(T* p, lua_State *L)
	{
		p->m_sLastUsedHighScoreName = SArg(1);
		COMMON_RETURN_SELF;
	}
	static int GetHighScoreList(T* p, lua_State *L)
	{
		if (LuaBinding::CheckLuaObjectType(L, 1, "Song"))
		{
			const Song *pSong = Luna<Song>::check(L, 1);
			const Steps *pSteps = Luna<Steps>::check(L, 2);
			HighScoreList &hsl = p->GetStepsHighScoreList(pSong, pSteps);
			hsl.PushSelf(L);
			return 1;
		}

		luaL_typerror(L, 1, "Song");
		COMMON_RETURN_SELF;
	}

	static int GetCategoryHighScoreList(T* p, lua_State *L)
	{
		StepsType pStepsType = Enum::Check<StepsType>(L, 1);
		RankingCategory pRankCat = Enum::Check<RankingCategory>(L, 2);
		HighScoreList &hsl = p->GetCategoryHighScoreList(pStepsType, pRankCat);
		hsl.PushSelf(L);
		return 1;
	}

	static int GetHighScoreListIfExists(T* p, lua_State *L)
	{
#define GET_IF_EXISTS(arga_type, argb_type) \
		const arga_type *parga = Luna<arga_type>::check(L, 1); \
		const argb_type *pargb = Luna<argb_type>::check(L, 2); \
		arga_type##ID arga_id; \
		arga_id.From##arga_type(parga); \
		argb_type##ID argb_id; \
		argb_id.From##argb_type(pargb); \
		std::map<arga_type##ID, Profile::HighScoresForA##arga_type>::iterator \
			main_scores= p->m_##arga_type##HighScores.find(arga_id); \
		if(main_scores == p->m_##arga_type##HighScores.end()) \
		{ \
			lua_pushnil(L); \
			return 1; \
		} \
		std::map<argb_type##ID, Profile::HighScoresForA##argb_type>::iterator \
			sub_scores= main_scores->second.m_##argb_type##HighScores.find(argb_id); \
		if(sub_scores == main_scores->second.m_##argb_type##HighScores.end()) \
		{ \
			lua_pushnil(L); \
			return 1; \
		} \
		sub_scores->second.hsl.PushSelf(L); \
		return 1;

		if (LuaBinding::CheckLuaObjectType(L, 1, "Song"))
		{
			GET_IF_EXISTS(Song, Steps);
		}
		luaL_typerror(L, 1, "Song");
		return 0;
#undef GET_IF_EXISTS
	}

	static int GetAllUsedHighScoreNames(T* p, lua_State *L)
	{
		std::set<RString> names;
		p->GetAllUsedHighScoreNames(names);
		lua_createtable(L, names.size(), 0);
		int next_name_index = 1;
		for (std::set<RString>::iterator name = names.begin(); name != names.end();
			++name)
		{
			lua_pushstring(L, name->c_str());
			lua_rawseti(L, -2, next_name_index);
			++next_name_index;
		}
		return 1;
	}

	static int GetCharacter(T* p, lua_State *L) { p->GetCharacter()->PushSelf(L); return 1; }
	static int SetCharacter(T* p, lua_State *L) { p->SetCharacter(SArg(1)); COMMON_RETURN_SELF; }
	static int GetTotalNumSongsPlayed(T* p, lua_State *L) { lua_pushnumber(L, p->m_iNumTotalSongsPlayed); return 1; }
	static int GetSongsActual(T* p, lua_State *L) { lua_pushnumber(L, p->GetSongsActual(Enum::Check<StepsType>(L, 1), Enum::Check<Difficulty>(L, 2))); return 1; }
	static int GetSongsPossible(T* p, lua_State *L) { lua_pushnumber(L, p->GetSongsPossible(Enum::Check<StepsType>(L, 1), Enum::Check<Difficulty>(L, 2))); return 1; }
	static int GetSongsPercentComplete(T* p, lua_State *L) { lua_pushnumber(L, p->GetSongsPercentComplete(Enum::Check<StepsType>(L, 1), Enum::Check<Difficulty>(L, 2))); return 1; }
	static int GetTotalStepsWithTopGrade(T* p, lua_State *L) { lua_pushnumber(L, p->GetTotalStepsWithTopGrade(Enum::Check<StepsType>(L, 1), Enum::Check<Difficulty>(L, 2), Enum::Check<Grade>(L, 3))); return 1; }
	static int GetNumTotalSongsPlayed(T* p, lua_State *L) { lua_pushnumber(L, p->m_iNumTotalSongsPlayed); return 1; }
	static int GetTotalSessions(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalSessions); return 1; }
	static int GetTotalSessionSeconds(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalSessionSeconds); return 1; }
	static int GetTotalGameplaySeconds(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalGameplaySeconds); return 1; }
	static int GetPlayerRating(T* p, lua_State *L) { lua_pushnumber(L, p->m_fPlayerRating); return 1; }
	static int GetMostPopularSong(T* p, lua_State *L)
	{
		Song *p2 = p->GetMostPopularSong();
		if (p2)
			p2->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetSongNumTimesPlayed(T* p, lua_State *L)
	{
		ASSERT(!lua_isnil(L, 1));
		Song *pS = Luna<Song>::check(L, 1);
		lua_pushnumber(L, p->GetSongNumTimesPlayed(pS));
		return 1;
	}
	static int HasPassedAnyStepsInSong(T* p, lua_State *L)
	{
		ASSERT(!lua_isnil(L, 1));
		Song *pS = Luna<Song>::check(L, 1);
		lua_pushboolean(L, p->HasPassedAnyStepsInSong(pS));
		return 1;
	}
	static int GetNumToasties(T* p, lua_State *L) { lua_pushnumber(L, p->m_iNumToasties); return 1; }
	static int GetTotalTapsAndHolds(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalTapsAndHolds); return 1; }
	static int GetTotalJumps(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalJumps); return 1; }
	static int GetTotalHolds(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalHolds); return 1; }
	static int GetTotalRolls(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalRolls); return 1; }
	static int GetTotalMines(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalMines); return 1; }
	static int GetTotalHands(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalHands); return 1; }
	static int GetTotalLifts(T* p, lua_State *L) { lua_pushnumber(L, p->m_iTotalLifts); return 1; }
	DEFINE_METHOD(GetTotalDancePoints, m_iTotalDancePoints);
	static int GetUserTable(T* p, lua_State *L) { p->m_UserTable.PushSelf(L); return 1; }
	static int GetNumFaves(T* p, lua_State *L) { lua_pushnumber(L, p->FavoritedCharts.size()); return 1; }
	static int GetLastPlayedSong(T* p, lua_State *L)
	{
		Song *pS = p->m_lastSong.ToSong();
		if (pS)
			pS->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetPlayerSkillsetRating(T* p, lua_State *L) {
		Skillset ss = Enum::Check<Skillset>(L, 1);
		lua_pushnumber(L, p->m_fPlayerSkillsets[ss]);
		return 1;
	}

	DEFINE_METHOD(GetGUID, m_sGuid);
	static int GetAllGoals(T* p, lua_State *L) {
		lua_newtable(L);
		int idx = 0;
		FOREACHUM(string, GoalsForChart, p->goalmap, i) {
			const string &ck = i->first;
			auto &sgv = i->second.Get();
			FOREACH(ScoreGoal, sgv, sg) {
				ScoreGoal &tsg = *sg;
				tsg.chartkey = ck;
				tsg.PushSelf(L);
				lua_rawseti(L, -2, idx + 1);
				idx++;
			}
		}
		return 1;
	}

	static int GetIgnoreStepCountCalories(T* p, lua_State *L) {
		lua_pushboolean(L, false);
		return 1;
	}
	static int CalculateCaloriesFromHeartRate(T* p, lua_State *L) {
		lua_pushnumber(L, 0);
		return 1;
	}

	static int IsCurrentChartPermamirror(T* p, lua_State *L) {
		bool o = false;

		if (GAMESTATE->m_pCurSteps[PLAYER_1]) {
			const string& ck = GAMESTATE->m_pCurSteps[PLAYER_1]->GetChartKey();

			if (p->PermaMirrorCharts.count(ck))
				o = true;
		}

		lua_pushboolean(L, o);
		return 1;
	}

	// ok i should probably handle this better -mina
	static int GetEasiestGoalForChartAndRate(T* p, lua_State *L) {
		string ck = SArg(1);
		if (!p->goalmap.count(ck)) {
			lua_pushnil(L);
			return 1;
		}

		auto& sgv = p->goalmap[ck].goals;
		bool herp = false;
		ScoreGoal& ez = sgv[0];
		for (auto& n : sgv)
			if (lround(n.rate * 10000.f) == lround(FArg(2) * 10000.f) && !n.achieved && n.percent <= ez.percent) {
				ez = n;
				herp = true;
			}
		if (herp)
			ez.PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	LunaProfile()
	{
		ADD_METHOD( AddScreenshot );
		ADD_METHOD( GetType );
		ADD_METHOD( GetPriority );
		ADD_METHOD( GetDisplayName );
		ADD_METHOD( SetDisplayName );
		ADD_METHOD( GetLastUsedHighScoreName );
		ADD_METHOD( SetLastUsedHighScoreName );
		ADD_METHOD( GetAllUsedHighScoreNames );
		ADD_METHOD( GetHighScoreListIfExists );
		ADD_METHOD( GetHighScoreList );
		ADD_METHOD( GetCategoryHighScoreList );
		ADD_METHOD( GetCharacter );
		ADD_METHOD( SetCharacter );
		ADD_METHOD( GetTotalNumSongsPlayed );
		ADD_METHOD( GetSongsActual );
		ADD_METHOD( GetSongsPossible );
		ADD_METHOD( GetSongsPercentComplete );
		ADD_METHOD( GetTotalStepsWithTopGrade );
		ADD_METHOD( GetNumTotalSongsPlayed );
		ADD_METHOD( GetTotalSessions );
		ADD_METHOD( GetTotalSessionSeconds );
		ADD_METHOD( GetTotalGameplaySeconds );
		ADD_METHOD( GetMostPopularSong );
		ADD_METHOD( GetSongNumTimesPlayed );
		ADD_METHOD( HasPassedAnyStepsInSong );
		ADD_METHOD( GetNumToasties );
		ADD_METHOD( GetTotalTapsAndHolds );
		ADD_METHOD( GetTotalJumps );
		ADD_METHOD( GetTotalHolds );
		ADD_METHOD( GetTotalRolls );
		ADD_METHOD( GetTotalMines );
		ADD_METHOD( GetTotalHands );
		ADD_METHOD( GetTotalLifts );
		ADD_METHOD( GetTotalDancePoints );
		ADD_METHOD( GetUserTable );
		ADD_METHOD( GetLastPlayedSong );
		ADD_METHOD( GetGUID );
		ADD_METHOD( GetPlayerRating );
		ADD_METHOD( GetPlayerSkillsetRating );
		ADD_METHOD( GetNumFaves );
		ADD_METHOD( GetAllGoals );
		ADD_METHOD(GetIgnoreStepCountCalories);
		ADD_METHOD(CalculateCaloriesFromHeartRate);
		ADD_METHOD(IsCurrentChartPermamirror);
		ADD_METHOD(GetEasiestGoalForChartAndRate);
	}
};

LUA_REGISTER_CLASS( Profile )
class LunaScoreGoal : public Luna<ScoreGoal>
{
public:
	DEFINE_METHOD( GetRate, rate );
	DEFINE_METHOD( GetPercent, percent );
	DEFINE_METHOD( GetPriority, priority );
	DEFINE_METHOD( IsAchieved, achieved );
	DEFINE_METHOD( GetComment, comment );
	DEFINE_METHOD( GetChartKey, chartkey);
	DEFINE_METHOD( WhenAssigned, timeassigned.GetString() );
	
	static int WhenAchieved(T* p, lua_State *L) {
		if (p->achieved)
			lua_pushstring(L, p->timeachieved.GetString());
		else
			lua_pushnil(L);

		return 1;
	}

	static int SetRate(T* p, lua_State *L) { 
		if (!p->achieved) {
			float newrate = FArg(1);
			CLAMP(newrate, 0.7f, 3.0f);
			p->rate = newrate;
			p->CheckVacuity();
		}
		
		return 1; 
	}

	static int SetPercent(T* p, lua_State *L) {
		if (!p->achieved) {
			float newpercent = FArg(1);
			CLAMP(newpercent, .8f, 1.f);
			p->percent = newpercent;
			p->CheckVacuity();
		}
		return 1;
	}

	static int SetPriority(T* p, lua_State *L) {
		if (!p->achieved) {
			int newpriority = IArg(1);
			CLAMP(newpriority, 0, 100);
			p->priority = newpriority;
		}
		return 1;
	}

	static int SetComment(T* p, lua_State *L) { p->comment = SArg(1); return 1; }

	static int Delete(T* p, lua_State *L) {
		PROFILEMAN->GetProfile(PLAYER_1)->DeleteGoal(p->chartkey, p->timeassigned);
		return 1;
	}

	static int GetPBUpTo(T* p, lua_State *L) {
		HighScore* pb = p->GetPBUpTo();
		if (!pb)
			lua_pushnil(L);
		else
			pb->PushSelf(L);
		return 1;
	}

	static int IsVacuous(T* p, lua_State *L) {
		if (p->achieved)
			lua_pushboolean(L, false);
		
		p->CheckVacuity();	// might be redundant
		lua_pushboolean(L, p->vacuous);
		return 1;
	}

	static int AchievedBy(T* p, lua_State *L) {
		if (p->achieved)
			lua_pushstring(L, p->scorekey);
		else
			lua_pushnil(L);
		return 1;
	}

	LunaScoreGoal()
	{
		ADD_METHOD( GetRate );
		ADD_METHOD( GetPercent );
		ADD_METHOD( GetPriority );
		ADD_METHOD( IsAchieved );
		ADD_METHOD( GetComment );
		ADD_METHOD( WhenAssigned );
		ADD_METHOD( WhenAchieved );
		ADD_METHOD( GetChartKey );
		ADD_METHOD( SetRate );
		ADD_METHOD( SetPercent );
		ADD_METHOD( SetPriority );
		ADD_METHOD( SetComment );
		ADD_METHOD( Delete );
		ADD_METHOD( GetPBUpTo );
		ADD_METHOD( IsVacuous );
		ADD_METHOD( AchievedBy );
	}
};
LUA_REGISTER_CLASS(ScoreGoal)
// lua end


/*
 * (c) 2001-2004 Chris Danford
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
