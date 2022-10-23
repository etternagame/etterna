#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Actor/Menus/MenuTimer.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenWithMenuElements.h"
#include "Etterna/Singletons/ThemeManager.h"

#include <algorithm>
#include <utility>

#define TIMER_STEALTH THEME->GetMetricB(m_sName, "TimerStealth")
#define SHOW_STAGE_DISPLAY THEME->GetMetricB(m_sName, "ShowStageDisplay")
#define FORCE_TIMER THEME->GetMetricB(m_sName, "ForceTimer")
#define STOP_MUSIC_ON_BACK THEME->GetMetricB(m_sName, "StopMusicOnBack")
#define WAIT_FOR_CHILDREN_BEFORE_TWEENING_OUT                                  \
	THEME->GetMetricB(m_sName, "WaitForChildrenBeforeTweeningOut")

REGISTER_SCREEN_CLASS(ScreenWithMenuElements);
ScreenWithMenuElements::ScreenWithMenuElements()
{
	m_MenuTimer = nullptr;
	m_bShouldAllowLateJoin = false;
}

void
ScreenWithMenuElements::Init()
{
	PLAY_MUSIC.Load(m_sName, "PlayMusic");
	MUSIC_ALIGN_BEAT.Load(m_sName, "MusicAlignBeat");
	DELAY_MUSIC_SECONDS.Load(m_sName, "DelayMusicSeconds");
	CANCEL_TRANSITIONS_OUT.Load(m_sName, "CancelTransitionsOut");
	TIMER_SECONDS.Load(m_sName, "TimerSeconds");
	TIMER_METRICS_GROUP.Load(m_sName, "TimerMetricsGroup");
	Screen::Init();

	ASSERT(this->m_SubActors.empty()); // don't call Init twice!

	if (TIMER_SECONDS != -1.0f) {
		ASSERT(m_MenuTimer == NULL); // don't load twice
		m_MenuTimer = new MenuTimer;
		m_MenuTimer->Load(TIMER_METRICS_GROUP.GetValue());
		m_MenuTimer->SetName("Timer");
		if (TIMER_STEALTH)
			m_MenuTimer->EnableStealth(true);
		LOAD_ALL_COMMANDS_AND_SET_XY(m_MenuTimer);
		ResetTimer();
		this->AddChild(m_MenuTimer);
	}

	m_sprUnderlay.Load(THEME->GetPathB(m_sName, "underlay"));
	m_sprUnderlay->SetName("Underlay");
	m_sprUnderlay->SetDrawOrder(DRAW_ORDER_UNDERLAY);
	this->AddChild(m_sprUnderlay);
	LOAD_ALL_COMMANDS(m_sprUnderlay);

	m_sprOverlay.Load(THEME->GetPathB(m_sName, "overlay"));
	m_sprOverlay->SetName("Overlay");
	m_sprOverlay->SetDrawOrder(DRAW_ORDER_OVERLAY);
	this->AddChild(m_sprOverlay);
	LOAD_ALL_COMMANDS(m_sprOverlay);

	// decorations
	{
		AutoActor decorations;
		decorations.LoadB(m_sName, "decorations");
		decorations->SetName("Decorations");
		decorations->SetDrawOrder(DRAW_ORDER_DECORATIONS);
		auto* pFrame =
		  dynamic_cast<ActorFrame*>(static_cast<Actor*>(decorations));
		if (pFrame != nullptr) {
			m_vDecorations = pFrame->GetChildren();
			for (auto& child : m_vDecorations) {
				this->AddChild(child);
			}
			pFrame->RemoveAllChildren();
		}
	}

	m_In.SetName("In");
	m_In.Load(THEME->GetPathB(m_sName, "in"));
	m_In.SetDrawOrder(DRAW_ORDER_TRANSITIONS);
	this->AddChild(&m_In);
	LOAD_ALL_COMMANDS(m_In);

	m_Out.SetName("Out");
	m_Out.Load(THEME->GetPathB(m_sName, "out"));
	m_Out.SetDrawOrder(DRAW_ORDER_TRANSITIONS);
	this->AddChild(&m_Out);
	LOAD_ALL_COMMANDS(m_Out);

	m_Cancel.SetName("Cancel");
	m_Cancel.Load(THEME->GetPathB(m_sName, "cancel"));
	m_Cancel.SetDrawOrder(DRAW_ORDER_TRANSITIONS);
	this->AddChild(&m_Cancel);
	LOAD_ALL_COMMANDS(m_Cancel);

	// Grab the music path here; don't GetPath during BeginScreen.
	if (PLAY_MUSIC)
		m_sPathToMusic = THEME->GetPathS(m_sName, "music");
}

