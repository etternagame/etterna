#include "Etterna/Globals/global.h"
#include "Etterna/Models/Fonts/FontCharAliases.h"
#include "Etterna/FileTypes/IniFile.h"
#include "RageUtil/File/RageFileManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "arch/Dialog/Dialog.h"
#if !defined(SMPACKAGE)
#include "Etterna/Actor/Base/ActorUtil.h"
#include "ProfileManager.h"
#include "ScreenManager.h"
#endif
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Globals/GameLoop.h" // For ChangeTheme
#include "Etterna/Models/Misc/LocalizedString.h"
#include "LuaManager.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Globals/SpecialFiles.h"
#include "Etterna/Models/Misc/SubscriptionManager.h"
#include "Etterna/FileTypes/XmlFileUtil.h"
#include "Core/Services/Locator.hpp"
#include "Core/Platform/Platform.hpp"

#include "PrefsManager.h"

#include <deque>
#include <algorithm>
#include <set>
#include <map>

using std::deque;
using std::map;
using std::set;

ThemeManager* THEME =
  nullptr; // global object accessible from anywhere in the program

static const std::string THEME_INFO_INI = "ThemeInfo.ini";

static const char* ElementCategoryNames[] = { "BGAnimations",
											  "Fonts",
											  "Graphics",
											  "Sounds",
											  "Other" };
XToString(ElementCategory);
StringToX(ElementCategory);

struct Theme
{
	std::string sThemeName;
};
// When looking for a metric or an element, search these from head to tail.
static deque<Theme> g_vThemes;
class LoadedThemeData
{
  public:
	IniFile iniMetrics;
	IniFile iniStrings;
	void ClearAll()
	{
		iniMetrics.Clear();
		iniStrings.Clear();
	}
};
LoadedThemeData* g_pLoadedThemeData = nullptr;

// For self-registering metrics
static SubscriptionManager<IThemeMetric> g_Subscribers;

class LocalizedStringImplThemeMetric
  : public ILocalizedStringImpl
  , public ThemeMetric<std::string>
{
  public:
	static ILocalizedStringImpl* Create()
	{
		return new LocalizedStringImplThemeMetric;
	}

	void Load(const std::string& sGroup, const std::string& sName) override
	{
		ThemeMetric<std::string>::Load(sGroup, sName);
	}

	void Read() override
	{
		if (!m_sName.empty() && THEME && THEME->IsThemeLoaded()) {
			THEME->GetString(m_sGroup, m_sName, m_currentValue);
			m_Value.SetFromNil();
		}
	}

	const std::string& GetLocalized() const override
	{
		if (IsLoaded()) {
			return GetValue();
		}
		std::string const& curLanguage =
		  (THEME && THEME->IsThemeLoaded() ? THEME->GetCurLanguage()
										   : "current");
		Locator::getLogger()->warn("Missing translation for {} in the {} language.",
				  m_sName.c_str(),
				  curLanguage.c_str());
		return m_sName;
	}
};

void
ThemeManager::Subscribe(IThemeMetric* p)
{
	g_Subscribers.Subscribe(p);

	// It's ThemeManager's responsibility to make sure all of its subscribers
	// are updated with current data.  If a metric is created after
	// a theme is loaded, ThemeManager should update it right away (not just
	// when the theme changes).
	if (THEME && !THEME->GetCurThemeName().empty())
		p->Read();
}

void
ThemeManager::Unsubscribe(IThemeMetric* p)
{
	g_Subscribers.Unsubscribe(p);
}

// We spend a lot of time doing redundant theme path lookups. Cache results.
static map<std::string, ThemeManager::PathInfo>
  g_ThemePathCache[NUM_ElementCategory];
void
ThemeManager::ClearThemePathCache()
{
	for (auto& i : g_ThemePathCache)
		i.clear();
}

static void
FileNameToMetricsGroupAndElement(const std::string& sFileName,
								 std::string& sMetricsGroupOut,
								 std::string& sElementOut)
{
	// split into class name and file name
	std::string::size_type iIndexOfFirstSpace = sFileName.find(' ');
	if (iIndexOfFirstSpace == string::npos) // no space
	{
		sMetricsGroupOut = "";
		sElementOut = sFileName;
	} else {
		sMetricsGroupOut = sFileName.substr(0, iIndexOfFirstSpace);
		sElementOut =
		  tail(sFileName, sFileName.size() - iIndexOfFirstSpace - 1);
	}
}

static std::string
MetricsGroupAndElementToFileName(const std::string& sMetricsGroup,
								 const std::string& sElement)
{
	if (sMetricsGroup.empty())
		return sElement;
	else
		return sMetricsGroup + " " + sElement;
}

ThemeManager::ThemeManager()
{
	THEME = this; // so that we can Register THEME on construction

	// Register with Lua.
	{
		Lua* L = LUA->Get();
		lua_pushstring(L, "THEME");
		this->PushSelf(L);
		lua_settable(L, LUA_GLOBALSINDEX);
		LUA->Release(L);
	}

	// We don't have any theme loaded until SwitchThemeAndLanguage is called.
	m_sCurThemeName = "";
	m_bPseudoLocalize = false;

	std::vector<std::string> arrayThemeNames;
	GetThemeNames(arrayThemeNames);
}

ThemeManager::~ThemeManager()
{
	g_vThemes.clear();
	SAFE_DELETE(g_pLoadedThemeData);

	// Unregister with Lua.
	LUA->UnsetGlobal("THEME");
}

void
ThemeManager::GetThemeNames(std::vector<std::string>& AddTo)
{
	FILEMAN->GetDirListing(SpecialFiles::THEMES_DIR + "*", AddTo, ONLY_DIR);
}

void
ThemeManager::GetSelectableThemeNames(std::vector<std::string>& AddTo)
{
	GetThemeNames(AddTo);
	for (int i = AddTo.size() - 1; i >= 0; i--) {
		if (!IsThemeNameValid(AddTo[i])) {
			AddTo.erase(AddTo.begin() + i);
		}
	}
}

int
ThemeManager::GetNumSelectableThemes()
{
	std::vector<std::string> vs;
	GetSelectableThemeNames(vs);
	return vs.size();
}

bool
ThemeManager::DoesThemeExist(const std::string& sThemeName)
{
	std::vector<std::string> asThemeNames;
	GetThemeNames(asThemeNames);
	for (unsigned i = 0; i < asThemeNames.size(); i++) {
		if (!CompareNoCase(sThemeName, asThemeNames[i]))
			return true;
	}
	return false;
}

bool
ThemeManager::IsThemeSelectable(std::string const& name)
{
	return IsThemeNameValid(name) && DoesThemeExist(name);
}

bool
ThemeManager::IsThemeNameValid(std::string const& name)
{
	return name.substr(0, 1) != "_";
}

