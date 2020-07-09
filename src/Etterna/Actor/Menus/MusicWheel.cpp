#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Models/Misc/Foreach.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/MessageManager.h"
#include "MusicWheel.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Globals/rngthing.h"

#include <algorithm>

#define NUM_WHEEL_ITEMS (static_cast<int>(ceil(NUM_WHEEL_ITEMS_TO_DRAW + 2)))
#define WHEEL_TEXT(s)                                                          \
	THEME->GetString("MusicWheel", ssprintf("%sText", s.c_str()));
#define CUSTOM_ITEM_WHEEL_TEXT(s)                                              \
	THEME->GetString("MusicWheel", ssprintf("CustomItem%sText", (s).c_str()));

static std::string
SECTION_COLORS_NAME(size_t i)
{
	return ssprintf("SectionColor%d", static_cast<int>(i + 1));
}
static std::string
CHOICE_NAME(std::string s)
{
	return ssprintf("Choice%s", s.c_str());
}
static std::string
CUSTOM_WHEEL_ITEM_NAME(std::string s)
{
	return ssprintf("CustomWheelItem%s", s.c_str());
}
static std::string
CUSTOM_WHEEL_ITEM_COLOR(std::string s)
{
	return ssprintf("%sColor", s.c_str());
}

static LocalizedString EMPTY_STRING("MusicWheel", "Empty");

AutoScreenMessage(
  SM_SongChanged); // TODO: Replace this with a Message and MESSAGEMAN
AutoScreenMessage(SM_SortOrderChanging);
AutoScreenMessage(SM_SortOrderChanged);

MusicWheelItem*
MusicWheel::MakeItem()
{
	return new MusicWheelItem;
}

void
MusicWheel::Load(const string& sType)
{
	ROULETTE_SLOW_DOWN_SWITCHES.Load(sType, "RouletteSlowDownSwitches");
	NUM_SECTION_COLORS.Load(sType, "NumSectionColors");
	SORT_MENU_COLOR.Load(sType, "SortMenuColor");
	MODE_MENU_CHOICE_NAMES.Load(sType, "ModeMenuChoiceNames");
	SORT_ORDERS.Load(sType, "SortOrders");
	USE_SECTIONS_WITH_PREFERRED_GROUP.Load(sType,
										   "UseSectionsWithPreferredGroup");
	HIDE_INACTIVE_SECTIONS.Load(sType, "OnlyShowActiveSection");
	HIDE_ACTIVE_SECTION_TITLE.Load(sType, "HideActiveSectionTitle");
	REMIND_WHEEL_POSITIONS.Load(sType, "RemindWheelPositions");
	vector<std::string> vsModeChoiceNames;
	split(MODE_MENU_CHOICE_NAMES, ",", vsModeChoiceNames);
	CHOICE.Load(sType, CHOICE_NAME, vsModeChoiceNames);
	SECTION_COLORS.Load(sType, SECTION_COLORS_NAME, NUM_SECTION_COLORS);

	CUSTOM_WHEEL_ITEM_NAMES.Load(sType, "CustomWheelItemNames");
	vector<std::string> vsCustomItemNames;
	split(CUSTOM_WHEEL_ITEM_NAMES, ",", vsCustomItemNames);
	CUSTOM_CHOICES.Load(sType, CUSTOM_WHEEL_ITEM_NAME, vsCustomItemNames);
	CUSTOM_CHOICE_COLORS.Load(
	  sType, CUSTOM_WHEEL_ITEM_COLOR, vsCustomItemNames);

	EMPTY_COLOR.Load(sType, "EmptyColor");

	WheelBase::Load(sType);

	m_soundChangeSort.Load(THEME->GetPathS(sType, "sort"));
	m_soundExpand.Load(THEME->GetPathS(sType, "expand"), true);
	m_soundCollapse.Load(THEME->GetPathS(sType, "collapse"), true);

	/* Sort SONGMAN's songs by CompareSongPointersByTitle, so we can do other
	 * sorts (with stable_sort) from its output, and title will be the secondary
	 * sort, without having to re-sort by title each time. */
	SONGMAN->SortSongs();

	FOREACH_ENUM(SortOrder, so) { m_WheelItemDatasStatus[so] = INVALID; }
}

void
MusicWheel::BeginScreen()
{
	RageTimer timer;
	std::string times;
	FOREACH_ENUM(SortOrder, so)
	{
		if (m_WheelItemDatasStatus[so] != INVALID) {
			m_WheelItemDatasStatus[so] = NEEDREFILTER;
		}
	}

	// Set m_LastModeMenuItem to the first item that matches the current mode.
	// (Do this after building wheel item data.)
	{
		const auto& from = getWheelItemsData(SORT_MODE_MENU);
		for (auto* i : from) {
			ASSERT(&*i->m_pAction != nullptr);
			if (i->m_pAction->DescribesCurrentModeForAllPlayers()) {
				m_sLastModeMenuItem = i->m_pAction->m_sName;
				break;
			}
		}
	}

	WheelBase::BeginScreen();

	GAMESTATE->m_SortOrder.Set(GAMESTATE->m_PreferredSortOrder);

	// Never start in the mode menu; some elements may not initialize correctly.
	if (GAMESTATE->m_SortOrder == SORT_MODE_MENU)
		GAMESTATE->m_SortOrder.Set(SortOrder_Invalid);

	GAMESTATE->m_SortOrder.Set(GAMESTATE->m_SortOrder);

	/* Only save the sort order if the player didn't already have one.
	 * If he did, don't overwrite it. */
	if (GAMESTATE->m_PreferredSortOrder == SortOrder_Invalid)
		GAMESTATE->m_PreferredSortOrder = GAMESTATE->m_SortOrder;

	if (GAMESTATE->m_sPreferredSongGroup != GROUP_ALL) {
		// If a preferred song group is set, open the group and select the
		// first song in the group. -aj
		SetOpenSection(GAMESTATE->m_sPreferredSongGroup);
		SelectSongOrCourse();
	} else if (!SelectSongOrCourse()) {
		// Select the the previously selected song (if any)
		SetOpenSection("");
	}

	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS) {
		// store the group song index, run this also here because it forgets the
		// current position when not changing the song if you came back from
		// gameplay or your last round song (profiles) is not the first one in
		// the group.
		for (unsigned idx = 0; idx < m_viWheelPositions.size(); idx++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(idx)) {
				m_viWheelPositions[idx] = m_iSelection;
			}
		}
	}

	// rebuild the WheelItems that appear on screen
	RebuildWheelItems();
}

MusicWheel::MusicWheel()
{
	FOREACH_ENUM(SortOrder, so) { m_WheelItemDatasStatus[so] = INVALID; }
}

MusicWheel::~MusicWheel()
{
	FOREACH_ENUM(SortOrder, so)
	{
		auto i = m__UnFilteredWheelItemDatas[so].begin();
		auto iEnd = m__UnFilteredWheelItemDatas[so].end();
		for (; i != iEnd; ++i) {
			delete *i;
		}
	}
}

