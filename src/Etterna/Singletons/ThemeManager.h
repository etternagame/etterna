#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include "Etterna/Models/Lua/LuaReference.h"
#include "RageUtil/Misc/RageTypes.h"

#include <set>

class IThemeMetric;
class IniFile;
struct lua_State;

enum ElementCategory
{
	EC_BGANIMATIONS,
	EC_FONTS,
	EC_GRAPHICS,
	EC_SOUNDS,
	EC_OTHER,
	NUM_ElementCategory,
	ElementCategory_Invalid
};
/** @brief A special foreach loop going through each ElementCategory. */
#define FOREACH_ElementCategory(ec) FOREACH_ENUM(ElementCategory, ec)
auto
ElementCategoryToString(ElementCategory ec) -> const std::string&;
auto
StringToElementCategory(const std::string& s) -> ElementCategory;

struct Theme;
/** @brief Manages theme paths and metrics. */
class ThemeManager
{
  public:
	ThemeManager();
	~ThemeManager();

	void GetThemeNames(std::vector<std::string>& AddTo);
	void GetSelectableThemeNames(std::vector<std::string>& AddTo);
	auto GetNumSelectableThemes() -> int;
	auto DoesThemeExist(const std::string& sThemeName) -> bool;
	auto IsThemeSelectable(std::string const& name) -> bool;
	auto IsThemeNameValid(std::string const& name) -> bool;
	auto GetThemeDisplayName(const std::string& sThemeName) -> std::string;
	auto GetThemeAuthor(const std::string& sThemeName) -> std::string;
	void GetLanguages(std::vector<std::string>& AddTo);
	auto DoesLanguageExist(const std::string& sLanguage) -> bool;
	void SwitchThemeAndLanguage(const std::string& sThemeName,
								const std::string& sLanguage,
								bool bPseudoLocalize,
								bool bForceThemeReload = false);
	void UpdateLuaGlobals();
	[[nodiscard]] auto GetCurThemeName() const -> std::string
	{
		return m_sCurThemeName;
	};
	[[nodiscard]] auto GetRealCurThemeName() const -> std::string
	{
		return m_sRealCurThemeName;
	};
	[[nodiscard]] auto IsThemeLoaded() const -> bool
	{
		return !m_sCurThemeName.empty();
	};
	[[nodiscard]] auto GetCurLanguage() const -> std::string
	{
		return m_sCurLanguage;
	};
	[[nodiscard]] auto GetCurThemeDir() const -> std::string
	{
		return GetThemeDirFromName(m_sCurThemeName);
	};
	auto GetNextTheme() -> std::string;
	auto GetNextSelectableTheme() -> std::string;
	void ReloadMetrics();
	void ReloadSubscribers();
	void ClearSubscribers();
	void GetOptionNames(std::vector<std::string>& AddTo);

	static void EvaluateString(std::string& sText);

	struct PathInfo
	{
		std::string sResolvedPath;
		std::string sMatchingMetricsGroup;
		std::string sMatchingElement;
	};

	auto GetPathInfo(PathInfo& out,
					 ElementCategory category,
					 const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false) -> bool;
	auto GetPath(ElementCategory category,
				 const std::string& sMetricsGroup,
				 const std::string& sElement,
				 bool bOptional = false) -> std::string;
	auto GetPathB(const std::string& sMetricsGroup,
				  const std::string& sElement,
				  bool bOptional = false) -> std::string
	{
		return GetPath(EC_BGANIMATIONS, sMetricsGroup, sElement, bOptional);
	};
	auto GetPathF(const std::string& sMetricsGroup,
				  const std::string& sElement,
				  bool bOptional = false) -> std::string
	{
		return GetPath(EC_FONTS, sMetricsGroup, sElement, bOptional);
	};
	auto GetPathG(const std::string& sMetricsGroup,
				  const std::string& sElement,
				  bool bOptional = false) -> std::string
	{
		return GetPath(EC_GRAPHICS, sMetricsGroup, sElement, bOptional);
	};
	auto GetPathS(const std::string& sMetricsGroup,
				  const std::string& sElement,
				  bool bOptional = false) -> std::string
	{
		return GetPath(EC_SOUNDS, sMetricsGroup, sElement, bOptional);
	};
	auto GetPathO(const std::string& sMetricsGroup,
				  const std::string& sElement,
				  bool bOptional = false) -> std::string
	{
		return GetPath(EC_OTHER, sMetricsGroup, sElement, bOptional);
	};
	void ClearThemePathCache();