std::string
ThemeManager::GetThemeDisplayName(const std::string& sThemeName)
{
	std::string sDir = GetThemeDirFromName(sThemeName);
	IniFile ini;
	ini.ReadFile(sDir + THEME_INFO_INI);

	std::string s;
	if (ini.GetValue("ThemeInfo", "DisplayName", s))
		return s;

	return sThemeName;
}

std::string
ThemeManager::GetThemeAuthor(const std::string& sThemeName)
{
	std::string sDir = GetThemeDirFromName(sThemeName);
	IniFile ini;
	ini.ReadFile(sDir + THEME_INFO_INI);

	std::string s;
	if (ini.GetValue("ThemeInfo", "Author", s))
		return s;

	return "[unknown author]";
}

void
ThemeManager::GetLanguages(std::vector<std::string>& AddTo)
{
	AddTo.clear();

	for (auto& g_vTheme : g_vThemes)
		GetLanguagesForTheme(g_vTheme.sThemeName, AddTo);

	// remove dupes
	std::sort(AddTo.begin(), AddTo.end());
	std::vector<std::string>::iterator it =
	  std::unique(AddTo.begin(), AddTo.end(), EqualsNoCase);
	AddTo.erase(it, AddTo.end());
}

bool
ThemeManager::DoesLanguageExist(const std::string& sLanguage)
{
	std::vector<std::string> asLanguages;
	GetLanguages(asLanguages);

	for (unsigned i = 0; i < asLanguages.size(); i++)
		if (CompareNoCase(sLanguage, asLanguages[i]) == 0)
			return true;
	return false;
}

void
ThemeManager::LoadThemeMetrics(const std::string& sThemeName_,
							   const std::string& sLanguage_)
{
	if (g_pLoadedThemeData == nullptr)
		g_pLoadedThemeData = new LoadedThemeData;

	// Don't delete and recreate LoadedThemeData.  There are references
	// iniMetrics and iniStrings on the stack, so Clear them instead.
	g_pLoadedThemeData->ClearAll();
	g_vThemes.clear();

	std::string sThemeName(sThemeName_);
	const std::string& sLanguage(sLanguage_);

	m_sCurThemeName = sThemeName;
	m_sCurLanguage = sLanguage;

	bool bLoadedBase = false;
	for (;;) {
		ASSERT_M(g_vThemes.size() < 20,
				 "Circular theme fallback references detected.");

		g_vThemes.push_back(Theme());
		Theme& t = g_vThemes.back();
		t.sThemeName = sThemeName;

		IniFile iniMetrics;
		IniFile iniStrings;
		iniMetrics.ReadFile(GetMetricsIniPath(sThemeName));
		// Load optional language inis (probably mounted by a package) first so
		// that they can be overridden by the current theme.
		{
			std::vector<std::string> vs;
			GetOptionalLanguageIniPaths(vs, sThemeName, sLanguage);
			for (auto& s : vs)
				iniStrings.ReadFile(s);
		}
		iniStrings.ReadFile(
		  GetLanguageIniPath(sThemeName, SpecialFiles::BASE_LANGUAGE));
		if (CompareNoCase(sLanguage, SpecialFiles::BASE_LANGUAGE)) {
			iniStrings.ReadFile(GetLanguageIniPath(sThemeName, sLanguage));
		}
		bool bIsBaseTheme =
		  !CompareNoCase(sThemeName, SpecialFiles::BASE_THEME_NAME);
		iniMetrics.GetValue("Global", "IsBaseTheme", bIsBaseTheme);
		if (bIsBaseTheme) {
			bLoadedBase = true;
		}
		/* Read the fallback theme. If no fallback theme is specified, and we
		 * haven't already loaded it, fall back on
		 * SpecialFiles::BASE_THEME_NAME. That way, default theme fallbacks can
		 * be disabled with "FallbackTheme=". */
		std::string sFallback;
		if (!iniMetrics.GetValue("Global", "FallbackTheme", sFallback)) {
			if (CompareNoCase(sThemeName, SpecialFiles::BASE_THEME_NAME) &&
				!bLoadedBase) {
				sFallback = SpecialFiles::BASE_THEME_NAME;
			}
		}
		/* We actually want to load themes bottom-to-top, loading fallback
		 * themes first, so derived themes overwrite metrics in fallback themes.
		 * But, we need to load the derived theme first, to find out the name of
		 * the fallback theme.  Avoid having to load IniFile twice, merging the
		 * fallback theme into the derived theme that we've already loaded. */
		XmlFileUtil::MergeIniUnder(&iniMetrics,
								   &g_pLoadedThemeData->iniMetrics);
		XmlFileUtil::MergeIniUnder(&iniStrings,
								   &g_pLoadedThemeData->iniStrings);

		if (sFallback.empty()) {
			break;
		}
		sThemeName = sFallback;
	}

	// Overlay metrics from the command line.
	std::string sMetric;
	for (int i = 0; GetCommandlineArgument("metric", &sMetric, i); ++i) {
		/* sMetric must be "foo::bar=baz". "foo" and "bar" never contain "=", so
		 * in "foo::bar=1+1=2", "baz" is always "1+1=2". Neither foo nor bar may
		 * be empty, but baz may be. */
		Regex re("^([^=]+)::([^=]+)=(.*)$");
		std::vector<std::string> sBits;
		if (!re.Compare(sMetric, sBits))
			RageException::Throw("Invalid argument \"--metric=%s\".",
								 sMetric.c_str());

		g_pLoadedThemeData->iniMetrics.SetValue(sBits[0], sBits[1], sBits[2]);
	}

	Locator::getLogger()->info("Theme: {}", m_sCurThemeName.c_str());
	Locator::getLogger()->info("Language: {}", m_sCurLanguage.c_str());
}

std::string
ThemeManager::GetDefaultLanguage()
{
	return Core::Platform::getLanguage();
}