// this is a trainwreck and i made it worse -mina
void
MusicWheel::ReloadSongList(bool searching, const std::string& findme)
{
	// if we fallthrough to pack name matching don't keep reloading if we found
	// a match -mina
	if (findme.size() > lastvalidsearch.size() && !groupnamesearchmatch.empty())
		return;

	// when cancelling a search stay in the pack of your match... this should be
	// more intuitive and relevant behavior -mina
	if (findme.empty() && !lastvalidsearch.empty()) {
		lastvalidsearch = "";
		m_WheelItemDatasStatus[GAMESTATE->m_SortOrder] = INVALID;
		readyWheelItemsData(GAMESTATE->m_SortOrder, false, findme);
		SetOpenSection(m_sExpandedSectionName);
		RebuildWheelItems();
		SelectSection(m_sExpandedSectionName);
		SetOpenSection(m_sExpandedSectionName);
		ChangeMusic(1);
		SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0.35f);
		return;
	}

	auto songIdxToPreserve = m_iSelection;
	// Remove the song from any sorting caches:
	FOREACH_ENUM(SortOrder, so)
	m_WheelItemDatasStatus[so] = INVALID;
	// rebuild the info associated with this sort order
	readyWheelItemsData(GAMESTATE->m_SortOrder, searching, findme);
	// re-open the section to refresh song counts, etc.
	SetOpenSection(m_sExpandedSectionName);
	// navigate to the song nearest to what was previously selected
	m_iSelection = songIdxToPreserve;
	RebuildWheelItems();
	// refresh the song preview
	SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);

	// when searching, automatically land on the first search result available
	// -mina & dadbearcop
	if (!findme.empty() || !hashList.empty()) {
		if (!groupnamesearchmatch.empty()) {
			SelectSection(groupnamesearchmatch);
			SetOpenSection(groupnamesearchmatch);
			ChangeMusic(1);
			SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0.35f);
			return;
		}
		Song* pSong = GAMESTATE->m_pCurSong;
		if (pSong != nullptr) {
			const auto& curSongTitle = pSong->GetDisplayMainTitle();
			if (!GetSelectedSection().empty() &&
				curSongTitle != prevSongTitle) {
				prevSongTitle = curSongTitle;
				SelectSongAfterSearch();
			}
		} else {
			SelectSongAfterSearch();
		}
	} else {
		SetOpenSection("");
	}
}

void
MusicWheel::SelectSongAfterSearch()
{
	auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	SelectSection(from[0]->m_sText);
	SetOpenSection(from[0]->m_sText);
	ChangeMusic(1);
}

/* If a song or course is set in GAMESTATE and available, select it.  Otherwise,
 * choose the first available song or course.  Return true if an item was set,
 * false if no items are available. */
bool
MusicWheel::SelectSongOrCourse()
{
	if ((GAMESTATE->m_pPreferredSong != nullptr) &&
		SelectSong(GAMESTATE->m_pPreferredSong))
		return true;
	if (GAMESTATE->m_pCurSong && SelectSong(GAMESTATE->m_pCurSong))
		return true;

	// Select the first selectable song based on the sort order...
	auto& wiWheelItems = getWheelItemsData(GAMESTATE->m_SortOrder);
	for (auto& wiWheelItem : wiWheelItems) {
		if (wiWheelItem->m_pSong)
			return SelectSong(wiWheelItem->m_pSong);
	}

	LOG->Trace("MusicWheel::MusicWheel() - No selectable songs or courses "
			   "found in WheelData");
	return false;
}

bool
MusicWheel::SelectSection(const std::string& SectionName)
{
	for (unsigned int i = 0; i < m_CurWheelItemData.size(); ++i) {
		if (m_CurWheelItemData[i]->m_sText == SectionName) {
			m_iSelection = i; // select it
			return true;
		}
	}

	return false;
}

bool
MusicWheel::SelectSong(const Song* p)
{
	if (p == nullptr)
		return false;

	unsigned i;
	auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	for (i = 0; i < from.size(); i++) {
		if (from[i]->m_pSong == p) {
			// make its group the currently expanded group
			SetOpenSection(from[i]->m_sText);

			// skip any playlist groups
			if (!SONGMAN->GetPlaylists().count(GetExpandedSectionName()))
				break;
		}
	}

	if (i == from.size())
		return false;

	for (i = 0; i < m_CurWheelItemData.size(); i++) {
		if (GetCurWheelItemData(i)->m_pSong == p)
			m_iSelection = i; // select it
	}
	ChangeMusic(
	  0); // Actually select it, come on guys how hard was this LOL - mina
	return true;
}

bool
MusicWheel::SelectModeMenuItem()
{
	// Select the last-chosen option.
	ASSERT(GAMESTATE->m_SortOrder == SORT_MODE_MENU);
	const auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	unsigned i;
	for (i = 0; i < from.size(); i++) {
		const auto& gc = *from[i]->m_pAction;
		if (gc.m_sName == m_sLastModeMenuItem)
			break;
	}
	if (i == from.size())
		return false;

	// make its group the currently expanded group
	SetOpenSection(from[i]->m_sText);

	for (i = 0; i < m_CurWheelItemData.size(); i++) {
		if (GetCurWheelItemData(i)->m_pAction->m_sName != m_sLastModeMenuItem)
			continue;
		m_iSelection = i; // select it
		break;
	}

	return true;
}

// bool MusicWheel::SelectCustomItem()

