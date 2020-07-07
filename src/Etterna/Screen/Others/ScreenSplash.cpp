#include "Etterna/Globals/global.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSplash.h"
#include "Etterna/Singletons/ThemeManager.h"

REGISTER_SCREEN_CLASS(ScreenSplash);

void
ScreenSplash::Init()
{
	ALLOW_START_TO_SKIP.Load(m_sName, "AllowStartToSkip");
	PREPARE_SCREEN.Load(m_sName, "PrepareScreen");

	ScreenWithMenuElements::Init();
}

void
ScreenSplash::BeginScreen()
{
	ScreenWithMenuElements::BeginScreen();
}

void
ScreenSplash::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_DoneFadingIn) {
		if (PREPARE_SCREEN)
			SCREENMAN->PrepareScreen(GetNextScreenName());
	} else if (SM == SM_MenuTimer) {
		StartTransitioningScreen(SM_GoToNextScreen);
	}

	ScreenWithMenuElements::HandleScreenMessage(SM);
}

bool
ScreenSplash::MenuBack(const InputEventPlus& input)
{
	Cancel(SM_GoToPrevScreen);
	return true;
}

bool
ScreenSplash::MenuStart(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;
	if (!ALLOW_START_TO_SKIP)
		return false;
	StartTransitioningScreen(SM_GoToNextScreen);
	return true;
}