void
ThemeManager::SwitchThemeAndLanguage(const std::string& sThemeName_,
									 const std::string& sLanguage_,
									 bool bPseudoLocalize,
									 bool bForceThemeReload)
{
	std::string sThemeName = sThemeName_;
	std::string sLanguage = sLanguage_;
	// todo: if the theme isn't selectable, find the next theme that is,
	// and change to that instead of asserting/crashing since
	// SpecialFiles::BASE_THEME_NAME is _fallback now. -aj
	if (!IsThemeSelectable(sThemeName)) {
		std::string to_try = PREFSMAN->m_sTheme.GetDefault();
		Locator::getLogger()->warn("Selected theme '{}' not found.  "
				  "Trying Theme preference default value '{}'.",
				  sThemeName.c_str(),
				  to_try.c_str());
		sThemeName = to_try;
		// sm-ssc's SpecialFiles::BASE_THEME_NAME is _fallback, which you can't
		// select. This requires a preference, which allows it to be adapted for
		// other purposes (e.g. PARASTAR).
		if (!IsThemeSelectable(sThemeName)) {
			to_try = PREFSMAN->m_sDefaultTheme.Get();
			Locator::getLogger()->warn("Theme preference defaults to '{}', which cannot be used."
					  "  Trying DefaultTheme preference '{}'.",
					  sThemeName.c_str(),
					  to_try.c_str());
			sThemeName = to_try;
			if (!IsThemeSelectable(sThemeName)) {
				std::vector<std::string> theme_names;
				GetSelectableThemeNames(theme_names);
				ASSERT_M(!theme_names.empty(),
						 "No themes found, unable to start stepmania.");
				to_try = theme_names[0];
				Locator::getLogger()->warn("DefaultTheme preference is '{}', which cannot be found. Using '{}'.",
				  sThemeName.c_str(),
				  to_try.c_str());
				sThemeName = to_try;
				PREFSMAN->m_sDefaultTheme.Set(to_try);
			}
		}
		PREFSMAN->m_sTheme.Set(sThemeName);
	}

	/* We haven't actually loaded the theme yet, so we can't check whether
	 * sLanguage exists. Just check for empty. */
	if (sLanguage.empty())
		sLanguage = GetDefaultLanguage();

	Locator::getLogger()->info("ThemeManager::SwitchThemeAndLanguage: \"{}\", \"{}\"",
				sThemeName.c_str(),
				sLanguage.c_str());

	bool bNothingChanging = sThemeName == m_sCurThemeName &&
							sLanguage == m_sCurLanguage &&
							m_bPseudoLocalize == bPseudoLocalize;
	if (bNothingChanging && !bForceThemeReload)
		return;

	m_bPseudoLocalize = bPseudoLocalize;

	// Load theme metrics. If only the language is changing, this is all
	// we need to reload.
	bool bThemeChanging = (sThemeName != m_sCurThemeName);
	LoadThemeMetrics(sThemeName, sLanguage);

	// Clear the theme path cache. This caches language-specific graphic paths,
	// so do this even if only the language is changing.
	ClearThemePathCache();
	if (bThemeChanging || bForceThemeReload) {
#if !defined(SMPACKAGE)
		// reload common sounds
		if (SCREENMAN != nullptr)
			SCREENMAN->ThemeChanged();

#endif

		/* Lua globals can use metrics which are cached, and vice versa.  Update
		 * Lua globals first; it's Lua's job to explicitly update cached metrics
		 * that it uses. */
		UpdateLuaGlobals();
	}

	// Use theme metrics for localization.
	LocalizedString::RegisterLocalizer(LocalizedStringImplThemeMetric::Create);

	ReloadSubscribers();
}

void
ThemeManager::ReloadSubscribers()
{
	// reload subscribers
	if (g_Subscribers.m_pSubscribers) {
		for (auto& p : *g_Subscribers.m_pSubscribers)
			p->Read();
	}
}

void
ThemeManager::ClearSubscribers()
{
	if (g_Subscribers.m_pSubscribers) {
		for (auto& p : *g_Subscribers.m_pSubscribers)
			p->Clear();
	}
}

void
ThemeManager::RunLuaScripts(const std::string& sMask, bool bUseThemeDir)
{
	/* Run all script files with the given mask in Lua for all themes.  Start
	 * from the deepest fallback theme and work outwards. */

	/* TODO: verify whether this final check is necessary. */
	const std::string sCurThemeName = m_sCurThemeName;
	m_sRealCurThemeName = m_sCurThemeName;
	std::deque<Theme>::const_iterator iter = g_vThemes.end();
	do {
		--iter;

		/* HACK: pretend to be the theme these are under by setting
		 * m_sCurThemeName to the name of the theme whose scripts are currently
		 * running; if those scripts call GetThemeName(), it'll return the theme
		 * the script is in. */

		m_sCurThemeName = iter->sThemeName;
		const std::string& sScriptDir =
		  bUseThemeDir ? GetThemeDirFromName(m_sCurThemeName) : "/";

		std::vector<std::string> asElementPaths;
		// get files from directories
		std::vector<std::string> asElementChildPaths;
		std::vector<std::string> arrayScriptDirs;
		FILEMAN->GetDirListing(sScriptDir + "Scripts/*", arrayScriptDirs, ONLY_DIR);
		SortStringArray(arrayScriptDirs);
		for (auto& s : arrayScriptDirs) // foreach dir in /Scripts/
		{
			// Find all Lua files in this directory, add them to asElementPaths
			const std::string& sScriptDirName = s;
			FILEMAN->GetDirListing(sScriptDir + "Scripts/" + sScriptDirName + "/" +
							sMask,
						  asElementChildPaths,
						  ANY_TYPE,
						  true);
			for (auto& sPath : asElementChildPaths) {
				// push these Lua files into the main element paths
				asElementPaths.push_back(sPath);
			}
		}

		// get regular Lua files
		FILEMAN->GetDirListing(
		  sScriptDir + "Scripts/" + sMask, asElementPaths, ANY_TYPE, true);

		// load Lua files
		for (auto& sPath : asElementPaths) {
			Locator::getLogger()->info("Loading \"{}\" ...", sPath.c_str());
			LuaHelpers::RunScriptFile(sPath);
		}
	} while (iter != g_vThemes.begin());

	/* TODO: verify whether this final check is necessary. */
	if (sCurThemeName != m_sCurThemeName) {
		Locator::getLogger()->warn("ThemeManager: theme name was not restored after RunLuaScripts");
		m_sCurThemeName = sCurThemeName;
	}
}

void
ThemeManager::UpdateLuaGlobals()
{
#if !defined(SMPACKAGE)
	// explicitly refresh cached metrics that we use.
	ScreenDimensions::ReloadScreenDimensions();

	// run global scripts
	RunLuaScripts("*.lua");
	// run theme scripts
	RunLuaScripts("*.lua", true);
#endif
}

std::string
ThemeManager::GetThemeDirFromName(const std::string& sThemeName)
{
	return SpecialFiles::THEMES_DIR + sThemeName + "/";
}

struct CompareLanguageTag
{
	std::string m_sLanguageString;
	CompareLanguageTag(const std::string& sLang)
	{
		m_sLanguageString = std::string("(lang ") + sLang + ")";
		Locator::getLogger()->trace("try \"{}\"", sLang.c_str());
		m_sLanguageString = make_lower(m_sLanguageString);
	}

	bool operator()(const std::string& sFile) const
	{
		std::string sLower(sFile);
		sLower = make_lower(sLower);
		size_t iPos = sLower.find(m_sLanguageString);
		return iPos != std::string::npos;
	}
};

/* If there's more than one result, check for language tags.  For example,
 *
 * ScreenCompany graphic (lang English).png
 * ScreenCompany graphic (lang French).png
 *
 * We still want to warn for ambiguous results.  Use std::unique to filter
 * files with the current language tag to the top, so choosing "ignore" from
 * the multiple-match dialog will cause it to default to the first entry, so
 * it'll still use a preferred language match if there were any. */
