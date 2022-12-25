#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/CommonMetrics.h"
#include "Etterna/Singletons/FilterManager.h"
#include "Etterna/Models/Misc/GameCommand.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/MessageManager.h"
#include "MusicWheel.h"
#include "Etterna/Singletons/NetworkSyncManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Songs/SongUtil.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Singletons/ThemeManager.h"
#include "Etterna/Singletons/ScoreManager.h"
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
  SM_SongChanged); // TODO(Sam): Replace this with a Message and MESSAGEMAN
AutoScreenMessage(SM_SortOrderChanging);
AutoScreenMessage(SM_SortOrderChanged);

auto
MusicWheel::MakeItem() -> MusicWheelItem*
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
	std::vector<std::string> vsModeChoiceNames;
	split(MODE_MENU_CHOICE_NAMES, ",", vsModeChoiceNames);
	CHOICE.Load(sType, CHOICE_NAME, vsModeChoiceNames);
	SECTION_COLORS.Load(sType, SECTION_COLORS_NAME, NUM_SECTION_COLORS);

	CUSTOM_WHEEL_ITEM_NAMES.Load(sType, "CustomWheelItemNames");
	std::vector<std::string> vsCustomItemNames;
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
			if (i->m_pAction->DescribesCurrentModeForAllPlayers()) {
				m_sLastModeMenuItem = i->m_pAction->m_sName;
				break;
			}
		}
	}

	WheelBase::BeginScreen();

	GAMESTATE->m_SortOrder.Set(GAMESTATE->m_PreferredSortOrder);

	// Never start in the mode menu; some elements may not initialize correctly.
	if (GAMESTATE->m_SortOrder == SORT_MODE_MENU) {
		GAMESTATE->m_SortOrder.Set(SortOrder_Invalid);
	}

	GAMESTATE->m_SortOrder.Set(GAMESTATE->m_SortOrder);

	/* Only save the sort order if the player didn't already have one.
	 * If he did, don't overwrite it. */
	if (GAMESTATE->m_PreferredSortOrder == SortOrder_Invalid) {
		GAMESTATE->m_PreferredSortOrder = GAMESTATE->m_SortOrder;
	}

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

