#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Models/Misc/Profile.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/FileTypes/XmlFile.h"
#include "Etterna/Models/StepsAndStyles/StepsUtil.h"

ThemeMetric<int> SORT_BPM_DIVISION("MusicWheel", "SortBPMDivision");
ThemeMetric<bool> SHOW_SECTIONS_IN_BPM_SORT("MusicWheel",
											"ShowSectionsInBPMSort");
void
SongUtil::GetSteps(const Song* pSong,
				   vector<Steps*>& arrayAddTo,
				   StepsType st,
				   Difficulty dc,
				   int iMeterLow,
				   int iMeterHigh,
				   const RString& sDescription,
				   const RString& sCredit,
				   bool bIncludeAutoGen,
				   unsigned uHash,
				   int iMaxToGet)
{
	if (!iMaxToGet)
		return;

	const vector<Steps*>& vpSteps = st == StepsType_Invalid
									  ? pSong->GetAllSteps()
									  : pSong->GetStepsByStepsType(st);
	for (unsigned i = 0; i < vpSteps.size();
		 i++) // for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if (dc != Difficulty_Invalid && dc != pSteps->GetDifficulty())
			continue;
		if (iMeterLow != -1 && iMeterLow > pSteps->GetMeter())
			continue;
		if (iMeterHigh != -1 && iMeterHigh < pSteps->GetMeter())
			continue;
		if (sDescription.size() && sDescription != pSteps->GetDescription())
			continue;
		if (sCredit.size() && sCredit != pSteps->GetCredit())
			continue;
		if (uHash != 0 && uHash != pSteps->GetHash())
			continue;

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
					  const RString& sDescription,
					  const RString& sCredit,
					  unsigned uHash,
					  bool bIncludeAutoGen)
{
	vector<Steps*> vpSteps;
	GetSteps(pSong,
			 vpSteps,
			 st,
			 dc,
			 iMeterLow,
			 iMeterHigh,
			 sDescription,
			 sCredit,
			 bIncludeAutoGen,
			 uHash,
			 1); // get max 1
	if (vpSteps.empty())
		return NULL;
	else
		return vpSteps[0];
}

Steps*
SongUtil::GetStepsByDifficulty(const Song* pSong,
							   StepsType st,
							   Difficulty dc,
							   bool bIncludeAutoGen)
{
	const vector<Steps*>& vpSteps = (st >= StepsType_Invalid)
									  ? pSong->GetAllSteps()
									  : pSong->GetStepsByStepsType(st);
	for (unsigned i = 0; i < vpSteps.size();
		 i++) // for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if (dc != Difficulty_Invalid && dc != pSteps->GetDifficulty())
			continue;

		return pSteps;
	}

	return NULL;
}

Steps*
SongUtil::GetStepsByMeter(const Song* pSong,
						  StepsType st,
						  int iMeterLow,
						  int iMeterHigh)
{
	const vector<Steps*>& vpSteps = (st == StepsType_Invalid)
									  ? pSong->GetAllSteps()
									  : pSong->GetStepsByStepsType(st);
	for (unsigned i = 0; i < vpSteps.size();
		 i++) // for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if (iMeterLow > pSteps->GetMeter())
			continue;
		if (iMeterHigh < pSteps->GetMeter())
			continue;

		return pSteps;
	}

	return NULL;
}

Steps*
SongUtil::GetStepsByDescription(const Song* pSong,
								StepsType st,
								const RString& sDescription)
{
	vector<Steps*> vNotes;
	GetSteps(pSong, vNotes, st, Difficulty_Invalid, -1, -1, sDescription, "");
	if (vNotes.size() == 0)
		return NULL;
	else
		return vNotes[0];
}

Steps*
SongUtil::GetStepsByCredit(const Song* pSong,
						   StepsType st,
						   const RString& sCredit)
{
	vector<Steps*> vNotes;
	GetSteps(pSong, vNotes, st, Difficulty_Invalid, -1, -1, "", sCredit);
	if (vNotes.size() == 0)
		return NULL;
	else
		return vNotes[0];
}