void
ThemeManager::FilterFileLanguages(std::vector<std::string>& asPaths)
{
	if (asPaths.size() <= 1)
		return;
	std::vector<std::string>::iterator it = std::partition(
	  asPaths.begin(), asPaths.end(), CompareLanguageTag(m_sCurLanguage));

	int iDist = distance(asPaths.begin(), it);
	if (iDist == 0) {
		// We didn't find any for the current language.  Try BASE_LANGUAGE.
		it = std::partition(asPaths.begin(),
							asPaths.end(),
							CompareLanguageTag(SpecialFiles::BASE_LANGUAGE));
		iDist = distance(asPaths.begin(), it);
	}

	if (iDist == 1)
		asPaths.erase(it, asPaths.end());
}

bool
ThemeManager::GetPathInfoToRaw(PathInfo& out,
							   const std::string& sThemeName_,
							   ElementCategory category,
							   const std::string& sMetricsGroup_,
							   const std::string& sElement_)
{
	/* Ugly: the parameters to this function may be a reference into g_vThemes,
	 * or something else that might suddenly go away when we call ReloadMetrics,
	 * so make a copy. */
	const std::string sThemeName = sThemeName_;
	const std::string sMetricsGroup = sMetricsGroup_;
	const std::string sElement = sElement_;

	const std::string sThemeDir = GetThemeDirFromName(sThemeName);
	const std::string& sCategory = ElementCategoryToString(category);

	std::vector<std::string> asElementPaths;

	// If sFileName already has an extension, we're looking for a specific file
	bool bLookingForSpecificFile = sElement.find_last_of('.') != sElement.npos;

	if (bLookingForSpecificFile) {
		FILEMAN->GetDirListing(
		  sThemeDir + sCategory + "/" +
			MetricsGroupAndElementToFileName(sMetricsGroup, sElement),
		  asElementPaths,
		  false,
		  true);
	} else // look for all files starting with sFileName that have types we can
		   // use
	{
		std::vector<std::string> asPaths;
		FILEMAN->GetDirListing(
		  sThemeDir + sCategory + "/" +
			MetricsGroupAndElementToFileName(sMetricsGroup, sElement) + "*",
		  asPaths,
		  false,
		  true);

		for (auto& asPath : asPaths) {
			// BGAnimations, Fonts, Graphics, Sounds, Other
			const std::string ext = GetExtension(asPath);
			bool matches = category == EC_OTHER || ext == "redir";
			if (!matches) {
				FileType ft = ActorUtil::GetFileType(asPath);
				switch (ft) {
					case FT_Bitmap:
					case FT_Sprite:
					case FT_Movie:
					case FT_Xml:
					case FT_Model:
					case FT_Lua:
						matches = category == EC_BGANIMATIONS ||
								  category == EC_GRAPHICS ||
								  category == EC_SOUNDS;
						break;
					case FT_Ini:
						matches = category == EC_FONTS;
						break;
					case FT_Directory: {
						std::string sXMLPath = asPath + "/default.xml";
						if (DoesFileExist(sXMLPath)) {
							asElementPaths.push_back(sXMLPath);
							break;
						}
						std::string sLuaPath = asPath + "/default.lua";
						if (DoesFileExist(sLuaPath)) {
							asElementPaths.push_back(sLuaPath);
							break;
						}
					}
						matches = category == EC_BGANIMATIONS ||
								  category == EC_GRAPHICS;
						break;
					case FT_Sound:
						matches = category == EC_SOUNDS;
						break;
					default:
						matches = false;
						break;
				}
			}
			if (matches) {
				asElementPaths.push_back(asPath);
			}
		}
	}

	if (asElementPaths.empty())
		return false; // This isn't fatal.

	FilterFileLanguages(asElementPaths);

	if (asElementPaths.size() > 1) {
		g_ThemePathCache[category].clear();

		std::string message = ssprintf(
		  "ThemeManager:  There is more than one theme element that matches "
		  "'%s/%s/%s'.  Please remove all but one of these matches: ",
		  sThemeName.c_str(),
		  sCategory.c_str(),
		  MetricsGroupAndElementToFileName(sMetricsGroup, sElement).c_str());
		message += asElementPaths[1];
		for (size_t i = 1; i < asElementPaths.size(); ++i) {
			message += ", " + asElementPaths[i];
		}

		switch (LuaHelpers::ReportScriptError(message, "", true)) {
			case Dialog::abort:
				RageException::Throw("%s", message.c_str());
				break;
			case Dialog::retry:
				ReloadMetrics();
				return GetPathInfoToRaw(
				  out, sThemeName_, category, sMetricsGroup_, sElement_);
			case Dialog::ignore:
			default:
				break;
		}
	}

	std::string sPath = asElementPaths[0];
	bool bIsARedirect = CompareNoCase(GetExtension(sPath), "redir") == 0;

	if (!bIsARedirect) {
		out.sResolvedPath = sPath;
		out.sMatchingMetricsGroup = sMetricsGroup;
		out.sMatchingElement = sElement;
		return true;
	}

	std::string sNewFileName;
	GetFileContents(sPath, sNewFileName, true);

	std::string sNewClassName, sNewFile;
	FileNameToMetricsGroupAndElement(sNewFileName, sNewClassName, sNewFile);

	/* Important: We need to do a full search.  For example, BG redirs in
	 * the default theme point to "_shared background", and themes override
	 * just "_shared background"; the redirs in the default theme should end
	 * up resolving to the overridden background. */
	/* Use GetPathToOptional because we don't want report that there's an
	 * element missing. Instead we want to report that the redirect is invalid.
	 */
	if (GetPathInfo(out, category, sNewClassName, sNewFile, true))
		return true;

	std::string sMessage =
	  ssprintf("ThemeManager:  The redirect '%s' points to "
			   "the file '%s', which does not exist. "
			   "Verify that this redirect is correct.",
			   sPath.c_str(),
			   sNewFileName.c_str());

	switch (LuaHelpers::ReportScriptError(sMessage, "", true)) {
		case Dialog::retry:
			ReloadMetrics();
			return GetPathInfoToRaw(
			  out, sThemeName_, category, sMetricsGroup_, sElement_);
		case Dialog::ignore:
			GetPathInfo(out, category, "", "_missing");
			return true;
		default:
			RageException::Throw("%s", sMessage.c_str());
	}
}

bool
ThemeManager::GetPathInfoToAndFallback(PathInfo& out,
									   ElementCategory category,
									   const std::string& sMetricsGroup_,
									   const std::string& sElement)
{
	std::string sMetricsGroup(sMetricsGroup_);

	int n = 100;
	while (n--) {
		for (auto& iter : g_vThemes) {
			// search with requested name
			if (GetPathInfoToRaw(
				  out, iter.sThemeName, category, sMetricsGroup, sElement))
				return true;
		}

		if (sMetricsGroup.empty())
			return false;

		// search fallback name (if any)
		sMetricsGroup = GetMetricsGroupFallback(sMetricsGroup);
		if (sMetricsGroup.empty())
			return false;
	}

	LuaHelpers::ReportScriptErrorFmt(
	  "Infinite recursion looking up theme element \"%s\"",
	  MetricsGroupAndElementToFileName(sMetricsGroup, sElement).c_str());
	return false;
}

