#include "Etterna/Globals/global.h"
#include "Profile.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/FileTypes/IniFile.h"
#include "Etterna/Singletons/LuaManager.h"
#include <MinaCalc/MinaCalc.h>
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileDriverDeflate.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/File/RageFileDriverDeflate.h"
#include "RageUtil/File/RageFileManager.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Singletons/CharacterManager.h"
#include "Etterna/Models/Misc/Character.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Singletons/ScoreManager.h"
#include <algorithm>
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Misc/XMLProfile.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "RageUtil/Misc/RageString.h"

/** @brief The filename for where one can edit their personal profile
 * information. */
const RString EDITABLE_INI = "Editable.ini";
/** @brief A tiny file containing the type and list priority. */
const RString TYPE_INI = "Type.ini";
/** @brief The filename containing the signature for STATS_XML's signature. */
const RString PUBLIC_KEY_FILE = "public.key";
const RString SCREENSHOTS_SUBDIR = "Screenshots/";
const RString EDIT_STEPS_SUBDIR = "Edits/";
// const RString UPLOAD_SUBDIR         = "Upload/";
const RString RIVAL_SUBDIR = "Rivals/";

#define GUID_SIZE_BYTES 8

#define MAX_EDITABLE_INI_SIZE_BYTES (2 * 1024) // 2KB
#define MAX_PLAYER_STATS_XML_SIZE_BYTES                                        \
	(400	 /* Songs */                                                       \
	 * 5	 /* Steps per Song */                                              \
	 * 5	 /* HighScores per Steps */                                        \
	 * 1024) /* size in bytes of a HighScores XNode */

const int DEFAULT_WEIGHT_POUNDS = 120;
const float DEFAULT_BIRTH_YEAR = 1995;

#if defined(_MSC_VER)
#pragma warning(disable : 4706) // assignment within conditional expression
#endif

static const char* ProfileTypeNames[] = {
	"Guest",
	"Normal",
	"Test",
};
XToString(ProfileType);
StringToX(ProfileType);
LuaXType(ProfileType);

int
Profile::HighScoresForASong::GetNumTimesPlayed() const
{
	int iCount = 0;
	for(auto const i : m_StepsHighScores)
		iCount += i.second.hsl.GetNumTimesPlayed();
	return iCount;
}

void
Profile::InitEditableData()
{
	m_sDisplayName = "";
	m_sCharacterID = "";
	m_sLastUsedHighScoreName = "";
}

void
Profile::ClearStats()
{
	// don't reset the Guid
	RString sGuid = m_sGuid;
	InitAll();
	m_sGuid = sGuid;
}

RString
Profile::MakeGuid()
{
	string s;
	s.reserve(GUID_SIZE_BYTES * 2);
	unsigned char buf[GUID_SIZE_BYTES];
	CryptManager::GetRandomBytes(buf, GUID_SIZE_BYTES);
	for (unsigned i = 0; i < GUID_SIZE_BYTES; i++)
		s += ssprintf("%02x", buf[i]);
	return s;
}

void
Profile::InitGeneralData()
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

	FOREACH_ENUM(PlayMode, i)
	m_iNumSongsPlayedByPlayMode[i] = 0;
	m_iNumSongsPlayedByStyle.clear();
	FOREACH_ENUM(Difficulty, i)
	m_iNumSongsPlayedByDifficulty[i] = 0;
	for (int i = 0; i < MAX_METER + 1; i++)
		m_iNumSongsPlayedByMeter[i] = 0;
	m_iNumTotalSongsPlayed = 0;
	ZERO(m_iNumStagesPassedByPlayMode);
	ZERO(m_iNumStagesPassedByGrade);
	ZERO(m_fPlayerSkillsets);

	m_UserTable.Unset();
}

void
Profile::InitSongScores()
{
	m_SongHighScores.clear();
}


void
Profile::InitScreenshotData()
{
	m_vScreenshots.clear();
}

RString
Profile::GetDisplayNameOrHighScoreName() const
{
	if (!m_sDisplayName.empty())
		return m_sDisplayName;
	else if (!m_sLastUsedHighScoreName.empty())
		return m_sLastUsedHighScoreName;
	else
		return RString();
}

Character*
Profile::GetCharacter() const
{
	vector<Character*> vpCharacters;
	CHARMAN->GetCharacters(vpCharacters);
	for(auto const c : vpCharacters){
		if (c->m_sCharacterID.CompareNoCase(m_sCharacterID) == 0)
			return c;
	}
	return CHARMAN->GetDefaultCharacter();
}

void
Profile::SetCharacter(const RString& sCharacterID)
{
	if (CHARMAN->GetCharacterFromID(sCharacterID))
		m_sCharacterID = sCharacterID;
}

int
Profile::GetTotalNumSongsPassed() const
{
	int iTotal = 0;
	FOREACH_ENUM(PlayMode, i)
	iTotal += m_iNumStagesPassedByPlayMode[i];
	return iTotal;
}

int
Profile::GetTotalStepsWithTopGrade(StepsType st, Difficulty d, Grade g) const
{
	int iCount = 0;

	for(auto const pSong : SONGMAN->GetAllSongs()){
		for(auto const pSteps : pSong->GetAllSteps()){
			if (pSteps->m_StepsType != st)
				continue; // skip

			if (pSteps->GetDifficulty() != d)
				continue; // skip

			const HighScoreList& hsl = GetStepsHighScoreList(pSong, pSteps);
			if (hsl.vHighScores.empty())
				continue; // skip

			if (hsl.vHighScores[0].GetGrade() == g)
				iCount++;
		}
	}

	return iCount;
}

float
Profile::GetSongsPossible(StepsType st, Difficulty dc) const
{
	int iTotalSteps = 0;

	// add steps high scores
	const vector<Song*>& vSongs = SONGMAN->GetAllSongs();
	for (unsigned i = 0; i < vSongs.size(); i++) {
		Song* pSong = vSongs[i];

		vector<Steps*> vSteps = pSong->GetAllSteps();
		for (unsigned j = 0; j < vSteps.size(); j++) {
			Steps* pSteps = vSteps[j];

			if (pSteps->m_StepsType != st)
				continue; // skip

			if (pSteps->GetDifficulty() != dc)
				continue; // skip

			iTotalSteps++;
		}
	}

	return static_cast<float>(iTotalSteps);
}

