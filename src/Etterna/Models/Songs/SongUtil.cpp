#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/Models/StepsAndStyles/StepsUtil.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Models/Misc/PlayerState.h"

#include <functional>
#include <algorithm>
#include <set>
#include <map>

using std::pair;
using std::set;

ThemeMetric<int> SORT_BPM_DIVISION("MusicWheel", "SortBPMDivision");
ThemeMetric<bool> SHOW_SECTIONS_IN_BPM_SORT("MusicWheel",
											"ShowSectionsInBPMSort");
void
SongUtil::GetSteps(const Song* pSong,
				   std::vector<Steps*>& arrayAddTo,
				   StepsType st,
				   Difficulty dc,
				   int iMeterLow,
				   int iMeterHigh,
				   bool filteringSteps,
				   const std::string& sDescription,
				   const std::string& sCredit,
				   bool bIncludeAutoGen,
				   unsigned uHash,
				   int iMaxToGet)
{
	if (!iMaxToGet)
		return;

	const auto& vpSteps = st == StepsType_Invalid
							? pSong->GetAllSteps()
							: pSong->GetStepsByStepsType(st);
	for (auto* pSteps : vpSteps) // for each of the Song's Steps
	{
		if (dc != Difficulty_Invalid && dc != pSteps->GetDifficulty())
			continue;
		if (iMeterLow != -1 && iMeterLow > pSteps->GetMeter())
			continue;
		if (iMeterHigh != -1 && iMeterHigh < pSteps->GetMeter())
			continue;
		if (!sDescription.empty() && sDescription != pSteps->GetDescription())
			continue;
		if (!sCredit.empty() && sCredit != pSteps->GetCredit())
			continue;
		if (uHash != 0 && uHash != pSteps->GetHash())
			continue;

		if (filteringSteps && FILTERMAN != nullptr && FILTERMAN->AnyActiveFilter()) {
			// iterating over all rates until it just works
			// explanation in MusicWheel::FilterBySkillsets
			auto success = false;
			for (auto currate = FILTERMAN->MaxFilterRate;
				 currate > FILTERMAN->MinFilterRate - .01f;
				 currate -= 0.1f) {
				if (pSong->ChartMatchesFilter(pSteps, currate)) {
					success = true;
					break;
				}
			}
			if (!success)
				continue;
		}

		arrayAddTo.push_back(pSteps);

		if (iMaxToGet != -1) {
			--iMaxToGet;
			if (!iMaxToGet)
				break;
		}
	}
}

Steps*
SongUtil::GetOneSteps(const Song* pSong,
					  StepsType st,
					  Difficulty dc,
					  int iMeterLow,
					  int iMeterHigh,
					  bool filteringSteps,
					  const std::string& sDescription,
					  const std::string& sCredit,
					  unsigned uHash,
					  bool bIncludeAutoGen)
{
	std::vector<Steps*> vpSteps;
	GetSteps(pSong,
			 vpSteps,
			 st,
			 dc,
			 iMeterLow,
			 iMeterHigh,
			 filteringSteps,
			 sDescription,
			 sCredit,
			 bIncludeAutoGen,
			 uHash,
			 1); // get max 1
	if (vpSteps.empty())
		return nullptr;
	return vpSteps[0];
}

Steps*
SongUtil::GetStepsByDifficulty(const Song* pSong,
							   StepsType st,
							   Difficulty dc,
							   bool bIncludeAutoGen)
{
	const auto& vpSteps = (st >= StepsType_Invalid)
							? pSong->GetAllSteps()
							: pSong->GetStepsByStepsType(st);
	for (auto* pSteps : vpSteps) // for each of the Song's Steps
	{
		if (dc != Difficulty_Invalid && dc != pSteps->GetDifficulty())
			continue;

		return pSteps;
	}

	return nullptr;
}

Steps*
SongUtil::GetStepsByMeter(const Song* pSong,
						  StepsType st,
						  int iMeterLow,
						  int iMeterHigh)
{
	const auto& vpSteps = (st == StepsType_Invalid)
							? pSong->GetAllSteps()
							: pSong->GetStepsByStepsType(st);
	for (auto* pSteps : vpSteps) // for each of the Song's Steps
	{
		if (iMeterLow > pSteps->GetMeter())
			continue;
		if (iMeterHigh < pSteps->GetMeter())
			continue;

		return pSteps;
	}

	return nullptr;
}

Steps*
SongUtil::GetStepsByDescription(const Song* pSong,
								StepsType st,
								const std::string& sDescription)
{
	std::vector<Steps*> vNotes;
	GetSteps(
	  pSong, vNotes, st, Difficulty_Invalid, -1, -1, false, sDescription, "");
	if (vNotes.empty())
		return nullptr;
	return vNotes[0];
}

Steps*
SongUtil::GetStepsByCredit(const Song* pSong,
						   StepsType st,
						   const std::string& sCredit)
{
	std::vector<Steps*> vNotes;
	GetSteps(pSong, vNotes, st, Difficulty_Invalid, -1, -1, false, "", sCredit);
	if (vNotes.empty())
		return nullptr;
	return vNotes[0];
}