void
ScreenWithMenuElements::BeginScreen()
{
	Screen::BeginScreen();

	m_In.Reset();
	m_Out.Reset();
	m_Cancel.Reset();

	TweenOnScreen();

	this->SortByDrawOrder();
	m_In.StartTransitioning(SM_DoneFadingIn);

	SOUND->PlayOnceFromAnnouncer(m_sName + " intro");
	StartPlayingMusic();

	/* Evaluate FirstUpdateCommand. */
	this->PlayCommand("FirstUpdate");
}

void
ScreenWithMenuElements::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_MenuTimer) {
		InputEventPlus iep;
		iep.pn = PLAYER_1;
		MenuStart(iep);
	}

	Screen::HandleScreenMessage(SM);
}

ScreenWithMenuElements::~ScreenWithMenuElements()
{
	SAFE_DELETE(m_MenuTimer);
	for (auto& a : m_vDecorations) {
		delete a;
	}
}

void
ScreenWithMenuElements::SetHelpText(const std::string& s)
{
	Message msg("SetHelpText");
	msg.SetParam("Text", s);
	this->HandleMessage(msg);
}

std::string
ScreenWithMenuElements::HandleLuaMusicFile(std::string const& path)
{
	FileType ft = ActorUtil::GetFileType(path);
	std::string ret = path;
	if (ft == FT_Lua) {
		std::string script;
		std::string error = "Lua runtime error: ";
		if (GetFileContents(path, script)) {
			Lua* L = LUA->Get();
			if (!LuaHelpers::RunScript(
				  L, script, "@" + path, error, 0, 1, true)) {
				ret = "";
			} else {
				// there are two possible ways to load a music file via Lua.
				// 1) return the path to the sound
				// (themer has to use THEME:GetPathS())
				std::string music_path_from_lua;
				LuaHelpers::Pop(L, music_path_from_lua);
				if (!music_path_from_lua.empty()) {
					ret = music_path_from_lua;
				} else {
					// 2) perhaps it's a table with some params? unsure if I
					// want to support this just yet. -aj
					Locator::getLogger()->warn("Lua music script did not return a path to a sound.");
					ret = "";
				}
			}
			LUA->Release(L);
		} else {
			Locator::getLogger()->warn("run script failed hardcore, lol");
			ret = "";
		}
	} else if (ft != FT_Sound) {
		LuaHelpers::ReportScriptErrorFmt("Music file %s is not a sound file.",
										 path.c_str());
		ret = "";
	}
	return ret;
}

void
ScreenWithMenuElements::StartPlayingMusic()
{
	/* Some screens should leave the music alone (eg. ScreenPlayerOptions music
	 * sample left over from ScreenSelectMusic). */
	if (PLAY_MUSIC) {
		GameSoundManager::PlayMusicParams pmp;
		pmp.sFile = HandleLuaMusicFile(m_sPathToMusic);
		if (!pmp.sFile.empty()) {
			pmp.bAlignBeat = MUSIC_ALIGN_BEAT;
			if (DELAY_MUSIC_SECONDS > 0.0f) {
				pmp.fStartSecond = -DELAY_MUSIC_SECONDS;
			}
			SOUND->PlayMusic(pmp);
		}
	}
}

void
ScreenWithMenuElements::Update(float fDeltaTime)
{
	Screen::Update(fDeltaTime);
}

void
ScreenWithMenuElements::ResetTimer()
{
	if (m_MenuTimer == nullptr)
		return;

	if (TIMER_SECONDS > 0.0f && (PREFSMAN->m_bMenuTimer || FORCE_TIMER)) {
		m_MenuTimer->SetSeconds(TIMER_SECONDS);
		m_MenuTimer->Start();
	} else {
		m_MenuTimer->Disable();
	}
}