float
Profile::GetSongsActual(StepsType st, Difficulty dc) const
{
	CHECKPOINT_M(ssprintf("Profile::GetSongsActual(%d,%d)", st, dc));

	float fTotalPercents = 0;

	// add steps high scores
	for(auto const &i : m_SongHighScores){
		const SongID& id = i.first;
		Song* pSong = id.ToSong();

		CHECKPOINT_M(ssprintf("Profile::GetSongsActual: %p", pSong));

		// If the Song isn't loaded on the current machine, then we can't
		// get radar values to compute dance points.
		if (pSong == NULL)
			continue;

		CHECKPOINT_M(ssprintf("Profile::GetSongsActual: song %s",
							  pSong->GetSongDir().c_str()));
		const HighScoresForASong& hsfas = i.second;

		for(auto const &j : hsfas.m_StepsHighScores){
			const StepsID& sid = j.first;
			Steps* pSteps = sid.ToSteps(pSong, true);
			CHECKPOINT_M(ssprintf(
			  "Profile::GetSongsActual: song %p, steps %p", pSong, pSteps));

			// If the Steps isn't loaded on the current machine, then we can't
			// get radar values to compute dance points.
			if (pSteps == NULL)
				continue;

			if (pSteps->m_StepsType != st)
				continue;

			CHECKPOINT_M(ssprintf("Profile::GetSongsActual: n %s = %p",
								  sid.ToString().c_str(),
								  pSteps));
			if (pSteps->GetDifficulty() != dc) {
				continue; // skip
			}

			CHECKPOINT_M(
			  ssprintf("Profile::GetSongsActual: difficulty %s is correct",
					   DifficultyToString(dc).c_str()));

			const HighScoresForASteps& h = j.second;
			const HighScoreList& hsl = h.hsl;

			fTotalPercents += hsl.GetTopScore().GetPercentDP();
		}
	}

	return fTotalPercents;
}

float
Profile::GetSongsPercentComplete(StepsType st, Difficulty dc) const
{
	return GetSongsActual(st, dc) / GetSongsPossible(st, dc);
}

int
Profile::GetSongNumTimesPlayed(const Song* pSong) const
{
	SongID songID;
	songID.FromSong(pSong);
	return GetSongNumTimesPlayed(songID);
}

int
Profile::GetSongNumTimesPlayed(const SongID& songID) const
{
	const HighScoresForASong* hsSong = GetHighScoresForASong(songID);
	if (hsSong == NULL)
		return 0;

	int iTotalNumTimesPlayed = 0;
	for(auto const &j : hsSong->m_StepsHighScores){
		const HighScoresForASteps& hsSteps = j.second;

		iTotalNumTimesPlayed += hsSteps.hsl.GetNumTimesPlayed();
	}
	return iTotalNumTimesPlayed;
}

/*
 * Get the profile default modifiers.  Return true if set, in which case
 * sModifiersOut will be set.  Return false if no modifier string is set, in
 * which case the theme defaults should be used.  Note that the null string
 * means "no modifiers are active", which is distinct from no modifier string
 * being set at all.
 *
 * In practice, we get the default modifiers from the theme the first time a
 * game is played, and from the profile every time thereafter.
 */
bool
Profile::GetDefaultModifiers(const Game* pGameType,
							 RString& sModifiersOut) const
{
	map<RString, RString>::const_iterator it;
	it = m_sDefaultModifiers.find(pGameType->m_szName);
	if (it == m_sDefaultModifiers.end())
		return false;
	sModifiersOut = it->second;
	return true;
}

void
Profile::SetDefaultModifiers(const Game* pGameType, const RString& sModifiers)
{
	if (sModifiers == "")
		m_sDefaultModifiers.erase(pGameType->m_szName);
	else
		m_sDefaultModifiers[pGameType->m_szName] = sModifiers;
}

Song*
Profile::GetMostPopularSong() const
{
	int iMaxNumTimesPlayed = 0;
	SongID id;
	for(auto const i : m_SongHighScores){
		int iNumTimesPlayed = i.second.GetNumTimesPlayed();
		if (i.first.ToSong() != NULL && iNumTimesPlayed > iMaxNumTimesPlayed) {
			id = i.first;
			iMaxNumTimesPlayed = iNumTimesPlayed;
		}
	}

	return id.ToSong();
}

// Steps high scores
void
Profile::AddStepsHighScore(const Song* pSong,
						   const Steps* pSteps,
						   HighScore hs,
						   int& iIndexOut)
{
	GetStepsHighScoreList(pSong, pSteps).AddHighScore(hs, iIndexOut, false);
}

const HighScoreList&
Profile::GetStepsHighScoreList(const Song* pSong, const Steps* pSteps) const
{
	return (const_cast<Profile*>(this))->GetStepsHighScoreList(pSong, pSteps);
}

HighScoreList&
Profile::GetStepsHighScoreList(const Song* pSong, const Steps* pSteps)
{
	SongID songID;
	songID.FromSong(pSong);

	StepsID stepsID;
	stepsID.FromSteps(pSteps);

	HighScoresForASong& hsSong =
	  m_SongHighScores[songID]; // operator[] inserts into map
	HighScoresForASteps& hsSteps =
	  hsSong.m_StepsHighScores[stepsID]; // operator[] inserts into map

	return hsSteps.hsl;
}

int
Profile::GetStepsNumTimesPlayed(const Song* pSong, const Steps* pSteps) const
{
	return GetStepsHighScoreList(pSong, pSteps).GetNumTimesPlayed();
}

DateTime
Profile::GetSongLastPlayedDateTime(const Song* pSong) const
{
	SongID id;
	id.FromSong(pSong);
	std::map<SongID, HighScoresForASong>::const_iterator iter =
	  m_SongHighScores.find(id);

	// don't call this unless has been played once
	ASSERT(iter != m_SongHighScores.end());
	ASSERT(!iter->second.m_StepsHighScores.empty());

	DateTime dtLatest; // starts out zeroed
	for(auto const i : iter->second.m_StepsHighScores){
		const HighScoreList& hsl = i.second.hsl;
		if (hsl.GetNumTimesPlayed() == 0)
			continue;
		if (dtLatest < hsl.GetLastPlayed())
			dtLatest = hsl.GetLastPlayed();
	}
	return dtLatest;
}

bool
Profile::HasPassedSteps(const Song* pSong, const Steps* pSteps) const
{
	const HighScoreList& hsl = GetStepsHighScoreList(pSong, pSteps);
	Grade grade = hsl.GetTopScore().GetGrade();
	switch (grade) {
		case Grade_Failed:
		case Grade_NoData:
			return false;
		default:
			return true;
	}
}

bool
Profile::HasPassedAnyStepsInSong(const Song* pSong) const
{
	for(auto const steps : pSong->GetAllSteps())
		if (HasPassedSteps(pSong, steps))
			return true;
	return false;
}

void
Profile::IncrementStepsPlayCount(const Song* pSong, const Steps* pSteps)
{
	DateTime now = DateTime::GetNowDate();
	GetStepsHighScoreList(pSong, pSteps).IncrementPlayCount(now);
}

Grade
Profile::GetBestGrade(const Song* pSong, StepsType st) const
{
	Grade gradeBest = Grade_Invalid;
	if (pSong != nullptr) {
		bool hasCurrentStyleSteps = false;
		FOREACH_ENUM_N(Difficulty, 6, i)
		{
			Steps* pSteps = SongUtil::GetStepsByDifficulty(pSong, st, i);
			if (pSteps != NULL) {
				hasCurrentStyleSteps = true;
				Grade dcg = SCOREMAN->GetBestGradeFor(pSteps->GetChartKey());
				if (gradeBest >= dcg) {
					gradeBest = dcg;
				}
			}
		}
		// If no grade was found for the current style/stepstype
		if (!hasCurrentStyleSteps) {
			// Get the best grade among all steps
			auto& allSteps = pSong->GetAllSteps();
			for (auto& stepsPtr : allSteps) {
				if (stepsPtr->m_StepsType ==
					st) // Skip already checked steps of type st
					continue;
				Grade dcg = SCOREMAN->GetBestGradeFor(stepsPtr->GetChartKey());
				if (gradeBest >= dcg) {
					gradeBest = dcg;
				}
			}
		}
	}

	return gradeBest;
}

