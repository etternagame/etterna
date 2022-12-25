#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/AnnouncerManager.h"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "Etterna/Models/Misc/Game.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/NoteSkinManager.h"
#include "Etterna/Models/Misc/PlayerOptions.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "ScreenOptionsMasterPrefs.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Etterna/Singletons/ThemeManager.h"

using namespace StringConversion;

static void
GetPrefsDefaultModifiers(PlayerOptions& po, SongOptions& so)
{
	po.FromString(PREFSMAN->m_sDefaultModifiers);
	so.FromString(PREFSMAN->m_sDefaultModifiers);
}

static void
SetPrefsDefaultModifiers(const PlayerOptions& po, const SongOptions& so)
{
	std::vector<std::string> as;
#define remove_empty_back()                                                    \
	if (as.back() == "") {                                                     \
		as.pop_back();                                                         \
	}
	as.push_back(po.GetString());
	remove_empty_back();
	as.push_back(so.GetString());
	remove_empty_back();
#undef remove_empty_back

	PREFSMAN->m_sDefaultModifiers.Set(join(", ", as));
}

/* Ugly: the input values may be a different type than the mapping.  For
 * example, the mapping may be an enum, and value an int.  This is because we
 * don't have FromString/ToString for every enum type.  Assume that the distance
 * between T and U can be represented as a float. */
template<class T, class U>
int
FindClosestEntry(T value, const U* mapping, unsigned cnt)
{
	int iBestIndex = 0;
	float best_dist = 0;
	bool have_best = false;

	for (unsigned i = 0; i < cnt; ++i) {
		const U val = mapping[i];
		float dist = value < val ? static_cast<float>(val - value)
								 : static_cast<float>(value - val);
		if (have_best && best_dist < dist)
			continue;

		have_best = true;
		best_dist = dist;

		iBestIndex = i;
	}

	if (have_best)
		return iBestIndex;
	else
		return 0;
}

template<class T>
static void
MoveMap(int& sel, T& opt, bool ToSel, const T* mapping, unsigned cnt)
{
	if (ToSel) {
		sel = FindClosestEntry(opt, mapping, cnt);
	} else {
		// sel -> opt
		opt = mapping[sel];
	}
}

template<class T>
static void
MoveMap(int& sel, IPreference& opt, bool ToSel, const T* mapping, unsigned cnt)
{
	if (ToSel) {
		std::string sOpt = opt.ToString();
		// This should really be T, but we can't FromString an enum.
		float val;
		FromString(sOpt, val);
		sel = FindClosestEntry(val, mapping, cnt);
	} else {
		// sel -> opt
		std::string sOpt = ToString(mapping[sel]);
		opt.FromString(sOpt);
	}
}

template<class T>
static void
MoveMap(int& sel,
		const ConfOption* pConfOption,
		bool ToSel,
		const T* mapping,
		unsigned cnt)
{
	ASSERT(pConfOption != NULL);
	IPreference* pPref =
	  IPreference::GetPreferenceByName(pConfOption->m_sPrefName);
	ASSERT_M(pPref != nullptr, pConfOption->m_sPrefName);

	MoveMap(sel, *pPref, ToSel, mapping, cnt);
}

template<class T>
static void
MovePref(int& iSel, bool bToSel, const ConfOption* pConfOption)
{
	IPreference* pPref =
	  IPreference::GetPreferenceByName(pConfOption->m_sPrefName);
	ASSERT_M(pPref != nullptr, pConfOption->m_sPrefName);

	if (bToSel) {
		// TODO: why not get the int directly from pPref?
		// Why are we writing it to a string and then back?
		T t;
		FromString(pPref->ToString(), t);
		iSel = static_cast<int>(t);
	} else {
		pPref->FromString(ToString(static_cast<T>(iSel)));
	}
}

template<>
void
MovePref<bool>(int& iSel, bool bToSel, const ConfOption* pConfOption)
{
	IPreference* pPref =
	  IPreference::GetPreferenceByName(pConfOption->m_sPrefName);
	ASSERT_M(pPref != nullptr, pConfOption->m_sPrefName);

	if (bToSel) {
		// TODO: why not get the int directly from pPref?
		// Why are we writing it to a string and then back?
		bool b;
		FromString(pPref->ToString(), b);
		iSel = b ? 1 : 0;
	} else {
		// If we don't make a specific instantiation of MovePref<bool>, there is
		// a compile warning here because of static_cast<bool>( iSel ) where
		// iSel is an int. What is the best way to remove that compile warning?
		pPref->FromString(ToString<bool>(iSel ? true : false));
	}
}