	auto HasMetric(const std::string& sMetricsGroup,
				   const std::string& sValueName) -> bool;
	void PushMetric(Lua* L,
					const std::string& sMetricsGroup,
					const std::string& sValueName);
	auto GetMetric(const std::string& sMetricsGroup,
				   const std::string& sValueName) -> std::string;
	auto GetMetricI(const std::string& sMetricsGroup,
					const std::string& sValueName) -> int;
	auto GetMetricF(const std::string& sMetricsGroup,
					const std::string& sValueName) -> float;
	auto GetMetricB(const std::string& sMetricsGroup,
					const std::string& sValueName) -> bool;
	auto GetMetricC(const std::string& sMetricsGroup,
					const std::string& sValueName) -> RageColor;
	auto GetMetricR(const std::string& sMetricsGroup,
					const std::string& sValueName) -> LuaReference;
#if !defined(SMPACKAGE)
	auto GetMetricA(const std::string& sMetricsGroup,
					const std::string& sValueName) -> apActorCommands;
#endif

	void GetMetric(const std::string& sMetricsGroup,
				   const std::string& sValueName,
				   LuaReference& valueOut);

	// Languages
	auto HasString(const std::string& sMetricsGroup,
				   const std::string& sValueName) -> bool;
	auto GetString(const std::string& sMetricsGroup,
				   const std::string& sValueName) -> std::string;
	void GetString(const std::string& sMetricsGroup,
				   const std::string& sValueName,
				   std::string& valueOut)
	{
		valueOut = GetString(sMetricsGroup, sValueName);
	}
	void FilterFileLanguages(std::vector<std::string>& asElementPaths);

	void GetMetricsThatBeginWith(const std::string& sMetricsGroup,
								 const std::string& sValueName,
								 std::set<std::string>& vsValueNamesOut);

	auto GetMetricsGroupFallback(const std::string& sMetricsGroup)
	  -> std::string;

	static auto GetBlankGraphicPath() -> std::string;

	// needs to be public for its binding to work
	void RunLuaScripts(const std::string& sMask, bool bUseThemeDir = false);

	// For self-registering metrics
	static void Subscribe(IThemeMetric* p);
	static void Unsubscribe(IThemeMetric* p);

	// Lua
	void PushSelf(lua_State* L);

  protected:
	void LoadThemeMetrics(const std::string& sThemeName,
						  const std::string& sLanguage_);
	auto GetMetricRaw(const IniFile& ini,
					  const std::string& sMetricsGroup,
					  const std::string& sValueName) -> std::string;
	auto GetMetricRawRecursive(const IniFile& ini,
							   const std::string& sMetricsGroup,
							   const std::string& sValueName,
							   std::string& sRet) -> bool;

	auto GetPathInfoToAndFallback(PathInfo& out,
								  ElementCategory category,
								  const std::string& sMetricsGroup,
								  const std::string& sFile) -> bool;
	auto GetPathInfoToRaw(PathInfo& out,
						  const std::string& sThemeName,
						  ElementCategory category,
						  const std::string& sMetricsGroup,
						  const std::string& sFile) -> bool;
	static auto GetThemeDirFromName(const std::string& sThemeName)
	  -> std::string;
	auto GetElementDir(const std::string& sThemeName) -> std::string;
	static auto GetMetricsIniPath(const std::string& sThemeName) -> std::string;
	static void GetLanguagesForTheme(const std::string& sThemeName,
									 std::vector<std::string>& asLanguagesOut);
	static auto GetLanguageIniPath(const std::string& sThemeName,
								   const std::string& sLanguage) -> std::string;
	void GetOptionalLanguageIniPaths(std::vector<std::string>& vsPathsOut,
									 const std::string& sThemeName,
									 const std::string& sLanguage);
	auto GetDefaultLanguage() -> std::string;

	std::string m_sCurThemeName;
	std::string m_sRealCurThemeName = "";
	std::string m_sCurLanguage;
	bool m_bPseudoLocalize;

  private:
	void AppendToLuaPackagePath(const std::string& path);
};

extern ThemeManager*
  THEME; // global and accessible from anywhere in our program

#endif