void
Profile::GetGrades(const Song* pSong,
				   StepsType st,
				   int iCounts[NUM_Grade]) const
{
	SongID songID;
	songID.FromSong(pSong);

	memset(iCounts, 0, sizeof(int) * NUM_Grade);
	const HighScoresForASong* hsSong = GetHighScoresForASong(songID);
	if (hsSong == NULL)
		return;

	FOREACH_ENUM(Grade, g)
	{
		for(auto const it : hsSong->m_StepsHighScores){
			const StepsID& id = it.first;
			if (!id.MatchesStepsType(st))
				continue;

			const HighScoresForASteps& hsSteps = it.second;
			if (hsSteps.hsl.GetTopScore().GetGrade() == g)
				iCounts[g]++;
		}
	}
}

void
Profile::GetAllUsedHighScoreNames(std::set<RString>& names)
{
#define GET_NAMES_FROM_MAP(main_member,                                        \
						   main_key_type,                                      \
						   main_value_type,                                    \
						   sub_member,                                         \
						   sub_key_type,                                       \
						   sub_value_type)                                     \
	for (std::map<main_key_type, main_value_type>::iterator main_entry =       \
		   (main_member).begin();                                              \
		 main_entry != (main_member).end();                                    \
		 ++main_entry) {                                                       \
		for (std::map<sub_key_type, sub_value_type>::iterator sub_entry =      \
			   main_entry->second.sub_member.begin();                          \
			 sub_entry != main_entry->second.sub_member.end();                 \
			 ++sub_entry) {                                                    \
			for (vector<HighScore>::iterator high_score =                      \
				   sub_entry->second.hsl.vHighScores.begin();                  \
				 high_score != sub_entry->second.hsl.vHighScores.end();        \
				 ++high_score) {                                               \
				if (high_score->GetName().size() > 0) {                        \
					names.insert(high_score->GetName());                       \
				}                                                              \
			}                                                                  \
		}                                                                      \
	}
	GET_NAMES_FROM_MAP(m_SongHighScores,
					   SongID,
					   HighScoresForASong,
					   m_StepsHighScores,
					   StepsID,
					   HighScoresForASteps);
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
void
Profile::MergeScoresFromOtherProfile(Profile* other,
									 bool skip_totals,
									 RString const& from_dir,
									 RString const& to_dir)
{
	if (!skip_totals) {
#define MERGE_FIELD(field_name) field_name += other->field_name;
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
		for (int i = 0; i < MAX_METER; ++i) {
			MERGE_FIELD(m_iNumSongsPlayedByMeter[i]);
		}
		MERGE_FIELD(m_iNumTotalSongsPlayed);
		FOREACH_ENUM(Grade, i) { MERGE_FIELD(m_iNumStagesPassedByGrade[i]); }
#undef MERGE_FIELD
	}
#define MERGE_SCORES_IN_MEMBER(main_member,                                    \
							   main_key_type,                                  \
							   main_value_type,                                \
							   sub_member,                                     \
							   sub_key_type,                                   \
							   sub_value_type)                                 \
	for (std::map<main_key_type, main_value_type>::iterator main_entry =       \
		   other->main_member.begin();                                         \
		 main_entry != other->main_member.end();                               \
		 ++main_entry) {                                                       \
		std::map<main_key_type, main_value_type>::iterator this_entry =        \
		  (main_member).find(main_entry->first);                               \
		if (this_entry == (main_member).end()) {                               \
			(main_member)[main_entry->first] = main_entry->second;             \
		} else {                                                               \
			for (std::map<sub_key_type, sub_value_type>::iterator sub_entry =  \
				   main_entry->second.sub_member.begin();                      \
				 sub_entry != main_entry->second.sub_member.end();             \
				 ++sub_entry) {                                                \
				std::map<sub_key_type, sub_value_type>::iterator this_sub =    \
				  this_entry->second.sub_member.find(sub_entry->first);        \
				if (this_sub == this_entry->second.sub_member.end()) {         \
					this_entry->second.sub_member[sub_entry->first] =          \
					  sub_entry->second;                                       \
				} else {                                                       \
					this_sub->second.hsl.MergeFromOtherHSL(                    \
					  sub_entry->second.hsl, false);                           \
				}                                                              \
			}                                                                  \
		}                                                                      \
	}
	MERGE_SCORES_IN_MEMBER(m_SongHighScores,
						   SongID,
						   HighScoresForASong,
						   m_StepsHighScores,
						   StepsID,
						   HighScoresForASteps);
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
						  other->m_vScreenshots.begin(),
						  other->m_vScreenshots.end());
	for (size_t sid = old_count; sid < m_vScreenshots.size(); ++sid) {
		RString old_path =
		  from_dir + "Screenshots/" + m_vScreenshots[sid].sFileName;
		RString new_path =
		  to_dir + "Screenshots/" + m_vScreenshots[sid].sFileName;
		// Only move the old screenshot over if it exists and won't stomp an
		// existing screenshot.
		if (FILEMAN->DoesFileExist(old_path) &&
			(!FILEMAN->DoesFileExist(new_path))) {
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

void
Profile::swap(Profile& other)
{
// Type is skipped because this is meant to be used only on matching types,
// to move profiles after the priorities have been assigned. -Kyz
// A bit of a misnomer, since it actually works on any type that has its
// own swap function, which includes the standard containers.
#define SWAP_STR_MEMBER(member_name) member_name.swap(other.member_name)
#define SWAP_GENERAL(member_name) std::swap(member_name, other.member_name)
#define SWAP_ARRAY(member_name, size)                                          \
	for (int i = 0; i < (size); ++i) {                                         \
		std::swap((member_name)[i], other.member_name[i]);                     \
	}                                                                          \
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
	SWAP_ARRAY(m_iNumSongsPlayedByMeter, MAX_METER + 1);
	SWAP_GENERAL(m_iNumTotalSongsPlayed);
	SWAP_ARRAY(m_iNumStagesPassedByPlayMode, NUM_PlayMode);
	SWAP_ARRAY(m_iNumStagesPassedByGrade, NUM_Grade);
	SWAP_GENERAL(m_UserTable);
	SWAP_STR_MEMBER(m_SongHighScores);
	SWAP_STR_MEMBER(m_vScreenshots);
#undef SWAP_STR_MEMBER
#undef SWAP_GENERAL
#undef SWAP_ARRAY
}


void
Profile::LoadCustomFunction(const RString& sDir)
{
	/* Get the theme's custom load function:
	 *   [Profile]
	 *   CustomLoadFunction=function(profile, profileDir) ... end
	 */
	Lua* L = LUA->Get();
	LuaReference customLoadFunc =
	  THEME->GetMetricR("Profile", "CustomLoadFunction");
	customLoadFunc.PushSelf(L);
	ASSERT_M(!lua_isnil(L, -1), "CustomLoadFunction not defined");

	// Pass profile and profile directory as arguments
	this->PushSelf(L);
	LuaHelpers::Push(L, sDir);

	// Run it
	RString Error = "Error running CustomLoadFunction: ";
	LuaHelpers::RunScriptOnStack(L, Error, 2, 0, true);

	LUA->Release(L);
}

void
Profile::HandleStatsPrefixChange(RString dir, bool require_signature)
{
	// Temp variables to preserve stuff across the reload.
	// Some stuff intentionally left out because the original reason for the
	// stats prefix was to allow scores from different game types to coexist.
	RString display_name = m_sDisplayName;
	RString character_id = m_sCharacterID;
	RString last_high_score_name = m_sLastUsedHighScoreName;
	ProfileType type = m_Type;
	int priority = m_ListPriority;
	RString guid = m_sGuid;
	map<RString, RString> default_mods = m_sDefaultModifiers;
	SortOrder sort_order = m_SortOrder;
	Difficulty last_diff = m_LastDifficulty;
	StepsType last_stepstype = m_LastStepsType;
	SongID last_song = m_lastSong;
	int total_sessions = m_iTotalSessions;
	int total_session_seconds = m_iTotalSessionSeconds;
	int total_gameplay_seconds = m_iTotalGameplaySeconds;
	LuaTable user_table = m_UserTable;
	bool need_to_create_file = false;
	if (IsAFile(dir + PROFILEMAN->GetStatsPrefix() + ETT_XML)) {
		LoadAllFromDir(dir, require_signature, NULL);
	} else {
		ClearStats();
		need_to_create_file = true;
	}
	m_sDisplayName = display_name;
	m_sCharacterID = character_id;
	m_sLastUsedHighScoreName = last_high_score_name;
	m_Type = type;
	m_ListPriority = priority;
	m_sGuid = guid;
	m_sDefaultModifiers = default_mods;
	m_SortOrder = sort_order;
	m_LastDifficulty = last_diff;
	m_LastStepsType = last_stepstype;
	m_lastSong = last_song;
	m_iTotalSessions = total_sessions;
	m_iTotalSessionSeconds = total_session_seconds;
	m_iTotalGameplaySeconds = total_gameplay_seconds;
	m_UserTable = user_table;
	if (need_to_create_file) {
		SaveAllToDir(dir, require_signature);
	}
}

static const float ld_update = 0.02f;
ProfileLoadResult
Profile::LoadAllFromDir(const RString& sDir,
						bool bRequireSignature,
						LoadingWindow* ld)
{
	if (PREFSMAN->m_verbose_log > 0)
		LOG->Trace("Profile::LoadAllFromDir( %s )", sDir.c_str());
	ASSERT(sDir.Right(1) == "/");

	InitAll();

	LoadTypeFromDir(sDir);
	DBProf.SetLoadingProfile(this);
	XMLProf.SetLoadingProfile(this);
	ProfileLoadResult ret = XMLProf.LoadEttFromDir(sDir);
	if (ret != ProfileLoadResult_Success)
		return ret;

	// Not critical if this fails
	LoadEditableDataFromDir(sDir);

	/*	we dont really need to be doing this automatically anymore, maybe
	reinstituting as a "migrate replay" button would be worth doing -mina
	// move old profile specific replays to the new aggregate folder
	RString oldreplaydir = sDir + "ReplayData/";

	if (FILEMAN->IsADirectory(oldreplaydir)) {
		vector<RString> replays;
		GetDirListing(oldreplaydir, replays);

		if (!replays.empty()) {
			RageTimer ld_timer;
			if (ld) {
				ld_timer.Touch();
				ld->SetIndeterminate(false);
				ld->SetTotalWork(replays.size());
				ld->SetText("Migrating replay data to new folder...");
			}
			int replayindex = 0;
			int onePercent =
			  std::max(static_cast<int>(replays.size() / 100), 1);

			for (auto r : replays) {
				if (ld && replayindex % onePercent == 0 &&
					ld_timer.Ago() > ld_update) {
					ld_timer.Touch();
					ld->SetProgress(replayindex);
					++replayindex;
				}
				FILEMAN->Move(oldreplaydir + r, "Save/Replays/" + r);
			}
		}
	}*/

	CalculateStatsFromScores(
	  ld); // note to self: figure out how to tell if this is necessary
	return ProfileLoadResult_Success;
}

void
Profile::LoadTypeFromDir(const RString& dir)
{
	m_Type = ProfileType_Normal;
	m_ListPriority = 0;
	RString fn = dir + TYPE_INI;
	if (FILEMAN->DoesFileExist(fn)) {
		IniFile ini;
		if (ini.ReadFile(fn)) {
			XNode const* data = ini.GetChild("ListPosition");
			if (data != NULL) {
				RString type_str;
				if (data->GetAttrValue("Type", type_str)) {
					m_Type = StringToProfileType(type_str);
					if (m_Type >= NUM_ProfileType) {
						m_Type = ProfileType_Normal;
					}
				}
				data->GetAttrValue("Priority", m_ListPriority);
			}
		}
	}
}

void
Profile::CalculateStatsFromScores(LoadingWindow* ld)
{
	if (PREFSMAN->m_verbose_log > 0)
		LOG->Trace("Calculating stats from scores");
	const vector<HighScore*>& all = SCOREMAN->GetAllProfileScores(m_sProfileID);
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

	SCOREMAN->RecalculateSSRs(ld, m_sProfileID);
	SCOREMAN->CalcPlayerRating(
	  m_fPlayerRating, m_fPlayerSkillsets, m_sProfileID);
}

void
Profile::CalculateStatsFromScores()
{
	CalculateStatsFromScores(nullptr);
}

bool
Profile::SaveAllToDir(const RString& sDir, bool bSignData) const
{
	m_LastPlayedDate = DateTime::GetNowDate();

	SaveTypeToDir(sDir);
	// Save editable.ini
	SaveEditableDataToDir(sDir);

	GAMESTATE->SaveCurrentSettingsToProfile(PLAYER_1);

	bool bSaved = XMLProf.SaveEttXmlToDir(sDir, this);
	SaveStatsWebPageToDir(sDir);

	// Empty directories if none exist.
	FILEMAN->CreateDir(sDir + SCREENSHOTS_SUBDIR);

	return bSaved;
}

void
Profile::SaveTypeToDir(const RString& dir) const
{
	IniFile ini;
	ini.SetValue("ListPosition", "Type", ProfileTypeToString(m_Type));
	ini.SetValue("ListPosition", "Priority", m_ListPriority);
	ini.WriteFile(dir + TYPE_INI);
}

void
Profile::SaveEditableDataToDir(const RString& sDir) const
{
	IniFile ini;

	ini.SetValue("Editable", "DisplayName", m_sDisplayName);
	ini.SetValue("Editable", "CharacterID", m_sCharacterID);
	ini.SetValue("Editable", "LastUsedHighScoreName", m_sLastUsedHighScoreName);

	ini.WriteFile(sDir + EDITABLE_INI);
}

ProfileLoadResult
Profile::LoadEditableDataFromDir(const RString& sDir)
{
	RString fn = sDir + EDITABLE_INI;

	// Don't load unreasonably large editable.xml files.
	int iBytes = FILEMAN->GetFileSizeInBytes(fn);
	if (iBytes > MAX_EDITABLE_INI_SIZE_BYTES) {
		LuaHelpers::ReportScriptErrorFmt(
		  "The file '%s' is unreasonably large. It won't be loaded.",
		  fn.c_str());
		return ProfileLoadResult_FailedTampered;
	}

	if (!IsAFile(fn))
		return ProfileLoadResult_FailedNoProfile;

	IniFile ini;
	ini.ReadFile(fn);

	ini.GetValue("Editable", "DisplayName", m_sDisplayName);
	ini.GetValue("Editable", "CharacterID", m_sCharacterID);
	ini.GetValue("Editable", "LastUsedHighScoreName", m_sLastUsedHighScoreName);

	// This is data that the user can change, so we have to validate it.
	wstring wstr = RStringToWstring(m_sDisplayName);
	if (wstr.size() > PROFILE_MAX_DISPLAY_NAME_LENGTH)
		wstr = wstr.substr(0, PROFILE_MAX_DISPLAY_NAME_LENGTH);
	m_sDisplayName = WStringToRString(wstr);
	// TODO: strip invalid chars?

	return ProfileLoadResult_Success;
}

void
Profile::AddStepTotals(int iTotalTapsAndHolds,
					   int iTotalJumps,
					   int iTotalHolds,
					   int iTotalRolls,
					   int iTotalMines,
					   int iTotalHands,
					   int iTotalLifts)
{
	m_iTotalTapsAndHolds += iTotalTapsAndHolds;
	m_iTotalJumps += iTotalJumps;
	m_iTotalHolds += iTotalHolds;
	m_iTotalRolls += iTotalRolls;
	m_iTotalMines += iTotalMines;
	m_iTotalHands += iTotalHands;
	m_iTotalLifts += iTotalLifts;
}

void
Profile::RemoveFromFavorites(const string& ck)
{
	FavoritedCharts.erase(ck);
}

void
Profile::RemoveFromPermaMirror(const string& ck)
{
	PermaMirrorCharts.erase(ck);
}

// more future goalman stuff (perhaps this should be standardized to "add" in
// order to match scoreman nomenclature) -mina
void
Profile::AddGoal(const string& ck)
{
	ScoreGoal goal;
	goal.timeassigned = DateTime::GetNowDateTime();
	goal.rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	goal.chartkey = ck;
	// duplication avoidance should be simpler than this? -mina
	if (goalmap.count(ck))
		for (auto& n : goalmap[ck].goals)
			if (n.rate == goal.rate && n.percent == goal.percent)
				return;

	goal.CheckVacuity();
	goalmap[ck].Add(goal);
	DLMAN->AddGoal(ck, goal.percent, goal.rate, goal.timeassigned);
	FillGoalTable();
	MESSAGEMAN->Broadcast("GoalTableRefresh");
}

void
Profile::FillGoalTable()
{
	goaltable.clear();
	for (auto& sgv : goalmap)
		for (auto& sg : sgv.second.goals)
			if (SONGMAN->GetStepsByChartkey(sg.chartkey))
				goaltable.emplace_back(&sg);

	auto comp = [](ScoreGoal* a, ScoreGoal* b) {
		return a->timeassigned > b->timeassigned;
	};
	sort(goaltable.begin(), goaltable.end(), comp);
}

XNode*
ScoreGoal::CreateNode() const
{
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

void
ScoreGoal::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "ScoreGoal");

	RString s;

	pNode->GetChildValue("Rate", rate);
	pNode->GetChildValue("Percent", percent);
	if (percent > 1.f) // goddamnit why didnt i think this through originally
		percent /= 100.f;
	pNode->GetChildValue("Priority", priority);
	pNode->GetChildValue("Achieved", achieved);
	pNode->GetChildValue("TimeAssigned", s);
	timeassigned.FromString(s);
	if (achieved) {
		pNode->GetChildValue("TimeAchieved", s);
		timeachieved.FromString(s);
		pNode->GetChildValue("ScoreKey", s);
		scorekey;
	}

	pNode->GetChildValue("Comment", comment);
}

HighScore*
ScoreGoal::GetPBUpTo()
{
	return SCOREMAN->GetChartPBUpTo(chartkey, rate);
}

void
ScoreGoal::CheckVacuity()
{
	HighScore* pb = SCOREMAN->GetChartPBAt(chartkey, rate);

	if (pb && pb->GetWifeScore() >= percent)
		vacuous = true;
	else
		vacuous = false;
}

void
ScoreGoal::UploadIfNotVacuous()
{
	if (!vacuous || timeachieved.GetString() != "")
		DLMAN->UpdateGoal(
		  chartkey, percent, rate, achieved, timeassigned, timeachieved);
}

// aaa too lazy to write comparators rn -mina
ScoreGoal&
Profile::GetLowestGoalForRate(const string& ck, float rate)
{
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

void
Profile::SetAnyAchievedGoals(const string& ck,
							 float& rate,
							 const HighScore& pscore)
{
	CHECKPOINT_M("Scanning for any goals that may have been accomplished.");

	if (!HasGoal(ck))
		return;

	auto& sgv = goalmap[ck].Get();
	for (size_t i = 0; i < sgv.size(); ++i) {
		ScoreGoal& tmp = sgv[i];
		if (lround(tmp.rate * 10000.f) == lround(rate * 10000.f) &&
			!tmp.achieved && tmp.percent < pscore.GetWifeScore()) {
			tmp.achieved = true;
			tmp.timeachieved = pscore.GetDateTime();
			tmp.scorekey = pscore.GetScoreKey();
			DLMAN->UpdateGoal(tmp.chartkey,
							  tmp.percent,
							  tmp.rate,
							  tmp.achieved,
							  tmp.timeassigned,
							  tmp.timeachieved);
		}
	}
}

void
Profile::RemoveGoal(const string& ck, DateTime assigned)
{
	auto& sgv = goalmap.at(ck).Get();
	for (size_t i = 0; i < sgv.size(); ++i) {
		if (sgv[i].timeassigned == assigned) {
			DLMAN->RemoveGoal(ck, sgv[i].percent, sgv[i].rate);
			sgv.erase(sgv.begin() + i);
		}
	}
}

void
Profile::SaveStatsWebPageToDir(const RString& sDir) const
{
	ASSERT(PROFILEMAN != NULL);
}

void
Profile::SaveMachinePublicKeyToDir(const RString& sDir) const
{
	if (PREFSMAN->m_bSignProfileData &&
		IsAFile(CRYPTMAN->GetPublicKeyFileName()))
		FileCopy(CRYPTMAN->GetPublicKeyFileName(), sDir + PUBLIC_KEY_FILE);
}

void
Profile::AddScreenshot(const Screenshot& screenshot)
{
	m_vScreenshots.push_back(screenshot);
}

const Profile::HighScoresForASong*
Profile::GetHighScoresForASong(const SongID& songID) const
{
	map<SongID, HighScoresForASong>::const_iterator it;
	it = m_SongHighScores.find(songID);
	if (it == m_SongHighScores.end())
		return NULL;
	return &it->second;
}

void
Profile::MoveBackupToDir(const RString& sFromDir, const RString& sToDir)
{
	XMLProfile::MoveBackupToDir(sFromDir, sToDir);

	if (FILEMAN->IsAFile(sFromDir + EDITABLE_INI))
		FILEMAN->Move(sFromDir + EDITABLE_INI, sToDir + EDITABLE_INI);
	if (FILEMAN->IsAFile(sFromDir + DONT_SHARE_SIG))
		FILEMAN->Move(sFromDir + DONT_SHARE_SIG, sToDir + DONT_SHARE_SIG);
}

RString
Profile::MakeUniqueFileNameNoExtension(const RString& sDir,
									   const RString& sFileNameBeginning)
{
	FILEMAN->FlushDirCache(sDir);
	// Find a file name for the screenshot
	vector<RString> files;
	GetDirListing(sDir + sFileNameBeginning + "*", files, false, false);
	sort(files.begin(), files.end());

	int iIndex = 0;

	for (int i = files.size() - 1; i >= 0; --i) {
		static Regex re("^" + sFileNameBeginning + "([0-9]{5})\\....$");
		vector<RString> matches;
		if (!re.Compare(files[i], matches))
			continue;

		ASSERT(matches.size() == 1);
		iIndex = StringToInt(matches[0]) + 1;
		break;
	}

	return MakeFileNameNoExtension(sFileNameBeginning, iIndex);
}

RString
Profile::MakeFileNameNoExtension(const RString& sFileNameBeginning, int iIndex)
{
	return sFileNameBeginning + ssprintf("%05d", iIndex);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the Profile. */
class LunaProfile : public Luna<Profile>
{
  public:
	static int AddScreenshot(T* p, lua_State* L)
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

	static int GetDisplayName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sDisplayName);
		return 1;
	}
	static int SetDisplayName(T* p, lua_State* L)
	{
		p->m_sDisplayName = SArg(1);
		COMMON_RETURN_SELF;
	}
	static int GetLastUsedHighScoreName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->m_sLastUsedHighScoreName);
		return 1;
	}
	static int SetLastUsedHighScoreName(T* p, lua_State* L)
	{
		p->m_sLastUsedHighScoreName = SArg(1);
		COMMON_RETURN_SELF;
	}
	static int GetHighScoreList(T* p, lua_State* L)
	{
		if (LuaBinding::CheckLuaObjectType(L, 1, "Song")) {
			const Song* pSong = Luna<Song>::check(L, 1);
			const Steps* pSteps = Luna<Steps>::check(L, 2);
			HighScoreList& hsl = p->GetStepsHighScoreList(pSong, pSteps);
			hsl.PushSelf(L);
			return 1;
		}

		luaL_typerror(L, 1, "Song");
		COMMON_RETURN_SELF;
	}



	static int GetHighScoreListIfExists(T* p, lua_State* L)
	{
#define GET_IF_EXISTS(arga_type, argb_type)                                    \
	const arga_type* parga = Luna<arga_type>::check(L, 1);                     \
	const argb_type* pargb = Luna<argb_type>::check(L, 2);                     \
	arga_type##ID arga_id;                                                     \
	arga_id.From##arga_type(parga);                                            \
	argb_type##ID argb_id;                                                     \
	argb_id.From##argb_type(pargb);                                            \
	std::map<arga_type##ID, Profile::HighScoresForA##arga_type>::iterator      \
	  main_scores = p->m_##arga_type##HighScores.find(arga_id);                \
	if (main_scores == p->m_##arga_type##HighScores.end()) {                   \
		lua_pushnil(L);                                                        \
		return 1;                                                              \
	}                                                                          \
	std::map<argb_type##ID, Profile::HighScoresForA##argb_type>::iterator      \
	  sub_scores =                                                             \
		main_scores->second.m_##argb_type##HighScores.find(argb_id);           \
	if (sub_scores == main_scores->second.m_##argb_type##HighScores.end()) {   \
		lua_pushnil(L);                                                        \
		return 1;                                                              \
	}                                                                          \
	sub_scores->second.hsl.PushSelf(L);                                        \
	return 1;

		if (LuaBinding::CheckLuaObjectType(L, 1, "Song")) {
			GET_IF_EXISTS(Song, Steps);
		}
		luaL_typerror(L, 1, "Song");
		return 0;
#undef GET_IF_EXISTS
	}

	static int GetAllUsedHighScoreNames(T* p, lua_State* L)
	{
		std::set<RString> names;
		p->GetAllUsedHighScoreNames(names);
		lua_createtable(L, names.size(), 0);
		int next_name_index = 1;
		for (std::set<RString>::iterator name = names.begin();
			 name != names.end();
			 ++name) {
			lua_pushstring(L, name->c_str());
			lua_rawseti(L, -2, next_name_index);
			++next_name_index;
		}
		return 1;
	}

	static int GetCharacter(T* p, lua_State* L)
	{
		p->GetCharacter()->PushSelf(L);
		return 1;
	}
	static int SetCharacter(T* p, lua_State* L)
	{
		p->SetCharacter(SArg(1));
		COMMON_RETURN_SELF;
	}
	static int GetTotalNumSongsPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iNumTotalSongsPlayed);
		return 1;
	}
	static int GetSongsActual(T* p, lua_State* L)
	{
		lua_pushnumber(L,
					   p->GetSongsActual(Enum::Check<StepsType>(L, 1),
										 Enum::Check<Difficulty>(L, 2)));
		return 1;
	}
	static int GetSongsPossible(T* p, lua_State* L)
	{
		lua_pushnumber(L,
					   p->GetSongsPossible(Enum::Check<StepsType>(L, 1),
										   Enum::Check<Difficulty>(L, 2)));
		return 1;
	}
	static int GetSongsPercentComplete(T* p, lua_State* L)
	{
		lua_pushnumber(
		  L,
		  p->GetSongsPercentComplete(Enum::Check<StepsType>(L, 1),
									 Enum::Check<Difficulty>(L, 2)));
		return 1;
	}
	static int GetTotalStepsWithTopGrade(T* p, lua_State* L)
	{
		lua_pushnumber(
		  L,
		  p->GetTotalStepsWithTopGrade(Enum::Check<StepsType>(L, 1),
									   Enum::Check<Difficulty>(L, 2),
									   Enum::Check<Grade>(L, 3)));
		return 1;
	}
	static int GetNumTotalSongsPlayed(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iNumTotalSongsPlayed);
		return 1;
	}
	static int GetTotalSessions(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalSessions);
		return 1;
	}
	static int GetTotalSessionSeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalSessionSeconds);
		return 1;
	}
	static int GetTotalGameplaySeconds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalGameplaySeconds);
		return 1;
	}
	static int GetPlayerRating(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_fPlayerRating);
		return 1;
	}
	static int GetMostPopularSong(T* p, lua_State* L)
	{
		Song* p2 = p->GetMostPopularSong();
		if (p2 != nullptr)
			p2->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetSongNumTimesPlayed(T* p, lua_State* L)
	{
		ASSERT(!lua_isnil(L, 1));
		Song* pS = Luna<Song>::check(L, 1);
		lua_pushnumber(L, p->GetSongNumTimesPlayed(pS));
		return 1;
	}
	static int HasPassedAnyStepsInSong(T* p, lua_State* L)
	{
		ASSERT(!lua_isnil(L, 1));
		Song* pS = Luna<Song>::check(L, 1);
		lua_pushboolean(L, p->HasPassedAnyStepsInSong(pS));
		return 1;
	}
	static int GetNumToasties(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iNumToasties);
		return 1;
	}
	static int GetTotalTapsAndHolds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalTapsAndHolds);
		return 1;
	}
	static int GetTotalJumps(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalJumps);
		return 1;
	}
	static int GetTotalHolds(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalHolds);
		return 1;
	}
	static int GetTotalRolls(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalRolls);
		return 1;
	}
	static int GetTotalMines(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalMines);
		return 1;
	}
	static int GetTotalHands(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalHands);
		return 1;
	}
	static int GetTotalLifts(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->m_iTotalLifts);
		return 1;
	}
	DEFINE_METHOD(GetTotalDancePoints, m_iTotalDancePoints);
	static int GetUserTable(T* p, lua_State* L)
	{
		p->m_UserTable.PushSelf(L);
		return 1;
	}
	static int GetNumFaves(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->FavoritedCharts.size());
		return 1;
	}
	static int GetLastPlayedSong(T* p, lua_State* L)
	{
		Song* pS = p->m_lastSong.ToSong();
		if (pS != nullptr)
			pS->PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}
	static int GetPlayerSkillsetRating(T* p, lua_State* L)
	{
		Skillset ss = Enum::Check<Skillset>(L, 1);
		lua_pushnumber(L, p->m_fPlayerSkillsets[ss]);
		return 1;
	}

	DEFINE_METHOD(GetGUID, m_sGuid);

	static int GetGoalTable(T* p, lua_State* L)
	{
		LuaHelpers::CreateTableFromArray(p->goaltable, L);
		return 1;
	}
	static int SetFromAll(T* p, lua_State* L)
	{
		p->FillGoalTable();
		p->filtermode = 1;
		p->asc = true;
		return 0;
	}

	static int SortByDate(T* p, lua_State* L)
	{
		if (p->sortmode == 1)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return a->timeassigned < b->timeassigned;
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return a->timeassigned > b->timeassigned;
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 1;
		p->asc = true;
		return 0;
	}

	static int SortByRate(T* p, lua_State* L)
	{
		if (p->sortmode == 2)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return a->rate < b->rate;
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return a->rate > b->rate;
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 2;
		p->asc = true;
		return 0;
	}

	static int SortByName(T* p, lua_State* L)
	{
		if (p->sortmode == 3)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return Rage::make_lower(
							 SONGMAN->GetSongByChartkey(a->chartkey)
							   ->GetDisplayMainTitle()) >
						   Rage::make_lower(
							 SONGMAN->GetSongByChartkey(b->chartkey)
							   ->GetDisplayMainTitle());
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return Rage::make_lower(SONGMAN->GetSongByChartkey(a->chartkey)
									  ->GetDisplayMainTitle()) <
				   Rage::make_lower(SONGMAN->GetSongByChartkey(b->chartkey)
									  ->GetDisplayMainTitle());
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 3;
		p->asc = true;
		return 0;
	}

	static int SortByPriority(T* p, lua_State* L)
	{
		if (p->sortmode == 4)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return a->priority > b->priority;
				}; // custom operators?
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return a->priority < b->priority;
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 4;
		p->asc = true;
		return 0;
	}

	static int SortByDiff(T* p, lua_State* L)
	{
		if (p->sortmode == 5)
			if (p->asc) {
				auto comp = [](ScoreGoal* a, ScoreGoal* b) {
					return SONGMAN->GetStepsByChartkey(a->chartkey)
							 ->GetMSD(a->rate, 0) <
						   SONGMAN->GetStepsByChartkey(b->chartkey)
							 ->GetMSD(b->rate, 0);
				};
				sort(p->goaltable.begin(), p->goaltable.end(), comp);
				p->asc = false;
				return 0;
			}
		auto comp = [](ScoreGoal* a, ScoreGoal* b) {
			return SONGMAN->GetStepsByChartkey(a->chartkey)
					 ->GetMSD(a->rate, 0) >
				   SONGMAN->GetStepsByChartkey(b->chartkey)->GetMSD(b->rate, 0);
		};
		sort(p->goaltable.begin(), p->goaltable.end(), comp);
		p->sortmode = 5;
		p->asc = true;
		return 0;
	}

	static int ToggleFilter(T* p, lua_State* L)
	{
		p->FillGoalTable();

		if (p->filtermode == 3) {
			p->filtermode = 1;
			return 0;
		}

		vector<ScoreGoal*> doot;
		if (p->filtermode == 1) {
			for (auto& sg : p->goaltable)
				if (sg->achieved)
					doot.emplace_back(sg);
			p->goaltable = doot;
			p->filtermode = 2;
			return 0;
		}
		if (p->filtermode == 2) {
			for (auto& sg : p->goaltable)
				if (!sg->achieved)
					doot.emplace_back(sg);
			p->goaltable = doot;
			p->filtermode = 3;
			return 0;
		}
		return 0;
	}
	static int GetFilterMode(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->filtermode);
		return 1;
	}
	static int GetIgnoreStepCountCalories(T* p, lua_State* L)
	{
		lua_pushboolean(L, 0);
		return 1;
	}
	static int CalculateCaloriesFromHeartRate(T* p, lua_State* L)
	{
		lua_pushnumber(L, 0);
		return 1;
	}

	static int IsCurrentChartPermamirror(T* p, lua_State* L)
	{
		bool o = false;

		if (GAMESTATE->m_pCurSteps) {
			const string& ck = GAMESTATE->m_pCurSteps->GetChartKey();

			if (p->PermaMirrorCharts.count(ck))
				o = true;
		}

		lua_pushboolean(L, static_cast<int>(o));
		return 1;
	}

	// ok i should probably handle this better -mina
	static int GetEasiestGoalForChartAndRate(T* p, lua_State* L)
	{
		string ck = SArg(1);
		if (!p->goalmap.count(ck)) {
			lua_pushnil(L);
			return 1;
		}

		auto& sgv = p->goalmap[ck].goals;
		bool herp = false;
		int ez = 0;
		for (size_t i = 0; i < sgv.size(); ++i)
			if (lround(sgv[i].rate * 10000.f) == lround(FArg(2) * 10000.f) &&
				!sgv[i].achieved && sgv[i].percent <= sgv[ez].percent) {
				ez = i;
				herp = true;
			}
		if (herp)
			sgv[ez].PushSelf(L);
		else
			lua_pushnil(L);
		return 1;
	}

	static int RenameProfile(T* p, lua_State* L)
	{
		p->m_sDisplayName = SArg(1);
		p->SaveEditableDataToDir(p->m_sProfileID);
		return 1;
	}

	LunaProfile()
	{
		ADD_METHOD(AddScreenshot);
		ADD_METHOD(GetType);
		ADD_METHOD(GetPriority);
		ADD_METHOD(GetDisplayName);
		ADD_METHOD(SetDisplayName);
		ADD_METHOD(GetLastUsedHighScoreName);
		ADD_METHOD(SetLastUsedHighScoreName);
		ADD_METHOD(GetAllUsedHighScoreNames);
		ADD_METHOD(GetHighScoreListIfExists);
		ADD_METHOD(GetHighScoreList);
		ADD_METHOD(GetCharacter);
		ADD_METHOD(SetCharacter);
		ADD_METHOD(GetTotalNumSongsPlayed);
		ADD_METHOD(GetSongsActual);
		ADD_METHOD(GetSongsPossible);
		ADD_METHOD(GetSongsPercentComplete);
		ADD_METHOD(GetTotalStepsWithTopGrade);
		ADD_METHOD(GetNumTotalSongsPlayed);
		ADD_METHOD(GetTotalSessions);
		ADD_METHOD(GetTotalSessionSeconds);
		ADD_METHOD(GetTotalGameplaySeconds);
		ADD_METHOD(GetMostPopularSong);
		ADD_METHOD(GetSongNumTimesPlayed);
		ADD_METHOD(HasPassedAnyStepsInSong);
		ADD_METHOD(GetNumToasties);
		ADD_METHOD(GetTotalTapsAndHolds);
		ADD_METHOD(GetTotalJumps);
		ADD_METHOD(GetTotalHolds);
		ADD_METHOD(GetTotalRolls);
		ADD_METHOD(GetTotalMines);
		ADD_METHOD(GetTotalHands);
		ADD_METHOD(GetTotalLifts);
		ADD_METHOD(GetTotalDancePoints);
		ADD_METHOD(GetUserTable);
		ADD_METHOD(GetLastPlayedSong);
		ADD_METHOD(GetGUID);
		ADD_METHOD(GetPlayerRating);
		ADD_METHOD(GetPlayerSkillsetRating);
		ADD_METHOD(GetNumFaves);
		ADD_METHOD(GetGoalTable);
		ADD_METHOD(GetIgnoreStepCountCalories);
		ADD_METHOD(CalculateCaloriesFromHeartRate);
		ADD_METHOD(IsCurrentChartPermamirror);
		ADD_METHOD(GetEasiestGoalForChartAndRate);
		ADD_METHOD(RenameProfile);
		ADD_METHOD(SetFromAll);
		ADD_METHOD(SortByDate);
		ADD_METHOD(SortByRate);
		ADD_METHOD(SortByPriority);
		ADD_METHOD(SortByName);
		ADD_METHOD(SortByDiff);
		ADD_METHOD(ToggleFilter);
		ADD_METHOD(GetFilterMode);
	}
};