void
MusicWheel::GetSongList(vector<Song*>& arraySongs, SortOrder so) const
{
	vector<Song*> apAllSongs;
	switch (so) {
		case SORT_FAVORITES:
			SONGMAN->GetFavoriteSongs(apAllSongs);
			break;
		case SORT_GROUP:
			// if we're not using sections with a preferred song group, and
			// there is a group to load, only load those songs. -aj
			if (GAMESTATE->m_sPreferredSongGroup != GROUP_ALL &&
				!USE_SECTIONS_WITH_PREFERRED_GROUP) {
				apAllSongs =
				  SONGMAN->GetSongs(GAMESTATE->m_sPreferredSongGroup);
				break;
			}
			// otherwise fall through
		default:
			apAllSongs = SONGMAN->GetAllSongs();
			break;
	}

	// copy only songs that have at least one Steps for the current GameMode
	for (auto* pSong : apAllSongs) {
		{
			// Online mode doesn't support auto set style.  A song that only has
			// dance-double steps will show up when dance-single was selected,
			// with no playable steps.  Then the game will crash when trying to
			// play it. -Kyz
			if (CommonMetrics::AUTO_SET_STYLE) {
				// with AUTO_SET_STYLE on and Autogen off, some songs may get
				// hidden. Search through every playable StepsType until you
				// find one, then add the song.
				// see Issue 147 for more information. -aj
				// http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=147
				std::set<StepsType> vStepsType;
				SongUtil::GetPlayableStepsTypes(pSong, vStepsType);

				for (const auto& st : vStepsType) {
					if (pSong->HasStepsType(st)) {
						arraySongs.emplace_back(pSong);
						break;
					}
				}
			} else {
				// If the song has at least one steps, add it.
				if (pSong->HasStepsType(
					  GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType))
					arraySongs.emplace_back(pSong);
			}
		}
	}
}
bool
contains(std::string container, const std::string& findme)
{
	std::transform(
	  begin(container), end(container), begin(container), ::tolower);
	return container.find(findme) != std::string::npos;
}
void
MusicWheel::FilterBySearch(vector<Song*>& inv, std::string findme)
{

	std::transform(begin(findme), end(findme), begin(findme), ::tolower);

	// Super Search: With specified stuff(Not just the title)
	auto super_search = false;
	auto artist = findme.find("artist=");
	auto author = findme.find("author=");
	auto title = findme.find("title=");
	auto subtitle = findme.find("subtitle=");

	// title is a substring of title
	// so if found that way, check again
	if (title == subtitle + 3) {
		title = findme.find("title=", title + 1);
	}

	string findartist;
	string findauthor;
	string findtitle;
	string findsubtitle;

	if (artist != findme.npos || author != findme.npos ||
		title != findme.npos || subtitle != findme.npos) {
		super_search = true;
		if (artist != findme.npos)
			findartist = findme.substr(
			  artist + 7, findme.find(static_cast<char>(artist), ';') - artist);
		if (author != findme.npos)
			findauthor = findme.substr(
			  author + 7, findme.find(static_cast<char>(author), ';') - author);
		if (title != findme.npos)
			findtitle = findme.substr(
			  title + 6, findme.find(static_cast<char>(title), ';') - title);
		if (subtitle != findme.npos)
			findsubtitle = findme.substr(
			  subtitle + 9,
			  findme.find(static_cast<char>(subtitle), ';') - subtitle);
	}

	// The giant block of code below is for optimization purposes.
	// Basically, we don't want to give a single fat lambda to the filter that
	// checks and short circuits. Instead, we want to just not check at all.
	// It's a baby sized optimization but adds up over time. The binary comments
	// help verify which things are being checked.
	vector<Song*> tmp;
	std::function<bool(Song*)> check;
	if (super_search == false) {
		// 0000
		check = [&findme](Song* x) {
			return contains(x->GetDisplayMainTitle(), findme);
		};
	} else {
		if (!findartist.empty() && !findtitle.empty() && !findauthor.empty() &&
			!findsubtitle.empty()) {
			// 1111
			check =
			  [&findauthor, &findartist, &findtitle, &findsubtitle](Song* x) {
				  return contains(x->GetDisplayArtist(), findartist) ||
						 contains(x->GetOrTryAtLeastToGetSimfileAuthor(),
								  findauthor) ||
						 contains(x->GetDisplayMainTitle(), findtitle) ||
						 contains(x->GetDisplaySubTitle(), findsubtitle);
			  };
		} else {
			if (!findsubtitle.empty()) {
				if (findauthor.empty() && findtitle.empty() &&
					findartist.empty()) {
					// 1000
					check = [&findsubtitle](Song* x) {
						return contains(x->GetDisplaySubTitle(), findsubtitle);
					};
				} else {
					if (findauthor.empty()) {
						if (findtitle.empty())
							// 1001
							check = [&findsubtitle, &findartist](Song* x) {
								return contains(x->GetDisplayArtist(),
												findartist) ||
									   contains(x->GetDisplaySubTitle(),
												findsubtitle);
							};
						else {
							if (findartist.empty())
								// 1010
								check = [&findsubtitle, &findtitle](Song* x) {
									return contains(x->GetDisplayMainTitle(),
													findtitle) ||
										   contains(x->GetDisplaySubTitle(),
													findsubtitle);
								};
							else
								// 1011
								check = [&findsubtitle,
										 &findartist,
										 &findtitle](Song* x) {
									return contains(x->GetDisplayArtist(),
													findartist) ||
										   contains(x->GetDisplayMainTitle(),
													findtitle) ||
										   contains(x->GetDisplaySubTitle(),
													findsubtitle);
								};
						}
					} else {
						if (findtitle.empty()) {
							if (findartist.empty())
								// 1100
								check = [&findsubtitle, &findauthor](Song* x) {
									return contains(
											 x->GetOrTryAtLeastToGetSimfileAuthor(),
											 findauthor) ||
										   contains(x->GetDisplaySubTitle(),
													findsubtitle);
								};
							else
								// 1101
								check = [&findsubtitle,
										 &findauthor,
										 &findartist](Song* x) {
									return contains(x->GetDisplayArtist(),
													findartist) ||
										   contains(
											 x->GetOrTryAtLeastToGetSimfileAuthor(),
											 findauthor) ||
										   contains(x->GetDisplaySubTitle(),
													findsubtitle);
								};
						} else {
							// 1110
							check = [&findsubtitle, &findauthor, &findtitle](
									  Song* x) {
								return contains(x->GetDisplayMainTitle(),
												findtitle) ||
									   contains(
										 x->GetOrTryAtLeastToGetSimfileAuthor(),
										 findauthor) ||
									   contains(x->GetDisplaySubTitle(),
												findsubtitle);
							};
						}
					}
				}
			} else {
				if (!findartist.empty() && !findtitle.empty() &&
					!findauthor.empty()) {
					// 0111
					check = [&findauthor, &findartist, &findtitle](Song* x) {
						return contains(x->GetDisplayArtist(), findartist) ||
							   contains(x->GetOrTryAtLeastToGetSimfileAuthor(),
										findauthor) ||
							   contains(x->GetDisplayMainTitle(), findtitle);
					};
				} else {
					if (findauthor.empty()) {
						if (findtitle.empty())
							// 0001
							check = [&findartist](Song* x) {
								return contains(x->GetDisplayArtist(),
												findartist);
							};
						else {
							if (findartist.empty())
								// 0010
								check = [&findtitle](Song* x) {
									return contains(x->GetDisplayMainTitle(),
													findtitle);
								};
							else
								// 0011
								check = [&findartist, &findtitle](Song* x) {
									return contains(x->GetDisplayArtist(),
													findartist) ||
										   contains(x->GetDisplayMainTitle(),
													findtitle);
								};
						}
					} else {
						if (findtitle.empty()) {
							if (findartist.empty())
								// 0100
								check = [&findauthor](Song* x) {
									return contains(
									  x->GetOrTryAtLeastToGetSimfileAuthor(),
									  findauthor);
								};
							else
								// 0101
								check = [&findauthor, &findartist](Song* x) {
									return contains(x->GetDisplayArtist(),
													findartist) ||
										   contains(
											 x->GetOrTryAtLeastToGetSimfileAuthor(),
											 findauthor);
								};
						} else {
							// 0110
							check = [&findauthor, &findtitle](Song* x) {
								return contains(x->GetDisplayMainTitle(),
												findtitle) ||
									   contains(
										 x->GetOrTryAtLeastToGetSimfileAuthor(),
										 findauthor);
							};
						}
					}
				}
			}
		}
	}

	for (auto& x : inv) {
		if (check(x))
			tmp.push_back(x);
	}
	if (!tmp.empty()) {
		lastvalidsearch = findme;
		groupnamesearchmatch = "";
		inv.swap(tmp);
	} else {
		if (SearchGroupNames(findme))
			return;
		FilterBySearch(inv, lastvalidsearch);
	}
}

