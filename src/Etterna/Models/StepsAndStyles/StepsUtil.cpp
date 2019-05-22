#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/ProfileManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Steps.h"
#include "StepsUtil.h"
#include "Etterna/FileTypes/XmlFile.h"


// Sorting stuff
map<const Steps*, RString> steps_sort_val;

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
StepsUtil::SortStepsPointerArrayByNumPlays(std::vector<Steps*>& vStepsPointers,
										   ProfileSlot slot,
										   bool bDescending)
{
	if (!PROFILEMAN->IsPersistentProfile(slot))
		return; // nothing to do since we don't have data
	Profile* pProfile = PROFILEMAN->GetProfile(slot);
	SortStepsPointerArrayByNumPlays(vStepsPointers, pProfile, bDescending);
}

void
StepsUtil::SortStepsPointerArrayByNumPlays(std::vector<Steps*>& vStepsPointers,
										   const Profile* pProfile,
										   bool bDecending)
{
	// ugly...
	std::vector<Song*> vpSongs = SONGMAN->GetAllSongs();
	std::vector<Steps*> vpAllSteps;
	map<Steps*, Song*> mapStepsToSong;
	{
		for (unsigned i = 0; i < vpSongs.size(); i++) {
			Song* pSong = vpSongs[i];
			std::vector<Steps*> vpSteps = pSong->GetAllSteps();
			for (unsigned j = 0; j < vpSteps.size(); j++) {
				Steps* pSteps = vpSteps[j];
				vpAllSteps.push_back(pSteps);
				mapStepsToSong[pSteps] = pSong;
			}
		}
	}

	ASSERT(pProfile != NULL);
	for (unsigned i = 0; i < vStepsPointers.size(); ++i) {
		Steps* pSteps = vStepsPointers[i];
		Song* pSong = mapStepsToSong[pSteps];
		steps_sort_val[vStepsPointers[i]] =
		  ssprintf("%9i", pProfile->GetStepsNumTimesPlayed(pSong, pSteps));
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
	float fScore1 = 0;
	float fScore2 = 0;

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
StepsUtil::SortNotesArrayByDifficulty(std::vector<Steps*>& arraySteps)
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
StepsUtil::SortStepsByTypeAndDifficulty(std::vector<Steps*>& arraySongPointers)
{
	sort(arraySongPointers.begin(),
		 arraySongPointers.end(),
		 CompareStepsPointersByTypeAndDifficulty);
}

bool
StepsUtil::CompareStepsPointersByDescription(const Steps* pStep1,
											 const Steps* pStep2)
{
	return pStep1->GetDescription().CompareNoCase(pStep2->GetDescription()) < 0;
}

void
StepsUtil::SortStepsByDescription(std::vector<Steps*>& arraySongPointers)
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
	if (p == NULL) {
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
		return NULL;

	SongID songID;
	songID.FromSong(p);

	Steps* pRet = NULL;
	if (dc == Difficulty_Edit) {
		pRet = SongUtil::GetOneSteps(
		  p, st, dc, -1, -1, sDescription, "", uHash, true);
	} else {
		pRet = SongUtil::GetOneSteps(p, st, dc, -1, -1, "", "", 0, true);
	}

	if (!bAllowNull && pRet == NULL)
		FAIL_M(ssprintf("%i, %i, \"%s\"", st, dc, sDescription.c_str()));

	m_Cache.Set(pRet);
	return pRet;
}

XNode*
StepsID::CreateNode() const
{
	XNode* pNode = new XNode("Steps");

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

	RString sTemp;

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

RString
StepsID::ToString() const
{
	RString s = GAMEMAN->GetStepsTypeInfo(st).szName;
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

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
