/* ScreenStatsOverlay - credits and statistics drawn on top of everything else.
 */

#ifndef ScreenStatsOverlay_H
#define ScreenStatsOverlay_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Actor/Base/Quad.h"
#include "Screen.h"
#include <chrono>

const int NUM_SKIPS_TO_SHOW = 5;

class ScreenStatsOverlay : public Screen
{
  public:
	void Init() override;

	void Update(float fDeltaTime) override;

  private:
	void AddTimestampLine(const std::string& txt, const RageColor& color);
	void UpdateSkips();

	BitmapText m_textStats;
	Quad m_quadStatBackground;
	Quad m_quadSkipBackground;
	BitmapText m_textSkips[NUM_SKIPS_TO_SHOW];
	std::chrono::steady_clock::time_point g_AccurateSkipTimer =
	  std::chrono::steady_clock::now();
	int m_LastSkip = 0;

	ThemeMetric<float> SKIP_X;
	ThemeMetric<float> SKIP_Y;
	ThemeMetric<float> SKIP_SPACING_Y;
	ThemeMetric<float> SKIP_WIDTH;
};

#endif