void
MusicWheel::SetHashList(const vector<string>& newHashList)
{
	hashList = newHashList;
}

void
MusicWheel::FilterByStepKeys(vector<Song*>& inv)
{
	vector<Song*> tmp;
	std::function<bool(Song*)> check = [this](Song* x) {
		for (auto& ck : hashList) {
			if (x->HasChartByHash(ck)) {
				return true;
			}
		}

		return false;
	};
	for (auto* x : inv) {
		if (check(x))
			tmp.emplace_back(x);
	}
	if (!tmp.empty()) {
		inv.swap(tmp);
	}
}

bool
MusicWheel::SearchGroupNames(const std::string& findme)
{
	const auto& grps = SONGMAN->GetSongGroupNames();
	for (const auto& grp : grps) {
		auto lc = make_lower(std::string(grp));
		auto droop = lc.find(findme);
		if (droop != lc.npos) {
			groupnamesearchmatch = grp;
			return true;
		}
	}
	groupnamesearchmatch = "";
	return false;
}

// should definitely house these in lower level functions so return can be
// called the iteration an outcome is determined on instead of clumsily using
// continue - mina
void
MusicWheel::FilterBySkillsets(vector<Song*>& inv) const
{
	vector<Song*> tmp;

	for (auto* song : inv) {
		auto addsong = false;
		for (auto currate = FILTERMAN->MaxFilterRate;
			 currate > FILTERMAN->m_pPlayerState->wtFFF - .01f;
			 currate -= 0.1f) { /* Iterate over all possible rates.
								 * The .01f delta is because floating points
								 * don't like exact equivalency*/
			if (song->MatchesFilter(currate)) {
				addsong = true;
				break; // We don't need to keep checking rates
			}
		}
		if (addsong)
			tmp.emplace_back(song);
	}
	inv.swap(tmp);
}

void
MusicWheel::BuildWheelItemDatas(
  vector<MusicWheelItemData*>& arrayWheelItemDatas,
  SortOrder so,
  bool searching,
  const std::string& findme)
{

	std::map<std::string, Commands> commanDZ;
	if (so == SORT_MODE_MENU) {
		arrayWheelItemDatas.clear(); // clear out the previous wheel items
		vector<std::string> vsNames;
		split(MODE_MENU_CHOICE_NAMES, ",", vsNames);
		for (unsigned i = 0; i < vsNames.size(); ++i) {
			MusicWheelItemData wid(
			  WheelItemDataType_Sort, nullptr, "", SORT_MENU_COLOR, 0);
			wid.m_pAction = HiddenPtr<GameCommand>(new GameCommand);
			wid.m_pAction->m_sName = vsNames[i];
			wid.m_pAction->Load(i, ParseCommands(CHOICE.GetValue(vsNames[i])));
			wid.m_sLabel = WHEEL_TEXT(vsNames[i]);

			if (!wid.m_pAction->IsPlayable())
				continue;

			arrayWheelItemDatas.emplace_back(new MusicWheelItemData(wid));
		}
	} else {
		// Make an array of Song*, then sort them
		vector<Song*> arraySongs;
		GetSongList(arraySongs, so);

		Message msg("FilterResults");
		msg.SetParam("Total", static_cast<int>(arraySongs.size()));

		if (FILTERMAN->filteringCommonPacks && NSMAN->IsETTP() &&
			!NSMAN->commonpacks.empty()) {
			vector<Song*> tmp;
			for (auto& song : arraySongs) {
				auto& group = song->m_sGroupName;
				for (auto& pack : NSMAN->commonpacks) {
					// If song pack is in packlist
					if (group == pack) {
						// Add and continue with next song
						tmp.emplace_back(song);
						goto continueOuterLoop;
					}
				}
			continueOuterLoop:;
			}
			arraySongs.swap(tmp);
		}

		if (searching)
			FilterBySearch(arraySongs, findme);

		if (!hashList.empty())
			FilterByStepKeys(arraySongs);

		if (FILTERMAN->AnyActiveFilter())
			FilterBySkillsets(arraySongs);

		msg.SetParam("Matches", static_cast<int>(arraySongs.size()));
		MESSAGEMAN->Broadcast(msg);

		auto bUseSections = true;

		// sort the songs
		switch (so) {
			case SORT_FAVORITES:
				break;
			case SORT_GROUP:
				SongUtil::SortSongPointerArrayByGroupAndTitle(arraySongs);

				if (USE_SECTIONS_WITH_PREFERRED_GROUP)
					bUseSections = true;
				else
					bUseSections =
					  GAMESTATE->m_sPreferredSongGroup == GROUP_ALL;
				break;
			case SORT_TITLE:
				SongUtil::SortSongPointerArrayByTitle(arraySongs);
				break;
			case SORT_BPM:
				SongUtil::SortSongPointerArrayByBPM(arraySongs);
				break;
			case SORT_TOP_GRADES:
				SongUtil::SortSongPointerArrayByWifeScore(arraySongs);
				break;
			case SORT_ARTIST:
				SongUtil::SortSongPointerArrayByArtist(arraySongs);
				break;
			case SORT_GENRE:
				SongUtil::SortSongPointerArrayByGenre(arraySongs);
				break;
			case SORT_Overall:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Overall);
				break;
			case SORT_Stream:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Stream);
				break;
			case SORT_Jumpstream:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Jumpstream);
				break;
			case SORT_Handstream:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Handstream);
				break;
			case SORT_Stamina:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Stamina);
				break;
			case SORT_JackSpeed:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_JackSpeed);
				break;
			case SORT_Chordjack:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Chordjack);
				break;
			case SORT_Technical:
				SongUtil::SortSongPointerArrayByGroupAndMSD(arraySongs,
															Skill_Technical);
				break;
			case SORT_LENGTH:
				SongUtil::SortSongPointerArrayByLength(arraySongs);
				break;
			default:
				FAIL_M("Unhandled sort order! Aborting...");
		}

		// Build an array of WheelItemDatas from the sorted list of Song*'s
		arrayWheelItemDatas.clear(); // clear out the previous wheel items
		arrayWheelItemDatas.reserve(arraySongs.size());

		switch (PREFSMAN->m_MusicWheelUsesSections) {
			case MusicWheelUsesSections_NEVER:
				bUseSections = false;
				break;
			case MusicWheelUsesSections_ABC_ONLY:
				if (so != SORT_TITLE && so != SORT_GROUP)
					bUseSections = false;
				break;
			default:
				break;
		}

		if (bUseSections) {
			// Sorting twice isn't necessary. Instead, modify the compatator
			// functions in Song.cpp to have the desired effect. -Chris
			/* Keeping groups together with the sorts is tricky and brittle; we
			 * keep getting OTHER split up without this. However, it puts the
			 * Grade and BPM sorts in the wrong order, and they're already
			 * correct, so don't re-sort for them. */
			/* We're using sections, so use the section name as the top-level
			 * sort. */
			switch (so) {
				case SORT_FAVORITES:
				case SORT_TOP_GRADES:
				case SORT_BPM:
					break; // don't sort by section
				default:
					SongUtil::SortSongPointerArrayBySectionName(arraySongs, so);
					break;
			}
		}

		allSongsFiltered = arraySongs;
		allSongsByGroupFiltered.clear();

		// make WheelItemDatas with sections

		if (so != SORT_GROUP) {
			// the old code, to unbreak title sort etc -mina
			std::string sLastSection;
			auto iSectionColorIndex = 0;
			for (unsigned i = 0; i < arraySongs.size(); i++) {
				auto* pSong = arraySongs[i];
				if (bUseSections) {
					auto sThisSection =
					  SongUtil::GetSectionNameFromSongAndSort(pSong, so);

					if (sThisSection != sLastSection) {
						auto iSectionCount = 0;
						// Count songs in this section
						unsigned j;
						for (j = i; j < arraySongs.size(); j++) {
							if (SongUtil::GetSectionNameFromSongAndSort(
								  arraySongs[j], so) != sThisSection)
								break;
						}
						iSectionCount = j - i;

						// new section, make a section item
						// todo: preferred sort section color handling? -aj
						auto colorSection =
						  (so == SORT_GROUP)
							? SONGMAN->GetSongGroupColor(pSong->m_sGroupName)
							: SECTION_COLORS.GetValue(iSectionColorIndex);
						iSectionColorIndex =
						  (iSectionColorIndex + 1) % NUM_SECTION_COLORS;
						arrayWheelItemDatas.emplace_back(
						  new MusicWheelItemData(WheelItemDataType_Section,
												 nullptr,
												 sThisSection,
												 colorSection,
												 iSectionCount));
						sLastSection = sThisSection;
					}
				}
				arrayWheelItemDatas.emplace_back(
				  new MusicWheelItemData(WheelItemDataType_Song,
										 pSong,
										 sLastSection,
										 SONGMAN->GetSongColor(pSong),
										 0));
			}
		} else {

			// forces sections for now because who doesnt use sections wtf -mina
			std::string sLastSection;
			auto iSectionColorIndex = 0;

			std::set<Song*> hurp;
			for (auto& a : arraySongs)
				hurp.emplace(a);

			auto& groups = SONGMAN->groupderps;

			std::map<std::string, std::string> shitterstrats;
			for (auto& n : groups) {
				shitterstrats[make_lower(n.first)] = n.first;
				SongUtil::SortSongPointerArrayByTitle(groups[n.first]);
			}

			for (auto& n : shitterstrats) {
				auto& gname = n.second;
				auto& gsongs = groups[n.second];

				auto colorSection = SONGMAN->GetSongGroupColor(gname);
				iSectionColorIndex =
				  (iSectionColorIndex + 1) % NUM_SECTION_COLORS;
				arrayWheelItemDatas.emplace_back(
				  new MusicWheelItemData(WheelItemDataType_Section,
										 nullptr,
										 gname,
										 colorSection,
										 gsongs.size()));

				// need to interact with the filter/search system so check if
				// the song is in the arraysongs set defined above -mina
				for (auto& s : gsongs)
					if (hurp.count(s)) {
						arrayWheelItemDatas.emplace_back(
						  new MusicWheelItemData(WheelItemDataType_Song,
												 s,
												 gname,
												 SONGMAN->GetSongColor(s),
												 0));
						if (allSongsByGroupFiltered.count(gname)) {
							allSongsByGroupFiltered[gname].emplace_back(s);
						} else {
							vector<Song*> v;
							v.emplace_back(s);
							allSongsByGroupFiltered[gname] = v;
						}
					}
			}
		}
	}
}

