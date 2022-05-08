#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "RageUtil/Sound/RageSound.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "ScreenExit.h"
#include "Etterna/Globals/GameLoop.h"

/* This screen used to wait for sounds to stop. However, implementing
 * GetPlayingSounds() is annoying, because sounds might be deleted at any time;
 * they aren't ours to have references to. Also, it's better to quit on command
 * instead of waiting several seconds for a sound to stop. */
REGISTER_SCREEN_CLASS(ScreenExit);

void
ScreenExit::Init()
{
	Screen::Init();

	m_Exited = false;

	GameLoop::setUserQuit();
}
