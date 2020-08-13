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

	auto ChangeSort(SortOrder new_so,
					bool allowSameSort = false)
	  -> bool;				 // return true if change successful
	auto NextSort() -> bool; // return true if change successful
	auto IsRouletting() const -> bool;

	auto Select()
	  -> bool override; // return true if this selection ends the screen
	auto GetSelectedType() -> WheelItemDataType
	{
		return GetCurWheelItemData(m_iSelection)->m_Type;
	}
	auto GetSelectedSong() -> Song*;
	auto GetSelectedSection() -> std::string
	{
		return GetCurWheelItemData(m_iSelection)->m_sText;
	}

	auto GetPreferredSelectionForRandomOrPortal() -> Song*;

	auto SelectSong(const Song* p) -> bool;
	auto SelectSection(const std::string& SectionName) -> bool;
	void SetOpenSection(const std::string& group) override;
	void ChangeMusic(int dist) override; /* +1 or -1 */ // CHECK THIS
	void FinishChangingSorts();
	void PlayerJoined();
	// sm-ssc additions
	auto JumpToNextGroup() -> std::string;
	auto JumpToPrevGroup() -> std::string;
	auto GetCurWheelItemData(int i) -> const MusicWheelItemData*
	{
		return dynamic_cast<const MusicWheelItemData*>(m_CurWheelItemData[i]);
	}

	virtual void ReloadSongList(bool searching, const std::string& findme);
	void SetHashList(const vector<string>& newHashList);
	void SetOutHashList(const vector<string>& newOutHashList);

	// multiplayer common pack filtering
	bool packlistFiltering{ false };

	vector<Song*> allSongsFiltered;
	std::map<std::string, vector<Song*>> allSongsByGroupFiltered;
	auto SelectSongOrCourse() -> bool;
	void SelectSongAfterSearch();

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	auto MakeItem() -> MusicWheelItem* override;

	vector<string> hashList;
	vector<string> outHashList;

	void GetSongList(vector<Song*>& arraySongs, SortOrder so) const;
	auto SelectModeMenuItem() -> bool;

	void FilterByAndAgainstStepKeys(vector<Song*>& inv);
	void FilterBySearch(vector<Song*>& inv, std::string findme_);
	auto SearchGroupNames(const std::string& findme) -> bool;
	static void FilterBySkillsets(vector<Song*>& inv);
	std::string lastvalidsearch;
	std::string groupnamesearchmatch;

	void UpdateSwitch() override;

	auto getWheelItemsData(SortOrder so) -> vector<MusicWheelItemData*>&;
	void readyWheelItemsData(SortOrder so,
							 bool searching,
							 const std::string& findme);

	std::string m_sLastModeMenuItem;
	RageSound m_soundChangeSort;

	auto WheelItemIsVisible(int n) -> bool;

	ThemeMetric<int> ROULETTE_SLOW_DOWN_SWITCHES;
	ThemeMetric<int> NUM_SECTION_COLORS;
	ThemeMetric<RageColor> SORT_MENU_COLOR;
	ThemeMetric<std::string> MODE_MENU_CHOICE_NAMES;
	ThemeMetricMap<std::string> CHOICE;
	ThemeMetric1D<RageColor> SECTION_COLORS;
	ThemeMetric<LuaReference> SORT_ORDERS;
	// sm-ssc additions:
	ThemeMetric<bool> USE_SECTIONS_WITH_PREFERRED_GROUP;
	ThemeMetric<bool> HIDE_INACTIVE_SECTIONS;
	ThemeMetric<bool> HIDE_ACTIVE_SECTION_TITLE;
	ThemeMetric<bool> REMIND_WHEEL_POSITIONS;
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
	} m_WheelItemDatasStatus[NUM_SortOrder]{};
	vector<MusicWheelItemData*> m__WheelItemDatas[NUM_SortOrder];
	vector<MusicWheelItemData*> m__UnFilteredWheelItemDatas[NUM_SortOrder];

	void BuildWheelItemDatas(vector<MusicWheelItemData*>& arrayWheelItemDatas,
							 SortOrder so,
							 bool searching,
							 const std::string& findme);
	void FilterWheelItemDatas(vector<MusicWheelItemData*>& aUnFilteredDatas,
							  vector<MusicWheelItemData*>& aFilteredData,
							  SortOrder so) const;
	std::string prevSongTitle;
};

#endif
