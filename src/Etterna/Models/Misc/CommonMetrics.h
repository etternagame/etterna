#ifndef COMMON_METRICS_H
#define COMMON_METRICS_H

#include "Difficulty.h"
#include "GameConstantsAndTypes.h"
#include "LocalizedString.h"
#include "ThemeMetric.h"

// Types
class ThemeMetricDifficultiesToShow : public ThemeMetric<RString>
{
  public:
	ThemeMetricDifficultiesToShow()
	  : m_v()
	{
	}
	ThemeMetricDifficultiesToShow(const RString& sGroup, const RString& sName);
	void Read();
	const vector<Difficulty>& GetValue() const;

  private:
	vector<Difficulty> m_v;
};
class ThemeMetricStepsTypesToShow : public ThemeMetric<RString>
{
  public:
	ThemeMetricStepsTypesToShow()
	  : m_v()
	{
	}
	ThemeMetricStepsTypesToShow(const RString& sGroup, const RString& sName);
	void Read();
	const vector<StepsType>& GetValue() const;

  private:
	vector<StepsType> m_v;
};

/**
 * @brief Definitions of metrics that are in the "Common" group.
 *
 * These metrics are used throughout the metrics file. */
namespace CommonMetrics {
/** @brief The screen that appears when pressing the operator button. */
extern ThemeMetric<RString> OPERATOR_MENU_SCREEN;
/** @brief The default modifiers to apply. */
extern ThemeMetric<RString> DEFAULT_MODIFIERS;
/** @brief The caption on the title bar. */
extern LocalizedString WINDOW_TITLE;
/** @brief Adjusts the assist tick sound's playback time. */
extern ThemeMetric<float> TICK_EARLY_SECONDS;
/** @brief the name of the default noteskin. */
extern ThemeMetric<RString> DEFAULT_NOTESKIN_NAME;
/** @brief Which difficulties are to be shown? */
extern ThemeMetricDifficultiesToShow DIFFICULTIES_TO_SHOW;
/**
 * @brief Which step types are to be shown?
 *
 * This metric (StepsTypesToHide) takes a list of StepsTypes to hide and
 * returns a list of StepsTypes to show. */
extern ThemeMetricStepsTypesToShow STEPS_TYPES_TO_SHOW;
/** @brief Does the player need to explicitly set a style? */
extern ThemeMetric<bool> AUTO_SET_STYLE;
/** @brief How many decimal places are used? */
extern ThemeMetric<int> PERCENT_SCORE_DECIMAL_PLACES;

extern ThemeMetric<RString> IMAGES_TO_CACHE;



RString
LocalizeOptionItem(const RString& s, bool bOptional);
};

#endif