// this is a trainwreck and i made it worse -mina
void
MusicWheel::ReloadSongList(bool searching, const std::string& findme)
{
	// if we fallthrough to pack name matching don't keep reloading if we found
	// a match -mina
	if (findme.size() > lastvalidsearch.size() &&
		!groupnamesearchmatch.empty()) {
		return;
	}

	auto matchesBefore = m__WheelItemDatas[GAMESTATE->m_SortOrder].size();
	Song* firstItemBefore = nullptr;
	if (matchesBefore > 1) {
		firstItemBefore = m__WheelItemDatas[GAMESTATE->m_SortOrder].at(1)->m_pSong;
	}

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
		SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0.35F);
		return;
	}

	const auto songIdxToPreserve = m_iSelection;
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

	auto matchesAfter = m__WheelItemDatas[GAMESTATE->m_SortOrder].size();

	// when searching, automatically land on the first search result available
	// -mina & dadbearcop
	if (!findme.empty() || !hashList.empty()) {
		if (!groupnamesearchmatch.empty()) {
			// matched on group name
			SelectSection(groupnamesearchmatch);
			SetOpenSection(groupnamesearchmatch);
			ChangeMusic(1);
			SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0.35F);
			return;
		}

		if (matchesAfter == matchesBefore) {
			// results didnt change, probably double enter
			// or multiple searches with no results
			// dont move selection
			if (matchesAfter > 1 && firstItemBefore != nullptr) {
				auto* song =
				  m__WheelItemDatas[GAMESTATE->m_SortOrder].at(1)->m_pSong;
				if (song == firstItemBefore) {
					// same song, dont move selection
				} else {
					SelectSongAfterSearch();
				}
			}
		} else {
			SelectSongAfterSearch();
		}
	} else {

		if (matchesBefore == matchesAfter) {
			// no results, stay on selection
		} else {
			SetOpenSection("");
		}
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
auto
MusicWheel::SelectSongOrCourse() -> bool
{
	if ((GAMESTATE->m_pPreferredSong != nullptr) &&
		SelectSong(GAMESTATE->m_pPreferredSong)) {
		return true;
	}
	if ((GAMESTATE->m_pCurSong != nullptr) &&
		SelectSong(GAMESTATE->m_pCurSong)) {
		return true;
	}

	// Select the first selectable song based on the sort order...
	auto& wiWheelItems = getWheelItemsData(GAMESTATE->m_SortOrder);
	for (auto& wiWheelItem : wiWheelItems) {
		if (wiWheelItem->m_pSong != nullptr) {
			return SelectSong(wiWheelItem->m_pSong);
		}
	}

	Locator::getLogger()->info(
	  "MusicWheel::MusicWheel() - No selectable songs "
	  "found in WheelData");
	return false;
}

auto
MusicWheel::SelectSection(const std::string& SectionName) -> bool
{
	for (auto i = 0; i < static_cast<int>(m_CurWheelItemData.size()); ++i) {
		if (m_CurWheelItemData[i]->m_sText == SectionName) {
			m_iSelection = i; // select it
			return true;
		}
	}

	return false;
}

auto
MusicWheel::SelectSong(const Song* p) -> bool
{
	if (p == nullptr) {
		return false;
	}

	unsigned i;
	auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	for (i = 0; i < from.size(); i++) {
		if (from[i]->m_pSong == p) {
			// make its group the currently expanded group
			SetOpenSection(from[i]->m_sText);

			// skip any playlist groups
			if (SongManager::GetPlaylists().count(GetExpandedSectionName()) ==
				0u) {
				break;
			}
		}
	}

	if (i == from.size()) {
		return false;
	}

	for (auto j = 0; j < static_cast<int>(m_CurWheelItemData.size()); ++j) {
		if (GetCurWheelItemData(j)->m_pSong == p) {
			m_iSelection = j; // select it
		}
	}

	ChangeMusic(
	  0); // Actually select it, come on guys how hard was this LOL - mina
	return true;
}

auto
MusicWheel::SelectModeMenuItem() -> bool
{
	// Select the last-chosen option.
	ASSERT(GAMESTATE->m_SortOrder == SORT_MODE_MENU);
	const auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	unsigned i;
	for (i = 0; i < from.size(); i++) {
		const auto& gc = *from[i]->m_pAction;
		if (gc.m_sName == m_sLastModeMenuItem) {
			break;
		}
	}

	if (i == from.size()) {
		return false;
	}

	// make its group the currently expanded group
	SetOpenSection(from[i]->m_sText);

	for (auto j = 0; j < static_cast<int>(m_CurWheelItemData.size()); j++) {
		if (GetCurWheelItemData(j)->m_pAction->m_sName != m_sLastModeMenuItem) {
			continue;
		}

		m_iSelection = j; // select it
		break;
	}

	return true;
}

// bool MusicWheel::SelectCustomItem()

void
MusicWheel::GetSongList(std::vector<Song*>& arraySongs,
						const SortOrder so) const
{
	std::vector<Song*> apAllSongs;
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
					  GAMESTATE->GetCurrentStyle(PLAYER_INVALID)
						->m_StepsType)) {
					arraySongs.emplace_back(pSong);
				}
			}
		}
	}
}
auto
contains(std::string container, const std::string& findme) -> bool
{
	std::transform(
	  begin(container), end(container), begin(container), ::tolower);
	return container.find(findme) != std::string::npos;
}
void
MusicWheel::FilterBySearch(std::vector<Song*>& inv, std::string findme)
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

	std::string findartist;
	std::string findauthor;
	std::string findtitle;
	std::string findsubtitle;

	if (artist != std::string::npos || author != std::string::npos ||
		title != std::string::npos || subtitle != std::string::npos) {
		super_search = true;
		if (artist != std::string::npos) {
			findartist = findme.substr(
			  artist + 7, findme.find(static_cast<char>(artist), ';') - artist);
		}
		if (author != std::string::npos) {
			findauthor = findme.substr(
			  author + 7, findme.find(static_cast<char>(author), ';') - author);
		}
		if (title != std::string::npos) {
			findtitle = findme.substr(
			  title + 6, findme.find(static_cast<char>(title), ';') - title);
		}
		if (subtitle != std::string::npos) {
			findsubtitle = findme.substr(
			  subtitle + 9,
			  findme.find(static_cast<char>(subtitle), ';') - subtitle);
		}
	}

	// The giant block of code below is for optimization purposes.
	// Basically, we don't want to give a single fat lambda to the filter that
	// checks and short circuits. Instead, we want to just not check at all.
	// It's a baby sized optimization but adds up over time. The binary comments
	// help verify which things are being checked.
	std::vector<Song*> tmp;
	std::function<bool(Song*)> check;
	if (!super_search) {
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
						if (findtitle.empty()) {
							// 1001
							check = [&findsubtitle, &findartist](Song* x) {
								return contains(x->GetDisplayArtist(),
												findartist) ||
									   contains(x->GetDisplaySubTitle(),
												findsubtitle);
							};
						} else {
							if (findartist.empty()) {
								// 1010
								check = [&findsubtitle, &findtitle](Song* x) {
									return contains(x->GetDisplayMainTitle(),
													findtitle) ||
										   contains(x->GetDisplaySubTitle(),
													findsubtitle);
								};
							} else {
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
							};
						}
					} else {
						if (findtitle.empty()) {
							if (findartist.empty()) {
								// 1100
								check = [&findsubtitle, &findauthor](Song* x) {
									return contains(
											 x->GetOrTryAtLeastToGetSimfileAuthor(),
											 findauthor) ||
										   contains(x->GetDisplaySubTitle(),
													findsubtitle);
								};
							} else {
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
						if (findtitle.empty()) {
							// 0001
							check = [&findartist](Song* x) {
								return contains(x->GetDisplayArtist(),
												findartist);
							};
						} else {
							if (findartist.empty()) {
								// 0010
								check = [&findtitle](Song* x) {
									return contains(x->GetDisplayMainTitle(),
													findtitle);
								};
							} else {
								// 0011
								check = [&findartist, &findtitle](Song* x) {
									return contains(x->GetDisplayArtist(),
													findartist) ||
										   contains(x->GetDisplayMainTitle(),
													findtitle);
								};
							};
						}
					} else {
						if (findtitle.empty()) {
							if (findartist.empty()) {
								// 0100
								check = [&findauthor](Song* x) {
									return contains(
									  x->GetOrTryAtLeastToGetSimfileAuthor(),
									  findauthor);
								};
							} else {
								// 0101
								check = [&findauthor, &findartist](Song* x) {
									return contains(x->GetDisplayArtist(),
													findartist) ||
										   contains(
											 x->GetOrTryAtLeastToGetSimfileAuthor(),
											 findauthor);
								};
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
		if (check(x)) {
			tmp.push_back(x);
		}
	}
	if (!tmp.empty()) {
		lastvalidsearch = findme;
		groupnamesearchmatch = "";
		inv.swap(tmp);
	} else {
		if (SearchGroupNames(findme)) {
			return;
		}
		FilterBySearch(inv, lastvalidsearch);
	}
}

void
MusicWheel::SetHashList(const std::vector<string>& newHashList)
{
	hashList = newHashList;
}
void
MusicWheel::SetOutHashList(const std::vector<string>& newOutHashList)
{
	outHashList = newOutHashList;
}

void
MusicWheel::FilterByAndAgainstStepKeys(std::vector<Song*>& inv)
{
	std::vector<Song*> tmp;
	const std::function<bool(Song*, std::vector<string>&)> check =
	  [](Song* x, std::vector<string>& hl) {
		  for (auto& ck : hl) {
			  if (x->HasChartByHash(ck)) {
				  return true;
			  }
		  }

		  return false;
	  };

	if (!hashList.empty()) {
		for (auto* x : inv) {
			if (check(x, hashList) && !check(x, outHashList)) {
				tmp.emplace_back(x);
			}
		}
	} else {
		for (auto* x : inv) {
			if (!check(x, outHashList)) {
				tmp.emplace_back(x);
			}
		}
	}
	if (!tmp.empty()) {
		inv.swap(tmp);
	}
}

auto
MusicWheel::SearchGroupNames(const std::string& findme) -> bool
{
	const auto& grps = SONGMAN->GetSongGroupNames();
	for (const auto& grp : grps) {
		auto lc = make_lower(std::string(grp));
		const auto droop = lc.find(findme);
		if (droop != std::basic_string<char,
									   std::char_traits<char>,
									   std::allocator<char>>::npos) {
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
MusicWheel::FilterBySkillsets(std::vector<Song*>& inv)
{
	std::vector<Song*> tmp;

	for (auto* song : inv) {
		auto addsong = false;
		for (auto currate = FILTERMAN->MaxFilterRate;
			 currate > FILTERMAN->MinFilterRate - .01F;
			 currate -= 0.1F) { /* Iterate over all possible rates.
								 * The .01f delta is because floating points
								 * don't like exact equivalency*/
			if (song->MatchesFilter(currate)) {
				addsong = true;
				break; // We don't need to keep checking rates
			}
		}
		if (addsong) {
			tmp.emplace_back(song);
		}
	}
	inv.swap(tmp);
}

void
MusicWheel::BuildWheelItemDatas(
  std::vector<std::unique_ptr<MusicWheelItemData>>& arrayWheelItemDatas,
  SortOrder so,
  bool searching,
  const std::string& findme)
{

	std::map<std::string, Commands> commanDZ;
	if (so == SORT_MODE_MENU) {
		arrayWheelItemDatas.clear(); // clear out the previous wheel items
		std::vector<std::string> vsNames;
		split(MODE_MENU_CHOICE_NAMES, ",", vsNames);
		for (auto i = 0; i < static_cast<int>(vsNames.size()); ++i) {
			auto wid = std::make_unique<MusicWheelItemData>(
			  WheelItemDataType_Sort, nullptr, "", SORT_MENU_COLOR, 0);
			wid->m_pAction = std::make_unique<GameCommand>();
			wid->m_pAction->m_sName = vsNames[i];
			wid->m_pAction->Load(i, ParseCommands(CHOICE.GetValue(vsNames[i])));
			wid->m_sLabel = WHEEL_TEXT(vsNames[i]);

			if (!wid->m_pAction->IsPlayable()) {
				continue;
			}

			arrayWheelItemDatas.emplace_back(std::move(wid));
		}
	} else {
		// Make an array of Song*, then sort them
		std::vector<Song*> arraySongs;
		GetSongList(arraySongs, so);

		Message msg("FilterResults");
		msg.SetParam("Total", static_cast<int>(arraySongs.size()));

		if (FILTERMAN->filteringCommonPacks && NSMAN->IsETTP() &&
			!NSMAN->commonpacks.empty()) {
			std::vector<Song*> tmp;
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

		if (searching) {
			FilterBySearch(arraySongs, findme);
		}

		if (!hashList.empty() || !outHashList.empty()) {
			FilterByAndAgainstStepKeys(arraySongs);
		}

		if (FILTERMAN->AnyActiveFilter()) {
			FilterBySkillsets(arraySongs);
		}

		msg.SetParam("Matches", static_cast<int>(arraySongs.size()));
		MESSAGEMAN->Broadcast(msg);

		auto bUseSections = true;

		// sort the songs
		switch (so) {
			case SORT_FAVORITES:
				bUseSections = false;
				break;
			case SORT_GROUP:
				SongUtil::SortSongPointerArrayByGroupAndTitle(arraySongs);

				if (USE_SECTIONS_WITH_PREFERRED_GROUP) {
					bUseSections = true;
				} else {
					bUseSections =
					  GAMESTATE->m_sPreferredSongGroup == GROUP_ALL;
				}
				break;
			case SORT_Ungrouped:
				[[fallthrough]];
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
				if (so != SORT_TITLE && so != SORT_GROUP) {
					bUseSections = false;
				}
				break;
			default:
				break;
		}

		if (bUseSections) {
			// Sorting twice isn't necessary. Instead, modify the comparator
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
		allSongsByGroupFiltered[so].clear();
		packProgressByGroup[so].clear();

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
								  arraySongs[j], so) != sThisSection) {
								break;
							}
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
						  std::make_unique<MusicWheelItemData>(
							WheelItemDataType_Section,
							nullptr,
							sThisSection,
							colorSection,
							iSectionCount));
						sLastSection = sThisSection;
					}
				}
				arrayWheelItemDatas.emplace_back(std::make_unique<MusicWheelItemData>(WheelItemDataType_Song,
										 pSong,
										 sLastSection,
										 SONGMAN->GetSongColor(pSong),
										 0));
				if (allSongsByGroupFiltered.at(so).count(sLastSection) != 0u) {
					allSongsByGroupFiltered.at(so)[sLastSection].emplace_back(pSong);
				} else {
					std::vector<Song*> v;
					v.emplace_back(pSong);
					allSongsByGroupFiltered.at(so)[sLastSection] = v;
				}
			}
		} else {

			// forces sections for now because who doesnt use sections wtf -mina
			std::string sLastSection;
			auto iSectionColorIndex = 0;

			std::set<Song*> hurp;
			for (auto& a : arraySongs) {
				hurp.emplace(a);
			}

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
				arrayWheelItemDatas.emplace_back(std::make_unique<MusicWheelItemData>(WheelItemDataType_Section,
										 nullptr,
										 gname,
										 colorSection,
										 gsongs.size()));

				// need to interact with the filter/search system so check if
				// the song is in the arraysongs set defined above -mina
				for (auto& s : gsongs) {
					if (hurp.count(s) != 0u) {
						arrayWheelItemDatas.emplace_back(std::make_unique<MusicWheelItemData>(WheelItemDataType_Song,
												 s,
												 gname,
												 SONGMAN->GetSongColor(s),
												 0));
						if (allSongsByGroupFiltered.at(so).count(gname) != 0u) {
							allSongsByGroupFiltered.at(so)[gname].emplace_back(s);
						} else {
							std::vector<Song*> v;
							v.emplace_back(s);
							allSongsByGroupFiltered.at(so)[gname] = v;
						}
					}
				}
			}
		}
		// calculate the pack progress numbers for the sortorder
		if (PREFSMAN->m_bPackProgressInWheel) {
			auto allsongs = allSongsByGroupFiltered.at(so);
			for (auto& groupname_songlist_pair : allsongs) {
				int num_played_songs = 0;
				for (auto& s : groupname_songlist_pair.second) {
					for (auto& chart : s->GetChartsOfCurrentGameMode()) {
						if (SCOREMAN->KeyHasScores(chart->GetChartKey())) {
							num_played_songs++;
							break;
						}
					}
				}
				packProgressByGroup.at(so)[groupname_songlist_pair.first] =
				  num_played_songs;
			}

		}
	}
}

