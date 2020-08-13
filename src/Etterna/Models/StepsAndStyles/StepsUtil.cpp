#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Singletons/ScoreManager.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Steps.h"
#include "StepsUtil.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <map>
#include <algorithm>

// Sorting stuff
std::map<const Steps*, std::string> steps_sort_val;

static bool
CompareStepsPointersBySortValueAscending(const Steps* pSteps1,
										 const Steps* pSteps2)
{
	return steps_sort_val[pSteps1] < steps_sort_val[pSteps2];
}

static bool
CompareStepsPointersBySortValueDescending(const Steps* pSteps1,
										  const Steps* pSteps2)
{
	return steps_sort_val[pSteps1] > steps_sort_val[pSteps2];
}

void
StepsUtil::SortStepsPointerArrayByNumPlays(vector<Steps*>& vStepsPointers,
										   ProfileSlot slot,
										   bool bDescending)
{
	auto pProfile = PROFILEMAN->GetProfile(slot);
	SortStepsPointerArrayByNumPlays(vStepsPointers, pProfile, bDescending);
}

void
StepsUtil::SortStepsPointerArrayByNumPlays(vector<Steps*>& vStepsPointers,
										   const Profile* pProfile,
										   bool bDecending)
{
	// ugly...
	auto vpSongs = SONGMAN->GetAllSongs();
	vector<Steps*> vpAllSteps;
	std::map<Steps*, Song*> mapStepsToSong;
	{
		for (auto pSong : vpSongs) {
			auto vpSteps = pSong->GetAllSteps();
			for (auto pSteps : vpSteps) {
				vpAllSteps.push_back(pSteps);
				mapStepsToSong[pSteps] = pSong;
			}
		}
	}

	ASSERT(pProfile != nullptr);
	for (auto& steps : vStepsPointers) {
		steps_sort_val[steps] = ssprintf(
		  "%9i",
		  SCOREMAN->GetScoresForChart(steps->GetChartKey())->GetNumScores());
	}
	stable_sort(vStepsPointers.begin(),
				vStepsPointers.end(),
				bDecending ? CompareStepsPointersBySortValueDescending
						   : CompareStepsPointersBySortValueAscending);
	steps_sort_val.clear();
}

bool
StepsUtil::CompareNotesPointersByRadarValues(const Steps* pSteps1,
											 const Steps* pSteps2)
{
	auto fScore1 = 0.F;
	auto fScore2 = 0.F;

	fScore1 += pSteps1->GetRadarValues()[RadarCategory_TapsAndHolds];
	fScore2 += pSteps2->GetRadarValues()[RadarCategory_TapsAndHolds];

	return fScore1 < fScore2;
}

bool
StepsUtil::CompareNotesPointersByMeter(const Steps* pSteps1,
									   const Steps* pSteps2)
{
	return pSteps1->GetMeter() < pSteps2->GetMeter();
}

bool
StepsUtil::CompareNotesPointersByDifficulty(const Steps* pSteps1,
											const Steps* pSteps2)
{
	return pSteps1->GetDifficulty() < pSteps2->GetDifficulty();
}

void
StepsUtil::SortNotesArrayByDifficulty(vector<Steps*>& arraySteps)
{
	/* Sort in reverse order of priority. Sort by description first, to get
	 * a predictable order for songs with no radar values (edits). */
	stable_sort(
	  arraySteps.begin(), arraySteps.end(), CompareStepsPointersByDescription);
	stable_sort(
	  arraySteps.begin(), arraySteps.end(), CompareNotesPointersByRadarValues);
	stable_sort(
	  arraySteps.begin(), arraySteps.end(), CompareNotesPointersByMeter);
	stable_sort(
	  arraySteps.begin(), arraySteps.end(), CompareNotesPointersByDifficulty);
}

bool
StepsUtil::CompareStepsPointersByTypeAndDifficulty(const Steps* pStep1,
												   const Steps* pStep2)
{
	if (pStep1->m_StepsType < pStep2->m_StepsType)
		return true;
	if (pStep1->m_StepsType > pStep2->m_StepsType)
		return false;
	return pStep1->GetDifficulty() < pStep2->GetDifficulty();
}

void
StepsUtil::SortStepsByTypeAndDifficulty(vector<Steps*>& arraySongPointers)
{
	sort(arraySongPointers.begin(),
		 arraySongPointers.end(),
		 CompareStepsPointersByTypeAndDifficulty);
}

bool
StepsUtil::CompareStepsPointersByDescription(const Steps* pStep1,
											 const Steps* pStep2)
{
	return CompareNoCase(pStep1->GetDescription(), pStep2->GetDescription()) <
		   0;
}