vector<MusicWheelItemData*>&
MusicWheel::getWheelItemsData(SortOrder so)
{
	// Update the popularity and init icons.
	readyWheelItemsData(so, false, "");
	return m__WheelItemDatas[so];
}

void
MusicWheel::readyWheelItemsData(SortOrder so,
								bool searching,
								const std::string& findme)
{
	if (m_WheelItemDatasStatus[so] != VALID) {
		RageTimer timer;

		auto& aUnFilteredDatas = m__UnFilteredWheelItemDatas[so];

		if (m_WheelItemDatasStatus[so] == INVALID) {
			BuildWheelItemDatas(aUnFilteredDatas, so, searching, findme);
		}
		FilterWheelItemDatas(aUnFilteredDatas, m__WheelItemDatas[so], so);
		m_WheelItemDatasStatus[so] = VALID;

		if (PREFSMAN->m_verbose_log > 0)
			LOG->Trace("MusicWheel sorting took: %f",
					   timer.GetTimeSinceStart());
	}
}

void
MusicWheel::FilterWheelItemDatas(vector<MusicWheelItemData*>& aUnFilteredDatas,
								 vector<MusicWheelItemData*>& aFilteredData,
								 SortOrder so) const
{
	aFilteredData.clear();

	unsigned unfilteredSize = aUnFilteredDatas.size();

	/* Only add WheelItemDataType_Portal if there's at least one song on the
	 * list. */
	auto bFoundAnySong = false;
	for (unsigned i = 0; i < unfilteredSize; i++) {
		if (aUnFilteredDatas[i]->m_Type == WheelItemDataType_Song) {
			bFoundAnySong = true;
			break;
		}
	}

	vector<bool> aiRemove;
	aiRemove.insert(aiRemove.begin(), unfilteredSize, false);

	/* Mark any songs that aren't playable in aiRemove. */

	for (unsigned i = 0; i < unfilteredSize; i++) {
		auto& WID = *aUnFilteredDatas[i];

		/* If we have no songs, remove Random and Portal. */
		if (WID.m_Type == WheelItemDataType_Random ||
			WID.m_Type == WheelItemDataType_Portal) {
			if (!bFoundAnySong)
				aiRemove[i] = true;
			continue;
		}

		/* Filter songs that we don't have enough stages to play. */
		if (WID.m_Type == WheelItemDataType_Song) {
			auto* pSong = WID.m_pSong;
			/* If the song has no steps for the current style, remove it. */
			if (!CommonMetrics::AUTO_SET_STYLE &&
				!pSong->HasStepsType(
				  GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType)) {
				aiRemove[i] = true;
				continue;
			}

			// if AutoSetStyle, make sure the song is playable in the end.
			if (!SongUtil::IsSongPlayable(pSong)) {
				aiRemove[i] = true;
				continue;
			}
		}
	}

	/* Filter out the songs we're removing. */

	aFilteredData.reserve(unfilteredSize);
	for (unsigned i = 0; i < unfilteredSize; i++) {
		if (aiRemove[i])
			continue;
		aFilteredData.emplace_back(aUnFilteredDatas[i]);
	}

	// Update the song count in each section header.
	unsigned filteredSize = aFilteredData.size();
	for (unsigned i = 0; i < filteredSize;) {
		auto& WID = *aFilteredData[i];
		++i;
		if (WID.m_Type != WheelItemDataType_Section)
			continue;

		// Count songs in this section
		WID.m_iSectionCount = 0;
		for (; i < filteredSize && aFilteredData[i]->m_sText == WID.m_sText;
			 ++i)
			++WID.m_iSectionCount;
	}

	// If we have any section headers with no songs, then we filtered all of the
	// songs in that group, so remove it.  This isn't optimized like the above
	// since this is a rare case.
	for (unsigned i = 0; i < filteredSize; ++i) {
		auto& WID = *aFilteredData[i];
		if (WID.m_Type != WheelItemDataType_Section)
			continue;
		if (WID.m_iSectionCount > 0)
			continue;
		aFilteredData.erase(aFilteredData.begin() + i,
							aFilteredData.begin() + i + 1);
		--i;
		--filteredSize;
	}

	// If we've filtered all items, insert a dummy.
	if (aFilteredData.empty())
		aFilteredData.emplace_back(new MusicWheelItemData(
		  WheelItemDataType_Section, nullptr, EMPTY_STRING, EMPTY_COLOR, 0));
}

