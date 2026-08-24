#include "Etterna/Globals/global.h"
#include "ScreenGameplaySpectate.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Singletons/StatsManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/GamePreferences.h"
#include "Etterna/Models/Misc/PlayerInfo.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "Core/Services/Locator.hpp"
#include "Etterna/Singletons/ReplayManager.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Singletons/NetworkSyncManager.h"

#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Models/Songs/SongOptions.h"

REGISTER_SCREEN_CLASS(ScreenGameplaySpectate);

AutoScreenMessage(SM_Spectator_InputUpdate);
AutoScreenMessage(SM_Spectator_HoldUpdate);

void
ScreenGameplaySpectate::FillPlayerInfo(std::vector<PlayerInfo>& playerInfoOut)
{
	playerInfoOut.clear();

	playerInfoOut.push_back(PlayerInfo());

	// for now, only 1 can be spectated
	playerInfoOut[0].Load(PLAYER_1,
						MultiPlayer_Invalid,
						true,
						Difficulty_Invalid,
						GameplayMode_Spectate);
}

ScreenGameplaySpectate::ScreenGameplaySpectate()
{
	ASSERT_M(
	  NSMAN->spectating,
	  "You tried to go into ScreenGameplaySpectate while not spectating.");

	// Set up rate
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = 1.F;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = 1.F;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = 1.F;
	GAMESTATE->m_SongOptions.GetStage().m_fMusicRate = 1.F;

	PlayerOptions po;
	po.Init();
	po.SetForReplay(true);
	po.FromString("");

	// Set up transforming mods
	{
		auto f = [&po](PlayerOptions& playerOptions) {
			std::copy(std::begin(po.m_bTurns),
					  std::end(po.m_bTurns),
					  std::begin(playerOptions.m_bTurns));
		};
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetPreferred());
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent());
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetSong());
		f(GAMESTATE->m_pPlayerState->m_PlayerOptions.GetStage());
	}
}

void
ScreenGameplaySpectate::Init()
{
	ScreenGameplay::Init();
}

void
ScreenGameplaySpectate::LoadPlayer()
{
	GetPlayerInfo()->m_pPlayer->Load();
}

void
ScreenGameplaySpectate::LoadScoreKeeper()
{
	if (GetPlayerInfo()->m_pPrimaryScoreKeeper != nullptr) {
		GetPlayerInfo()->m_pPrimaryScoreKeeper->Load(
		  m_apSongsQueue, GetPlayerInfo()->m_vpStepsQueue);
	}
}

ScreenGameplaySpectate::~ScreenGameplaySpectate()
{
	Locator::getLogger()->debug("ScreenGameplaySpectate::~ScreenGameplaySpectate()");

}

void
ScreenGameplaySpectate::HandleScreenMessage(const ScreenMessage& SM)
{
	if (SM == SM_Spectator_InputUpdate) {
		Message msg("SpectatorInputUpdate");
		GetPlayerInfo()->m_pPlayer->HandleMessage(msg);
		return;
	}
	else if (SM == SM_Spectator_HoldUpdate) {
		Message msg("SpectatorHoldUpdate");
		GetPlayerInfo()->m_pPlayer->HandleMessage(msg);
		return;
	}


	ScreenGameplay::HandleScreenMessage(SM);
}