Steps*
SongUtil::GetClosestNotes(const Song* pSong,
						  StepsType st,
						  Difficulty dc,
						  bool bIgnoreLocked)
{
	ASSERT(dc != Difficulty_Invalid);

	const auto& vpSteps = (st == StepsType_Invalid)
							? pSong->GetAllSteps()
							: pSong->GetStepsByStepsType(st);
	Steps* pClosest = nullptr;
	auto iClosestDistance = 999;
	for (auto* pSteps : vpSteps) // for each of the Song's Steps
	{
		if (pSteps->GetDifficulty() == Difficulty_Edit && dc != Difficulty_Edit)
			continue;

		const auto iDistance = abs(dc - pSteps->GetDifficulty());
		if (iDistance < iClosestDistance) {
			pClosest = pSteps;
			iClosestDistance = iDistance;
		}
	}

	return pClosest;
}

/* Make any duplicate difficulties edits.  (Note that BMS files do a first pass
 * on this; see BMSLoader::SlideDuplicateDifficulties.) */
void
SongUtil::AdjustDuplicateSteps(Song* pSong)
{
	FOREACH_ENUM(StepsType, st)
	{
		FOREACH_ENUM(Difficulty, dc)
		{
			if (dc == Difficulty_Edit)
				continue;

			std::vector<Steps*> vSteps;
			GetSteps(pSong, vSteps, st, dc);

			/* Delete steps that are completely identical.  This happened due to
			 * a bug in an earlier version. */
			DeleteDuplicateSteps(pSong, vSteps);

			StepsUtil::SortNotesArrayByDifficulty(vSteps);
			for (unsigned k = 1; k < vSteps.size(); k++) {
				vSteps[k]->SetDifficulty(Difficulty_Edit);
				vSteps[k]->SetDupeDiff(true);
				if (vSteps[k]->GetDescription().empty()) {
					/* "Hard Edit" */
					auto EditName =
					  Capitalize(DifficultyToString(dc)) + " Edit";
					vSteps[k]->SetDescription(EditName);
				}
			}
		}

		/* XXX: Don't allow edits to have descriptions that look like regular
		 * difficulties. These are confusing, and they're ambiguous when passed
		 * to GetStepsByID. */
	}
}
/**
 * @brief Remove the initial whitespace characters.
 * @param s the string to left trim.
 * @return the trimmed string.
 */
static std::string
RemoveInitialWhitespace(std::string s)
{
	const auto i = s.find_first_not_of(" \t\r\n");
	if (i != std::string::npos)
		s.erase(0, i);
	return s;
}

/* This is called within TidyUpData, before autogen notes are added. */
void
SongUtil::DeleteDuplicateSteps(Song* pSong, std::vector<Steps*>& vSteps)
{
	/* vSteps have the same StepsType and Difficulty.  Delete them if they have
	 * the same m_sDescription, m_sCredit, m_iMeter and SMNoteData. */
	for (unsigned i = 0; i < vSteps.size(); i++) {
		const Steps* s1 = vSteps[i];

		for (auto j = i + 1; j < vSteps.size(); j++) {
			const Steps* s2 = vSteps[j];

			if (s1->GetDescription() != s2->GetDescription())
				continue;
			if (s1->GetCredit() != s2->GetCredit())
				continue;
			if (s1->GetMeter() != s2->GetMeter())
				continue;
			/* Compare, ignoring whitespace. */
			std::string sSMNoteData1;
			s1->GetSMNoteData(sSMNoteData1);
			std::string sSMNoteData2;
			s2->GetSMNoteData(sSMNoteData2);
			if (RemoveInitialWhitespace(sSMNoteData1) !=
				RemoveInitialWhitespace(sSMNoteData2))
				continue;

			Locator::getLogger()->trace(
			  "Removed {} duplicate steps in song \"{}\" with "
			  "description \"{}\", step author \"{}\", and meter "
			  "\"{}\"",
			  (void*)s2,
			  pSong->GetSongDir().c_str(),
			  s1->GetDescription().c_str(),
			  s1->GetCredit().c_str(),
			  s1->GetMeter());

			pSong->DeleteSteps(s2, false);

			vSteps.erase(vSteps.begin() + j);
			--j;
		}
	}
}

/////////////////////////////////////
// Sorting
/////////////////////////////////////

static LocalizedString SORT_NOT_AVAILABLE("Sort", "NotAvailable");
static LocalizedString SORT_OTHER("Sort", "Other");

/* Just calculating GetNumTimesPlayed within the sort is pretty slow, so let's
 * precompute it.  (This could be generalized with a template.) */
static std::map<const Song*, std::string> g_mapSongSortVal;

static bool
CompareSongPointersBySortValueAscending(const Song* pSong1, const Song* pSong2)
{
	return g_mapSongSortVal[pSong1] < g_mapSongSortVal[pSong2];
}

