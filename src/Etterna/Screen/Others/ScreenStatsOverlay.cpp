#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "ScreenStatsOverlay.h"

REGISTER_SCREEN_CLASS(ScreenStatsOverlay);

void
ScreenStatsOverlay::Init()
{
	Screen::Init();

	m_textStats.LoadFromFont(THEME->GetPathF(m_sName, "stats"));
	m_textStats.SetName("Stats");
	LOAD_ALL_COMMANDS_AND_SET_XY_AND_ON_COMMAND(m_textStats);

	RectF rectStats = RectF(SCREEN_WIDTH - 80, 0, SCREEN_WIDTH, 80);
	m_quadStatBackground.StretchTo(rectStats);
	m_quadStatBackground.SetDiffuse(RageColor(0, 0, 0, 0.4f));
	this->AddChild(&m_quadStatBackground);

	this->AddChild(&m_textStats);

	/* "Was that a skip?"  This displays a message when an update takes
	 * abnormally long, to quantify skips more precisely, verify them
	 * when they're subtle, and show the time it happened, so you can pinpoint
	 * the time in the log. */
	m_LastSkip = 0;

	SKIP_X.Load(m_sName, "SkipX");
	SKIP_Y.Load(m_sName, "SkipY");
	SKIP_SPACING_Y.Load(m_sName, "SkipSpacingY");
	SKIP_WIDTH.Load(m_sName, "SkipWidth");

	RectF rectSkips =
	  RectF(SKIP_X - SKIP_WIDTH / 2,
			SKIP_Y - (SKIP_SPACING_Y * NUM_SKIPS_TO_SHOW) / 2 - 10,
			SKIP_X + SKIP_WIDTH / 2,
			SKIP_Y + (SKIP_SPACING_Y * NUM_SKIPS_TO_SHOW) / 2 + 10);
	m_quadSkipBackground.StretchTo(rectSkips);
	m_quadSkipBackground.SetDiffuse(RageColor(0, 0, 0, 0.4f));
	this->AddChild(&m_quadSkipBackground);

	for (int i = 0; i < NUM_SKIPS_TO_SHOW; i++) {
		/* This is somewhat big.  Let's put it on the right side, where
		 * it'll obscure the 2P side during gameplay; there's nowhere to put
		 * it that doesn't obscure something, and it's just a diagnostic. */
		m_textSkips[i].LoadFromFont(THEME->GetPathF("Common", "normal"));
		m_textSkips[i].SetX(SKIP_X);
		m_textSkips[i].SetY(SCALE(i,
								  0,
								  NUM_SKIPS_TO_SHOW - 1,
								  rectSkips.top + 10,
								  rectSkips.bottom - 10));
		m_textSkips[i].SetDiffuse(RageColor(1, 1, 1, 0));
		m_textSkips[i].SetShadowLength(0);
		this->AddChild(&m_textSkips[i]);
	}

	this->SubscribeToMessage("ShowStatsChanged");
}

void
ScreenStatsOverlay::Update(float fDeltaTime)
{
	Screen::Update(fDeltaTime);

	static bool bShowStatsWasOn = false;

	if (PREFSMAN->m_bShowStats && !bShowStatsWasOn) {
		// Reset skip timer when we toggle Stats on so we don't show a large
		// skip from the span when stats were turned off.
		g_AccurateSkipTimer = std::chrono::steady_clock::now();
	}
	bShowStatsWasOn = PREFSMAN->m_bShowStats.Get();

	this->SetVisible(PREFSMAN->m_bShowStats);
	if (PREFSMAN->m_bShowStats) {
		m_textStats.SetText(DISPLAY->GetStats());
		if (PREFSMAN->m_bShowSkips) {
			m_quadSkipBackground.SetVisible(true);
			UpdateSkips();
		} else {
			m_quadSkipBackground.SetVisible(false);
		}
	}
}

void
ScreenStatsOverlay::AddTimestampLine(const std::string& txt,
									 const RageColor& color)
{
	m_textSkips[m_LastSkip].SetText(txt);
	m_textSkips[m_LastSkip].StopTweening();
	m_textSkips[m_LastSkip].SetDiffuse(RageColor(1, 1, 1, 1));
	m_textSkips[m_LastSkip].BeginTweening(0.2f);
	m_textSkips[m_LastSkip].SetDiffuse(color);
	m_textSkips[m_LastSkip].BeginTweening(3.0f);
	m_textSkips[m_LastSkip].BeginTweening(0.2f);
	m_textSkips[m_LastSkip].SetDiffuse(RageColor(1, 1, 1, 0));

	m_LastSkip++;
	m_LastSkip %= NUM_SKIPS_TO_SHOW;
}

// TODO: check for predictive vsync to check if time is > refresh rate, rather
// than double.
void
ScreenStatsOverlay::UpdateSkips()
{
	/* Use our own timer, so we ignore `/tab. */
	std::chrono::duration<float> timeDelta =
	  std::chrono::steady_clock::now() - g_AccurateSkipTimer;
	const float UpdateTime = timeDelta.count();
	g_AccurateSkipTimer = std::chrono::steady_clock::now();

	/* FPS is 0 for a little while after we load a screen; don't report
	 * during this time. Do clear the timer, though, so we don't report
	 * a big "skip" after this period passes.
	 * Also disregard differences bellow 1ms as runtime will be inconsistent */
	if ((DISPLAY->GetFPS() == 0) || UpdateTime <= 0.001f)
		return;

	/* We want to display skips.  We expect to get updates of about 1.0/FPS ms.
	 */
	const float ExpectedUpdate = 1.0f / DISPLAY->GetFPS();

	/* These are thresholds for severity of skips.  The smallest
	 * is slightly above expected, to tolerate normal jitter. */
	const float Thresholds[] = {
		ExpectedUpdate * 2.0f, ExpectedUpdate * 4.0f, 0.1f, -1
	};

	int skip = 0;
	if (!DISPLAY->IsPredictiveFrameLimit()) {
		while (Thresholds[skip] != -1 && UpdateTime > Thresholds[skip])
			skip++;
	} else {
		if (UpdateTime > ExpectedUpdate)
			skip++;
	}

	if (skip != 0) {
		float skipTime = 1000 * (UpdateTime - ExpectedUpdate);
		if (skipTime >= PREFSMAN->m_bAllowedLag.Get() * 1000) {
			static const RageColor colors[] = {
				RageColor(0, 0, 0, 0),			/* unused */
				RageColor(1.0f, 1.0f, 1.0f, 1), /* white*/
				RageColor(1.0f, 1.0f, 0.0f, 1), /* yellow */
				RageColor(1.0f, 0.4f, 0.4f, 1)	/* light red */
			};

			AddTimestampLine(ssprintf("Lag: %.3fms", skipTime), colors[skip]);

			if (PREFSMAN->m_bLogSkips)
				Locator::getLogger()->trace("Lag: {:.3f}ms", skipTime);
		}
	}
}