bool
ThemeManager::GetPathInfo(PathInfo& out,
						  ElementCategory category,
						  const std::string& sMetricsGroup_,
						  const std::string& sElement_,
						  bool bOptional)
{
	/* Ugly: the parameters to this function may be a reference into g_vThemes,
	 * or something else that might suddenly go away when we call ReloadMetrics.
	 */
	const std::string sMetricsGroup = sMetricsGroup_;
	const std::string sElement = sElement_;

	std::string sFileName =
	  MetricsGroupAndElementToFileName(sMetricsGroup, sElement);

	map<std::string, PathInfo>& Cache = g_ThemePathCache[category];
	{
		map<std::string, PathInfo>::const_iterator i;

		i = Cache.find(sFileName);
		if (i != Cache.end()) {
			out = i->second;
			return true;
		}
	}

try_element_again:

	// search the current theme
	if (GetPathInfoToAndFallback(
		  out, category, sMetricsGroup, sElement)) // we found something
	{
		Cache[sFileName] = out;
		return true;
	}

	if (bOptional) {
		Cache[sFileName] = PathInfo(); // clear cache entry
		return false;
	}

	const std::string& sCategory = ElementCategoryToString(category);

	// We can't fall back on _missing in Other: the file types are unknown.
	std::string sMessage =
	  "The theme element \"" + sCategory + "/" + sFileName + "\" is missing.";
	Dialog::Result res;
	if (category != EC_OTHER)
		res = Dialog::AbortRetryIgnore(sMessage, "MissingThemeElement");
	else
		res = Dialog::AbortRetry(sMessage, "MissingThemeElement");
	switch (res) {
		case Dialog::retry:
			ReloadMetrics();
			goto try_element_again;
		case Dialog::ignore: {
			std::string element = sCategory + '/' + sFileName;
			std::string error =
			  "could not be found in \"" +
			  GetThemeDirFromName(m_sCurThemeName) + "\" or \"" +
			  GetThemeDirFromName(SpecialFiles::BASE_THEME_NAME) + "\".";
			Locator::getLogger()->warn("Theme element {} {}", element.c_str(), error.c_str());
			LuaHelpers::ScriptErrorMessage("'" + element + "' " + error);
		}

			// Err?
			if (sFileName == "_missing")
				RageException::Throw(
				  "\"_missing\" isn't present in \"%s%s\".",
				  GetThemeDirFromName(SpecialFiles::BASE_THEME_NAME).c_str(),
				  sCategory.c_str());

			GetPathInfo(out, category, "", "_missing");
			Cache[sFileName] = out;
			return true;
		case Dialog::abort:
            Locator::getLogger()->warn("Theme element {}/{} could not be found in \"{}\" or \"{}\"",
			  sCategory, sFileName, GetThemeDirFromName(m_sCurThemeName).c_str(),
			  GetThemeDirFromName(SpecialFiles::BASE_THEME_NAME).c_str());
			RageException::Throw(
			  "Theme element \"%s/%s\" could not be found in \"%s\" or \"%s\".",
			  sCategory.c_str(),
			  sFileName.c_str(),
			  GetThemeDirFromName(m_sCurThemeName).c_str(),
			  GetThemeDirFromName(SpecialFiles::BASE_THEME_NAME).c_str());
			DEFAULT_FAIL(res);
	}
	FAIL_M(""); // Silence gcc 4.
}

std::string
ThemeManager::GetPath(ElementCategory category,
					  const std::string& sMetricsGroup,
					  const std::string& sElement,
					  bool bOptional)
{
	PathInfo pi;
	if (!GetPathInfo(pi, category, sMetricsGroup, sElement, bOptional))
		return "";
	if (!bOptional && pi.sResolvedPath.empty()) {
		LuaHelpers::ReportScriptErrorFmt(
		  "Theme element not found and not "
		  "optional: Category: %s  Metrics group: %s  Element name: %s.",
		  ElementCategoryToString(category).c_str(),
		  sMetricsGroup.c_str(),
		  sElement.c_str());
	}
	return pi.sResolvedPath;
}

std::string
ThemeManager::GetMetricsIniPath(const std::string& sThemeName)
{
	return GetThemeDirFromName(sThemeName) + SpecialFiles::METRICS_FILE;
}

bool
ThemeManager::HasMetric(const std::string& sMetricsGroup,
						const std::string& sValueName)
{
	std::string sThrowAway;
	if (sMetricsGroup.empty() || sValueName.empty()) {
		return false;
	}
	return GetMetricRawRecursive(
	  g_pLoadedThemeData->iniMetrics, sMetricsGroup, sValueName, sThrowAway);
}

bool
ThemeManager::HasString(const std::string& sMetricsGroup,
						const std::string& sValueName)
{
	std::string sThrowAway;
	if (sMetricsGroup.empty() || sValueName.empty()) {
		return false;
	}
	return GetMetricRawRecursive(
	  g_pLoadedThemeData->iniStrings, sMetricsGroup, sValueName, sThrowAway);
}

// the strings that were here before were moved to StepMania.cpp;
// sorry for the inconvienence/sloppy coding. -aj
void
ThemeManager::ReloadMetrics()
{
	FILEMAN->FlushDirCache(GetCurThemeDir());

	// Reloading Lua scripts can cause crashes; don't do this. -aj
	// UpdateLuaGlobals();

	// force a reload of the metrics cache
	LoadThemeMetrics(m_sCurThemeName, m_sCurLanguage);
	ReloadSubscribers();

	ClearThemePathCache();
}

std::string
ThemeManager::GetMetricsGroupFallback(const std::string& sMetricsGroup)
{
	ASSERT(g_pLoadedThemeData != NULL);

	// always look in iniMetrics for "Fallback"
	std::string sFallback;
	if (!GetMetricRawRecursive(
		  g_pLoadedThemeData->iniMetrics, sMetricsGroup, "Fallback", sFallback))
		return std::string();

	Lua* L = LUA->Get();
	LuaHelpers::RunExpression(L, sFallback);
	std::string sRet;
	LuaHelpers::Pop(L, sRet);
	LUA->Release(L);

	return sRet;
}

bool
ThemeManager::GetMetricRawRecursive(const IniFile& ini,
									const std::string& sMetricsGroup_,
									const std::string& sValueName,
									std::string& sOut)
{
	ASSERT(!sValueName.empty());
	std::string sMetricsGroup(sMetricsGroup_);

	int n = 100;
	while (n--) {
		if (ini.GetValue(sMetricsGroup, sValueName, sOut))
			return true;

		if (!sValueName.compare("Fallback"))
			return false;

		sMetricsGroup = GetMetricsGroupFallback(sMetricsGroup);
		if (sMetricsGroup.empty())
			return false;
	}

	LuaHelpers::ReportScriptErrorFmt(
	  "Infinite recursion looking up theme metric \"%s::%s\".",
	  sMetricsGroup.c_str(),
	  sValueName.c_str());
	return false;
}