void
SongUtil::MakeSortString(std::string& s)
{
	s = make_upper(s);

	// Make sure that non-alphanumeric strings are placed at the very end.
	if (!s.empty()) {
		if (s[0] == '.') // like the song ".59"
			s.erase(s.begin());

		if (s[0] == '#')
			return;

		if ((s[0] < 'A' || s[0] > 'Z') && (s[0] < '0' || s[0] > '9'))
			s = static_cast<char>(126) + s;
	}
}

std::string
SongUtil::MakeSortString(const string& in)
{
	auto s = make_upper(in);

	// Make sure that non-alphanumeric strings are placed at the very end.
	if (!s.empty())
		if ((s[0] < 'A' || s[0] > 'Z') && (s[0] < '0' || s[0] > '9'))
			s = static_cast<char>(126) + s;
	return s;
}

static bool
CompareSongPointersByTitle(const Song* pSong1, const Song* pSong2)
{
	// Prefer transliterations to full titles
	auto s1 = pSong1->GetTranslitMainTitle();
	auto s2 = pSong2->GetTranslitMainTitle();
	if (s1 == s2) {
		s1 = pSong1->GetTranslitSubTitle();
		s2 = pSong2->GetTranslitSubTitle();
	}

	SongUtil::MakeSortString(s1);
	SongUtil::MakeSortString(s2);

	const auto ret = strcmp(s1.c_str(), s2.c_str());
	if (ret < 0)
		return true;
	if (ret > 0)
		return false;

	/* The titles are the same.  Ensure we get a consistent ordering
	 * by comparing the unique SongFilePaths. */
	return CompareNoCase(pSong1->GetSongFilePath(), pSong2->GetSongFilePath()) <
		   0;
}

static bool
CompareSongPointersByArtist(const Song* pSong1, const Song* pSong2)
{
	// Prefer transliterations
	auto s1 = pSong1->GetTranslitArtist();
	auto s2 = pSong2->GetTranslitArtist();
	if (s1 == s2) {
		s1 = pSong1->GetTranslitMainTitle();
		s2 = pSong2->GetTranslitMainTitle();
	}

	SongUtil::MakeSortString(s1);
	SongUtil::MakeSortString(s2);

	const auto ret = strcmp(s1.c_str(), s2.c_str());
	if (ret < 0)
		return true;
	if (ret > 0)
		return false;

	/* They are the same.  Ensure we get a consistent ordering
	 * by comparing the unique SongFilePaths. */
	return CompareNoCase(pSong1->GetSongFilePath(), pSong2->GetSongFilePath()) <
		   0;
}

static bool
CompareSongPointersByMSD(const Song* pSong1, const Song* pSong2, Skillset ss)
{
	// Prefer transliterations to full titles
	const auto msd1 = pSong1->HighestMSDOfSkillset(
	  ss, GAMESTATE->m_SongOptions.Get(ModsLevel_Current).m_fMusicRate, true);
	const auto msd2 = pSong2->HighestMSDOfSkillset(
	  ss, GAMESTATE->m_SongOptions.Get(ModsLevel_Current).m_fMusicRate, true);

	if (msd1 < msd2)
		return true;
	if (msd1 > msd2)
		return false;

	/* The titles are the same.  Ensure we get a consistent ordering
	 * by comparing the unique SongFilePaths. */
	return CompareNoCase(pSong1->GetSongFilePath(), pSong2->GetSongFilePath()) <
		   0;
}
void
SongUtil::SortSongPointerArrayByTitle(std::vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByTitle);
}

static bool
CompareSongPointersByBPM(const Song* pSong1, const Song* pSong2)
{
	DisplayBpms bpms1, bpms2;
	pSong1->GetDisplayBpms(bpms1, true);
	pSong2->GetDisplayBpms(bpms2, true);

	if (bpms1.GetMax() < bpms2.GetMax())
		return true;
	if (bpms1.GetMax() > bpms2.GetMax())
		return false;

	return CompareStringsAsc(pSong1->GetSongFilePath(),
							 pSong2->GetSongFilePath());
}

void
SongUtil::SortSongPointerArrayByBPM(std::vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByBPM);
}

static bool
CompareSongPointersByLength(const Song* a, const Song* b)
{
	auto len_a = 0.F;
	for (const auto& s : a->GetChartsMatchingFilter()) {
		const auto& len = s->GetLengthSeconds();
		// if we hit the current preferred difficulty just force use the value
		if (s->GetDifficulty() == GAMESTATE->m_PreferredDifficulty) {
			len_a = len;
			break;
		}

		len_a = len > len_a ? len : len_a;
	}

	// OH NO COPY PASTE WHAT EVER WILL WE DO MAYBE USE A 10 LINE MACRO????
	auto len_b = 0.F;
	for (const auto& s : b->GetChartsMatchingFilter()) {
		const auto& len = s->GetLengthSeconds();

		if (s->GetDifficulty() == GAMESTATE->m_PreferredDifficulty) {
			len_b = len;
			break;
		}

		len_b = len > len_b ? len : len_b;
	}

	if (len_a < len_b)
		return true;
	if (len_a > len_b)
		return false;

	return CompareStringsAsc(a->GetSongFilePath(), b->GetSongFilePath());
}