static void
MoveNop(int& iSel, bool bToSel, const ConfOption* pConfOption)
{
	if (bToSel)
		iSel = 0;
}

// TODO: Write GenerateValueList() function that can use ints and floats. -aj

static void
GameChoices(std::vector<std::string>& out)
{
	std::vector<const Game*> aGames;
	GAMEMAN->GetEnabledGames(aGames);
	for (auto& g : aGames) {
		std::string sGameName = g->m_szName;
		out.push_back(sGameName);
	}
}

static void
GameSel(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	std::vector<std::string> choices;
	pConfOption->MakeOptionsList(choices);

	if (ToSel) {
		const std::string sCurGameName = PREFSMAN->GetCurrentGame();

		sel = 0;
		for (unsigned i = 0; i < choices.size(); ++i)
			if (!strcasecmp(choices[i].c_str(), sCurGameName.c_str()))
				sel = i;
	} else {
		std::vector<const Game*> aGames;
		GAMEMAN->GetEnabledGames(aGames);
		PREFSMAN->SetCurrentGame(aGames[sel]->m_szName);
	}
}

static void
LanguageChoices(std::vector<std::string>& out)
{
	std::vector<std::string> vs;
	THEME->GetLanguages(vs);
	SortStringArray(vs, true);

	for (auto& s : vs) {
		const LanguageInfo* pLI = GetLanguageInfo(s);
		if (pLI)
			out.push_back(
			  THEME->GetString("NativeLanguageNames", pLI->szEnglishName));
		else
			out.push_back(s);
	}
}

static void
Language(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	std::vector<std::string> vs;
	THEME->GetLanguages(vs);
	SortStringArray(vs, true);

	if (ToSel) {
		sel = -1;
		for (unsigned i = 0; sel == -1 && i < vs.size(); ++i)
			if (!strcasecmp(vs[i].c_str(), THEME->GetCurLanguage().c_str()))
				sel = i;

		// If the current language doesn't exist, we'll show BASE_LANGUAGE, so
		// select that.
		for (unsigned i = 0; sel == -1 && i < vs.size(); ++i)
			if (!strcasecmp(vs[i].c_str(), SpecialFiles::BASE_LANGUAGE.c_str()))
				sel = i;

		if (sel == -1) {
			Locator::getLogger()->warn("Couldn't find language \"{}\" or fallback \"{}\"; using \"{}\"",
			  THEME->GetCurLanguage().c_str(), SpecialFiles::BASE_LANGUAGE.c_str(),
			  vs[0].c_str());
			sel = 0;
		}
	} else {
		const std::string& sNewLanguage = vs[sel];

		PREFSMAN->m_sLanguage.Set(sNewLanguage);
		if (THEME->GetCurLanguage() != sNewLanguage)
			THEME->SwitchThemeAndLanguage(THEME->GetCurThemeName(),
										  PREFSMAN->m_sLanguage,
										  PREFSMAN->m_bPseudoLocalize);
	}
}

static void
ThemeChoices(std::vector<std::string>& out)
{
	THEME->GetSelectableThemeNames(out);
	for (auto& s : out) {
		s = THEME->GetThemeDisplayName(s);
	}
}

static DisplaySpecs display_specs;
static void
cache_display_specs()
{
	if (display_specs.empty()) {
		DISPLAY->GetDisplaySpecs(display_specs);
	}
}

static void
DisplayResolutionChoices(std::vector<std::string>& out)
{
	cache_display_specs();
	for (auto& iter : display_specs) {
		std::string s = ssprintf(
		  "%dx%d", iter.currentMode()->width, iter.currentMode()->height);
		out.push_back(s);
	}
}

static void
RequestedTheme(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	std::vector<std::string> choices;
	pConfOption->MakeOptionsList(choices);

	std::vector<std::string> vsThemeNames;
	THEME->GetSelectableThemeNames(vsThemeNames);

	if (ToSel) {
		sel = 0;
		for (unsigned i = 1; i < vsThemeNames.size(); i++)
			if (!strcasecmp(vsThemeNames[i].c_str(),
							PREFSMAN->m_sTheme.Get().c_str()))
				sel = i;
	} else {
		const std::string sNewTheme = vsThemeNames[sel];
		PREFSMAN->m_sTheme.Set(
		  sNewTheme); // OPT_APPLY_THEME will load the theme
	}
}