void
ScreenGameplaySpectate::Update(const float fDeltaTime)
{
	if (GAMESTATE->m_pCurSong == nullptr) {
		ScreenWithMenuElements::Update(fDeltaTime); // NOLINT(bugprone-parent-virtual-call)
		return;
	}

	UpdateSongPosition();

	if (m_bZeroDeltaOnNextUpdate) {
		ScreenWithMenuElements::Update(0); // NOLINT(bugprone-parent-virtual-call)
		m_bZeroDeltaOnNextUpdate = false;
	} else {
		ScreenWithMenuElements::Update(fDeltaTime); // NOLINT(bugprone-parent-virtual-call)
	}

	if (SCREENMAN->GetTopScreen() != this) {
		return;
	}

	m_AutoKeysounds.Update(fDeltaTime);

	GetPlayerInfo()->m_SoundEffectControl.Update(fDeltaTime);

	{
		const auto fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		auto p = m_pSoundMusic->GetParams();
		if (std::fabs(p.m_fSpeed - fSpeed) > 0.01F && fSpeed >= 0.0F) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	switch (m_DancingState) {
		case STATE_DANCING: {
			{
				float fSecondsToStartFadingOutMusic;
				float fSecondsToStartTransitioningOut;
				GetMusicEndTiming(fSecondsToStartFadingOutMusic,
								  fSecondsToStartTransitioningOut);

				const auto bAllReallyFailed =
				  STATSMAN->m_CurStageStats.Failed();
				if (bAllReallyFailed) {
					fSecondsToStartTransitioningOut += BEGIN_FAILED_DELAY;
				}

				if (GAMESTATE->m_Position.m_fMusicSeconds >=
					  fSecondsToStartTransitioningOut &&
					!m_NextSong.IsTransitioning() && !GAMESTATE->GetPaused()) {
					this->PostScreenMessage(SM_NotesEnded, 0);
				}
			}
		}
		default:
			break;
	}

	PlayTicks();
	SendCrossedMessages();

	// ArrowEffects::Update call moved because having it happen once per
	// NoteField (which means twice in two player) seemed wasteful. -Kyz
	ArrowEffects::Update();
}

auto
ScreenGameplaySpectate::Input(const InputEventPlus& input) -> bool
{
	// LOG->Trace( "ScreenGameplaySpectate::Input()" );

	Message msg("");
	if (m_Codes.InputMessage(input, msg)) {
		this->HandleMessage(msg);
	}

	if (m_DancingState != STATE_OUTRO && GAMESTATE->IsHumanPlayer(input.pn) &&
		!m_Cancel.IsTransitioning()) {

		// Exiting gameplay by pressing Back (Immediate Exit)
		auto bHoldingBack = false;
		if (GAMESTATE->GetCurrentStyle(input.pn)->GameInputToColumn(
			  input.GameI) == Column_Invalid) {
			bHoldingBack |= input.MenuI == GAME_BUTTON_BACK;
		}

		if (bHoldingBack) {
			if (((!PREFSMAN->m_bDelayedBack && input.type == IET_FIRST_PRESS) ||
				 (input.DeviceI.device == DEVICE_KEYBOARD &&
				  input.type == IET_REPEAT) ||
				 (input.DeviceI.device != DEVICE_KEYBOARD &&
				  INPUTFILTER->GetSecsHeld(input.DeviceI) >= 1.0F))) {
				Locator::getLogger()->info("Player {} went back", input.pn + 1);
				BeginBackingOutFromGameplay();
			} else if (PREFSMAN->m_bDelayedBack &&
					   input.type == IET_FIRST_PRESS) {
				m_textDebug.SetText(GIVE_UP_BACK_TEXT);
				m_textDebug.PlayCommand("BackOn");
			} else if (PREFSMAN->m_bDelayedBack && input.type == IET_RELEASE) {
				m_textDebug.PlayCommand("TweenOff");
			}

			return true;
		}
	}

	if (!input.GameI.IsValid()) {
		return false;
	}

	return false;
}

void
ScreenGameplaySpectate::SaveStats()
{
	// Reload the notedata after finishing in case we truncated it
	SetupNoteDataFromRow(GAMESTATE->m_pCurSteps, -1);

	// Reload the replay data to make sure it is clean for calculations
	REPLAYS->InitReplayPlaybackForScore(REPLAYS->GetActiveReplayScore(),
										Player::GetTimingWindowScale());

	ScreenGameplay::SaveStats();
}

void
ScreenGameplaySpectate::StageFinished(bool bBackedOut)
{
	Locator::getLogger()->info("Finishing Stage");
	if (bBackedOut) {
		GAMESTATE->CancelStage();
		return;
	}

	Locator::getLogger()->info("Done Finishing Stage");
}

// lua
class LunaScreenGameplaySpectate : public Luna<ScreenGameplaySpectate>
{
  public:
	

	LunaScreenGameplaySpectate()
	{
		
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplaySpectate, ScreenGameplay)