void
SongUtil::SortSongPointerArrayByLength(std::vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByLength);
}

void
AppendOctal(int n, int digits, std::string& out)
{
	for (auto p = digits - 1; p >= 0; --p) {
		const auto shift = p * 3;
		const auto n2 = (n >> shift) & 0x7;
		out.insert(out.end(), static_cast<char>(n2 + '0'));
	}
}

static auto
get_best_wife_score_for_song_and_profile(const Song* song, const Profile* p)
  -> float
{
	assert(p != nullptr);
	auto st = GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())
				->m_StepsType;
	auto score = p->GetBestWifeScore(song, st);

	// if the only score is a fail we want them all sorted into the same group
	// and all fails best wife scores should be 0% (technically)
	// so ... set them to .001
	// because that is different from 0, for files that have no score
	if (score <= 0.F) {
		// alas, it is possible to get a D if you have nofail on...
		auto g = p->GetBestGrade(song, st);
		if (g == Grade_Failed)
			// the F tier will be filled with random looking F scores
			return 0.001F;
		else if (g == Grade_Tier16)
			// this fills up the D tier with weird looking scores
			// their order changes seemingly randomly
			// to be tbh honest thats not important
			return 0.002F;
		else
			// ????
			return 0.F;
	}
	return score;
}

static int
CompareSongPointersByBestWifeScore(const Song* a, const Song* b)
{
	const auto* p = PROFILEMAN->GetProfile(PLAYER_1);
	return get_best_wife_score_for_song_and_profile(a, p) >
		   get_best_wife_score_for_song_and_profile(b, p);
}

void
SongUtil::SortSongPointerArrayByWifeScore(std::vector<Song*>& v)
{
	sort(v.begin(), v.end(), CompareSongPointersByBestWifeScore);
}

void
SongUtil::SortSongPointerArrayByArtist(std::vector<Song*>& vpSongsInOut)
{
	for (auto& i : vpSongsInOut)
		g_mapSongSortVal[i] =
		  MakeSortString(std::string(i->GetTranslitArtist()));
	stable_sort(vpSongsInOut.begin(),
				vpSongsInOut.end(),
				CompareSongPointersBySortValueAscending);
}

/* This is for internal use, not display; sorting by Unicode codepoints isn't
 * very interesting for display. */
void
SongUtil::SortSongPointerArrayByDisplayArtist(std::vector<Song*>& vpSongsInOut)
{
	for (auto& i : vpSongsInOut)
		g_mapSongSortVal[i] = MakeSortString(i->GetDisplayArtist());
	stable_sort(vpSongsInOut.begin(),
				vpSongsInOut.end(),
				CompareSongPointersBySortValueAscending);
}

static int
CompareSongPointersByGenre(const Song* pSong1, const Song* pSong2)
{
	return pSong1->m_sGenre < pSong2->m_sGenre;
}

void
SongUtil::SortSongPointerArrayByGenre(std::vector<Song*>& vpSongsInOut)
{
	stable_sort(
	  vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByGenre);
}

int
SongUtil::CompareSongPointersByGroup(const Song* pSong1, const Song* pSong2)
{
	return pSong1->m_sGroupName < pSong2->m_sGroupName;
}

static int
CompareSongPointersByGroupAndTitle(const Song* pSong1, const Song* pSong2)
{
	const auto g = SongUtil::CompareSongPointersByGroup(pSong1, pSong2);
	if (g == 0)
		/* Same group; compare by name. */
		return CompareSongPointersByTitle(pSong1, pSong2);
	return g < 0;
}

static int
CompareSongPointersByGroup(const Song* pSong1, const Song* pSong2)
{
	const auto& sGroup1 = pSong1->m_sGroupName;
	const auto& sGroup2 = pSong2->m_sGroupName;

	if (sGroup1 < sGroup2)
		return -1;
	if (sGroup1 > sGroup2)
		return 1;
	return 0;
}

std::function<int(const Song* pSong1, const Song* pSong2)>
CompareSongPointersByGroupAndMSD(Skillset ss)
{
	return [ss](const Song* pSong1, const Song* pSong2) {
		const auto g = CompareSongPointersByGroup(pSong1, pSong2);
		if (g == 0)
			/* Same group; compare by MSD. */
			return static_cast<int>(
			  CompareSongPointersByMSD(pSong1, pSong2, ss));
		return static_cast<int>(g < 0);
	};
}
void
SongUtil::SortSongPointerArrayByGroupAndTitle(std::vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(),
		 vpSongsInOut.end(),
		 CompareSongPointersByGroupAndTitle);
}
void
SongUtil::SortSongPointerArrayByGroupAndMSD(std::vector<Song*>& vpSongsInOut,
											Skillset ss)
{
	sort(vpSongsInOut.begin(),
		 vpSongsInOut.end(),
		 CompareSongPointersByGroupAndMSD(ss));
}