static LocalizedString OFF("ScreenOptionsMasterPrefs", "Off");
static void
AnnouncerChoices(std::vector<std::string>& out)
{
	ANNOUNCER->GetAnnouncerNames(out);
	out.insert(out.begin(), OFF);
}

static void
Announcer(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	std::vector<std::string> choices;
	pConfOption->MakeOptionsList(choices);

	if (ToSel) {
		sel = 0;
		for (unsigned i = 1; i < choices.size(); i++)
			if (!strcasecmp(choices[i].c_str(),
							ANNOUNCER->GetCurAnnouncerName().c_str()))
				sel = i;
	} else {
		const std::string sNewAnnouncer = sel ? choices[sel] : std::string("");
		ANNOUNCER->SwitchAnnouncer(sNewAnnouncer);
		PREFSMAN->m_sAnnouncer.Set(sNewAnnouncer);
	}
}

static void
DefaultNoteSkinChoices(std::vector<std::string>& out)
{
	NOTESKIN->GetNoteSkinNames(out);
}

static void
DefaultNoteSkin(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	std::vector<std::string> choices;
	pConfOption->MakeOptionsList(choices);

	if (ToSel) {
		PlayerOptions po;
		po.FromString(PREFSMAN->m_sDefaultModifiers);
		sel = 0;
		for (unsigned i = 0; i < choices.size(); i++)
			if (!strcasecmp(choices[i].c_str(), po.m_sNoteSkin.c_str()))
				sel = i;
	} else {
		PlayerOptions po;
		SongOptions so;
		GetPrefsDefaultModifiers(po, so);
		po.m_sNoteSkin = choices[sel];
		SetPrefsDefaultModifiers(po, so);
	}
}

static void
DefaultFailType(int& sel, bool to_sel, const ConfOption* conf_option)
{
	if (to_sel) {

		PlayerOptions po;
		po.FromString(PREFSMAN->m_sDefaultModifiers);
		sel = po.m_FailType;
	} else {
		PlayerOptions po;
		SongOptions so;
		GetPrefsDefaultModifiers(po, so);
		po.m_FailType = static_cast<FailType>(sel);
		SetPrefsDefaultModifiers(po, so);
	}
}

