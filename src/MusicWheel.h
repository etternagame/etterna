/* MusicWheel - A wheel with song names used in the Select Music screen. */

#ifndef MUSIC_WHEEL_H
#define MUSIC_WHEEL_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "MusicWheelItem.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "WheelBase.h"

class Song;
using std::string;

struct CompareSongPointerArrayBySectionName;

class MusicWheel : public WheelBase
{
	friend struct CompareSongPointerArrayBySectionName;

  public:
	~MusicWheel() override;
	void Load(const string& sType) override;
	void BeginScreen();

	bool ChangeSort(
	  SortOrder new_so,
	  bool allowSameSort = false); // return true if change successful
	bool NextSort();			   // return true if change successful
	bool IsRouletting() const;

	bool Select() override; // return true if this selection ends the screen
	WheelItemDataType GetSelectedType()
	{
		return GetCurWheelItemData(m_iSelection)->m_Type;
	}
	Song* GetSelectedSong();
	RString GetSelectedSection()
	{
		return GetCurWheelItemData(m_iSelection)->m_sText;
	}

	Song* GetPreferredSelectionForRandomOrPortal();

	bool SelectSong(const Song* p);
	bool SelectSection(const RString& SectionName);
	void SetOpenSection(const RString& group) override;
	SortOrder GetSortOrder() const { return m_SortOrder; }
	void ChangeMusic(int dist) override; /* +1 or -1 */ // CHECK THIS
	void FinishChangingSorts();
	void PlayerJoined();
	// sm-ssc additions
	RString JumpToNextGroup();
	RString JumpToPrevGroup();
	const MusicWheelItemData* GetCurWheelItemData(int i)
	{
		return (const MusicWheelItemData*)m_CurWheelItemData[i];
	}

	virtual void ReloadSongList(bool searching, RString findme);
	void SetHashList(const vector<string>& newHashList);

	// multiplayer common pack filtering
	bool packlistFiltering{ false };

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	MusicWheelItem* MakeItem() override;

	vector<string> hashList;
	void GetSongList(vector<Song*>& arraySongs, SortOrder so);
	bool SelectSongOrCourse();
	bool SelectModeMenuItem();

	void FilterByStepKeys(vector<Song*>& inv);
	void FilterBySearch(vector<Song*>& inv, RString findme);
	bool SearchGroupNames(RString& findme);
	void FilterBySkillsets(vector<Song*>& inv);
	RString lastvalidsearch;
	RString groupnamesearchmatch;

	void UpdateSwitch() override;

	vector<MusicWheelItemData*>& getWheelItemsData(SortOrder so);
	void readyWheelItemsData(SortOrder so, bool searching, RString findme);

	RString m_sLastModeMenuItem;
	SortOrder m_SortOrder;
	RageSound m_soundChangeSort;

	bool WheelItemIsVisible(int n);

	ThemeMetric<float> ROULETTE_SWITCH_SECONDS;
	ThemeMetric<int> ROULETTE_SLOW_DOWN_SWITCHES;
	ThemeMetric<int> NUM_SECTION_COLORS;
	ThemeMetric<RageColor> SONG_REAL_EXTRA_COLOR;
	ThemeMetric<RageColor> SORT_MENU_COLOR;
	ThemeMetric<bool> RANDOM_PICKS_LOCKED_SONGS;
	ThemeMetric<int> MOST_PLAYED_SONGS_TO_SHOW;
	ThemeMetric<int> RECENT_SONGS_TO_SHOW;
	ThemeMetric<RString> MODE_MENU_CHOICE_NAMES;
	ThemeMetricMap<RString> CHOICE;
	ThemeMetric1D<RageColor> SECTION_COLORS;
	ThemeMetric<LuaReference> SORT_ORDERS;
	ThemeMetric<bool> SHOW_EASY_FLAG;
	// sm-ssc additions:
	ThemeMetric<bool> USE_SECTIONS_WITH_PREFERRED_GROUP;
	ThemeMetric<bool> HIDE_INACTIVE_SECTIONS;
	ThemeMetric<bool> HIDE_ACTIVE_SECTION_TITLE;
	ThemeMetric<bool> REMIND_WHEEL_POSITIONS;
	ThemeMetric<RageColor> ROULETTE_COLOR;
	ThemeMetric<RageColor> RANDOM_COLOR;
	ThemeMetric<RageColor> PORTAL_COLOR;
	ThemeMetric<RageColor> EMPTY_COLOR;
	vector<int> m_viWheelPositions;
	ThemeMetric<RString> CUSTOM_WHEEL_ITEM_NAMES;
	ThemeMetricMap<RString> CUSTOM_CHOICES;
	ThemeMetricMap<RageColor> CUSTOM_CHOICE_COLORS;

  private:
	// use getWheelItemsData instead of touching this one
	enum
	{
		INVALID,
		NEEDREFILTER,
		VALID
	} m_WheelItemDatasStatus[NUM_SortOrder];
	vector<MusicWheelItemData*> m__WheelItemDatas[NUM_SortOrder];
	vector<MusicWheelItemData*> m__UnFilteredWheelItemDatas[NUM_SortOrder];

	void BuildWheelItemDatas(vector<MusicWheelItemData*>& arrayWheelItems,
							 SortOrder so,
							 bool searching,
							 RString findme);
	void FilterWheelItemDatas(vector<MusicWheelItemData*>& aUnFilteredDatas,
							  vector<MusicWheelItemData*>& aFilteredData,
							  SortOrder so);
	void SelectSongAfterSearch();
	RString prevSongTitle;
};

#endif

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard
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