void
MusicWheel::UpdateSwitch()
{
	switch (m_WheelState) {
		case STATE_FLYING_OFF_BEFORE_NEXT_SORT: {
			const Song* pPrevSelectedSong =
			  GetCurWheelItemData(m_iSelection)->m_pSong;

			SCREENMAN->PostMessageToTopScreen(SM_SortOrderChanged, 0);

			SetOpenSection(SongUtil::GetSectionNameFromSongAndSort(
			  pPrevSelectedSong, GAMESTATE->m_SortOrder));

			m_iSelection = 0;

			// Select the previously selected item
			switch (GAMESTATE->m_SortOrder) {
				default:
					// Look for the last selected song or course
					SelectSongOrCourse();
					break;
				case SORT_MODE_MENU:
					SelectModeMenuItem();
					break;
			}

			// Change difficulty for sorts by meter
			// XXX: do this with GameCommand?
			StepsType st;
			Difficulty dc;
			if (SongUtil::GetStepsTypeAndDifficultyFromSortOrder(
				  GAMESTATE->m_SortOrder, st, dc)) {
				ASSERT(dc != Difficulty_Invalid);
				if (GAMESTATE->IsPlayerEnabled(PLAYER_1))
					GAMESTATE->m_PreferredDifficulty.Set(dc);
			}

			SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);
			RebuildWheelItems();
			TweenOnScreenForSort();
		} break;

		case STATE_FLYING_ON_AFTER_NEXT_SORT:
			m_WheelState = STATE_SELECTING; // now, wait for input
			break;

		case STATE_SELECTING:
			m_fTimeLeftInState = 0;
			break;
		case STATE_ROULETTE_SPINNING:
		case STATE_RANDOM_SPINNING:
			break;
		case STATE_LOCKED:
			break;
		case STATE_ROULETTE_SLOWING_DOWN:
			if (m_iSwitchesLeftInSpinDown == 0) {
				m_WheelState = STATE_LOCKED;
				m_fTimeLeftInState = 0;
				SCREENMAN->PlayStartSound();
				m_fLockedWheelVelocity = 0;

				// Send this again so the screen starts sample music.
				SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);
				MESSAGEMAN->Broadcast("RouletteStopped");
			} else {
				--m_iSwitchesLeftInSpinDown;
				const float SwitchTimes[] = { 0.5f, 1.3f, 0.8f, 0.4f, 0.2f };
				ASSERT(m_iSwitchesLeftInSpinDown >= 0 &&
					   m_iSwitchesLeftInSpinDown <= 4);
				m_fTimeLeftInState = SwitchTimes[m_iSwitchesLeftInSpinDown];
				m_Moving = 0;

				LOG->Trace(
				  "m_iSwitchesLeftInSpinDown id %d, m_fTimeLeftInState is %f",
				  m_iSwitchesLeftInSpinDown,
				  m_fTimeLeftInState);

				if (m_iSwitchesLeftInSpinDown == 0)
					ChangeMusic(randomf(0, 1) >= 0.5f ? 1 : -1);
				else
					ChangeMusic(1);
			}
			break;
		default:
			FAIL_M(ssprintf("Invalid wheel state: %i", m_WheelState));
	}
}

void
MusicWheel::ChangeMusic(int iDist)
{
	m_iSelection += iDist;
	wrap(m_iSelection, m_CurWheelItemData.size());

	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS) {
		// store the group song index
		for (unsigned idx = 0; idx < m_viWheelPositions.size(); idx++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(idx)) {
				m_viWheelPositions[idx] = m_iSelection;
			}
		}
	}

	RebuildWheelItems(iDist);

	m_fPositionOffsetFromSelection += iDist;

	SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);

	// If we're moving automatically, don't play this; it'll be called in
	// Update.
	if (!IsMoving())
		m_soundChangeMusic.Play(true);
}

bool
MusicWheel::ChangeSort(SortOrder new_so,
					   bool allowSameSort) // return true if change successful
{
	ASSERT(new_so < NUM_SortOrder);
	if (GAMESTATE->m_SortOrder == new_so && !allowSameSort) {
		return false;
	}

	// Don't change to SORT_MODE_MENU if it doesn't have at least two choices.
	if (new_so == SORT_MODE_MENU && getWheelItemsData(new_so).size() < 2) {
		return false;
	}

	switch (m_WheelState) {
		case STATE_SELECTING:
		case STATE_FLYING_ON_AFTER_NEXT_SORT:
			break; // fall through
		default:
			return false; // don't continue
	}

	SCREENMAN->PostMessageToTopScreen(SM_SortOrderChanging, 0);

	m_soundChangeSort.Play(true);

	TweenOffScreenForSort();

	// Save the new preference.
	if (IsSongSort(new_so))
		GAMESTATE->m_PreferredSortOrder = new_so;
	GAMESTATE->m_SortOrder.Set(new_so);

	return true;
}

bool
MusicWheel::NextSort() // return true if change successful
{
	// don't allow NextSort when on the mode menu
	if (GAMESTATE->m_SortOrder == SORT_MODE_MENU)
		return false;

	vector<SortOrder> aSortOrders;
	{
		auto* L = LUA->Get();
		SORT_ORDERS.PushSelf(L);
		FOREACH_LUATABLEI(L, -1, i)
		{
			auto so = Enum::Check<SortOrder>(L, -1, true);
			aSortOrders.emplace_back(so);
		}
		lua_pop(L, 1);
		LUA->Release(L);
	}

	// find the index of the current sort
	auto cur = 0;
	while (cur < static_cast<int>(aSortOrders.size()) &&
		   aSortOrders[cur] != GAMESTATE->m_SortOrder)
		++cur;

	// move to the next sort with wrapping
	++cur;
	wrap(cur, aSortOrders.size());

	// apply new sort
	auto soNew = aSortOrders[cur];
	return ChangeSort(soNew);
}