auto
MusicWheel::getWheelItemsData(SortOrder so) -> std::vector<MusicWheelItemData*>&
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
		auto before = std::chrono::steady_clock::now();

		auto& aUnFilteredDatas = m__UnFilteredWheelItemDatas[so];

		// this is about to be invalidated, so save the information
		Song* cursong = nullptr;
		std::string curtxt{};
		WheelItemDataType curtype = WheelItemDataType_Invalid;
		if (!m_CurWheelItemData.empty()) {
			auto cur = GetCurWheelItemData(m_iSelection);
			cursong = cur->m_pSong;
			curtxt = cur->m_sText;
			curtype = cur->m_Type;
		}

		if (m_WheelItemDatasStatus[so] == INVALID) {
			BuildWheelItemDatas(aUnFilteredDatas, so, searching, findme);
		}
		auto ptrSelectedWheelItemData = FilterWheelItemDatas(
		  aUnFilteredDatas, m__WheelItemDatas[so], cursong, curtxt, curtype);
		m_WheelItemDatasStatus[so] = VALID;

		m_CurWheelItemData.clear();
		m_nearestCompatibleWheelItemData = ptrSelectedWheelItemData;

		auto now = std::chrono::steady_clock::now();
		Locator::getLogger()->info(
		  "MusicWheel sorting took: {}ms",
		  std::chrono::duration<float, std::milli>(now - before).count());
	}
}

