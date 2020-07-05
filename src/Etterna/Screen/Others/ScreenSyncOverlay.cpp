#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "Etterna/Models/Misc/AdjustSync.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/InputEventPlus.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "ScreenSyncOverlay.h"
#include "Etterna/Models/Songs/Song.h"

static bool previousGameplayState;
static AutosyncType lastSyncType;
static PlayerController lastController;

static bool
IsGameplay()
{
	return (SCREENMAN != nullptr) && (SCREENMAN->GetTopScreen() != nullptr) &&
		   SCREENMAN->GetTopScreen()->GetScreenType() == gameplay;
}

REGISTER_SCREEN_CLASS(ScreenSyncOverlay);

void
ScreenSyncOverlay::Init()
{
	Screen::Init();
	m_overlay.Load(THEME->GetPathB(m_sName, "overlay"));
	AddChild(m_overlay);

	// When the screen is initialized we know it will not be gameplay
	// but we want Update to start in the correct state to hide help.
	previousGameplayState = true;

	Update(0);
}

void
ScreenSyncOverlay::Update(float fDeltaTime)
{
	bool isGameplay = IsGameplay();

	this->SetVisible(isGameplay);

	if (!isGameplay) {
		if (previousGameplayState) {
			previousGameplayState = isGameplay;
			HideHelp();
		}

		return;
	}

	Screen::Update(fDeltaTime);

	UpdateText();
}

bool g_bShowAutoplay = true;
void
ScreenSyncOverlay::SetShowAutoplay(bool b)
{
	g_bShowAutoplay = b;
}

static LocalizedString AUTO_PLAY("ScreenSyncOverlay", "AutoPlay");
static LocalizedString AUTO_PLAY_CPU("ScreenSyncOverlay", "AutoPlayCPU");
static LocalizedString AUTO_PLAY_REPLAY("ScreenSyncOverlay", "Replay");
static LocalizedString AUTO_SYNC_SONG("ScreenSyncOverlay", "AutoSync Song");
static LocalizedString AUTO_SYNC_MACHINE("ScreenSyncOverlay",
										 "AutoSync Machine");
static LocalizedString OLD_OFFSET("ScreenSyncOverlay", "Old offset");
static LocalizedString NEW_OFFSET("ScreenSyncOverlay", "New offset");
static LocalizedString COLLECTING_SAMPLE("ScreenSyncOverlay",
										 "Collecting sample");
static LocalizedString STANDARD_DEVIATION("ScreenSyncOverlay",
										  "Standard deviation");
void
ScreenSyncOverlay::UpdateText(bool forcedChange)
{
	// Update Status
	vector<std::string> vs;

	PlayerController pc = GamePreferences::m_AutoPlay.Get();

	if (g_bShowAutoplay) {
		switch (pc) {
			case PC_HUMAN:
				break;
			case PC_AUTOPLAY:
				vs.push_back(AUTO_PLAY);
				break;
			case PC_CPU:
				vs.push_back(AUTO_PLAY_CPU);
				break;
			case PC_REPLAY:
				vs.push_back(AUTO_PLAY_REPLAY);
				break;
			default:
				FAIL_M(ssprintf("Invalid PlayerController: %i", pc));
		}
	}

	AutosyncType type = GAMESTATE->m_SongOptions.GetCurrent().m_AutosyncType;
	switch (type) {
		case AutosyncType_Off:
			break;
		case AutosyncType_Song:
			vs.push_back(AUTO_SYNC_SONG);
			break;
		case AutosyncType_Machine:
			vs.push_back(AUTO_SYNC_MACHINE);
			break;
		default:
			FAIL_M(ssprintf("Invalid autosync type: %i", type));
	}

	if (GAMESTATE->m_pCurSong != nullptr) {
		AdjustSync::GetSyncChangeTextGlobal(vs);
		AdjustSync::GetSyncChangeTextSong(vs);
	}

	if (forcedChange || !vs.empty() || type != lastSyncType ||
		pc != lastController) {
		Message set_status("SetStatus");
		set_status.SetParam("text", join("\n", vs));
		m_overlay->HandleMessage(set_status);
	}

	// Update SyncInfo
	bool visible =
	  GAMESTATE->m_SongOptions.GetCurrent().m_AutosyncType != AutosyncType_Off;
	std::string s;
	if (visible) {
		float fNew = PREFSMAN->m_fGlobalOffsetSeconds;
		float fOld = AdjustSync::s_fGlobalOffsetSecondsOriginal;
		float fStdDev = AdjustSync::s_fStandardDeviation;
		s += OLD_OFFSET.GetValue() + ssprintf(": %0.3f\n", fOld);
		s += NEW_OFFSET.GetValue() + ssprintf(": %0.3f\n", fNew);
		s += STANDARD_DEVIATION.GetValue() + ssprintf(": %0.3f\n", fStdDev);
		s += COLLECTING_SAMPLE.GetValue() +
			 ssprintf(": %d / %d",
					  AdjustSync::s_iAutosyncOffsetSample + 1,
					  AdjustSync::OFFSET_SAMPLE_COUNT);
	}

	if (forcedChange || visible || type != lastSyncType ||
		pc != lastController) {
		Message set_adjustments("SetAdjustments");
		set_adjustments.SetParam("visible", visible);
		set_adjustments.SetParam("text", s);
		m_overlay->HandleMessage(set_adjustments);
	}
	lastSyncType = type;
	lastController = pc;
}