void
SongUtil::SortSongPointerArrayByNumPlays(std::vector<Song*>& vpSongsInOut,
										 ProfileSlot slot,
										 bool bDescending)
{
	auto* pProfile = PROFILEMAN->GetProfile(slot);
	SortSongPointerArrayByNumPlays(vpSongsInOut, pProfile, bDescending);
}

// dumb and should be handled by scoreman
void
SongUtil::SortSongPointerArrayByNumPlays(std::vector<Song*>& vpSongsInOut,
										 const Profile* pProfile,
										 bool bDescending)
{
	return;
}

std::string
SongUtil::GetSectionNameFromSongAndSort(const Song* pSong, SortOrder so)
{
	if (pSong == nullptr)
		return std::string();

	switch (so) {
		case SORT_FAVORITES:
		case SORT_GROUP:
		case SORT_Overall:
		case SORT_Stream:
		case SORT_Jumpstream:
		case SORT_Handstream:
		case SORT_Stamina:
		case SORT_JackSpeed:
		case SORT_Chordjack:
		case SORT_Technical:
			// guaranteed not empty
			return pSong->m_sGroupName;
		case SORT_TITLE:
		case SORT_ARTIST: {
			std::string s;
			switch (so) {
				case SORT_TITLE:
					s = pSong->GetTranslitMainTitle();
					break;
				case SORT_ARTIST:
					s = pSong->GetTranslitArtist();
					break;
				default:
					FAIL_M(ssprintf("Unexpected SortOrder: %i", so));
			}
			MakeSortString(s); // resulting string will be uppercase

			if (s.empty())
				return std::string();
			if (s[0] >= '0' && s[0] <= '9')
				return "0-9";
			if (s[0] < 'A' || s[0] > 'Z')
				return SORT_OTHER.GetValue();
			return s.substr(0, 1);
		}
		case SORT_GENRE:
			if (!pSong->m_sGenre.empty())
				return pSong->m_sGenre;
			return SORT_NOT_AVAILABLE.GetValue();
		case SORT_BPM: {
			if (SHOW_SECTIONS_IN_BPM_SORT) {
				const int iBPMGroupSize = SORT_BPM_DIVISION;
				DisplayBpms bpms;
				pSong->GetDisplayBpms(bpms, true);
				auto iMaxBPM = static_cast<int>(bpms.GetMax());
				iMaxBPM += iBPMGroupSize - (iMaxBPM % iBPMGroupSize) - 1;
				return ssprintf(
				  "%03d-%03d", iMaxBPM - (iBPMGroupSize - 1), iMaxBPM);
			}
			return std::string();
		}
		case SORT_LENGTH: {
			const auto iSortLengthSize = 60;

			auto len_a = 0.F;
			// should probably be an actual util function because copy pasted
			// from length sort above

			for (const auto& s : pSong->GetChartsMatchingFilter()) {
				const auto& len = s->GetLengthSeconds();

				if (s->GetDifficulty() == GAMESTATE->m_PreferredDifficulty) {
					len_a = len;
					break;
				}

				len_a = len > len_a ? len : len_a;
			}

			auto iMaxLength = static_cast<int>(len_a);
			iMaxLength +=
			  (iSortLengthSize - (iMaxLength % iSortLengthSize) - 1);
			const auto iMinLength = iMaxLength - (iSortLengthSize - 1);
			return ssprintf(
			  "%s-%s",
			  SecondsToMMSS(static_cast<float>(iMinLength)).c_str(),
			  SecondsToMMSS(static_cast<float>(iMaxLength)).c_str());
		}
		case SORT_TOP_GRADES: {
			auto* p = PROFILEMAN->GetProfile(PLAYER_1);
			const auto* s =
			  GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber());
			if (p == nullptr || s == nullptr)
				return GradeToLocalizedString(Grade_Invalid);

			return GradeToLocalizedString(
			  PROFILEMAN->GetProfile(PLAYER_1)->GetBestGrade(pSong,
															 s->m_StepsType));
		}
		case SORT_MODE_MENU:
			return std::string();
		case SORT_Ungrouped:
			return "";
		default:
			FAIL_M(ssprintf("Invalid SortOrder: %i", so));
	}
}

void
SongUtil::SortSongPointerArrayBySectionName(std::vector<Song*>& vpSongsInOut,
											SortOrder so)
{
	const auto sOther = SORT_OTHER.GetValue();
	for (auto& i : vpSongsInOut) {
		auto val = GetSectionNameFromSongAndSort(i, so);

		// Make sure 0-9 comes first and OTHER comes last.
		if (val == "0-9")
			val = "0";
		else if (val == sOther)
			val = "2";
		else {
			MakeSortString(val);
			val = "1" + val;
		}

		g_mapSongSortVal[i] = val;
	}

	stable_sort(vpSongsInOut.begin(),
				vpSongsInOut.end(),
				CompareSongPointersBySortValueAscending);
	g_mapSongSortVal.clear();
}