std::string
ThemeManager::GetMetricRaw(const IniFile& ini,
						   const std::string& sMetricsGroup_,
						   const std::string& sValueName_)
{
	/* Ugly: the parameters to this function may be a reference into g_vThemes,
	 * or something else that might suddenly go away when we call ReloadMetrics.
	 */
	const std::string sMetricsGroup = sMetricsGroup_;
	const std::string sValueName = sValueName_;

	for (;;) {
		std::string ret;
		if (ThemeManager::GetMetricRawRecursive(
			  ini, sMetricsGroup, sValueName, ret)) {
			return ret;
		}
		std::string sCurMetricPath = GetMetricsIniPath(m_sCurThemeName);
		std::string sDefaultMetricPath =
		  GetMetricsIniPath(SpecialFiles::BASE_THEME_NAME);

		std::string sType;
		if (&ini == &g_pLoadedThemeData->iniStrings)
			sType = "String";
		else if (&ini == &g_pLoadedThemeData->iniMetrics)
			sType = "Metric";
		else
			FAIL_M("");

		std::string sMessage = ssprintf("%s \"%s::%s\" is missing.",
										sType.c_str(),
										sMetricsGroup.c_str(),
										sValueName.c_str());

		switch (LuaHelpers::ReportScriptError(sMessage, "", true)) {
			case Dialog::abort: {
				RageException::Throw(
				  "%s \"%s::%s\" could not be found in \"%s\"' or \"%s\".",
				  sType.c_str(),
				  sMetricsGroup.c_str(),
				  sValueName.c_str(),
				  sCurMetricPath.c_str(),
				  sDefaultMetricPath.c_str());
			}
			case Dialog::retry:
				ReloadMetrics();
				continue;
			case Dialog::ignore:
				Locator::getLogger()->warn("{} {}::{} could not be found in \"{}\" or \"{}\".",
			            sType, sMetricsGroup, sValueName, sCurMetricPath.c_str(),
						sDefaultMetricPath.c_str());
				return std::string();
			default:
				FAIL_M("Unexpected answer to Abort/Retry/Ignore dialog");
		}
	}
}

template<typename T>
void
GetAndConvertMetric(const std::string& sMetricsGroup,
					const std::string& sValueName,
					T& out)
{
	Lua* L = LUA->Get();

	THEME->PushMetric(L, sMetricsGroup, sValueName);
	LuaHelpers::FromStack(L, out, -1);
	lua_pop(L, 1);

	LUA->Release(L);
}

/* Get a string metric. */
std::string
ThemeManager::GetMetric(const std::string& sMetricsGroup,
						const std::string& sValueName)
{
	std::string sRet;
	GetAndConvertMetric(sMetricsGroup, sValueName, sRet);
	return sRet;
}

int
ThemeManager::GetMetricI(const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	int iRet = 0;
	GetAndConvertMetric(sMetricsGroup, sValueName, iRet);
	return iRet;
}

float
ThemeManager::GetMetricF(const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	float fRet = 0;
	GetAndConvertMetric(sMetricsGroup, sValueName, fRet);
	return fRet;
}

bool
ThemeManager::GetMetricB(const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	bool bRet = 0;
	GetAndConvertMetric(sMetricsGroup, sValueName, bRet);
	return bRet;
}

RageColor
ThemeManager::GetMetricC(const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	RageColor ret;
	GetAndConvertMetric(sMetricsGroup, sValueName, ret);
	return ret;
}

LuaReference
ThemeManager::GetMetricR(const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	LuaReference ref;
	GetMetric(sMetricsGroup, sValueName, ref);
	return ref;
}

void
ThemeManager::PushMetric(Lua* L,
						 const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	if (sMetricsGroup.empty() || sValueName.empty()) {
		LuaHelpers::ReportScriptError("PushMetric:  Attempted to fetch metric "
									  "with empty group name or empty value "
									  "name.");
		lua_pushnil(L);
		return;
	}
	std::string sValue =
	  GetMetricRaw(g_pLoadedThemeData->iniMetrics, sMetricsGroup, sValueName);

	std::string sName =
	  ssprintf("%s::%s", sMetricsGroup.c_str(), sValueName.c_str());
	if (EndsWith(sValueName, "Command")) {
		LuaHelpers::ParseCommandList(L, sValue, sName, false);
	} else {
		// Remove unary +, eg. "+50"; Lua doesn't support that.
		if (!sValue.empty() && sValue[0] == '+')
			sValue.erase(0, 1);

		LuaHelpers::RunExpression(L, sValue, sName);
	}
}

void
ThemeManager::GetMetric(const std::string& sMetricsGroup,
						const std::string& sValueName,
						LuaReference& valueOut)
{
	Lua* L = LUA->Get();
	PushMetric(L, sMetricsGroup, sValueName);
	valueOut.SetFromStack(L);
	LUA->Release(L);
}

#if !defined(SMPACKAGE)
apActorCommands
ThemeManager::GetMetricA(const std::string& sMetricsGroup,
						 const std::string& sValueName)
{
	LuaReference* pRef = new LuaReference;
	GetMetric(sMetricsGroup, sValueName, *pRef);
	return apActorCommands(pRef);
}
#endif

void
ThemeManager::EvaluateString(std::string& sText)
{
	FontCharAliases::ReplaceMarkers(sText);
}

std::string
ThemeManager::GetNextTheme()
{
	std::vector<std::string> as;
	GetThemeNames(as);
	unsigned i;
	for (i = 0; i < as.size(); i++)
		if (CompareNoCase(as[i], m_sCurThemeName) == 0)
			break;
	int iNewIndex = (i + 1) % as.size();
	return as[iNewIndex];
}

std::string
ThemeManager::GetNextSelectableTheme()
{
	std::vector<std::string> as;
	GetSelectableThemeNames(as);
	unsigned i;
	for (i = 0; i < as.size(); i++) {
		if (CompareNoCase(as[i], m_sCurThemeName) == 0)
			break;
	}

	int iNewIndex = (i + 1) % as.size();
	return as[iNewIndex];
}

void
ThemeManager::GetLanguagesForTheme(const std::string& sThemeName,
								   std::vector<std::string>& asLanguagesOut)
{
	std::string sLanguageDir =
	  GetThemeDirFromName(sThemeName) + SpecialFiles::LANGUAGES_SUBDIR;
	std::vector<std::string> as;
	FILEMAN->GetDirListing(sLanguageDir + "*.ini", as, ONLY_FILE);

	for (auto& s : as) {
		// ignore metrics.ini
		if (CompareNoCase(s, SpecialFiles::METRICS_FILE) == 0)
			continue;

		// Ignore filenames with a space.  These are optional language inis that
		// probably came from a mounted package.
		if (s.find(' ') != std::string::npos)
			continue;

		// strip ".ini"
		std::string s2 = s.substr(0, s.size() - 4);

		asLanguagesOut.push_back(s2);
	}
}