MusicWheelItemData*
MusicWheel::FilterWheelItemDatas(
  std::vector<std::unique_ptr<MusicWheelItemData>>& aUnFilteredDatas,
  std::vector<MusicWheelItemData*>& aFilteredData,
  const Song* currentSong,
  const std::string& currentText,
  const WheelItemDataType& currentType) const
{
	aFilteredData.clear();

	const unsigned unfilteredSize = aUnFilteredDatas.size();

	MusicWheelItemData* nearestCompatibleWheelItemData = nullptr;

	/* Only add WheelItemDataType_Portal if there's at least one song on the
	 * list. */
	auto bFoundAnySong = false;
	for (unsigned i = 0; i < unfilteredSize; i++) {
		if (aUnFilteredDatas[i]->m_Type == WheelItemDataType_Song) {
			bFoundAnySong = true;
			break;
		}
	}

	std::vector<bool> aiRemove;
	aiRemove.insert(aiRemove.begin(), unfilteredSize, false);

	/* Mark any songs that aren't playable in aiRemove. */

	for (unsigned i = 0; i < unfilteredSize; i++) {
		auto& WID = *aUnFilteredDatas[i];

		/* If we have no songs, remove Random and Portal. */
		if (WID.m_Type == WheelItemDataType_Random ||
			WID.m_Type == WheelItemDataType_Portal) {
			if (!bFoundAnySong) {
				aiRemove[i] = true;
			}
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
		if (aiRemove[i]) {
			continue;
		}
		aFilteredData.emplace_back(aUnFilteredDatas[i].get());
	}

	std::function<void(MusicWheelItemData&)> catchSections;
	if (currentType != WheelItemDataType_Invalid &&
		currentType != WheelItemDataType_Song) {
		catchSections = [&nearestCompatibleWheelItemData,
						 &currentType,
						 &currentText](MusicWheelItemData& WID) {
			if (WID.m_Type == currentType && WID.m_sText == currentText) {
				nearestCompatibleWheelItemData = &WID;
			}
		};
	} else {
		catchSections = [](MusicWheelItemData& WID) {};
	}

	std::function<void(MusicWheelItemData&)> catchSongs;
	if (currentType == WheelItemDataType_Song && currentSong != nullptr) {
		catchSongs = [&nearestCompatibleWheelItemData,
					  &currentSong](MusicWheelItemData& WID) {
			if (WID.m_pSong == currentSong) {
				nearestCompatibleWheelItemData = &WID;
			}
			++WID.m_iSectionCount;
		};
	} else {
		catchSongs = [](MusicWheelItemData& WID) { ++WID.m_iSectionCount; };
	}

	// Update the song count in each section header.
	unsigned filteredSize = aFilteredData.size();
	for (unsigned i = 0; i < filteredSize;) {
		auto& WID = *aFilteredData[i];
		++i;
		if (WID.m_Type != WheelItemDataType_Section) {
			continue;
		}

		// Count songs in this section
		WID.m_iSectionCount = 0;
		for (; i < filteredSize && aFilteredData[i]->m_sText == WID.m_sText;
			 ++i) {
			catchSongs(WID);
		}
	}

	// If we have any section headers with no songs, then we filtered all of the
	// songs in that group, so remove it.  This isn't optimized like the above
	// since this is a rare case.
	for (unsigned i = 0; i < filteredSize; ++i) {
		auto& WID = *aFilteredData[i];
		catchSections(WID);
		if (WID.m_Type != WheelItemDataType_Section) {
			continue;
		}
		if (WID.m_iSectionCount > 0) {
			continue;
		}
		aFilteredData.erase(aFilteredData.begin() + i,
							aFilteredData.begin() + i + 1);
		--i;
		--filteredSize;
	}

	// If we've filtered all items, insert a static dummy.
	if (aFilteredData.empty()) {
		static MusicWheelItemData EmptyDummy(
		  WheelItemDataType_Section, nullptr, EMPTY_STRING, EMPTY_COLOR, 0);
		aFilteredData.emplace_back(&EmptyDummy);
	}

	return nearestCompatibleWheelItemData;
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

			// should reset preferred difficulty here if possible

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
				const float SwitchTimes[] = { 0.5F, 1.3F, 0.8F, 0.4F, 0.2F };
				ASSERT(m_iSwitchesLeftInSpinDown >= 0 &&
					   m_iSwitchesLeftInSpinDown <= 4);
				m_fTimeLeftInState = SwitchTimes[m_iSwitchesLeftInSpinDown];
				m_Moving = 0;

				Locator::getLogger()->trace(
				  "m_iSwitchesLeftInSpinDown id {}, m_fTimeLeftInState is {}",
				  m_iSwitchesLeftInSpinDown,
				  m_fTimeLeftInState);

				if (m_iSwitchesLeftInSpinDown == 0) {
					ChangeMusic(randomf(0, 1) >= 0.5F ? 1 : -1);
				} else {
					ChangeMusic(1);
				}
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

	m_fPositionOffsetFromSelection += static_cast<float>(iDist);

	SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);

	// If we're moving automatically, don't play this; it'll be called in
	// Update.
	if (IsMoving() == 0) {
		m_soundChangeMusic.Play(true);
	}
}