Steps*
SongUtil::GetClosestNotes(const Song* pSong,
						  StepsType st,
						  Difficulty dc,
						  bool bIgnoreLocked)
{
	ASSERT(dc != Difficulty_Invalid);

	const vector<Steps*>& vpSteps = (st == StepsType_Invalid)
									  ? pSong->GetAllSteps()
									  : pSong->GetStepsByStepsType(st);
	Steps* pClosest = NULL;
	int iClosestDistance = 999;
	for (unsigned i = 0; i < vpSteps.size();
		 i++) // for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if (pSteps->GetDifficulty() == Difficulty_Edit && dc != Difficulty_Edit)
			continue;

		int iDistance = abs(dc - pSteps->GetDifficulty());
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

			vector<Steps*> vSteps;
			SongUtil::GetSteps(pSong, vSteps, st, dc);

			/* Delete steps that are completely identical.  This happened due to
			 * a bug in an earlier version. */
			DeleteDuplicateSteps(pSong, vSteps);

			StepsUtil::SortNotesArrayByDifficulty(vSteps);
			for (unsigned k = 1; k < vSteps.size(); k++) {
				vSteps[k]->SetDifficulty(Difficulty_Edit);
				vSteps[k]->SetDupeDiff(true);
				if (vSteps[k]->GetDescription() == "") {
					/* "Hard Edit" */
					RString EditName =
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
static RString
RemoveInitialWhitespace(RString s)
{
	size_t i = s.find_first_not_of(" \t\r\n");
	if (i != s.npos)
		s.erase(0, i);
	return s;
}

/* This is called within TidyUpData, before autogen notes are added. */
void
SongUtil::DeleteDuplicateSteps(Song* pSong, vector<Steps*>& vSteps)
{
	/* vSteps have the same StepsType and Difficulty.  Delete them if they have
	 * the same m_sDescription, m_sCredit, m_iMeter and SMNoteData. */
	for (unsigned i = 0; i < vSteps.size(); i++) {
		const Steps* s1 = vSteps[i];

		for (unsigned j = i + 1; j < vSteps.size(); j++) {
			const Steps* s2 = vSteps[j];

			if (s1->GetDescription() != s2->GetDescription())
				continue;
			if (s1->GetCredit() != s2->GetCredit())
				continue;
			if (s1->GetMeter() != s2->GetMeter())
				continue;
			/* Compare, ignoring whitespace. */
			RString sSMNoteData1;
			s1->GetSMNoteData(sSMNoteData1);
			RString sSMNoteData2;
			s2->GetSMNoteData(sSMNoteData2);
			if (RemoveInitialWhitespace(sSMNoteData1) !=
				RemoveInitialWhitespace(sSMNoteData2))
				continue;

			LOG->Trace("Removed %p duplicate steps in song \"%s\" with "
					   "description \"%s\", step author \"%s\", and meter "
					   "\"%i\"",
					   s2,
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
static map<const Song*, RString> g_mapSongSortVal;

static bool
CompareSongPointersBySortValueAscending(const Song* pSong1, const Song* pSong2)
{
	return g_mapSongSortVal[pSong1] < g_mapSongSortVal[pSong2];
}

static bool
CompareSongPointersBySortValueDescending(const Song* pSong1, const Song* pSong2)
{
	return g_mapSongSortVal[pSong1] > g_mapSongSortVal[pSong2];
}

void
SongUtil::MakeSortString(RString& s)
{
	s.MakeUpper();

	// Make sure that non-alphanumeric strings are placed at the very end.
	if (s.size() > 0) {
		if (s[0] == '.') // like the song ".59"
			s.erase(s.begin());

		if (s[0] == '#')
			return;

		if ((s[0] < 'A' || s[0] > 'Z') && (s[0] < '0' || s[0] > '9'))
			s = char(126) + s;
	}
}

RString
SongUtil::MakeSortString(const string& in)
{
	RString s = in;
	s.MakeUpper();

	// Make sure that non-alphanumeric strings are placed at the very end.
	if (s.size() > 0)
		if ((s[0] < 'A' || s[0] > 'Z') && (s[0] < '0' || s[0] > '9'))
			s = char(126) + s;
	return s;
}

static bool
CompareSongPointersByTitle(const Song* pSong1, const Song* pSong2)
{
	// Prefer transliterations to full titles
	RString s1 = pSong1->GetTranslitMainTitle();
	RString s2 = pSong2->GetTranslitMainTitle();
	if (s1 == s2) {
		s1 = pSong1->GetTranslitSubTitle();
		s2 = pSong2->GetTranslitSubTitle();
	}

	SongUtil::MakeSortString(s1);
	SongUtil::MakeSortString(s2);

	int ret = strcmp(s1, s2);
	if (ret < 0)
		return true;
	if (ret > 0)
		return false;

	/* The titles are the same.  Ensure we get a consistent ordering
	 * by comparing the unique SongFilePaths. */
	return CompareNoCaseLUL(pSong1->GetSongFilePath(),
							pSong2->GetSongFilePath()) < 0;
}

static bool
CompareSongPointersByMSD(const Song* pSong1, const Song* pSong2, Skillset ss)
{
	// Prefer transliterations to full titles
	float msd1 = pSong1->HighestMSDOfSkillset(
	  ss, GAMESTATE->m_SongOptions.Get(ModsLevel_Current).m_fMusicRate);
	float msd2 = pSong2->HighestMSDOfSkillset(
	  ss, GAMESTATE->m_SongOptions.Get(ModsLevel_Current).m_fMusicRate);

	if (msd1 < msd2)
		return true;
	if (msd1 > msd2)
		return false;

	/* The titles are the same.  Ensure we get a consistent ordering
	 * by comparing the unique SongFilePaths. */
	return CompareNoCaseLUL(pSong1->GetSongFilePath(),
							pSong2->GetSongFilePath()) < 0;
}
void
SongUtil::SortSongPointerArrayByTitle(vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByTitle);
}

static bool
CompareSongPointersByBPM(const Song* pSong1, const Song* pSong2)
{
	DisplayBpms bpms1, bpms2;
	pSong1->GetDisplayBpms(bpms1);
	pSong2->GetDisplayBpms(bpms2);

	if (bpms1.GetMax() < bpms2.GetMax())
		return true;
	if (bpms1.GetMax() > bpms2.GetMax())
		return false;

	return CompareRStringsAsc(pSong1->GetSongFilePath(),
							  pSong2->GetSongFilePath());
}

void
SongUtil::SortSongPointerArrayByBPM(vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByBPM);
}

static bool
CompareSongPointersByLength(const Song* pSong1, const Song* pSong2)
{
	float length1, length2;
	length1 = pSong1->m_fMusicLengthSeconds;
	length2 = pSong2->m_fMusicLengthSeconds;

	if (length1 < length2)
		return true;
	if (length1 > length2)
		return false;

	return CompareRStringsAsc(pSong1->GetSongFilePath(),
							  pSong2->GetSongFilePath());
}

void
SongUtil::SortSongPointerArrayByLength(vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(), vpSongsInOut.end(), CompareSongPointersByLength);
}

void
AppendOctal(int n, int digits, RString& out)
{
	for (int p = digits - 1; p >= 0; --p) {
		const int shift = p * 3;
		int n2 = (n >> shift) & 0x7;
		out.insert(out.end(), (char)(n2 + '0'));
	}
}

static bool
CompDescending(const pair<Song*, int>& a, const pair<Song*, int>& b)
{
	return a.second < b.second;
}
static bool
CompAscending(const pair<Song*, int>& a, const pair<Song*, int>& b)
{
	return a.second > b.second;
}

void
SongUtil::SortSongPointerArrayByGrades(vector<Song*>& vpSongsInOut,
									   bool bDescending)
{
	/* Optimize by pre-writing a string to compare, since doing
	 * GetNumNotesWithGrade inside the sort is too slow. */
	typedef pair<Song*, int> val;
	vector<val> vals;
	vals.reserve(vpSongsInOut.size());
	const Profile* pProfile = PROFILEMAN->GetProfile(PLAYER_1);

	for (unsigned i = 0; i < vpSongsInOut.size(); ++i) {
		Song* pSong = vpSongsInOut[i];
		ASSERT(pProfile != NULL);
		int g = static_cast<int>(pProfile->GetBestGrade(
		  pSong,
		  GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber())
			->m_StepsType));
		vals.push_back(val(pSong, g));
	}

	sort(
	  vals.begin(), vals.end(), bDescending ? CompDescending : CompAscending);

	for (unsigned i = 0; i < vpSongsInOut.size(); ++i)
		vpSongsInOut[i] = vals[i].first;
}

void
SongUtil::SortSongPointerArrayByArtist(vector<Song*>& vpSongsInOut)
{
	for (unsigned i = 0; i < vpSongsInOut.size(); ++i)
		g_mapSongSortVal[vpSongsInOut[i]] =
		  MakeSortString(RString(vpSongsInOut[i]->GetTranslitArtist()));
	stable_sort(vpSongsInOut.begin(),
				vpSongsInOut.end(),
				CompareSongPointersBySortValueAscending);
}

/* This is for internal use, not display; sorting by Unicode codepoints isn't
 * very interesting for display. */
void
SongUtil::SortSongPointerArrayByDisplayArtist(vector<Song*>& vpSongsInOut)
{
	for (unsigned i = 0; i < vpSongsInOut.size(); ++i)
		g_mapSongSortVal[vpSongsInOut[i]] =
		  MakeSortString(vpSongsInOut[i]->GetDisplayArtist());
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
SongUtil::SortSongPointerArrayByGenre(vector<Song*>& vpSongsInOut)
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
	int g = SongUtil::CompareSongPointersByGroup(pSong1, pSong2);
	if (g == 0)
		/* Same group; compare by name. */
		return CompareSongPointersByTitle(pSong1, pSong2);
	return g < 0;
}

static int
CompareSongPointersByGroup(const Song* pSong1, const Song* pSong2)
{
	const RString& sGroup1 = pSong1->m_sGroupName;
	const RString& sGroup2 = pSong2->m_sGroupName;

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
		int g = CompareSongPointersByGroup(pSong1, pSong2);
		if (g == 0)
			/* Same group; compare by MSD. */
			return static_cast<int>(
			  CompareSongPointersByMSD(pSong1, pSong2, ss));
		return static_cast<int>(g < 0);
	};
}
void
SongUtil::SortSongPointerArrayByGroupAndTitle(vector<Song*>& vpSongsInOut)
{
	sort(vpSongsInOut.begin(),
		 vpSongsInOut.end(),
		 CompareSongPointersByGroupAndTitle);
}
void
SongUtil::SortSongPointerArrayByGroupAndMSD(vector<Song*>& vpSongsInOut,
											Skillset ss)
{
	sort(vpSongsInOut.begin(),
		 vpSongsInOut.end(),
		 CompareSongPointersByGroupAndMSD(ss));
}

void
SongUtil::SortSongPointerArrayByNumPlays(vector<Song*>& vpSongsInOut,
										 ProfileSlot slot,
										 bool bDescending)
{
	Profile* pProfile = PROFILEMAN->GetProfile(slot);
	SortSongPointerArrayByNumPlays(vpSongsInOut, pProfile, bDescending);
}

// dumb and should be handled by scoreman
void
SongUtil::SortSongPointerArrayByNumPlays(vector<Song*>& vpSongsInOut,
										 const Profile* pProfile,
										 bool bDescending)
{
	return;
}

RString
SongUtil::GetSectionNameFromSongAndSort(const Song* pSong, SortOrder so)
{
	if (pSong == NULL)
		return RString();

	switch (so) {
		case SORT_FAVORITES:
		case SORT_PREFERRED:
			return SONGMAN->SongToPreferredSortSectionName(pSong);
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
			RString s;
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
				return RString();
			else if (s[0] >= '0' && s[0] <= '9')
				return "0-9";
			else if (s[0] < 'A' || s[0] > 'Z')
				return SORT_OTHER.GetValue();
			else
				return s.Left(1);
		}
		case SORT_GENRE:
			if (!pSong->m_sGenre.empty())
				return pSong->m_sGenre;
			return SORT_NOT_AVAILABLE.GetValue();
		case SORT_BPM: {
			if (SHOW_SECTIONS_IN_BPM_SORT) {
				const int iBPMGroupSize = SORT_BPM_DIVISION;
				DisplayBpms bpms;
				pSong->GetDisplayBpms(bpms);
				int iMaxBPM = (int)bpms.GetMax();
				iMaxBPM += iBPMGroupSize - (iMaxBPM % iBPMGroupSize) - 1;
				return ssprintf(
				  "%03d-%03d", iMaxBPM - (iBPMGroupSize - 1), iMaxBPM);
			} else
				return RString();
		}
		case SORT_POPULARITY:
		case SORT_RECENT:
			return RString();
		case SORT_LENGTH: {
			const int iSortLengthSize = 60;
			int iMaxLength = static_cast<int>(pSong->m_fMusicLengthSeconds);
			iMaxLength +=
			  (iSortLengthSize - (iMaxLength % iSortLengthSize) - 1);
			int iMinLength = iMaxLength - (iSortLengthSize - 1);
			return ssprintf(
			  "%s-%s",
			  SecondsToMMSS(static_cast<float>(iMinLength)).c_str(),
			  SecondsToMMSS(static_cast<float>(iMaxLength)).c_str());
		}
		case SORT_TOP_GRADES: {
			auto p = PROFILEMAN->GetProfile(PLAYER_1);
			auto s =
			  GAMESTATE->GetCurrentStyle(GAMESTATE->GetMasterPlayerNumber());
			if (p == nullptr || s == nullptr)
				return GradeToLocalizedString(Grade_Invalid);

			return GradeToLocalizedString(
					 PROFILEMAN->GetProfile(PLAYER_1)->GetBestGrade(
					   pSong, s->m_StepsType))
			  .c_str();
		}
		case SORT_MODE_MENU:
			return RString();
		default:
			FAIL_M(ssprintf("Invalid SortOrder: %i", so));
	}
}

void
SongUtil::SortSongPointerArrayBySectionName(vector<Song*>& vpSongsInOut,
											SortOrder so)
{
	RString sOther = SORT_OTHER.GetValue();
	for (unsigned i = 0; i < vpSongsInOut.size(); ++i) {
		RString val = GetSectionNameFromSongAndSort(vpSongsInOut[i], so);

		// Make sure 0-9 comes first and OTHER comes last.
		if (val == "0-9")
			val = "0";
		else if (val == sOther)
			val = "2";
		else {
			MakeSortString(val);
			val = "1" + val;
		}

		g_mapSongSortVal[vpSongsInOut[i]] = val;
	}

	stable_sort(vpSongsInOut.begin(),
				vpSongsInOut.end(),
				CompareSongPointersBySortValueAscending);
	g_mapSongSortVal.clear();
}

void
SongUtil::SortSongPointerArrayByStepsTypeAndMeter(vector<Song*>& vpSongsInOut,
												  StepsType st,
												  Difficulty dc)
{
	g_mapSongSortVal.clear();
	for (unsigned i = 0; i < vpSongsInOut.size(); ++i) {
		// Ignore locked steps.
		const Steps* pSteps = GetClosestNotes(vpSongsInOut[i], st, dc, true);
		RString& s = g_mapSongSortVal[vpSongsInOut[i]];
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
								  const RString& sPreferredDescription,
								  const Steps* pExclude)
{
	FOREACH_CONST(Steps*, pSong->GetAllSteps(), s)
	{
		Steps* pSteps = *s;

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
							const RString& name,
							const Steps* pExclude)
{
	FOREACH_CONST(Steps*, pSong->GetAllSteps(), s)
	{
		Steps* pSteps = *s;

		if (pSteps->m_StepsType != st)
			continue;
		if (pSteps == pExclude)
			continue;
		if (pSteps->GetChartName() == name)
			return false;
	}
	return true;
}

RString
SongUtil::MakeUniqueEditDescription(const Song* pSong,
									StepsType st,
									const RString& sPreferredDescription)
{
	if (IsEditDescriptionUnique(pSong, st, sPreferredDescription, NULL))
		return sPreferredDescription;

	RString sTemp;

	for (int i = 0; i < 1000; i++) {
		// make name "My Edit" -> "My Edit2"
		RString sNum = ssprintf("%d", i + 1);
		sTemp = sPreferredDescription.Left(MAX_STEPS_DESCRIPTION_LENGTH -
										   sNum.size()) +
				sNum;

		if (IsEditDescriptionUnique(pSong, st, sTemp, NULL))
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
SongUtil::ValidateCurrentEditStepsDescription(const RString& sAnswer,
											  RString& sErrorOut)
{
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	Song* pSong = pSteps->m_pSong;

	ASSERT(pSteps->IsAnEdit());

	if (sAnswer.empty()) {
		sErrorOut = YOU_MUST_SUPPLY_NAME;
		return false;
	}

	static const RString sInvalidChars = "\\/:*?\"<>|";
	if (strpbrk(sAnswer, sInvalidChars) != NULL) {
		sErrorOut =
		  ssprintf(EDIT_NAME_CANNOT_CONTAIN.GetValue(), sInvalidChars.c_str());
		return false;
	}

	// Steps name must be unique for this song.
	vector<Steps*> v;
	GetSteps(pSong, v, StepsType_Invalid, Difficulty_Edit);
	FOREACH_CONST(Steps*, v, s)
	{
		if (pSteps == *s)
			continue; // don't compare name against ourself

		if ((*s)->GetDescription() == sAnswer) {
			sErrorOut = EDIT_NAME_CONFLICTS;
			return false;
		}
	}

	return true;
}

bool
SongUtil::ValidateCurrentStepsDescription(const RString& sAnswer,
										  RString& sErrorOut)
{
	if (sAnswer.empty())
		return true;

	/* Don't allow duplicate edit names within the same StepsType; edit names
	 * uniquely identify the edit. */
	Steps* pSteps = GAMESTATE->m_pCurSteps;

	// If unchanged:
	if (pSteps->GetDescription() == sAnswer)
		return true;

	if (pSteps->IsAnEdit()) {
		return SongUtil::ValidateCurrentEditStepsDescription(sAnswer,
															 sErrorOut);
	}

	return true;
}

bool
SongUtil::ValidateCurrentStepsChartName(const RString& answer, RString& error)
{
	if (answer.empty())
		return true;

	static const RString sInvalidChars = "\\/:*?\"<>|";
	if (strpbrk(answer, sInvalidChars) != NULL) {
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
	bool result = SongUtil::IsChartNameUnique(
	  GAMESTATE->m_pCurSong, pSteps->m_StepsType, answer, pSteps);
	if (!result)
		error = CHART_NAME_CONFLICTS;
	return result;
}

static LocalizedString AUTHOR_NAME_CANNOT_CONTAIN(
  "SongUtil",
  "The step author's name cannot contain any of the following characters: %s");

bool
SongUtil::ValidateCurrentStepsCredit(const RString& sAnswer, RString& sErrorOut)
{
	if (sAnswer.empty())
		return true;

	Steps* pSteps = GAMESTATE->m_pCurSteps;
	// If unchanged:
	if (pSteps->GetCredit() == sAnswer)
		return true;

	// Borrow from EditDescription testing. Perhaps this should be abstracted?
	// -Wolfman2000
	static const RString sInvalidChars = "\\/:*?\"<>|";
	if (strpbrk(sAnswer, sInvalidChars) != NULL) {
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
SongUtil::ValidateCurrentSongPreview(const RString& answer, RString& error)
{
	if (answer.empty()) {
		return true;
	}
	Song* song = GAMESTATE->m_pCurSong;
	RString real_file = song->m_PreviewFile;
	song->m_PreviewFile = answer;
	RString path = song->GetPreviewMusicPath();
	bool valid = DoesFileExist(path);
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
SongUtil::ValidateCurrentStepsMusic(const RString& answer, RString& error)
{
	if (answer.empty())
		return true;
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	RString real_file = pSteps->GetMusicFile();
	pSteps->SetMusicFile(answer);
	RString path = pSteps->GetMusicPath();
	bool valid = DoesFileExist(path);
	pSteps->SetMusicFile(real_file);
	if (!valid) {
		error = ssprintf(MUSIC_DOES_NOT_EXIST.GetValue(), answer.c_str());
	}
	return valid;
}

void
SongUtil::GetAllSongGenres(vector<RString>& vsOut)
{
	set<RString> genres;
	FOREACH_CONST(Song*, SONGMAN->GetAllSongs(), song)
	{
		if (!(*song)->m_sGenre.empty())
			genres.insert((*song)->m_sGenre);
	}

	FOREACHS_CONST(RString, genres, s) { vsOut.push_back(*s); }
}

void
SongUtil::GetPlayableStepsTypes(const Song* pSong, set<StepsType>& vOut)
{
	vector<const Style*> vpPossibleStyles;
	// If AutoSetStyle, or a Style hasn't been chosen, check StepsTypes for all
	// Styles.
	if (CommonMetrics::AUTO_SET_STYLE ||
		GAMESTATE->GetCurrentStyle(PLAYER_INVALID) == NULL)
		GAMEMAN->GetCompatibleStyles(GAMESTATE->m_pCurGame,
									 GAMESTATE->GetNumPlayersEnabled(),
									 vpPossibleStyles);
	else
		vpPossibleStyles.push_back(GAMESTATE->GetCurrentStyle(PLAYER_INVALID));

	set<StepsType> vStepsTypes;
	FOREACH(const Style*, vpPossibleStyles, s)
	vStepsTypes.insert((*s)->m_StepsType);

	/* filter out hidden StepsTypes */
	const vector<StepsType>& vstToShow =
	  CommonMetrics::STEPS_TYPES_TO_SHOW.GetValue();
	FOREACHS(StepsType, vStepsTypes, st)
	{
		bool bShowThisStepsType =
		  find(vstToShow.begin(), vstToShow.end(), *st) != vstToShow.end();

		int iNumPlayers = GAMESTATE->GetNumPlayersEnabled();
		iNumPlayers = max(iNumPlayers, 1);

		if (bShowThisStepsType)
			vOut.insert(*st);
	}
}

void
SongUtil::GetPlayableSteps(const Song* pSong, vector<Steps*>& vOut)
{
	set<StepsType> vStepsType;
	GetPlayableStepsTypes(pSong, vStepsType);

	FOREACHS(StepsType, vStepsType, st)
	SongUtil::GetSteps(pSong, vOut, *st);

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
	vector<Steps*> vpSteps;
	GetPlayableSteps(pSong, vpSteps);
	return find(vpSteps.begin(), vpSteps.end(), pSteps) != vpSteps.end();
}

bool
SongUtil::IsSongPlayable(Song* s)
{
	const vector<Steps*>& steps = s->GetAllSteps();
	// I'm sure there is a foreach loop, but I don't
	FOREACH(Steps*, const_cast<vector<Steps*>&>(steps), step)
	{
		if (IsStepsPlayable(s, *step)) {
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
	if (p)
		sDir = p->GetSongDir();
	else
		sDir = "";

	// HACK for backwards compatibility:
	// Strip off leading "/".  2005/05/21 file layer changes added a leading
	// slash.
	if (sDir.Left(1) == "/")
		sDir.erase(sDir.begin());

	m_Cache.Unset();
}

Song*
SongID::ToSong() const
{
	Song* pRet = NULL;
	if (!m_Cache.Get(&pRet)) {
		if (!sDir.empty()) {
			// HACK for backwards compatibility: Re-add the leading "/".
			// 2005/05/21 file layer changes added a leading slash.
			RString sDir2 = sDir;
			if (sDir2.Left(1) != "/") {
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
	XNode* pNode = new XNode("Song");
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

RString
SongID::ToString() const
{
	return (sDir.empty() ? RString() : sDir);
}

bool
SongID::IsValid() const
{
	return ToSong() != NULL;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

namespace {
int
GetPlayableSteps(lua_State* L)
{
	const Song* pSong = Luna<Song>::check(L, 1, true);
	vector<Steps*> vSteps;
	SongUtil::GetPlayableSteps(pSong, vSteps);
	LuaHelpers::CreateTableFromArray<Steps*>(vSteps, L);
	return 1;
}
int
IsStepsTypePlayable(lua_State* L)
{
	Song* pSong = Luna<Song>::check(L, 1, true);
	StepsType st = Enum::Check<StepsType>(L, 2);
	bool b = SongUtil::IsStepsTypePlayable(pSong, st);
	LuaHelpers::Push(L, b);
	return 1;
}
int
IsStepsPlayable(lua_State* L)
{
	Song* pSong = Luna<Song>::check(L, 1, true);
	Steps* pSteps = Luna<Steps>::check(L, 2, true);
	bool b = SongUtil::IsStepsPlayable(pSong, pSteps);
	LuaHelpers::Push(L, b);
	return 1;
}

const luaL_Reg SongUtilTable[] = { LIST_METHOD(GetPlayableSteps),
								   LIST_METHOD(IsStepsTypePlayable),
								   LIST_METHOD(IsStepsPlayable),
								   { NULL, NULL } };
}

LUA_REGISTER_NAMESPACE(SongUtil)