void
SongUtil::SortSongPointerArrayByStepsTypeAndMeter(std::vector<Song*>& vpSongsInOut,
												  StepsType st,
												  Difficulty dc)
{
	g_mapSongSortVal.clear();
	for (auto& i : vpSongsInOut) {
		// Ignore locked steps.
		const Steps* pSteps = GetClosestNotes(i, st, dc, true);
		auto& s = g_mapSongSortVal[i];
		s = ssprintf("%03d", pSteps ? pSteps->GetMeter() : 0);

		/* pSteps may not be exactly the difficulty we want; for example, we
		 * may be sorting by Hard difficulty and a song may have no Hard steps.
		 * In this case, we can end up with unintuitive ties; for example,
		 * pSteps may be Medium with a meter of 5, which will sort it among the
		 * 5-meter Hard songs.  Break the tie, by adding the difficulty to the
		 * sort as well. That way, we'll always put Medium 5s before Hard 5s. If
		 * all songs are using the preferred difficulty (dc), this will be a
		 * no-op. */
		s += ssprintf("%c", (pSteps ? pSteps->GetDifficulty() : 0) + '0');
	}
	stable_sort(vpSongsInOut.begin(),
				vpSongsInOut.end(),
				CompareSongPointersBySortValueAscending);
}

bool
SongUtil::IsEditDescriptionUnique(const Song* pSong,
								  StepsType st,
								  const std::string& sPreferredDescription,
								  const Steps* pExclude)
{
	for (const auto& s : pSong->GetAllSteps()) {
		auto* pSteps = s;

		if (pSteps->GetDifficulty() != Difficulty_Edit)
			continue;
		if (pSteps->m_StepsType != st)
			continue;
		if (pSteps == pExclude)
			continue;
		if (pSteps->GetDescription() == sPreferredDescription)
			return false;
	}
	return true;
}

bool
SongUtil::IsChartNameUnique(const Song* pSong,
							StepsType st,
							const std::string& name,
							const Steps* pExclude)
{
	for (const auto& pSteps : pSong->GetAllSteps()) {
		if (pSteps->m_StepsType != st)
			continue;
		if (pSteps == pExclude)
			continue;
		if (pSteps->GetChartName() == name)
			return false;
	}
	return true;
}

std::string
SongUtil::MakeUniqueEditDescription(const Song* pSong,
									StepsType st,
									const std::string& sPreferredDescription)
{
	if (IsEditDescriptionUnique(pSong, st, sPreferredDescription, nullptr))
		return sPreferredDescription;

	for (auto i = 0; i < 1000; i++) {
		// make name "My Edit" -> "My Edit2"
		auto sNum = ssprintf("%d", i + 1);
		std::string sTemp = sPreferredDescription.substr(
							  0, MAX_STEPS_DESCRIPTION_LENGTH - sNum.size()) +
							sNum;

		if (IsEditDescriptionUnique(pSong, st, sTemp, nullptr))
			return sTemp;
	}

	// Edit limit guards should prevent this
	FAIL_M("Exceeded limit of 1000 edits per song");
}

static LocalizedString YOU_MUST_SUPPLY_NAME(
  "SongUtil",
  "You must supply a name for your new edit.");
static LocalizedString CHART_NAME_CONFLICTS("SongUtil",
											"The name you chose conflicts with "
											"another chart. Please use a "
											"different name.");
static LocalizedString EDIT_NAME_CONFLICTS("SongUtil",
										   "The name you chose conflicts with "
										   "another edit. Please use a "
										   "different name.");
static LocalizedString CHART_NAME_CANNOT_CONTAIN(
  "SongUtil",
  "The chart name cannot contain any of the following characters: %s");
static LocalizedString EDIT_NAME_CANNOT_CONTAIN(
  "SongUtil",
  "The edit name cannot contain any of the following characters: %s");

bool
SongUtil::ValidateCurrentStepsDescription(const std::string& sAnswer,
										  std::string& sErrorOut)
{
	if (sAnswer.empty())
		return true;

	/* Don't allow duplicate edit names within the same StepsType; edit names
	 * uniquely identify the edit. */
	Steps* pSteps = GAMESTATE->m_pCurSteps;

	// If unchanged:
	if (pSteps->GetDescription() == sAnswer)
		return true;

	return true;
}