auto
MusicWheel::ChangeSort(SortOrder new_so,
					   bool allowSameSort)
  -> bool // return true if change successful
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
	if (IsSongSort(new_so)) {
		GAMESTATE->m_PreferredSortOrder = new_so;
	}
	GAMESTATE->m_SortOrder.Set(new_so);

	return true;
}

auto
MusicWheel::NextSort() -> bool // return true if change successful
{
	// don't allow NextSort when on the mode menu
	if (GAMESTATE->m_SortOrder == SORT_MODE_MENU) {
		return false;
	}

	std::vector<SortOrder> aSortOrders;
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
		   aSortOrders[cur] != GAMESTATE->m_SortOrder) {
		++cur;
	}

	// move to the next sort with wrapping
	++cur;
	wrap(cur, aSortOrders.size());

	// apply new sort
	const auto soNew = aSortOrders[cur];
	return ChangeSort(soNew);
}

auto
MusicWheel::Select() -> bool // return true if this selection ends the screen
{
	Locator::getLogger()->trace("MusicWheel::Select()");

	switch (m_WheelState) {
		case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
		case STATE_ROULETTE_SLOWING_DOWN:
			return false;
		case STATE_ROULETTE_SPINNING:
			m_WheelState = STATE_ROULETTE_SLOWING_DOWN;
			m_iSwitchesLeftInSpinDown =
			  ROULETTE_SLOW_DOWN_SWITCHES / 2 + 1 +
			  RandomInt(ROULETTE_SLOW_DOWN_SWITCHES / 2);
			m_fTimeLeftInState = 0.1F;
			return false;
		case STATE_RANDOM_SPINNING:
			m_fPositionOffsetFromSelection =
			  std::max(m_fPositionOffsetFromSelection, 0.3F);
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

	if (!WheelBase::Select()) {
		return false;
	}

	switch (m_CurWheelItemData[m_iSelection]->m_Type) {
		case WheelItemDataType_Roulette:
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
				   ->m_pAction->m_sScreen.empty()) {
				return true;
			}
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
	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS) {
		m_viWheelPositions.resize(SONGMAN->GetNumSongGroups());
	}
	
	const WheelItemBaseData* old = nullptr;
	if (!m_CurWheelItemData.empty()) {
		old = GetCurWheelItemData(m_iSelection);
	} else {
		old = m_nearestCompatibleWheelItemData;
	}

	m_CurWheelItemData.clear();
	auto& from = getWheelItemsData(GAMESTATE->m_SortOrder);
	m_CurWheelItemData.reserve(from.size());
	for (auto& i : from) {
		auto& d = *i;

		// Hide songs/courses which are not in the active section
		if ((d.m_Type == WheelItemDataType_Song ||
			 d.m_Type == WheelItemDataType_Course) &&
			!d.m_sText.empty() && d.m_sText != group) {
			continue;
		}

		// In certain situations (e.g. simulating Pump it Up or IIDX),
		// themes may want to hide inactive section headings as well.
		if (HIDE_INACTIVE_SECTIONS && d.m_Type == WheelItemDataType_Section &&
			!group.empty()) {
			// Based on the HideActiveSectionTitle metric, we either
			// hide all section titles, or only those which are not
			// currently open.
			if (HIDE_ACTIVE_SECTION_TITLE || d.m_sText != group) {
				continue;
			}
		}

		m_CurWheelItemData.emplace_back(&d);
	}

	// restore the past group song index
	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS) {
		for (auto idx = 0; idx < (int)m_viWheelPositions.size(); ++idx) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(idx)) {
				m_iSelection = m_viWheelPositions[idx];
			}
		}
	} else {
		// Try to select the item that was selected before changing groups
		m_iSelection = 0;
		
		for (auto i = 0; i < static_cast<int>(m_CurWheelItemData.size()); ++i) {
			if (m_CurWheelItemData[i] == old) {
				m_iSelection = i;
				break;
			}
		}
	}

	RebuildWheelItems();
}

