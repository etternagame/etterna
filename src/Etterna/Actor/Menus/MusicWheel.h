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
	MusicWheel();
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
	std::string GetSelectedSection()
	{
		return GetCurWheelItemData(m_iSelection)->m_sText;
	}

	Song* GetPreferredSelectionForRandomOrPortal();

	bool SelectSong(const Song* p);
	bool SelectSection(const std::string& SectionName);
	void SetOpenSection(const std::string& group) override;
	void ChangeMusic(int dist) override; /* +1 or -1 */ // CHECK THIS
	void FinishChangingSorts();
	void PlayerJoined();
	// sm-ssc additions
	std::string JumpToNextGroup();
	std::string JumpToPrevGroup();
	const MusicWheelItemData* GetCurWheelItemData(int i)
	{
		return static_cast<const MusicWheelItemData*>(m_CurWheelItemData[i]);
	}

	virtual void ReloadSongList(bool searching, const std::string& findme);
	void SetHashList(const vector<string>& newHashList);

	// multiplayer common pack filtering
	bool packlistFiltering{ false };

	vector<Song*> allSongsFiltered;
	map<std::string, vector<Song*>> allSongsByGroupFiltered;
	bool SelectSongOrCourse();
	void SelectSongAfterSearch();

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	MusicWheelItem* MakeItem() override;

	vector<string> hashList;
	void GetSongList(vector<Song*>& arraySongs, SortOrder so);
	bool SelectModeMenuItem();

	void FilterByStepKeys(vector<Song*>& inv);
	void FilterBySearch(vector<Song*>& inv, std::string findme_);
	bool SearchGroupNames(const std::string& findme);
	void FilterBySkillsets(vector<Song*>& inv);
	std::string lastvalidsearch;
	std::string groupnamesearchmatch;

	void UpdateSwitch() override;

	vector<MusicWheelItemData*>& getWheelItemsData(SortOrder so);
	void readyWheelItemsData(SortOrder so,
							 bool searching,
							 const std::string& findme);

	std::string m_sLastModeMenuItem;
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
	ThemeMetric<std::string> MODE_MENU_CHOICE_NAMES;
	ThemeMetricMap<std::string> CHOICE;
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
	ThemeMetric<std::string> CUSTOM_WHEEL_ITEM_NAMES;
	ThemeMetricMap<std::string> CUSTOM_CHOICES;
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
							 const std::string& findme);
	void FilterWheelItemDatas(vector<MusicWheelItemData*>& aUnFilteredDatas,
							  vector<MusicWheelItemData*>& aFilteredData,
							  SortOrder so);
	std::string prevSongTitle;
};

#endif