bool
SongUtil::ValidateCurrentStepsChartName(const std::string& answer,
										std::string& error)
{
	if (answer.empty())
		return true;

	static const std::string sInvalidChars = "\\/:*?\"<>|";
	if (strpbrk(answer.c_str(), sInvalidChars.c_str()) != nullptr) {
		error =
		  ssprintf(CHART_NAME_CANNOT_CONTAIN.GetValue(), sInvalidChars.c_str());
		return false;
	}

	/* Don't allow duplicate title names within the same StepsType.
	 * We need some way of identifying the unique charts. */
	Steps* pSteps = GAMESTATE->m_pCurSteps;

	if (pSteps->GetChartName() == answer)
		return true;

	// TODO next commit: borrow code from EditStepsDescription.
	const auto result = IsChartNameUnique(
	  GAMESTATE->m_pCurSong, pSteps->m_StepsType, answer, pSteps);
	if (!result)
		error = CHART_NAME_CONFLICTS;
	return result;
}

static LocalizedString AUTHOR_NAME_CANNOT_CONTAIN(
  "SongUtil",
  "The step author's name cannot contain any of the following characters: %s");

bool
SongUtil::ValidateCurrentStepsCredit(const std::string& sAnswer,
									 std::string& sErrorOut)
{
	if (sAnswer.empty())
		return true;

	Steps* pSteps = GAMESTATE->m_pCurSteps;
	// If unchanged:
	if (pSteps->GetCredit() == sAnswer)
		return true;

	// Borrow from EditDescription testing. Perhaps this should be abstracted?
	// -Wolfman2000
	static const std::string sInvalidChars = "\\/:*?\"<>|";
	if (strpbrk(sAnswer.c_str(), sInvalidChars.c_str()) != nullptr) {
		sErrorOut = ssprintf(AUTHOR_NAME_CANNOT_CONTAIN.GetValue(),
							 sInvalidChars.c_str());
		return false;
	}

	return true;
}

static LocalizedString PREVIEW_DOES_NOT_EXIST(
  "SongUtil",
  "The preview file '%s' does not exist.");
bool
SongUtil::ValidateCurrentSongPreview(const std::string& answer,
									 std::string& error)
{
	if (answer.empty()) {
		return true;
	}
	Song* song = GAMESTATE->m_pCurSong;
	const auto real_file = song->m_PreviewFile;
	song->m_PreviewFile = answer;
	const auto path = song->GetPreviewMusicPath();
	const auto valid = DoesFileExist(path);
	song->m_PreviewFile = real_file;
	if (!valid) {
		error = ssprintf(PREVIEW_DOES_NOT_EXIST.GetValue(), answer.c_str());
	}
	return valid;
}

static LocalizedString MUSIC_DOES_NOT_EXIST(
  "SongUtil",
  "The music file '%s' does not exist.");
bool
SongUtil::ValidateCurrentStepsMusic(const std::string& answer,
									std::string& error)
{
	if (answer.empty())
		return true;
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	const auto real_file = pSteps->GetMusicFile();
	pSteps->SetMusicFile(answer);
	const auto path = pSteps->GetMusicPath();
	const auto valid = DoesFileExist(path);
	pSteps->SetMusicFile(real_file);
	if (!valid) {
		error = ssprintf(MUSIC_DOES_NOT_EXIST.GetValue(), answer.c_str());
	}
	return valid;
}

void
SongUtil::GetAllSongGenres(std::vector<std::string>& vsOut)
{
	set<std::string> genres;
	for (const auto& song : SONGMAN->GetAllSongs()) {
		if (!song->m_sGenre.empty())
			genres.insert(song->m_sGenre);
	}

	for (const auto& s : genres) {
		vsOut.push_back(s);
	}
}

void
SongUtil::GetPlayableStepsTypes(const Song* pSong, set<StepsType>& vOut)
{
	std::vector<const Style*> vpPossibleStyles;
	// If AutoSetStyle, or a Style hasn't been chosen, check StepsTypes for all
	// Styles.
	if (CommonMetrics::AUTO_SET_STYLE ||
		GAMESTATE->GetCurrentStyle(PLAYER_INVALID) == nullptr)
		GAMEMAN->GetCompatibleStyles(GAMESTATE->m_pCurGame,
									 GAMESTATE->GetNumPlayersEnabled(),
									 vpPossibleStyles);
	else
		vpPossibleStyles.push_back(GAMESTATE->GetCurrentStyle(PLAYER_INVALID));

	set<StepsType> vStepsTypes;
	for (auto& s : vpPossibleStyles) {
		vStepsTypes.insert(s->m_StepsType);
	}

	/* filter out hidden StepsTypes */
	const auto& vstToShow = CommonMetrics::STEPS_TYPES_TO_SHOW.GetValue();
	for (const auto& st : vStepsTypes) {
		const auto bShowThisStepsType =
		  find(vstToShow.begin(), vstToShow.end(), st) != vstToShow.end();

		auto iNumPlayers = GAMESTATE->GetNumPlayersEnabled();
		iNumPlayers = std::max(iNumPlayers, 1);

		if (bShowThisStepsType)
			vOut.insert(st);
	}
}

