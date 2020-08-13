#include "Etterna/Globals/global.h"
#include "CommonMetrics.h"
#include "Etterna/Singletons/GameManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include "RageUtil/Utils/RageUtil.h"

#include <algorithm>

ThemeMetric<std::string> CommonMetrics::OPERATOR_MENU_SCREEN(
  "Common",
  "OperatorMenuScreen");
ThemeMetric<std::string> CommonMetrics::DEFAULT_MODIFIERS("Common",
														  "DefaultModifiers");
LocalizedString CommonMetrics::WINDOW_TITLE("Common", "WindowTitle");
ThemeMetric<float> CommonMetrics::TICK_EARLY_SECONDS("ScreenGameplay",
													 "TickEarlySeconds");
ThemeMetric<std::string> CommonMetrics::DEFAULT_NOTESKIN_NAME(
  "Common",
  "DefaultNoteSkinName");
ThemeMetricDifficultiesToShow CommonMetrics::DIFFICULTIES_TO_SHOW(
  "Common",
  "DifficultiesToShow");
ThemeMetricStepsTypesToShow CommonMetrics::STEPS_TYPES_TO_SHOW(
  "Common",
  "StepsTypesToHide");
ThemeMetric<bool> CommonMetrics::AUTO_SET_STYLE("Common", "AutoSetStyle");
ThemeMetric<int> CommonMetrics::PERCENT_SCORE_DECIMAL_PLACES(
  "Common",
  "PercentScoreDecimalPlaces");
ThemeMetric<std::string> CommonMetrics::IMAGES_TO_CACHE("Common", "ImageCache");

ThemeMetricDifficultiesToShow::ThemeMetricDifficultiesToShow(
  const std::string& sGroup,
  const std::string& sName)
  : ThemeMetric<std::string>(sGroup, sName)
{
	// re-read because ThemeMetric::ThemeMetric calls ThemeMetric::Read, not the
	// derived one
	if (IsLoaded())
		Read();
}
void
ThemeMetricDifficultiesToShow::Read()
{
	ASSERT(tail(GetName(), 6) == "ToShow");

	ThemeMetric<std::string>::Read();

	m_v.clear();

	vector<std::string> v;
	split(ThemeMetric<std::string>::GetValue(), ",", v);
	if (v.empty()) {
		LuaHelpers::ReportScriptError(
		  "DifficultiesToShow must have at least one entry.");
		return;
	}

	for (auto& i : v) {
		auto d = StringToDifficulty(i);
		if (d == Difficulty_Invalid) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Unknown difficulty \"%s\" in CourseDifficultiesToShow.",
			  i.c_str());
		} else {
			m_v.push_back(d);
		}
	}
}
const vector<Difficulty>&
ThemeMetricDifficultiesToShow::GetValue() const
{
	return m_v;
}

static void
RemoveStepsTypes(vector<StepsType>& inout,
				 const std::string& sStepsTypesToRemove)
{
	vector<std::string> v;
	split(sStepsTypesToRemove, ",", v);
	if (v.empty())
		return; // Nothing to do!

	// subtract StepsTypes
	for (auto& i : v) {
		auto st = GAMEMAN->StringToStepsType(i);
		if (st == StepsType_Invalid) {
			LuaHelpers::ReportScriptErrorFmt(
			  "Invalid StepsType value '%s' in '%s'",
			  i.c_str(),
			  sStepsTypesToRemove.c_str());
			continue;
		}

		const auto iter = std::find(inout.begin(), inout.end(), st);
		if (iter != inout.end())
			inout.erase(iter);
	}
}
ThemeMetricStepsTypesToShow::ThemeMetricStepsTypesToShow(
  const std::string& sGroup,
  const std::string& sName)
  : ThemeMetric<std::string>(sGroup, sName)
{
	// re-read because ThemeMetric::ThemeMetric calls ThemeMetric::Read, not the
	// derived one
	if (IsLoaded())
		Read();
}
void
ThemeMetricStepsTypesToShow::Read()
{
	ASSERT(tail(GetName(), 6) == "ToHide");

	ThemeMetric<std::string>::Read();

	m_v.clear();
	GAMEMAN->GetStepsTypesForGame(GAMESTATE->m_pCurGame, m_v);

	RemoveStepsTypes(m_v, ThemeMetric<std::string>::GetValue());
}
const vector<StepsType>&
ThemeMetricStepsTypesToShow::GetValue() const
{
	return m_v;
}

std::string
CommonMetrics::LocalizeOptionItem(const std::string& s, bool bOptional)
{
	if (bOptional && !THEME->HasString("OptionNames", s))
		return s;
	return THEME->GetString("OptionNames", s);
}

LuaFunction(LocalizeOptionItem,
			CommonMetrics::LocalizeOptionItem(SArg(1), true));