std::string
ThemeManager::GetLanguageIniPath(const std::string& sThemeName,
								 const std::string& sLanguage)
{
	return GetThemeDirFromName(sThemeName) + SpecialFiles::LANGUAGES_SUBDIR +
		   sLanguage + ".ini";
}

void
ThemeManager::GetOptionalLanguageIniPaths(std::vector<std::string>& vsPathsOut,
										  const std::string& sThemeName,
										  const std::string& sLanguage)
{
	// optional ini names look like: "en PackageName.ini"
	FILEMAN->GetDirListing(GetThemeDirFromName(sThemeName) +
							 SpecialFiles::LANGUAGES_SUBDIR + sLanguage +
							 " *.ini",
						   vsPathsOut,
						   ONLY_FILE,
						   true);
}

void
ThemeManager::GetOptionNames(std::vector<std::string>& AddTo)
{
	const XNode* cur = g_pLoadedThemeData->iniStrings.GetChild("OptionNames");
	if (cur) {
		FOREACH_CONST_Attr(cur, p) AddTo.push_back(p->first);
	}
}

static std::string
PseudoLocalize(std::string s)
{
	s_replace(s, "a", "\xc3\xa0\xc3\xa1"); // àá
	s_replace(s, "A", "\xc3\x80\xc3\x80"); // ÀÀ
	s_replace(s, "e", "\xc3\xa9\xc3\xa9"); // éé
	s_replace(s, "E", "\xc3\x89\xc3\x89"); // ÉÉ
	s_replace(s, "i", "\xc3\xad\xc3\xad"); // íí
	s_replace(s, "I", "\xc3\x8d\xc3\x8d"); // ÍÍ
	s_replace(s, "o", "\xc3\xb3\xc3\xb3"); // óó
	s_replace(s, "O", "\xc3\x93\xc3\x93"); // ÓÓ
	s_replace(s, "u", "\xc3\xbc\xc3\xbc"); // üü
	s_replace(s, "U", "\xc3\x9c\xc3\x9c"); // ÜÜ
	s_replace(s, "n", "\xc3\xb1");		   // ñ
	s_replace(s, "N", "\xc3\x91");		   // Ñ
	s_replace(s, "c", "\xc3\xa7");		   // ç
	s_replace(s, "C", "\xc3\x87");		   // Ç
	// transformations that help expose punctuation assumptions
	// s.Replace( ":", " :" );	// this messes up "::" help text tip separator
	// markers
	s_replace(s, "?", " ?");
	s_replace(s, "!", " !");

	return s;
}

std::string
ThemeManager::GetString(const std::string& sMetricsGroup,
						const std::string& sValueName_)
{
	std::string sValueName = sValueName_;
	if (sMetricsGroup.empty() || sValueName.empty()) {
		LuaHelpers::ReportScriptError("PushMetric:  Attempted to fetch metric "
									  "with empty group name or empty value "
									  "name.");
		return "";
	}

	// TODO: Handle escaping = with \=
	DEBUG_ASSERT(sValueName.find('=') == sValueName.npos);

	// TODO: Move this escaping into IniFile?
	s_replace(sValueName, "\r\n", "\\n");
	s_replace(sValueName, "\n", "\\n");

	ASSERT(g_pLoadedThemeData != NULL);
	std::string s =
	  GetMetricRaw(g_pLoadedThemeData->iniStrings, sMetricsGroup, sValueName);
	FontCharAliases::ReplaceMarkers(s);

	// Don't EvalulateString.  Strings are raw and shouldn't allow Lua.
	// EvaluateString( s );

	s_replace(s, "\\n", "\n");

	if (m_bPseudoLocalize) {
		// pseudolocalize ignoring replace markers.  e.g.: "%{steps} steps:
		// %{author}"
		std::string sTranslated;

		for (; true;) {
			std::string::size_type pos = s.find("%{");
			if (pos == s.npos) {
				sTranslated += PseudoLocalize(s);
				s = std::string();
				break;
			} else {
				sTranslated += PseudoLocalize(s.substr(0, pos));
				s.erase(s.begin(), s.begin() + pos);
			}

			pos = s.find('}');
			sTranslated += s.substr(0, pos + 1);
			s.erase(s.begin(), s.begin() + pos + 1);
		}

		s = sTranslated;
	}

	return s;
}

void
ThemeManager::GetMetricsThatBeginWith(const std::string& sMetricsGroup_,
									  const std::string& sValueName,
									  set<std::string>& vsValueNamesOut)
{
	std::string sMetricsGroup(sMetricsGroup_);
	while (!sMetricsGroup.empty()) {
		const XNode* cur =
		  g_pLoadedThemeData->iniMetrics.GetChild(sMetricsGroup);
		if (cur != nullptr) {
			// Iterate over all metrics that match.
			for (XAttrs::const_iterator j =
				   cur->m_attrs.lower_bound(sValueName);
				 j != cur->m_attrs.end();
				 ++j) {
				const std::string& sv = j->first;
				if (sv.substr(0, sValueName.size()) == sValueName)
					vsValueNamesOut.insert(sv);
				else // we passed the last metric that matched sValueName
					break;
			}
		}

		// put the fallback (if any) in sMetricsGroup
		sMetricsGroup = GetMetricsGroupFallback(sMetricsGroup);
	}
}

std::string
ThemeManager::GetBlankGraphicPath()
{
	return SpecialFiles::THEMES_DIR + SpecialFiles::BASE_THEME_NAME + "/" +
		   ElementCategoryToString(EC_GRAPHICS) + "/_blank.png";
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

/** @brief Allow Lua to have access to the ThemeManager. */
class LunaThemeManager : public Luna<ThemeManager>
{
  public:
	static int ReloadMetrics(T* p, lua_State* L)
	{
		p->ReloadMetrics();
		return 0;
	}

	static int HasMetric(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasMetric(SArg(1), SArg(2)));
		return 1;
	}
	static int GetMetric(T* p, lua_State* L)
	{
		std::string group = SArg(1);
		std::string name = SArg(2);
		if (group.empty() || name.empty()) {
			luaL_error(
			  L,
			  "Cannot fetch metric with empty group name or empty value name.");
		}
		p->PushMetric(L, group, name);
		return 1;
	}
	static int HasString(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->HasString(SArg(1), SArg(2)));
		return 1;
	}
	static int GetString(T* p, lua_State* L)
	{
		std::string group = SArg(1);
		std::string name = SArg(2);
		if (group.empty() || name.empty()) {
			luaL_error(
			  L,
			  "Cannot fetch string with empty group name or empty value name.");
		}
		lua_pushstring(L, p->GetString(group, name).c_str());
		return 1;
	}
	static int GetPathInfoB(T* p, lua_State* L)
	{
		ThemeManager::PathInfo pi;
		p->GetPathInfo(pi, EC_BGANIMATIONS, SArg(1), SArg(2));
		lua_pushstring(L, pi.sResolvedPath.c_str());
		lua_pushstring(L, pi.sMatchingMetricsGroup.c_str());
		lua_pushstring(L, pi.sMatchingElement.c_str());
		return 3;
	}
	// GENERAL_GET_PATH uses lua_toboolean instead of BArg because that
	// makes it optional. -Kyz