// Background options
static void
BGBrightness(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	// TODO: I hate the way the list of numbers is duplicated here and where the
	// option is created. Try to find a way to only use the same list once.
	// Do that for all of these float and int lists.
	const float mapping[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
							  0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
BGBrightnessNoZero(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
							  0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
BGBrightnessOrStatic(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { 0.5f, 0.25f, 0.5f, 0.75f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));

	IPreference* pSongBackgroundsPref =
	  IPreference::GetPreferenceByName("SongBackgrounds");
	if (ToSel && pSongBackgroundsPref->ToString() == "0")
		sel = 0;
	if (!ToSel)
		pSongBackgroundsPref->FromString(sel ? "1" : "0");
}

static void
NumBackgrounds(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { 1, 5, 10, 15, 20 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

// Input options
static void
MusicWheelSwitchSpeed(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { 5, 10, 15, 25 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
InputDebounceTime(int& sel, bool to_sel, ConfOption const* conf_option)
{
	float const mapping[] = { 0.0f,	 0.01f, 0.02f, 0.03f, 0.04f, 0.05f,
							  0.06f, 0.07f, 0.08f, 0.09f, 0.1f };
	MoveMap(sel, conf_option, to_sel, mapping, ARRAYLEN(mapping));
}

// Machine options
/** @brief Timing Window scale */
static void
TimingWindowScale(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	// we are no longer supporting j1-3, they will be set to 1.f like j4
	// to avoid issues with expected array sizes that i do not want to debug
	auto& ts = GAMESTATE->timingscales;
	float mapping[9]; // hardcodered because ide yell at me
	for (size_t i = 0; i < ts.size(); ++i)
		mapping[i] = ts[i];
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

/** @brief Life Difficulty scale */
static void
LifeDifficulty(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	// original StepMania 5 values (implemented 2008/03/12)
	// const float mapping[] = { 2.0f,1.50f,1.00f,0.66f,0.33f };
	// 3.9 modified so that L6 is L4 (in SM5 some time after)
	// const float mapping[] = { 1.20f,1.00f,0.80f,0.60f,0.40f,0.33f,0.25f };

	// StepMania 3.9 and 4.0 values:
	const float mapping[] = { 1.60f, 1.40f, 1.20f, 1.00f, 0.80f, 0.60f, 0.40f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

#include "Etterna/Singletons/LuaManager.h"

static int
GetTimingDifficulty()
{
	int iTimingDifficulty = 0;
	TimingWindowScale(
	  iTimingDifficulty, true, ConfOption::Find("TimingWindowScale"));
	iTimingDifficulty++; // TimingDifficulty returns an index
	return iTimingDifficulty;
}
LuaFunction(GetTimingDifficulty, GetTimingDifficulty());
static int
SetTimingDifficulty(float judge)
{
	auto opt = ConfOption::Find("TimingWindowScale");
	IPreference* pPref = IPreference::GetPreferenceByName(opt->m_sPrefName);
	pPref->FromString(ToString(judge));
	return 1;
}
LuaFunction(SetTimingDifficulty,
			SetTimingDifficulty(FArg(1))) static int GetLifeDifficulty()
{
	int iLifeDifficulty = 0;
	LifeDifficulty(iLifeDifficulty, true, ConfOption::Find("LifeDifficulty"));
	iLifeDifficulty++; // LifeDifficulty returns an index
	return iLifeDifficulty;
}
LuaFunction(GetLifeDifficulty, GetLifeDifficulty());

// Graphic options
struct res_t
{
	int w{ 0 }, h{ 0 };
	res_t() = default;
	res_t(int w_, int h_)
	  : w(w_)
	  , h(h_)
	{
	}

	res_t& operator-=(res_t const& rhs)
	{
		w -= rhs.w;
		h -= rhs.h;
		return *this;
	}

	// Ugly: allow convert to a float for FindClosestEntry.
	explicit operator float() const { return w * 5000.0f + h; }
};

inline bool
operator<(res_t const& lhs, res_t const& rhs)
{
	if (lhs.w != rhs.w) {
		return lhs.w < rhs.w;
	}
	return lhs.h < rhs.h;
}
inline bool
operator>(res_t const& lhs, res_t const& rhs)
{
	return operator<(rhs, lhs);
}
inline bool
operator<=(res_t const& lhs, res_t const& rhs)
{
	return !operator<(rhs, lhs);
}
inline bool
operator>=(res_t const& lhs, res_t const& rhs)
{
	return !operator<(lhs, rhs);
}

inline res_t
operator-(res_t lhs, res_t const& rhs)
{
	lhs -= rhs;
	return lhs;
}

static void
DisplayResolutionM(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	static std::vector<res_t> res_choices;

	if (res_choices.empty()) {
		cache_display_specs();
		for (auto& iter : display_specs) {
			if (iter.currentMode() != nullptr) {
				res_choices.push_back(
				  res_t(iter.currentMode()->width, iter.currentMode()->height));
			}
		}
	}

	res_t sel_res(PREFSMAN->m_iDisplayWidth, PREFSMAN->m_iDisplayHeight);
	MoveMap(sel, sel_res, ToSel, &res_choices[0], res_choices.size());
	if (!ToSel) {
		PREFSMAN->m_iDisplayWidth.Set(sel_res.w);
		PREFSMAN->m_iDisplayHeight.Set(sel_res.h);
	}
}

static void
DisplayColorDepth(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { 16, 32 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
MaxTextureResolution(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { 256, 512, 1024, 2048 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
TextureColorDepth(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { 16, 32 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
MovieColorDepth(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { 16, 32 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
RefreshRate(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const int mapping[] = { static_cast<int>(REFRESH_DEFAULT),
							60,
							70,
							72,
							75,
							80,
							85,
							90,
							100,
							120,
							150 };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
DisplayAspectRatio(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { 3 / 4.f,	  1,		4 / 3.0f, 5 / 4.0f,
							  16 / 10.0f, 16 / 9.f, 8 / 3.f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

/* Simpler DisplayAspectRatio setting, which only offers "on" and "off".
 * "On" can be 16:9 or 16:10. */
static void
WideScreen16_10(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { 4 / 3.0f, 16 / 10.0f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
WideScreen16_9(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { 4 / 3.0f, 16 / 9.0f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

// BackgroundCache code isn't live yet -aj
/*
static void BackgroundCache( int &sel, bool ToSel, const ConfOption *pConfOption
)
{
	const BackgroundCache mapping[] = { BackgroundCacheMode_Off,
BackgroundCacheMode_LowResPreload, BackgroundCacheMode_LowResLoadOnDemand,
BackgroundCacheMode_Full }; MoveMap( sel, pConfOption, ToSel, mapping,
ARRAYLEN(mapping) );
}
*/

// Sound options
static void
SoundVolume(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
							  0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
VisualDelaySeconds(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	const float mapping[] = { -0.125f, -0.1f, -0.075f, -0.05f, -0.025f, 0.0f,
							  0.025f,  0.05f, 0.075f,  0.1f,   0.125f };
	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static void
GlobalOffsetSeconds(int& sel, bool ToSel, const ConfOption* pConfOption)
{
	float mapping[41];
	for (int i = 0; i < 41; ++i)
		mapping[i] = SCALE(i, 0.0f, 40.0f, -0.1f, +0.1f);

	MoveMap(sel, pConfOption, ToSel, mapping, ARRAYLEN(mapping));
}

static std::vector<ConfOption> g_ConfOptions;
static void
InitializeConfOptions()
{
	if (!g_ConfOptions.empty())
		return;

	// Clear the display_specs so that we don't get problems from
	// caching it.  If the DisplayResolution option row is on the screen, it'll
	// recache the list. -Kyz
	display_specs.clear();

	// There are a couple ways of getting the current preference column or
	// turning a new choice in the interface into a new preference. The easiest
	// is when the interface choices are an exact mapping to the values the
	// preference can be. In that case, the easiest thing to do is use
	// MovePref<bool or enum>. The next easiest case is when there is a
	// hardcoded mapping that is not 1-1, such as CoinModeNoHome. In that case,
	// you need to remap the result of MovePref<enum> to the correct mapping.
	// Harder yet is when there is a float or a dynamic set of options, such as
	// Language or Theme. Those require individual attention.
#define ADD(x) g_ConfOptions.push_back(x)
	// Select game
	ADD(ConfOption("Game", GameSel, GameChoices));
	g_ConfOptions.back().m_iEffects = OPT_CHANGE_GAME;

	// Appearance options
	ADD(ConfOption("Language", Language, LanguageChoices));
	ADD(ConfOption("Theme", RequestedTheme, ThemeChoices));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_THEME;

	ADD(ConfOption("Announcer", Announcer, AnnouncerChoices));
	ADD(ConfOption("DefaultNoteSkin", DefaultNoteSkin, DefaultNoteSkinChoices));
	ADD(ConfOption("NoGlow", MovePref<bool>, "On", "Off"));
	ADD(ConfOption("FullTapExplosions", MovePref<bool>, "Short", "Full"));
	ADD(ConfOption("ReplaysUseScoreMods", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("EnablePitchRates", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("LiftsOnOsuHolds", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("AllowStartToGiveUp", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("MusicWheelUsesSections",
				   MovePref<MusicWheelUsesSections>,
				   "Never",
				   "Always",
				   "Title Only"));
	ADD(ConfOption(
	  "ShowNativeLanguage", MovePref<bool>, "Romanization", "Native Language"));
	ADD(ConfOption("ShowLyrics", MovePref<bool>, "Hide", "Show"));

	// Misc options
	ADD(ConfOption("FastLoad", MovePref<bool>, "Off", "On"));

	// Background options
	ADD(ConfOption("RandomBackgroundMode",
				   MovePref<RandomBackgroundMode>,
				   "Off",
				   "Animations"));
	ADD(ConfOption("BGBrightness",
				   BGBrightness,
				   "|0%",
				   "|10%",
				   "|20%",
				   "|30%",
				   "|40%",
				   "|50%",
				   "|60%",
				   "|70%",
				   "|80%",
				   "|90%",
				   "|100%"));
	ADD(ConfOption("BGBrightnessNoZero",
				   BGBrightnessNoZero,
				   "|10%",
				   "|20%",
				   "|30%",
				   "|40%",
				   "|50%",
				   "|60%",
				   "|70%",
				   "|80%",
				   "|90%",
				   "|100%"));
	g_ConfOptions.back().m_sPrefName = "BGBrightness";
	ADD(ConfOption("BGBrightnessOrStatic",
				   BGBrightnessOrStatic,
				   "Disabled",
				   "25% Bright",
				   "50% Bright",
				   "75% Bright"));
	g_ConfOptions.back().m_sPrefName = "BGBrightness";
	ADD(ConfOption("StretchBackgrounds",
				   MovePref<bool>,
				   "Off",
				   "On")); // Deprecated, unused by default/_fallback. -Kyz
	ADD(ConfOption("BackgroundFitMode",
				   MovePref<BackgroundFitMode>,
				   "CoverDistort",
				   "CoverPreserve",
				   "FitInside",
				   "FitInsideAvoidLetter",
				   "FitInsideAvoidPillar"));
	ADD(ConfOption("ShowBackgrounds", MovePref<bool>, "Off", "On"));

	ADD(ConfOption("ShowDanger", MovePref<bool>, "Hide", "Show"));
	ADD(ConfOption(
	  "NumBackgrounds", NumBackgrounds, "|1", "|5", "|10", "|15", "|20"));

	// Input options
	ADD(ConfOption(
	  "AutoMapOnJoyChange", MovePref<bool>, "Off", "On (recommended)"));
	ADD(ConfOption("OnlyDedicatedMenuButtons",
				   MovePref<bool>,
				   "Use Gameplay Buttons",
				   "Only Dedicated Buttons"));
	ADD(ConfOption(
	  "AutoPlay", MovePref<PlayerController>, "Off", "On", "CPU-Controlled"));
	ADD(ConfOption("DelayedBack", MovePref<bool>, "Instant", "Hold"));
	ADD(
	  ConfOption("AllowHoldForOptions", MovePref<bool>, "Double Tap", "Hold"));
	ADD(ConfOption("ArcadeOptionsNavigation",
				   MovePref<bool>,
				   "StepMania Style",
				   "Arcade Style"));
	ADD(ConfOption(
	  "ThreeKeyNavigation", MovePref<bool>, "Five Key Menu", "Three Key Menu"));
	ADD(ConfOption("MusicWheelSwitchSpeed",
				   MusicWheelSwitchSpeed,
				   "Slow",
				   "Normal",
				   "Fast",
				   "Really Fast"));
	ADD(ConfOption("InputDebounceTime",
				   InputDebounceTime,
				   "0ms",
				   "10ms",
				   "20ms",
				   "30ms",
				   "40ms",
				   "50ms",
				   "60ms",
				   "70ms",
				   "80ms",
				   "90ms",
				   "100ms"));
	ADD(ConfOption("AxisFix", MovePref<bool>, "Off", "On"));

	ADD(ConfOption("Center1Player", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("EasterEggs", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("SortBySSRNormPercent", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("UseMidGrades", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("PackProgressInWheel", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("EnableMinidumpUpload", MovePref<bool>, "Off", "On"));

	// Machine options
	ADD(ConfOption("TimingWindowScale",
				   TimingWindowScale,
				   "|1",
				   "|2",
				   "|3",
				   "|4",
				   "|5",
				   "|6",
				   "|7",
				   "|8",
				   "Justice"));
	ADD(ConfOption("LifeDifficulty",
				   LifeDifficulty,
				   "|1",
				   "|2",
				   "|3",
				   "|4",
				   "|5",
				   "|6",
				   "|7"));
	g_ConfOptions.back().m_sPrefName = "LifeDifficultyScale";
	ADD(ConfOption(
	  "DefaultFailType", DefaultFailType, "Immediate", "ImmediateContinue"));
	ADD(ConfOption("ShowSongOptions", MovePref<Maybe>, "Ask", "Hide", "Show"));
	ADD(ConfOption("MinTNSToHideNotes",
				   MovePref<TapNoteScore>,
				   "TNS_None",
				   "TNS_HitMine",
				   "TNS_AvoidMine",
				   "TNS_CheckpointMiss",
				   "TNS_Miss",
				   "TNS_W5",
				   "TNS_W4",
				   "TNS_W3",
				   "TNS_W2",
				   "TNS_W1",
				   "TNS_CheckpointHit"));

	// Graphic options
	ADD(ConfOption("Windowed", MovePref<bool>, "Full Screen", "Windowed"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption(
	  "DisplayResolution", DisplayResolutionM, DisplayResolutionChoices));
	g_ConfOptions.back().m_iEffects =
	  OPT_APPLY_GRAPHICS | OPT_APPLY_ASPECT_RATIO;
	ADD(ConfOption("DisplayAspectRatio",
				   DisplayAspectRatio,
				   "|3:4",
				   "|1:1",
				   "|4:3",
				   "|5:4",
				   "|16:10",
				   "|16:9",
				   "|8:3"));
	g_ConfOptions.back().m_iEffects =
	  OPT_APPLY_GRAPHICS | OPT_APPLY_ASPECT_RATIO; // need OPT_APPLY_GRAPHICS
												   // because if windowed the
												   // width may change to make
												   // square pixels
	ADD(ConfOption("DisplayColorDepth", DisplayColorDepth, "16bit", "32bit"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("HighResolutionTextures",
				   MovePref<HighResolutionTextures>,
				   "Auto",
				   "Force Off",
				   "Force On"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("MaxTextureResolution",
				   MaxTextureResolution,
				   "|256",
				   "|512",
				   "|1024",
				   "|2048"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("TextureColorDepth", TextureColorDepth, "16bit", "32bit"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("MovieColorDepth", MovieColorDepth, "16bit", "32bit"));
	ADD(ConfOption("DelayedTextureDeletion", MovePref<bool>, "Off", "On"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("SmoothLines", MovePref<bool>, "Off", "On"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("RefreshRate",
				   RefreshRate,
				   "Default",
				   "|60",
				   "|70",
				   "|72",
				   "|75",
				   "|80",
				   "|85",
				   "|90",
				   "|100",
				   "|120",
				   "|150"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("Vsync", MovePref<bool>, "No", "Yes"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_GRAPHICS;
	ADD(ConfOption("FastNoteRendering", MovePref<bool>, "Off", "On"));
	ADD(ConfOption("ShowStats", MovePref<bool>, "Off", "On"));
	// Sound options
	ADD(ConfOption("AttractSoundFrequency",
				   MovePref<AttractSoundFrequency>,
				   "Never",
				   "Always",
				   "2 Times",
				   "3 Times",
				   "4 Times",
				   "5 Times"));
	ADD(ConfOption("SoundVolume",
				   SoundVolume,
				   "Silent",
				   "|10%",
				   "|20%",
				   "|30%",
				   "|40%",
				   "|50%",
				   "|60%",
				   "|70%",
				   "|80%",
				   "|90%",
				   "|100%"));
	g_ConfOptions.back().m_iEffects = OPT_APPLY_SOUND;
	ADD(ConfOption("VisualDelaySeconds",
				   VisualDelaySeconds,
				   "|-5",
				   "|-4",
				   "|-3",
				   "|-2",
				   "|-1",
				   "|0",
				   "|+1",
				   "|+2",
				   "|+3",
				   "|+4",
				   "|+5"));
	{
		ConfOption c("GlobalOffsetSeconds", GlobalOffsetSeconds);
		for (int i = -100; i <= +100; i += 5)
			c.AddOption(ssprintf("%+i ms", i));
		ADD(c);
	}
	ADD(ConfOption("EnableMineHitSound", MovePref<bool>, "No", "Yes"));
	ADD(ConfOption("Invalid", MoveNop, "|Invalid option"));
}

// Get a mask of effects to apply if the given option changes.
int
ConfOption::GetEffects() const
{
	return m_iEffects | OPT_SAVE_PREFERENCES;
}

ConfOption*
ConfOption::Find(const std::string& name)
{
	InitializeConfOptions();
	for (auto& g_ConfOption : g_ConfOptions) {
		ConfOption* opt = &g_ConfOption;
		std::string match(opt->name);
		if (CompareNoCase(match, name))
			continue;
		return opt;
	}

	return nullptr;
}

void
ConfOption::UpdateAvailableOptions()
{
	if (MakeOptionsListCB != nullptr) {
		names.clear();
		MakeOptionsListCB(names);
	}
}

void
ConfOption::MakeOptionsList(std::vector<std::string>& out) const
{
	out = names;
}

static const char* OptEffectNames[] = { "SavePreferences", "ApplyGraphics",
										"ApplyTheme",	   "ChangeGame",
										"ApplySound",	   "ApplySong",
										"ApplyAspectRatio" };
XToString(OptEffect);
StringToX(OptEffect);
LuaXType(OptEffect);