bool
MusicWheel::Select() // return true if this selection ends the screen
{
	LOG->Trace("MusicWheel::Select()");

	switch (m_WheelState) {
		case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
		case STATE_ROULETTE_SLOWING_DOWN:
			return false;
		case STATE_ROULETTE_SPINNING:
			m_WheelState = STATE_ROULETTE_SLOWING_DOWN;
			m_iSwitchesLeftInSpinDown =
			  ROULETTE_SLOW_DOWN_SWITCHES / 2 + 1 +
			  RandomInt(ROULETTE_SLOW_DOWN_SWITCHES / 2);
			m_fTimeLeftInState = 0.1f;
			return false;
		case STATE_RANDOM_SPINNING:
			m_fPositionOffsetFromSelection =
			  std::max(m_fPositionOffsetFromSelection, 0.3f);
			m_WheelState = STATE_LOCKED;
			SCREENMAN->PlayStartSound();
			m_fLockedWheelVelocity = 0;
			// Set m_Moving to zero to stop the sounds from playing.
			m_Moving = 0;
			SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);
			return true;
		default:
			break;
	}

	if (!WheelBase::Select())
		return false;

	switch (m_CurWheelItemData[m_iSelection]->m_Type) {
		case WheelItemDataType_Roulette:
			return false;
		case WheelItemDataType_Random:
			return false;
		case WheelItemDataType_Sort:
			GetCurWheelItemData(m_iSelection)->m_pAction->ApplyToAllPlayers();
			ChangeSort(GAMESTATE->m_PreferredSortOrder);
			m_sLastModeMenuItem =
			  GetCurWheelItemData(m_iSelection)->m_pAction->m_sName;
			return false;
		case WheelItemDataType_Custom:
			GetCurWheelItemData(m_iSelection)->m_pAction->ApplyToAllPlayers();
			if (!GetCurWheelItemData(m_iSelection)
				   ->m_pAction->m_sScreen.empty())
				return true;
			return false;
		default:
			return true;
	}
}

void
MusicWheel::SetOpenSection(const std::string& group)
{
	// LOG->Trace( "SetOpenSection %s", group.c_str() );
	m_sExpandedSectionName = group;
	GAMESTATE->sExpandedSectionName = group;

	// wheel positions = num song groups
	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS)
		m_viWheelPositions.resize(SONGMAN->GetNumSongGroups());

	const WheelItemBaseData* old = nullptr;
	if (!m_CurWheelItemData.empty())
		old = GetCurWheelItemData(m_iSelection);

	vector<const Style*> vpPossibleStyles;
	if (CommonMetrics::AUTO_SET_STYLE)
		GAMEMAN->GetCompatibleStyles(GAMESTATE->m_pCurGame,
									 GAMESTATE->GetNumPlayersEnabled(),
									 vpPossibleStyles);

	m_CurWheelItemData.clear();
	auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	m_CurWheelItemData.reserve(from.size());
	for (auto& i : from) {
		auto& d = *i;

		// Hide songs/courses which are not in the active section
		if ((d.m_Type == WheelItemDataType_Song ||
			 d.m_Type == WheelItemDataType_Course) &&
			!d.m_sText.empty() && d.m_sText != group)
			continue;

		// In certain situations (e.g. simulating Pump it Up or IIDX),
		// themes may want to hide inactive section headings as well.
		if (HIDE_INACTIVE_SECTIONS && d.m_Type == WheelItemDataType_Section &&
			!group.empty()) {
			// Based on the HideActiveSectionTitle metric, we either
			// hide all section titles, or only those which are not
			// currently open.
			if (HIDE_ACTIVE_SECTION_TITLE || d.m_sText != group)
				continue;
		}

		m_CurWheelItemData.emplace_back(&d);
	}

	// restore the past group song index
	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS) {
		for (unsigned idx = 0; idx < m_viWheelPositions.size(); idx++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(idx)) {
				m_iSelection = m_viWheelPositions[idx];
			}
		}
	} else {
		// Try to select the item that was selected before changing groups
		m_iSelection = 0;

		for (unsigned i = 0; i < m_CurWheelItemData.size(); i++) {
			if (m_CurWheelItemData[i] == old) {
				m_iSelection = i;
				break;
			}
		}
	}

	RebuildWheelItems();
}