LUA_REGISTER_CLASS(Profile)
class LunaScoreGoal : public Luna<ScoreGoal>
{
  public:
	DEFINE_METHOD(GetRate, rate);
	DEFINE_METHOD(GetPercent, percent);
	DEFINE_METHOD(GetPriority, priority);
	DEFINE_METHOD(IsAchieved, achieved);
	DEFINE_METHOD(GetComment, comment);
	DEFINE_METHOD(GetChartKey, chartkey);
	DEFINE_METHOD(WhenAssigned, timeassigned.GetString());

	static int WhenAchieved(T* p, lua_State* L)
	{
		if (p->achieved)
			lua_pushstring(L, p->timeachieved.GetString());
		else
			lua_pushnil(L);

		return 1;
	}

	static int SetRate(T* p, lua_State* L)
	{
		if (!p->achieved) {
			float newrate = FArg(1);
			CLAMP(newrate, 0.7f, 3.0f);
			p->rate = newrate;
			p->CheckVacuity();
			p->UploadIfNotVacuous();
		}
		return 1;
	}

	static int SetPercent(T* p, lua_State* L)
	{
		if (!p->achieved) {
			float newpercent = FArg(1);
			CLAMP(newpercent, .8f, 1.f);

			if (p->percent < 0.995f && newpercent > 0.995f)
				newpercent = 0.9975f;
			if (p->percent < 0.9990f && newpercent > 0.9997f)
				newpercent = 0.9997f;

			p->percent = newpercent;
			p->CheckVacuity();
			p->UploadIfNotVacuous();
		}
		return 1;
	}

