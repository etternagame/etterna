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
const std::string&
ElementCategoryToString(ElementCategory ec);
ElementCategory
StringToElementCategory(const std::string& s);

struct Theme;
/** @brief Manages theme paths and metrics. */
class ThemeManager
{
  public:
	ThemeManager();
	~ThemeManager();

	void GetThemeNames(vector<std::string>& AddTo);
	void GetSelectableThemeNames(vector<std::string>& AddTo);
	int GetNumSelectableThemes();
	bool DoesThemeExist(const std::string& sThemeName);
	bool IsThemeSelectable(std::string const& name);
	bool IsThemeNameValid(std::string const& name);
	std::string GetThemeDisplayName(const std::string& sThemeName);
	std::string GetThemeAuthor(const std::string& sThemeName);
	void GetLanguages(vector<std::string>& AddTo);
	bool DoesLanguageExist(const std::string& sLanguage);
	void SwitchThemeAndLanguage(const std::string& sThemeName,
								const std::string& sLanguage,
								bool bPseudoLocalize,
								bool bForceThemeReload = false);
	void UpdateLuaGlobals();
	std::string GetCurThemeName() const { return m_sCurThemeName; };
	std::string GetRealCurThemeName() const { return m_sRealCurThemeName; };
	bool IsThemeLoaded() const { return !m_sCurThemeName.empty(); };
	std::string GetCurLanguage() const { return m_sCurLanguage; };
	std::string GetCurThemeDir() const
	{
		return GetThemeDirFromName(m_sCurThemeName);
	};
	std::string GetNextTheme();
	std::string GetNextSelectableTheme();
	void ReloadMetrics();
	void ReloadSubscribers();
	void ClearSubscribers();
	void GetOptionNames(vector<std::string>& AddTo);

	static void EvaluateString(std::string& sText);

	struct PathInfo
	{
		std::string sResolvedPath;
		std::string sMatchingMetricsGroup;
		std::string sMatchingElement;
	};

	bool GetPathInfo(PathInfo& out,
					 ElementCategory category,
					 const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false);
	std::string GetPath(ElementCategory category,
					const std::string& sMetricsGroup,
					const std::string& sElement,
					bool bOptional = false);
	std::string GetPathB(const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false)
	{
		return GetPath(EC_BGANIMATIONS, sMetricsGroup, sElement, bOptional);
	};
	std::string GetPathF(const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false)
	{
		return GetPath(EC_FONTS, sMetricsGroup, sElement, bOptional);
	};
	std::string GetPathG(const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false)
	{
		return GetPath(EC_GRAPHICS, sMetricsGroup, sElement, bOptional);
	};
	std::string GetPathS(const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false)
	{
		return GetPath(EC_SOUNDS, sMetricsGroup, sElement, bOptional);
	};
	std::string GetPathO(const std::string& sMetricsGroup,
					 const std::string& sElement,
					 bool bOptional = false)
	{
		return GetPath(EC_OTHER, sMetricsGroup, sElement, bOptional);
	};
	void ClearThemePathCache();

	bool HasMetric(const std::string& sMetricsGroup, const std::string& sValueName);
	void PushMetric(Lua* L,
					const std::string& sMetricsGroup,
					const std::string& sValueName);
	std::string GetMetric(const std::string& sMetricsGroup, const std::string& sValueName);
	int GetMetricI(const std::string& sMetricsGroup, const std::string& sValueName);
	float GetMetricF(const std::string& sMetricsGroup, const std::string& sValueName);
	bool GetMetricB(const std::string& sMetricsGroup, const std::string& sValueName);
	RageColor GetMetricC(const std::string& sMetricsGroup,
						 const std::string& sValueName);
	LuaReference GetMetricR(const std::string& sMetricsGroup,
							const std::string& sValueName);
#if !defined(SMPACKAGE)
	apActorCommands GetMetricA(const std::string& sMetricsGroup,
							   const std::string& sValueName);
#endif

	void GetMetric(const std::string& sMetricsGroup,
				   const std::string& sValueName,
				   LuaReference& valueOut);

	// Languages
	bool HasString(const std::string& sMetricsGroup, const std::string& sValueName);
	std::string GetString(const std::string& sMetricsGroup, const std::string& sValueName);
	void GetString(const std::string& sMetricsGroup,
				   const std::string& sValueName,
				   std::string& valueOut)
	{
		valueOut = GetString(sMetricsGroup, sValueName);
	}
	void FilterFileLanguages(vector<std::string>& asElementPaths);

	void GetMetricsThatBeginWith(const std::string& sMetricsGroup,
								 const std::string& sValueName,
								 set<std::string>& vsValueNamesOut);

	std::string GetMetricsGroupFallback(const std::string& sMetricsGroup);

	static std::string GetBlankGraphicPath();

	// needs to be public for its binding to work
	void RunLuaScripts(const std::string& sMask, bool bUseThemeDir = false);

	// For self-registering metrics
	static void Subscribe(IThemeMetric* p);
	static void Unsubscribe(IThemeMetric* p);

	// Lua
	void PushSelf(lua_State* L);

  protected:
	void LoadThemeMetrics(const std::string& sThemeName, const std::string& sLanguage_);
	std::string GetMetricRaw(const IniFile& ini,
						 const std::string& sMetricsGroup,
						 const std::string& sValueName);
	bool GetMetricRawRecursive(const IniFile& ini,
							   const std::string& sMetricsGroup,
							   const std::string& sValueName,
							   std::string& sRet);

	bool GetPathInfoToAndFallback(PathInfo& out,
								  ElementCategory category,
								  const std::string& sMetricsGroup,
								  const std::string& sFile);
	bool GetPathInfoToRaw(PathInfo& out,
						  const std::string& sThemeName,
						  ElementCategory category,
						  const std::string& sMetricsGroup,
						  const std::string& sFile);
	static std::string GetThemeDirFromName(const std::string& sThemeName);
	std::string GetElementDir(const std::string& sThemeName);
	static std::string GetMetricsIniPath(const std::string& sThemeName);
	static void GetLanguagesForTheme(const std::string& sThemeName,
									 vector<std::string>& asLanguagesOut);
	static std::string GetLanguageIniPath(const std::string& sThemeName,
									  const std::string& sLanguage);
	void GetOptionalLanguageIniPaths(vector<std::string>& vsPathsOut,
									 const std::string& sThemeName,
									 const std::string& sLanguage);
	std::string GetDefaultLanguage();

	std::string m_sCurThemeName;
	std::string m_sRealCurThemeName = "";
	std::string m_sCurLanguage;
	bool m_bPseudoLocalize;
};

extern ThemeManager*
  THEME; // global and accessible from anywhere in our program

#endif