void
ScreenWithMenuElements::StartTransitioningScreen(ScreenMessage smSendWhenDone)
{
	TweenOffScreen();

	m_Out.StartTransitioning(std::move(smSendWhenDone));
}

void
ScreenWithMenuElements::TweenOnScreen()
{
	this->PlayCommand("On");
}

void
ScreenWithMenuElements::TweenOffScreen()
{
	if (m_MenuTimer != nullptr) {
		m_MenuTimer->SetSeconds(0);
		m_MenuTimer->Stop();
	}

	this->PlayCommand("Off");

	// If we're a stacked screen, then there's someone else between us and the
	// background, so don't tween it off.
	if (!SCREENMAN->IsStackedScreen(this))
		SCREENMAN->PlaySharedBackgroundOffCommand();
}

void
ScreenWithMenuElements::Cancel(ScreenMessage smSendWhenDone)
{
	m_sNextScreen = GetPrevScreen();

	if (CANCEL_TRANSITIONS_OUT) {
		StartTransitioningScreen(smSendWhenDone);
		COMMAND(m_Out, "Cancel");
		return;
	}

	this->PlayCommand("Cancel");

	if (m_Cancel.IsTransitioning())
		return; // ignore

	if (STOP_MUSIC_ON_BACK)
		SOUND->StopMusic();

	if (m_MenuTimer != nullptr)
		m_MenuTimer->Stop();
	m_Cancel.StartTransitioning(smSendWhenDone);
	COMMAND(m_Cancel, "Cancel");
}

bool
ScreenWithMenuElements::IsTransitioning()
{
	return m_In.IsTransitioning() || m_Out.IsTransitioning() ||
		   m_Cancel.IsTransitioning();
}

void
ScreenWithMenuElements::StopTimer()
{
	if (m_MenuTimer != nullptr)
		m_MenuTimer->Stop();
}

REGISTER_SCREEN_CLASS(ScreenWithMenuElementsBasic);
REGISTER_SCREEN_CLASS(ScreenWithMenuElementsSimple);

bool
ScreenWithMenuElementsSimple::MenuStart(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;
	if (m_fLockInputSecs > 0)
		return false;

	StartTransitioningScreen(SM_GoToNextScreen);
	return true;
}

bool
ScreenWithMenuElementsSimple::MenuBack(const InputEventPlus& input)
{
	if (IsTransitioning())
		return false;
	if (m_fLockInputSecs > 0)
		return false;

	Cancel(SM_GoToPrevScreen);
	return true;
}

void
ScreenWithMenuElements::UpdateTimedFunctions(float fDeltaTime)
{
	if (IsTransitioning())
		return;
	Screen::UpdateTimedFunctions(fDeltaTime);
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"
/** @brief Allow Lua to have access to the ScreenWithMenuElements. */
class LunaScreenWithMenuElements : public Luna<ScreenWithMenuElements>
{
  public:
	static int Cancel(T* p, lua_State* L)
	{
		p->Cancel(SM_GoToPrevScreen);
		COMMON_RETURN_SELF;
	}
	static int IsTransitioning(T* p, lua_State* L)
	{
		lua_pushboolean(L, p->IsTransitioning());
		return 1;
	}
	static int SetAllowLateJoin(T* p, lua_State* L)
	{
		p->m_bShouldAllowLateJoin = BArg(1);
		COMMON_RETURN_SELF;
	}

	static int StartTransitioningScreen(T* p, lua_State* L)
	{
		std::string sMessage = SArg(1);
		ScreenMessage SM = ScreenMessageHelpers::ToScreenMessage(sMessage);
		p->StartTransitioningScreen(SM);
		COMMON_RETURN_SELF;
	}

	LunaScreenWithMenuElements()
	{
		ADD_METHOD(Cancel);
		ADD_METHOD(IsTransitioning);
		ADD_METHOD(SetAllowLateJoin);
		ADD_METHOD(StartTransitioningScreen);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenWithMenuElements, Screen)

/** @brief Allow Lua to have access to the ScreenWithMenuElementsSimple. */
class LunaScreenWithMenuElementsSimple
  : public Luna<ScreenWithMenuElementsSimple>
{
  public:
	LunaScreenWithMenuElementsSimple() = default;
};

LUA_REGISTER_DERIVED_CLASS(ScreenWithMenuElementsSimple, ScreenWithMenuElements)

// lua end
