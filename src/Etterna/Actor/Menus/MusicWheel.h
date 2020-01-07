/* MusicWheel - A wheel with song names used in the Select Music screen. */

#ifndef MUSIC_WHEEL_H
#define MUSIC_WHEEL_H

#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "MusicWheelItem.h"
#include "RageUtil/Sound/RageSound.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "WheelBase.h"

class Song;

struct CompareSongPointerArrayBySectionName;

class MusicWheel : public WheelBase
{
	friend struct CompareSongPointerArrayBySectionName;

  public:
	~MusicWheel() override;
	void Load(const std::string& sType) override;
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
	void SetHashList(const std::vector<std::string>& newHashList);

	// multiplayer common pack filtering
	bool packlistFiltering{ false };

	std::vector<Song*> allSongsFiltered;
	std::map<RString, std::vector<Song*>> allSongsByGroupFiltered;

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	MusicWheelItem* MakeItem() override;

	std::vector<std::string> hashList;
	void GetSongList(std::vector<Song*>& arraySongs, SortOrder so);
	bool SelectSongOrCourse();
	bool SelectModeMenuItem();

	void FilterByStepKeys(std::vector<Song*>& inv);
	void FilterBySearch(std::vector<Song*>& inv, RString findme);
	bool SearchGroupNames(RString& findme);
	void FilterBySkillsets(std::vector<Song*>& inv);
	RString lastvalidsearch;
	RString groupnamesearchmatch;

	void UpdateSwitch() override;

	std::vector<MusicWheelItemData*>& getWheelItemsData(SortOrder so);
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
	std::vector<int> m_viWheelPositions;
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
	std::vector<MusicWheelItemData*> m__WheelItemDatas[NUM_SortOrder];
	std::vector<MusicWheelItemData*> m__UnFilteredWheelItemDatas[NUM_SortOrder];

	void BuildWheelItemDatas(std::vector<MusicWheelItemData*>& arrayWheelItems,
							 SortOrder so,
							 bool searching,
							 RString findme);
	void FilterWheelItemDatas(std::vector<MusicWheelItemData*>& aUnFilteredDatas,
							  std::vector<MusicWheelItemData*>& aFilteredData,
							  SortOrder so);
	void SelectSongAfterSearch();
	RString prevSongTitle;
};

#endif