static LocalizedString CANT_SYNC_WHILE_PLAYING_A_COURSE(
  "ScreenSyncOverlay",
  "Can't sync while playing a course.");
static LocalizedString SYNC_CHANGES_REVERTED("ScreenSyncOverlay",
											 "Sync changes reverted.");
bool
ScreenSyncOverlay::Input(const InputEventPlus& input)
{
	bool isGameplay = IsGameplay();

	if (!isGameplay)
		return Screen::Input(input);

	if (input.DeviceI.device != DEVICE_KEYBOARD)
		return Screen::Input(input);

	enum Action
	{
		RevertSyncChanges,
		ChangeGlobalOffset,
		ChangeSongOffset,
		Action_Invalid
	};
	Action a = Action_Invalid;

	bool bIncrease = true;
	switch (input.DeviceI.button) {
		case KEY_F4:
			a = RevertSyncChanges;
			break;
		case KEY_F11:
			bIncrease = false; /* fall through */
		case KEY_F12:
			if (INPUTFILTER->IsBeingPressed(
				  DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT)) ||
				INPUTFILTER->IsBeingPressed(
				  DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)))
				a = ChangeGlobalOffset;
			else
				a = ChangeSongOffset;
			break;

		default:
			return Screen::Input(input);
	}

	if (GAMESTATE->IsPlaylistCourse() && a != ChangeGlobalOffset) {
		SCREENMAN->SystemMessage(CANT_SYNC_WHILE_PLAYING_A_COURSE);
		return true;
	}

	// Release the lookup tables being used for the timing data because
	// changing the timing data invalidates them. -Kyz
	if (a != Action_Invalid) {
		if (GAMESTATE->m_pCurSteps) {
			GAMESTATE->m_pCurSteps->GetTimingData()->ReleaseLookup();
		}
	}

	switch (a) {
		case RevertSyncChanges:
			if (input.type != IET_FIRST_PRESS)
				return false;
			SCREENMAN->SystemMessage(SYNC_CHANGES_REVERTED);
			AdjustSync::RevertSyncChanges();
			break;
		case ChangeGlobalOffset:
		case ChangeSongOffset: {
			float fDelta = bIncrease ? +0.02f : -0.02f;
			if (INPUTFILTER->IsBeingPressed(
				  DeviceInput(DEVICE_KEYBOARD, KEY_RALT)) ||
				INPUTFILTER->IsBeingPressed(
				  DeviceInput(DEVICE_KEYBOARD, KEY_LALT))) {
				fDelta /= 20; /* 1ms */
			}
			switch (input.type) {
				case IET_RELEASE:
					fDelta *= 0;
					break;
				case IET_REPEAT: {
					if (INPUTFILTER->GetSecsHeld(input.DeviceI) < 1.0f)
						fDelta *= 0;
					else
						fDelta *= 10;
				}
				default:
					break;
			}

			switch (a) {
				case ChangeGlobalOffset: {
					PREFSMAN->m_fGlobalOffsetSeconds.Set(
					  PREFSMAN->m_fGlobalOffsetSeconds + fDelta);
					break;
				}

				case ChangeSongOffset: {
					if (GAMESTATE->m_pCurSong != nullptr) {
						GAMESTATE->m_pCurSong->m_SongTiming
						  .m_fBeat0OffsetInSeconds += fDelta;
						const vector<Steps*>& vpSteps =
						  GAMESTATE->m_pCurSong->GetAllSteps();
						for (auto& s : const_cast<vector<Steps*>&>(vpSteps)) {
							// Empty means it inherits song timing,
							// which has already been updated.
							if (s->m_Timing.empty())
								continue;
							s->m_Timing.m_fBeat0OffsetInSeconds += fDelta;
						}
					}
					break;
				}
				default:
					break;
			}
		} break;
		default:
			FAIL_M(ssprintf("Invalid sync action choice: %i", a));
	}

	if (!previousGameplayState) {
		previousGameplayState = isGameplay;
		ShowHelp();
	}

	UpdateText(true);
	return true;
}

void
ScreenSyncOverlay::ShowHelp()
{
	m_overlay->PlayCommand("Show");
}

void
ScreenSyncOverlay::HideHelp()
{
	m_overlay->PlayCommand("Hide");
}