#define GENERAL_GET_PATH(get_path_name)                                        \
	static int get_path_name(T* p, lua_State* L)                               \
	{                                                                          \
		lua_pushstring(                                                        \
		  L,                                                                   \
		  p->get_path_name(SArg(1), SArg(2), lua_toboolean(L, 3) != 0)         \
			.c_str());                                                         \
		return 1;                                                              \
	}
	GENERAL_GET_PATH(GetPathF);
	GENERAL_GET_PATH(GetPathG);
	GENERAL_GET_PATH(GetPathB);
	GENERAL_GET_PATH(GetPathS);
	GENERAL_GET_PATH(GetPathO);
#undef GENERAL_GET_PATH

	static int RunLuaScripts(T* p, lua_State* L)
	{
		p->RunLuaScripts(SArg(1));
		return 1;
	}

	static int GetSelectableThemeNames(T* p, lua_State* L)
	{
		// pushes a table of theme folders from GetSelectableThemeNames()
		// lua_pushnumber(L, p->GetNumSelectableThemes() );
		std::vector<std::string> sThemes;
		p->GetSelectableThemeNames(sThemes);
		LuaHelpers::CreateTableFromArray<std::string>(sThemes, L);
		return 1;
	}

	static int GetNumSelectableThemes(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetNumSelectableThemes());
		return 1;
	}

	DEFINE_METHOD(GetCurrentThemeDirectory, GetCurThemeDir());
	static int GetLanguages(T* p, lua_State* L)
	{
		// effectively the same as the method in ScreenOptionsMasterPrefs
		std::vector<std::string> langs;
		p->GetLanguages(langs);
		SortStringArray(langs);

		std::vector<std::string> result;
		for (auto& s : langs) {
			result.push_back(s);
		}
		
		LuaHelpers::CreateTableFromArray<std::string>(result, L);
		return 1;
	}
	DEFINE_METHOD(GetCurLanguage, GetCurLanguage());
	static int GetThemeDisplayName(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetThemeDisplayName(p->GetCurThemeName()).c_str());
		return 1;
	}
	static int GetRealThemeDisplayName(T* p, lua_State* L)
	{
		lua_pushstring(
		  L, p->GetThemeDisplayName(p->GetRealCurThemeName()).c_str());
		return 1;
	}
	static int GetThemeAuthor(T* p, lua_State* L)
	{
		lua_pushstring(L, p->GetThemeAuthor(p->GetCurThemeName()).c_str());
		return 1;
	}
	DEFINE_METHOD(DoesThemeExist, DoesThemeExist(SArg(1)));
	DEFINE_METHOD(IsThemeSelectable, IsThemeSelectable(SArg(1)));
	DEFINE_METHOD(DoesLanguageExist, DoesLanguageExist(SArg(1)));
	DEFINE_METHOD(GetCurThemeName, GetCurThemeName());
	DEFINE_METHOD(GetRealCurThemeName, GetRealCurThemeName());

	static void PushMetricNamesInGroup(IniFile const& ini, lua_State* L)
	{
		std::string group_name = SArg(1);
		const XNode* metric_node = ini.GetChild(group_name);
		if (metric_node != nullptr) {
			// Placed in a table indexed by number, so the order is always the
			// same.
			lua_createtable(L, metric_node->m_attrs.size(), 0);
			int next_index = 1;
			for (const auto& m_attr : metric_node->m_attrs) {
				LuaHelpers::Push(L, m_attr.first);
				lua_rawseti(L, -2, next_index);
				++next_index;
			}
		} else {
			lua_pushnil(L);
		}
	}

	static int GetMetricNamesInGroup(T* p, lua_State* L)
	{
		PushMetricNamesInGroup(g_pLoadedThemeData->iniMetrics, L);
		return 1;
	}

	static int GetStringNamesInGroup(T* p, lua_State* L)
	{
		PushMetricNamesInGroup(g_pLoadedThemeData->iniStrings, L);
		return 1;
	}

	static int SetTheme(T* p, lua_State* L)
	{
		std::string theme_name = SArg(1);
		if (!p->IsThemeSelectable(theme_name)) {
			luaL_error(L, "SetTheme: Invalid Theme: '%s'", theme_name.c_str());
		}
		GameLoop::ChangeTheme(theme_name);
		return 0;
	}
	static int SwitchThemeAndLanguage(T* p, lua_State* L)
	{
		std::string theme_name = SArg(1);
		if (!p->IsThemeSelectable(theme_name)) {
			luaL_error(L, "SetTheme: Invalid Theme: '%s'", theme_name.c_str());
		}
		std::string lang_name = SArg(2);

		p->SwitchThemeAndLanguage(theme_name, lang_name, PREFSMAN->m_bPseudoLocalize);
		return 0;
	}

	LunaThemeManager()
	{
		ADD_METHOD(ReloadMetrics);
		ADD_METHOD(GetMetric);
		ADD_METHOD(GetString);
		ADD_METHOD(GetPathInfoB);
		ADD_METHOD(GetPathF);
		ADD_METHOD(GetPathG);
		ADD_METHOD(GetPathB);
		ADD_METHOD(GetPathS);
		ADD_METHOD(GetPathO);
		ADD_METHOD(RunLuaScripts);
		ADD_METHOD(GetSelectableThemeNames);
		ADD_METHOD(GetNumSelectableThemes);
		ADD_METHOD(GetCurrentThemeDirectory);
		ADD_METHOD(GetLanguages);
		ADD_METHOD(GetCurLanguage);
		ADD_METHOD(GetThemeDisplayName);
		ADD_METHOD(GetRealThemeDisplayName);
		ADD_METHOD(GetThemeAuthor);
		ADD_METHOD(DoesThemeExist);
		ADD_METHOD(IsThemeSelectable);
		ADD_METHOD(DoesLanguageExist);
		ADD_METHOD(GetCurThemeName);
		ADD_METHOD(GetRealCurThemeName);
		ADD_METHOD(HasMetric);
		ADD_METHOD(HasString);
		ADD_METHOD(GetMetricNamesInGroup);
		ADD_METHOD(GetStringNamesInGroup);
		ADD_METHOD(SetTheme);
		ADD_METHOD(SwitchThemeAndLanguage);
	}
};

LUA_REGISTER_CLASS(ThemeManager)
// lua end