void
SongUtil::GetPlayableSteps(const Song* pSong,
						   std::vector<Steps*>& vOut,
						   bool filteringSteps)
{
	set<StepsType> vStepsType;
	GetPlayableStepsTypes(pSong, vStepsType);

	for (const auto& st : vStepsType) {
		GetSteps(pSong, vOut, st, Difficulty_Invalid, -1, -1, filteringSteps);
	}

	StepsUtil::SortNotesArrayByDifficulty(vOut);
	StepsUtil::SortStepsByTypeAndDifficulty(vOut);
}

bool
SongUtil::IsStepsTypePlayable(Song* pSong, StepsType st)
{
	set<StepsType> vStepsType;
	GetPlayableStepsTypes(pSong, vStepsType);
	return vStepsType.find(st) != vStepsType.end();
}

bool
SongUtil::IsStepsPlayable(Song* pSong, Steps* pSteps)
{
	std::vector<Steps*> vpSteps;
	GetPlayableSteps(pSong, vpSteps);
	return find(vpSteps.begin(), vpSteps.end(), pSteps) != vpSteps.end();
}

bool
SongUtil::IsSongPlayable(Song* s)
{
	for (const auto& step : s->GetAllSteps()) {
		if (IsStepsPlayable(s, step)) {
			return true;
		}
	}

	return false;
}

bool
SongUtil::GetStepsTypeAndDifficultyFromSortOrder(SortOrder so,
												 StepsType& stOut,
												 Difficulty& dcOut)
{
	return true;
}

//////////////////////////////////
// SongID
//////////////////////////////////

void
SongID::FromSong(const Song* p)
{
	if (p != nullptr)
		sDir = p->GetSongDir();
	else
		sDir = "";

	// HACK for backwards compatibility:
	// Strip off leading "/".  2005/05/21 file layer changes added a leading
	// slash.
	if (!sDir.empty()) {
		if (sDir.front() == '/') {
			sDir.erase(sDir.begin());
		}
	}

	m_Cache.Unset();
}

Song*
SongID::ToSong() const
{
	Song* pRet = nullptr;
	if (!m_Cache.Get(&pRet)) {
		if (!sDir.empty()) {
			// HACK for backwards compatibility: Re-add the leading "/".
			// 2005/05/21 file layer changes added a leading slash.
			auto sDir2 = sDir;
			if (sDir2.front() != '/') {
				sDir2 = "/" + sDir2;
			}
			pRet = SONGMAN->GetSongFromDir(sDir2);
		}
		m_Cache.Set(pRet);
	}
	return pRet;
}

XNode*
SongID::CreateNode() const
{
	auto* pNode = new XNode("Song");
	pNode->AppendAttr("Dir", sDir);
	return pNode;
}

void
SongID::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "Song");
	pNode->GetAttrValue("Dir", sDir);
	m_Cache.Unset();
}

void
SongID::LoadFromString(const char* dir)
{
	sDir = dir;
	m_Cache.Unset();
}

std::string
SongID::ToString() const
{
	return (sDir.empty() ? std::string() : sDir);
}

bool
SongID::IsValid() const
{
	return ToSong() != nullptr;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

namespace {
int
GetPlayableSteps(lua_State* L)
{
	const Song* pSong = Luna<Song>::check(L, 1, true);
	std::vector<Steps*> vSteps;
	SongUtil::GetPlayableSteps(pSong, vSteps);
	LuaHelpers::CreateTableFromArray<Steps*>(vSteps, L);
	return 1;
}
int
IsStepsTypePlayable(lua_State* L)
{
	auto* pSong = Luna<Song>::check(L, 1, true);
	const auto st = Enum::Check<StepsType>(L, 2);
	const auto b = SongUtil::IsStepsTypePlayable(pSong, st);
	LuaHelpers::Push(L, b);
	return 1;
}
int
IsStepsPlayable(lua_State* L)
{
	auto* pSong = Luna<Song>::check(L, 1, true);
	auto* pSteps = Luna<Steps>::check(L, 2, true);
	const auto b = SongUtil::IsStepsPlayable(pSong, pSteps);
	LuaHelpers::Push(L, b);
	return 1;
}

int
SongTitleComparator(lua_State* L)
{
	auto* p1 = Luna<Song>::check(L, 1, true);
	auto* p2 = Luna<Song>::check(L, 2, true);
	const auto b = CompareSongPointersByTitle(p1, p2);
	LuaHelpers::Push(L, b);
	return 1;
}

int
SongArtistComparator(lua_State* L)
{
	auto* p1 = Luna<Song>::check(L, 1, true);
	auto* p2 = Luna<Song>::check(L, 2, true);
	const auto b = CompareSongPointersByArtist(p1, p2);
	LuaHelpers::Push(L, b);
	return 1;
}

const luaL_Reg SongUtilTable[] = { LIST_METHOD(GetPlayableSteps),
								   LIST_METHOD(IsStepsTypePlayable),
								   LIST_METHOD(IsStepsPlayable),
								   LIST_METHOD(SongTitleComparator),
								   LIST_METHOD(SongArtistComparator),
								   { nullptr, nullptr } };
}

LUA_REGISTER_NAMESPACE(SongUtil)