// sm-ssc additions: jump to group
std::string
MusicWheel::JumpToNextGroup()
{
	// Thanks to Juanelote for this logic:
	if (HIDE_INACTIVE_SECTIONS) {
		// todo: make it work with other sort types
		unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for (unsigned i = 0; i < iNumGroups; i++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i)) {
				if (i < iNumGroups - 1)
					return SONGMAN->GetSongGroupByIndex(i + 1);
				// i = 0;
				return SONGMAN->GetSongGroupByIndex(0);
			}
		}
	} else {
		unsigned int iLastSelection = m_iSelection;
		for (unsigned int i = m_iSelection; i < m_CurWheelItemData.size();
			 ++i) {
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section &&
				i != static_cast<unsigned int>(m_iSelection)) {
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// it should not get down here, but it might happen... only search up to
		// the previous selection.
		for (unsigned int i = 0; i < iLastSelection; ++i) {
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section &&
				i != static_cast<unsigned int>(m_iSelection)) {
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}
	// it shouldn't get here, but just in case...
	return "";
}

std::string
MusicWheel::JumpToPrevGroup()
{
	if (HIDE_INACTIVE_SECTIONS) {
		unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for (unsigned i = 0; i < iNumGroups; i++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i)) {
				if (i > 0)
					return SONGMAN->GetSongGroupByIndex(i - 1);
				// i = iNumGroups - 1;
				return SONGMAN->GetSongGroupByIndex(iNumGroups - 1);
			}
		}
	} else {
		for (unsigned int i = m_iSelection; i > 0; --i) {
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section &&
				i != static_cast<unsigned int>(m_iSelection)) {
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// in case it wasn't found above:
		for (unsigned int i = m_CurWheelItemData.size() - 1; i > 0; --i) {
			LOG->Trace("JumpToPrevGroup iteration 2 | i = %u", i);
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section) {
				m_iSelection = i;
				LOG->Trace("finding it in #2 | i = %u | text = %s",
						   i,
						   m_CurWheelItemData[i]->m_sText.c_str());
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}
	// it shouldn't get here, but just in case...
	return "";
}

// Called on late join. Selectable courses may have changed; reopen the section.
void
MusicWheel::PlayerJoined()
{
	// If someone joins, there may be songs on the wheel that should not be
	// selectable, or there may be songs that have become selectable.
	// Set the status of all the wheel item data vectors to invalid so that
	// readyWheelItemsData will rebuild all the data next time
	// getWheelItemsData is called for that SortOrder.  SetOpenSection calls
	// readyWheelItemsData to get the items, and RebuildWheelItems when its
	// done, so invalidating and calling SetOpenSection is all we need to do.
	// -Kyz
	// Also removed the weird checks for course mode and autogen because
	// it seems weird that courses wouldn't also be affected by a player
	// joining, and not doing it in autogen causes other weird problems. -Kyz
	FOREACH_ENUM(SortOrder, so) { m_WheelItemDatasStatus[so] = INVALID; }
	SetOpenSection(m_sExpandedSectionName);
}

bool
MusicWheel::IsRouletting() const
{
	return m_WheelState == STATE_ROULETTE_SPINNING ||
		   m_WheelState == STATE_ROULETTE_SLOWING_DOWN ||
		   m_WheelState == STATE_RANDOM_SPINNING;
}

Song*
MusicWheel::GetSelectedSong()
{
	switch (m_CurWheelItemData[m_iSelection]->m_Type) {
		case WheelItemDataType_Portal:
			return GetPreferredSelectionForRandomOrPortal();
		default:
			return GetCurWheelItemData(m_iSelection)->m_pSong;
	}
}

/* Find a random song.  If possible, find one that has the preferred
 * difficulties of each player.  Prefer songs in the active group, if any.
 *
 * Note that if this is called, we *must* find a song.  We will only be called
 * if the active sort has at least one song, but there may be no open group.
 * This means that any filters and preferences applied here must be optional. */
Song*
MusicWheel::GetPreferredSelectionForRandomOrPortal()
{
	// probe to find a song that has the preferred
	// difficulties of each player
	vector<Difficulty> vDifficultiesToRequire;

	if (GAMESTATE->m_PreferredDifficulty == Difficulty_Invalid) {
		// skip
	}

	// TRICKY: Don't require that edits be present if perferred
	// difficulty is Difficulty_Edit.  Otherwise, players could use this
	// to set up a 100% chance of getting a particular locked song by
	// having a single edit for a locked song.
	else if (GAMESTATE->m_PreferredDifficulty == Difficulty_Edit) {
		// skip
	} else {

		vDifficultiesToRequire.emplace_back(GAMESTATE->m_PreferredDifficulty);
	}

	auto sPreferredGroup = m_sExpandedSectionName;
	auto& wid = getWheelItemsData(GAMESTATE->m_SortOrder);

	auto st = GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType;

#define NUM_PROBES 1000
	for (auto i = 0; i < NUM_PROBES; i++) {
		auto isValid = true;
		/* Maintaining difficulties is higher priority than maintaining
		 * the current group. */
		if (i == NUM_PROBES / 4)
			sPreferredGroup = "";
		if (i == NUM_PROBES / 2)
			vDifficultiesToRequire.clear();

		auto iSelection = RandomInt(wid.size());
		if (wid[iSelection]->m_Type != WheelItemDataType_Song)
			continue;

		const Song* pSong = wid[iSelection]->m_pSong;

		if (!sPreferredGroup.empty() &&
			wid[iSelection]->m_sText != sPreferredGroup)
			continue;

		FOREACH(Difficulty, vDifficultiesToRequire, d)
		{
			if (!pSong->HasStepsTypeAndDifficulty(st, *d)) {
				isValid = false;
				break;
			}
		}

		if (isValid) {
			return wid[iSelection]->m_pSong;
		}
	}
	LuaHelpers::ReportScriptError("Couldn't find any songs");
	return wid[0]->m_pSong;
}

void
MusicWheel::FinishChangingSorts()
{
	FinishTweening();
	m_WheelState = STATE_SELECTING;
	m_fTimeLeftInState = 0;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

class LunaMusicWheel : public Luna<MusicWheel>
{
  public:
	static int ChangeSort(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			lua_pushboolean(L, 0);
		} else {
			auto so = Enum::Check<SortOrder>(L, 1);
			lua_pushboolean(L, p->ChangeSort(so));
		}
		return 1;
	}
	DEFINE_METHOD(GetSelectedSection, GetSelectedSection());
	static int IsRouletting(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsRouletting());
		return 1;
	}
	static int SelectSong(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			lua_pushboolean(L, 0);
		} else {
			auto* pS = Luna<Song>::check(L, 1, true);
			lua_pushboolean(L, p->SelectSong(pS));
		}
		return 1;
	}
	static int SongSearch(T* p, lua_State* L)
	{
		p->ReloadSongList(true, SArg(1));
		return 1;
	}
	static int ReloadSongList(T* p, lua_State* L)
	{
		p->ReloadSongList(false, "");
		return 1;
	}
	static int Move(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) {
			p->Move(0);
		} else {
			p->Move(IArg(1));
		}
		return 1;
	}
	static int MoveAndCheckType(T* p, lua_State* L)
	{
		auto n = IArg(1);
		p->Move(n);
		auto tt = p->GetSelectedType();
		LuaHelpers::Push(L, tt);

		return 1;
	}

	static int FilterByStepKeys(T* p, lua_State* L)
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_pushvalue(L, 1);
		vector<string> newHashList;
		LuaHelpers::ReadArrayFromTable(newHashList, L);
		lua_pop(L, 1);
		p->SetHashList(newHashList);
		p->ReloadSongList(false, "");
		return 1;
	}

	static int SetPackListFiltering(T* p, lua_State* L)
	{
		auto old = FILTERMAN->filteringCommonPacks;
		FILTERMAN->filteringCommonPacks = BArg(1);
		lua_pushboolean(L, FILTERMAN->filteringCommonPacks);
		if (old == FILTERMAN->filteringCommonPacks)
			return 1;
		p->ReloadSongList(false, "");
		return 1;
	}

	static int GetSongs(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (size_t i = 0; i < p->allSongsFiltered.size(); ++i) {
			p->allSongsFiltered[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int GetSongsInGroup(T* p, lua_State* L)
	{
		lua_newtable(L);
		const auto* group = SArg(1);

		if (p->allSongsByGroupFiltered.count(group) == 0)
			return 1;

		for (size_t i = 0; i < p->allSongsByGroupFiltered[group].size(); ++i) {
			p->allSongsByGroupFiltered[group][i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	LunaMusicWheel()
	{
		ADD_METHOD(ChangeSort);
		ADD_METHOD(GetSelectedSection);
		ADD_METHOD(IsRouletting);
		ADD_METHOD(SelectSong);
		ADD_METHOD(SongSearch);
		ADD_METHOD(ReloadSongList);
		ADD_METHOD(Move);
		ADD_METHOD(MoveAndCheckType);
		ADD_METHOD(FilterByStepKeys);
		ADD_METHOD(SetPackListFiltering);
		ADD_METHOD(GetSongs);
		ADD_METHOD(GetSongsInGroup);
	}
};

LUA_REGISTER_DERIVED_CLASS(MusicWheel, WheelBase)
// lua end