void
StepsUtil::SortStepsByDescription(vector<Steps*>& arraySongPointers)
{
	sort(arraySongPointers.begin(),
		 arraySongPointers.end(),
		 CompareStepsPointersByDescription);
}

////////////////////////////////
// StepsID
////////////////////////////////

void
StepsID::FromSteps(const Steps* p)
{
	if (p == nullptr) {
		st = StepsType_Invalid;
		dc = Difficulty_Invalid;
		sDescription = "";
		uHash = 0;
	} else {
		st = p->m_StepsType;
		dc = p->GetDifficulty();
		ck = p->GetChartKey();
		if (dc == Difficulty_Edit) {
			sDescription = p->GetDescription();
			uHash = p->GetHash();
		} else {
			sDescription = "";
			uHash = 0;
		}
	}

	m_Cache.Unset();
}

/* XXX: Don't allow duplicate edit descriptions, and don't allow edit
 * descriptions to be difficulty names (eg. "Hard").  If we do that, this will
 * be completely unambiguous.
 *
 * XXX: Unless two memcards are inserted and there's overlap in the names.  In
 * that case, maybe both edits should be renamed to "Pn: foo"; as long as we
 * don't write them back out (which we don't do except in the editor), it won't
 * be permanent. We could do this during the actual Steps::GetID() call,
 * instead, but then it'd have to have access to Song::m_LoadedFromProfile. */

Steps*
StepsID::ToSteps(const Song* p, bool bAllowNull) const
{
	if (st == StepsType_Invalid || dc == Difficulty_Invalid)
		return nullptr;

	SongID songID;
	songID.FromSong(p);

	Steps* pRet = nullptr;
	if (dc == Difficulty_Edit) {
		pRet = SongUtil::GetOneSteps(
		  p, st, dc, -1, -1, false, sDescription, "", uHash, true);
	} else {
		pRet = SongUtil::GetOneSteps(p, st, dc, -1, -1, false, "", "", 0, true);
	}

	if (!bAllowNull && pRet == nullptr)
		FAIL_M(ssprintf("%i, %i, \"%s\"", st, dc, sDescription.c_str()));

	m_Cache.Set(pRet);
	return pRet;
}

XNode*
StepsID::CreateNode() const
{
	auto pNode = new XNode("Steps");

	pNode->AppendAttr("StepsType", GAMEMAN->GetStepsTypeInfo(st).szName);
	pNode->AppendAttr("Difficulty", DifficultyToString(dc));
	pNode->AppendAttr(" ChartKey", ck);
	if (dc == Difficulty_Edit) {
		pNode->AppendAttr("Description", sDescription);
		pNode->AppendAttr("Hash", uHash);
	}

	return pNode;
}

void
StepsID::LoadFromNode(const XNode* pNode)
{
	ASSERT(pNode->GetName() == "Steps");

	std::string sTemp;

	pNode->GetAttrValue("StepsType", sTemp);
	st = GAMEMAN->StringToStepsType(sTemp);

	pNode->GetAttrValue("Difficulty", sTemp);
	dc = StringToDifficulty(sTemp);

	pNode->GetAttrValue("ChartKey", sTemp);
	ck = sTemp;

	if (dc == Difficulty_Edit) {
		pNode->GetAttrValue("Description", sDescription);
		pNode->GetAttrValue("Hash", uHash);
	} else {
		sDescription = "";
		uHash = 0;
	}

	m_Cache.Unset();
}

std::string
StepsID::ToString() const
{
	std::string s = GAMEMAN->GetStepsTypeInfo(st).szName;
	s += " " + DifficultyToString(dc);
	if (dc == Difficulty_Edit) {
		s += " " + sDescription;
		s += ssprintf(" %u", uHash);
	}
	return s;
}

bool
StepsID::IsValid() const
{
	return st != StepsType_Invalid && dc != Difficulty_Invalid;
}

bool
StepsID::operator<(const StepsID& rhs) const
{
#define COMP(a)                                                                \
	if ((a) < rhs.a)                                                           \
		return true;                                                           \
	if ((a) > rhs.a)                                                           \
		return false;
	COMP(st);
	COMP(dc);
	COMP(sDescription);
	// See explanation in class declaration. -Kyz
	if (uHash != 0 && rhs.uHash != 0) {
		COMP(uHash);
	}
#undef COMP
	return false;
}

bool
StepsID::operator==(const StepsID& rhs) const
{
#define COMP(a)                                                                \
	if ((a) != rhs.a)                                                          \
		return false;
	COMP(st);
	COMP(dc);
	COMP(sDescription);
	// See explanation in class declaration. -Kyz
	if (uHash != 0 && rhs.uHash != 0) {
		COMP(uHash);
	}
#undef COMP
	return true;
}