	static int SetPriority(T* p, lua_State* L)
	{
		if (!p->achieved) {
			int newpriority = IArg(1);
			CLAMP(newpriority, 0, 100);
			p->priority = newpriority;
			p->UploadIfNotVacuous();
		}
		return 1;
	}

	static int SetComment(T* p, lua_State* L)
	{
		p->comment = SArg(1);
		return 1;
	}

	static int Delete(T* p, lua_State* L)
	{
		PROFILEMAN->GetProfile(PLAYER_1)->RemoveGoal(p->chartkey,
													 p->timeassigned);
		return 0;
	}

	static int GetPBUpTo(T* p, lua_State* L)
	{
		HighScore* pb = p->GetPBUpTo();
		if (pb == nullptr)
			lua_pushnil(L);
		else
			pb->PushSelf(L);
		return 1;
	}

	static int IsVacuous(T* p, lua_State* L)
	{
		if (p->achieved)
			lua_pushboolean(L, 0);

		p->CheckVacuity(); // might be redundant
		lua_pushboolean(L, p->vacuous);
		return 1;
	}

	static int AchievedBy(T* p, lua_State* L)
	{
		if (p->achieved)
			lua_pushstring(L, p->scorekey);
		else
			lua_pushnil(L);
		return 1;
	}

	LunaScoreGoal()
	{
		ADD_METHOD(GetRate);
		ADD_METHOD(GetPercent);
		ADD_METHOD(GetPriority);
		ADD_METHOD(IsAchieved);
		ADD_METHOD(GetComment);
		ADD_METHOD(WhenAssigned);
		ADD_METHOD(WhenAchieved);
		ADD_METHOD(GetChartKey);
		ADD_METHOD(SetRate);
		ADD_METHOD(SetPercent);
		ADD_METHOD(SetPriority);
		ADD_METHOD(SetComment);
		ADD_METHOD(Delete);
		ADD_METHOD(GetPBUpTo);
		ADD_METHOD(IsVacuous);
		ADD_METHOD(AchievedBy);
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
