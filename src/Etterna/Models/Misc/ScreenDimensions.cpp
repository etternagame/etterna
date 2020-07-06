#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Preference.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "ScreenDimensions.h"
#include "ThemeMetric.h"

static ThemeMetric<float> THEME_SCREEN_WIDTH("Common", "ScreenWidth");
static ThemeMetric<float> THEME_SCREEN_HEIGHT("Common", "ScreenHeight");

/* The theme's logical resolution specifies the minimum screen width and
 * the minimum screen height with a 4:3 aspect ratio. Scale just one
 * of the dimensions up to meet the requested aspect ratio. */

/* The theme resolution isn't necessarily 4:3; a natively widescreen
 * theme would have eg. 16:9 or 16:10.
 *
 * Note that "aspect ratio" here always means DAR (display aspect ratio: the
 * aspect ratio of the physical display); we don't care about the PAR (pixel
 * aspect ratio: the aspect ratio of a pixel). */

float
ScreenDimensions::GetThemeAspectRatio()
{
	return THEME_NATIVE_ASPECT;
}

/* ceilf was originally lrintf. However, lrintf causes odd resolutions like
 * 639x480 (4:3) and 853x480 (16:9). ceilf gives the correct values of 640x480
 * and 854x480 (should really be 852 so that SCREEN_CENTER_X == 426 and not 427)
 * respectively. -aj */
float
ScreenDimensions::GetScreenWidth()
{
	float fAspect = PREFSMAN->m_fDisplayAspectRatio;
	float fScale = 1;
	if (fAspect > THEME_NATIVE_ASPECT)
		fScale = fAspect / THEME_NATIVE_ASPECT;
	ASSERT(fScale >= 1);
	// ceilf causes the width to come out odd when it shouldn't.
	// 576 * 1.7778 = 1024.0128, which is rounded to 1025. -Kyz
	auto width = (int)ceilf(THEME_SCREEN_WIDTH * fScale);
	width -= width % 2;
	return static_cast<float>(width);
}

float
ScreenDimensions::GetScreenHeight()
{
	float fAspect = PREFSMAN->m_fDisplayAspectRatio;
	float fScale = 1;
	if (fAspect < THEME_NATIVE_ASPECT)
		fScale = THEME_NATIVE_ASPECT / fAspect;
	ASSERT(fScale >= 1);
	return static_cast<float>(ceilf(THEME_SCREEN_HEIGHT * fScale));
}

void
ScreenDimensions::ReloadScreenDimensions()
{
	// Important: explicitly refresh cached metrics that we use.
	THEME_SCREEN_WIDTH.Read();
	THEME_SCREEN_HEIGHT.Read();

	LUA->SetGlobal("SCREEN_WIDTH", (int)SCREEN_WIDTH);
	LUA->SetGlobal("SCREEN_HEIGHT", (int)SCREEN_HEIGHT);
	LUA->SetGlobal("SCREEN_LEFT", (int)SCREEN_LEFT);
	LUA->SetGlobal("SCREEN_RIGHT", (int)SCREEN_RIGHT);
	LUA->SetGlobal("SCREEN_TOP", (int)SCREEN_TOP);
	LUA->SetGlobal("SCREEN_BOTTOM", (int)SCREEN_BOTTOM);
	LUA->SetGlobal("SCREEN_CENTER_X", (int)SCREEN_CENTER_X);
	LUA->SetGlobal("SCREEN_CENTER_Y", (int)SCREEN_CENTER_Y);

	LUA->SetGlobal("ASPECT_SCALE_FACTOR", (int)ASPECT_SCALE_FACTOR);
}

LuaFunction(GetScreenAspectRatio, PREFSMAN->m_fDisplayAspectRatio);
LuaFunction(GetThemeAspectRatio, ScreenDimensions::GetThemeAspectRatio());