// sm-ssc additions: jump to group
auto
MusicWheel::JumpToNextGroup() -> std::string
{
	// Thanks to Juanelote for this logic:
	if (HIDE_INACTIVE_SECTIONS) {
		// todo: make it work with other sort types
		const unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for (unsigned i = 0; i < iNumGroups; i++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i)) {
				if (i < iNumGroups - 1) {
					return SONGMAN->GetSongGroupByIndex(i + 1);
				}
				// i = 0;
				return SONGMAN->GetSongGroupByIndex(0);
			}
		}
	} else {
		const auto iLastSelection = m_iSelection;
		for (auto i = m_iSelection;
			 i < static_cast<int>(m_CurWheelItemData.size());
			 ++i) {
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section &&
				i != m_iSelection) {
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// it should not get down here, but it might happen... only search up to
		// the previous selection.
		for (auto i = 0; i < iLastSelection; ++i) {
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section &&
				i != m_iSelection) {
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}
	// it shouldn't get here, but just in case...
	return "";
}

auto
MusicWheel::JumpToPrevGroup() -> std::string
{
	if (HIDE_INACTIVE_SECTIONS) {
		const unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for (unsigned i = 0; i < iNumGroups; i++) {
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i)) {
				if (i > 0) {
					return SONGMAN->GetSongGroupByIndex(i - 1);
				}
				// i = iNumGroups - 1;
				return SONGMAN->GetSongGroupByIndex(iNumGroups - 1);
			}
		}
	} else {
		for (auto i = m_iSelection; i > 0; --i) {
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section &&
				i != m_iSelection) {
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// in case it wasn't found above:
		for (auto i = static_cast<int>(m_CurWheelItemData.size() - 1U); i > 0;
			 --i) {
			Locator::getLogger()->trace("JumpToPrevGroup iteration 2 | i = {}",
										i);
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section) {
				m_iSelection = i;
				Locator::getLogger()->trace(
				  "finding it in #2 | i = {} | text = {}",
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

auto
MusicWheel::IsRouletting() const -> bool
{
	return m_WheelState == STATE_ROULETTE_SPINNING ||
		   m_WheelState == STATE_ROULETTE_SLOWING_DOWN ||
		   m_WheelState == STATE_RANDOM_SPINNING;
}

auto
MusicWheel::GetSelectedSong() -> Song*
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
auto
MusicWheel::GetPreferredSelectionForRandomOrPortal() -> Song*
{
	// probe to find a song that has the preferred
	// difficulties of each player
	std::vector<Difficulty> vDifficultiesToRequire;

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

	const auto st = GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType;

	constexpr auto NUM_PROBES = 1000;
	for (auto i = 0; i < NUM_PROBES; i++) {
		auto isValid = true;
		/* Maintaining difficulties is higher priority than maintaining
		 * the current group. */
		if (i == NUM_PROBES / 4) {
			sPreferredGroup = "";
		}
		if (i == NUM_PROBES / 2) {
			vDifficultiesToRequire.clear();
		}

		const auto iSelection = RandomInt(wid.size());
		if (wid[iSelection]->m_Type != WheelItemDataType_Song) {
			continue;
		}

		const Song* pSong = wid[iSelection]->m_pSong;

		if (!sPreferredGroup.empty() &&
			wid[iSelection]->m_sText != sPreferredGroup) {
			continue;
		}

		for (auto& d : vDifficultiesToRequire) {
			if (!pSong->HasStepsTypeAndDifficulty(st, d)) {
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
	static auto ChangeSort(T* p, lua_State* L) -> int
	{
		if (lua_isnil(L, 1)) {
			lua_pushboolean(L, 0);
		} else {
			const auto so = Enum::Check<SortOrder>(L, 1);
			lua_pushboolean(L, static_cast<int>(p->ChangeSort(so)));
		}
		return 1;
	}
	DEFINE_METHOD(GetSelectedSection, GetSelectedSection());
	static auto IsRouletting(T* p, lua_State* L) -> int
	{
		lua_pushboolean(L, static_cast<int>(p->IsRouletting()));
		return 1;
	}
	static auto SelectSong(T* p, lua_State* L) -> int
	{
		if (lua_isnil(L, 1)) {
			lua_pushboolean(L, 0);
		} else {
			auto* pS = Luna<Song>::check(L, 1, true);
			lua_pushboolean(L, static_cast<int>(p->SelectSong(pS)));
		}
		return 1;
	}
	static auto SongSearch(T* p, lua_State* L) -> int
	{
		p->ReloadSongList(true, SArg(1));
		return 1;
	}
	static auto ReloadSongList(T* p, lua_State* /*L*/) -> int
	{
		p->ReloadSongList(false, "");
		return 1;
	}
	static auto Move(T* p, lua_State* L) -> int
	{
		if (lua_isnil(L, 1)) {
			p->Move(0);
		} else {
			p->Move(IArg(1));
		}
		return 1;
	}
	static auto MoveAndCheckType(T* p, lua_State* L) -> int
	{
		const auto n = IArg(1);
		p->Move(n);
		const auto tt = p->GetSelectedType();
		LuaHelpers::Push(L, tt);

		return 1;
	}

	static auto FilterByStepKeys(T* p, lua_State* L) -> int
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_pushvalue(L, 1);
		std::vector<string> newHashList;
		LuaHelpers::ReadArrayFromTable(newHashList, L);
		lua_pop(L, 1);
		p->SetHashList(newHashList);

		std::vector<string> newOutHashList;
		p->SetOutHashList(newOutHashList);

		p->ReloadSongList(false, "");
		return 1;
	}

	static auto FilterByAndAgainstStepKeys(T* p, lua_State* L) -> int
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_pushvalue(L, 1);
		std::vector<string> newHashList;
		LuaHelpers::ReadArrayFromTable(newHashList, L);
		lua_pop(L, 1);
		luaL_checktype(L, 2, LUA_TTABLE);
		lua_pushvalue(L, 2);
		std::vector<string> newOutHashList;
		LuaHelpers::ReadArrayFromTable(newOutHashList, L);
		lua_pop(L, 1);

		p->SetHashList(newHashList);
		p->SetOutHashList(newOutHashList);
		p->ReloadSongList(false, "");

		return 1;
	}

	static auto SetPackListFiltering(T* p, lua_State* L) -> int
	{
		const auto old = FILTERMAN->filteringCommonPacks;
		FILTERMAN->filteringCommonPacks = BArg(1);
		lua_pushboolean(L, static_cast<int>(FILTERMAN->filteringCommonPacks));
		if (old == FILTERMAN->filteringCommonPacks) {
			return 1;
		}
		p->ReloadSongList(false, "");
		return 1;
	}

	static auto GetSongs(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		for (auto i = 0; i < static_cast<int>(p->allSongsFiltered.size());
			 ++i) {
			p->allSongsFiltered[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static auto GetSongsInGroup(T* p, lua_State* L) -> int
	{
		lua_newtable(L);
		const auto* group = SArg(1);

		if (p->allSongsByGroupFiltered.at(GAMESTATE->m_SortOrder).count(group) == 0) {
			return 1;
		}

		for (auto i = 0;
			 i < static_cast<int>(p->allSongsByGroupFiltered.at(GAMESTATE->m_SortOrder)[group].size());
			 ++i) {
			p->allSongsByGroupFiltered.at(GAMESTATE->m_SortOrder)[group][i]->PushSelf(L);
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
		ADD_METHOD(FilterByAndAgainstStepKeys);
		ADD_METHOD(SetPackListFiltering);
		ADD_METHOD(GetSongs);
		ADD_METHOD(GetSongsInGroup);
	}
};

LUA_REGISTER_DERIVED_CLASS(MusicWheel, WheelBase)
// lua end
