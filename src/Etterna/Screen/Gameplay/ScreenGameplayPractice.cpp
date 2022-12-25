#include "Etterna/Globals/global.h"
#include "ScreenGameplayPractice.h"
#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Actor/Gameplay/ArrowEffects.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/PlayerInfo.h"
#include "Etterna/Models/Misc/PlayerStageStats.h"
#include "Etterna/Models/NoteData/NoteDataUtil.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Actor/Gameplay/Player.h"
#include "Etterna/Actor/Gameplay/PlayerPractice.h"
#include "Etterna/Singletons/DownloadManager.h"
#include "Etterna/Singletons/GameSoundManager.h"
#include "RageUtil/Misc/RageInput.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Singletons/LuaManager.h"
#include "Core/Services/Locator.hpp"

#include <algorithm>

#include "Etterna/Models/Misc/AdjustSync.h"

REGISTER_SCREEN_CLASS(ScreenGameplayPractice);

void
ScreenGameplayPractice::FillPlayerInfo(PlayerInfo* playerInfoOut)
{
	playerInfoOut->Load(PLAYER_1,
						MultiPlayer_Invalid,
						true,
						Difficulty_Invalid,
						GameplayMode_Practice);
}

ScreenGameplayPractice::ScreenGameplayPractice()
{
	// covered by base class constructor
}

void
ScreenGameplayPractice::Init()
{
	ScreenGameplay::Init();
}

ScreenGameplayPractice::~ScreenGameplayPractice()
{
	Locator::getLogger()->debug("ScreenGameplayReplay::~ScreenGameplayReplay()");
}

auto
ScreenGameplayPractice::Input(const InputEventPlus& input) -> bool
{
	// override default input here so we can reload the song
	// ... i wonder if this is doable.
	// haha dont try to break it please
	if (!IsTransitioning()) {
		const auto bHoldingCtrl =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LCTRL)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RCTRL));
		const auto bHoldingShift =
		  INPUTFILTER->IsBeingPressed(
			DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT)) ||
		  INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT));

		auto c = INPUTMAN->DeviceInputToChar(input.DeviceI, false);
		MakeUpper(&c, 1);

		// if you press ctrl+shift+R, you reload the song like in Music Select
		// the catch is that this can break a lot of things probably
		// so be careful with it

		// initial implementation below, might just end up restarting the song
		// and reloading there instead
		if (bHoldingCtrl && bHoldingShift && c == 'R') {
			Song* cursong = GAMESTATE->m_pCurSong;
			auto chartsForThisSong = cursong->GetAllSteps();
			std::vector<std::string> oldKeys;
			oldKeys.reserve(chartsForThisSong.size());
			for (auto* k : chartsForThisSong) {
				oldKeys.emplace_back(k->GetChartKey());
			}

			const auto success = cursong->ReloadFromSongDir();
			SONGMAN->ReconcileChartKeysForReloadedSong(cursong, oldKeys);

			if (!success || GAMESTATE->m_pCurSteps->GetNoteData().IsEmpty()) {
				Locator::getLogger()->error("The Player attempted something resulting in an "
						   "unrecoverable error while in Gameplay Practice and "
						   "has been ejected.");
				BeginBackingOutFromGameplay();
				return true;
			}

			AdjustSync::ResetOriginalSyncData();
			SetupNoteDataFromRow(GAMESTATE->m_pCurSteps);
			if (!m_vPlayerInfo.m_NoteData.IsEmpty())
				m_vPlayerInfo.GetPlayerState()->ResetCacheInfo();

			float fSecondsToStartFadingOutMusic;
			float fSecondsToStartTransitioningOut;
			GetMusicEndTiming(fSecondsToStartFadingOutMusic,
							  fSecondsToStartTransitioningOut);
			auto p = m_pSoundMusic->GetParams();
			const auto basicSongStart =
			  GAMESTATE->m_pCurSong->GetFirstSecond() * p.m_fSpeed;

			// not using a loop region so just set the new sound params
			if (loopEnd == loopStart) {
				p.m_StartSecond = basicSongStart;
				p.m_LengthSeconds =
				  fSecondsToStartTransitioningOut - p.m_StartSecond;
			} else {
				// using a loop region, check to see if either end is outside
				// the new song length
				if (loopStart < basicSongStart) {
					loopStart = basicSongStart;
					p.m_StartSecond = basicSongStart;
				}
				if (loopEnd > fSecondsToStartTransitioningOut) {
					loopEnd = fSecondsToStartTransitioningOut;
					p.m_LengthSeconds =
					  fSecondsToStartTransitioningOut - p.m_StartSecond;
				}
			}
			RageTimer tm;
			const auto fSeconds =
			  m_pSoundMusic->GetPositionSeconds(nullptr, &tm);
			if (fSeconds > fSecondsToStartTransitioningOut ||
				fSeconds < p.m_StartSecond) {
				// i want to make sure things are done in a very particular
				// order
				m_pSoundMusic->SetParams(p);
				SOUND->SetSoundPosition(m_pSoundMusic, p.m_StartSecond);
			} else {
				m_pSoundMusic->SetParams(p);
			}

			// let the theme know we just changed so many things internally
			MESSAGEMAN->Broadcast("PracticeModeReload");
			return true;
		}
	}

	return ScreenGameplay::Input(input);
}

void
ScreenGameplayPractice::Update(const float fDeltaTime)
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

	m_vPlayerInfo.m_SoundEffectControl.Update(fDeltaTime);

	{
		const auto fSpeed = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		auto p = m_pSoundMusic->GetParams();
		if (std::fabs(p.m_fSpeed - fSpeed) > 0.01F && fSpeed >= 0.0F) {
			p.m_fSpeed = fSpeed;
			m_pSoundMusic->SetParams(p);
		}
	}

	// If we are using a loop region, check if the music looped
	// If it did, reset the notedata.
	if (!m_Out.IsTransitioning() && loopStart != loopEnd &&
		GAMESTATE->m_Position.m_fMusicSeconds + 0.1F < lastReportedSeconds) {
		if (!m_GiveUpTimer.IsZero()) {
			return;
		}

		auto* const td = GAMESTATE->m_pCurSteps->GetTimingData();
		const auto startBeat = td->GetBeatFromElapsedTime(loopStart);
		const auto endBeat = td->GetBeatFromElapsedTime(loopEnd);

		const auto rowStart = BeatToNoteRow(startBeat);
		const auto rowEnd = BeatToNoteRow(endBeat);

		if (rowStart < rowEnd) {
			SetupNoteDataFromRow(GAMESTATE->m_pCurSteps, rowStart, rowEnd);
		}
		if (PREFSMAN->m_bEasterEggs) {
			m_Toasty.Reset();
		}

		// Reset the wife/judge counter related visible stuff
		auto* pl = dynamic_cast<PlayerPractice*>(m_vPlayerInfo.m_pPlayer);
		ASSERT_M(pl != nullptr,
				 "Dynamic cast in ScreenGameplayPractice::Update failed.");
		pl->PositionReset();
	}
	lastReportedSeconds = GAMESTATE->m_Position.m_fMusicSeconds;

	switch (m_DancingState) {
		case STATE_DANCING: {
			// Handle the "give up" timer
			// except hard code it to fire after 1 second
			// instead of checking metrics
			// because we want to exit fast on demand

			const auto bGiveUpTimerFired =
			  !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > 1.F;
			m_gave_up = bGiveUpTimerFired;

			if (bGiveUpTimerFired) {
				m_vPlayerInfo.GetPlayerStageStats()->gaveuplikeadumbass = true;
				m_vPlayerInfo.GetPlayerStageStats()->m_bDisqualified = true;
				Locator::getLogger()->info("Exited Practice Mode to Evaluation");
				this->PostScreenMessage(SM_LeaveGameplay, 0);
				return;
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

void
ScreenGameplayPractice::SetupNoteDataFromRow(Steps* pSteps,
											 int minRow,
											 int maxRow)
{
	NoteData originalNoteData;
	pSteps->GetNoteData(originalNoteData);

	const auto* pStyle = GAMESTATE->GetCurrentStyle(m_vPlayerInfo.m_pn);
	NoteData ndTransformed;
	pStyle->GetTransformedNoteDataForStyle(
	  m_vPlayerInfo.GetStepsAndTrailIndex(), originalNoteData, ndTransformed);

	m_vPlayerInfo.GetPlayerState()->Update(0);

	NoteDataUtil::RemoveAllButRange(ndTransformed, minRow, maxRow);

	// load player
	{
		m_vPlayerInfo.m_NoteData = ndTransformed;
		NoteDataUtil::RemoveAllTapsOfType(m_vPlayerInfo.m_NoteData,
										  TapNoteType_AutoKeysound);
		ReloadPlayer();
	}

	// load auto keysounds
	{
		auto nd = ndTransformed;
		NoteDataUtil::RemoveAllTapsExceptForType(nd, TapNoteType_AutoKeysound);
		m_AutoKeysounds.Load(m_vPlayerInfo.GetStepsAndTrailIndex(), nd);
	}

	{
		std::string sType;
		switch (GAMESTATE->m_SongOptions.GetCurrent().m_SoundEffectType) {
			case SoundEffectType_Off:
				sType = "SoundEffectControl_Off";
				break;
			case SoundEffectType_Speed:
				sType = "SoundEffectControl_Speed";
				break;
			case SoundEffectType_Pitch:
				sType = "SoundEffectControl_Pitch";
				break;
			default:
				break;
		}

		m_vPlayerInfo.m_SoundEffectControl.Load(
		  sType, m_vPlayerInfo.GetPlayerState(), &m_vPlayerInfo.m_NoteData);
	}
}

void
ScreenGameplayPractice::TogglePause()
{
	// True if we were paused before now
	const auto oldPause = GAMESTATE->GetPaused();
	// True if we are becoming paused
	const auto newPause = !GAMESTATE->GetPaused();

	if (oldPause) {
		const auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		m_pSoundMusic->Stop();

		RageTimer tm;
		const auto fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);

		float fSecondsToStartFadingOutMusic;
		float fSecondsToStartTransitioningOut;
		GetMusicEndTiming(fSecondsToStartFadingOutMusic,
						  fSecondsToStartTransitioningOut);

		// Restart the music with the proper params
		auto p = m_pSoundMusic->GetParams();
		p.m_StartSecond = fSeconds - 0.01F;
		p.m_fSpeed = rate;
		// If using a loop region, use the loop params instead
		if (loopStart != loopEnd) {
			p.m_fFadeOutSeconds = 1.F;
			p.m_LengthSeconds = loopEnd + 5.F - loopStart;
			p.StopMode = RageSoundParams::M_LOOP;
		} else {
			p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
			p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
								MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
			p.StopMode = RageSoundParams::M_CONTINUE;
		}
		p.m_bAccurateSync = true;

		// Go
		m_pSoundMusic->Play(false, &p);

		// To force the music to actually loop like it should, need to do this
		// after starting
		if (loopStart != loopEnd) {
			p.m_StartSecond = loopStart - 2.F;
			m_pSoundMusic->SetParams(p);
		}
	}

	m_pSoundMusic->Pause(newPause);
	GAMESTATE->SetPaused(newPause);
}

void
ScreenGameplayPractice::SetSongPosition(float newSongPositionSeconds,
										float noteDelay,
										bool hardSeek,
										bool unpause)
{
	const auto isPaused = GAMESTATE->GetPaused();
	auto p = m_pSoundMusic->GetParams();

	// Letting this execute will freeze the music most of the time and thats bad
	if (loopStart != loopEnd && newSongPositionSeconds > loopEnd) {
		return;
	}

	// If paused, we need to move fast so dont use slow seeking
	// but if we want to hard seek, we dont care about speed
	p.m_bAccurateSync = !isPaused || hardSeek;
	m_pSoundMusic->SetParams(p);

	// realign mp3 files by seeking backwards to force a full reseek, then
	// seeking forward to finish the job
	if (hardSeek &&
		newSongPositionSeconds > GAMESTATE->m_Position.m_fMusicSeconds) {
		SOUND->SetSoundPosition(m_pSoundMusic,
								GAMESTATE->m_Position.m_fMusicSeconds - 0.01F);
	}

	// Set the final position
	SOUND->SetSoundPosition(m_pSoundMusic, newSongPositionSeconds - noteDelay);
	UpdateSongPosition();

	// Unpause the music if we want it unpaused
	if (unpause && isPaused) {
		m_pSoundMusic->Pause(false);
		GAMESTATE->SetPaused(false);
	}

	// Restart the notedata for the row we just moved to until the end of the
	// file
	Steps* pSteps = GAMESTATE->m_pCurSteps;
	auto* const pTiming = pSteps->GetTimingData();
	const auto fNotesBeat =
	  pTiming->GetBeatFromElapsedTime(newSongPositionSeconds);
	const auto rowNow = BeatToNoteRow(fNotesBeat);
	lastReportedSeconds = newSongPositionSeconds;

	// When using a loop region, just keep the loaded Notedata in the region
	if (loopStart != loopEnd) {
		const auto endBeat = pTiming->GetBeatFromElapsedTime(loopEnd);
		const auto rowEnd = BeatToNoteRow(endBeat);
		const auto startBeat = pTiming->GetBeatFromElapsedTime(loopStart);
		const auto rowStart = BeatToNoteRow(startBeat);
		const auto rowUsed = std::max(rowStart, rowNow);
		// Assert crash if this check isn't done
		if (rowUsed < rowEnd) {
			SetupNoteDataFromRow(pSteps, rowUsed, rowEnd);
		}
	} else {
		SetupNoteDataFromRow(pSteps, rowNow);
	}

	// Reset the wife/judge counter related visible stuff
	auto* pl = dynamic_cast<PlayerPractice*>(m_vPlayerInfo.m_pPlayer);
	ASSERT_M(pl != nullptr,
			 "Dynamic cast in ScreenGameplayPractice::SetSongPosition failed.");
	pl->RenderAllNotesIgnoreScores();
	pl->PositionReset();

	if (PREFSMAN->m_bEasterEggs) {
		m_Toasty.Reset();
	}

	// just having a message we can respond to directly is probably the best way
	// to reset lua elements
	MESSAGEMAN->Broadcast("PracticeModeReset");
}

auto
ScreenGameplayPractice::AddToRate(float amountAdded) -> float
{
	const auto rate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	const auto newRate = std::floor((rate + amountAdded) * 100 + 0.5) / 100;

	// Rates outside of this range may crash
	// Use 0.25 because of floating point errors...
	if (newRate <= 0.25F || newRate > 3.F) {
		return rate;
	}

	RageTimer tm;
	const auto fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);

	float fSecondsToStartFadingOutMusic;
	float fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	// Set Music params using new rate
	const auto rate_plus = static_cast<float>(newRate);
	RageSoundParams p;
	p.m_fSpeed = rate_plus;
	GAMESTATE->m_SongOptions.GetSong().m_fMusicRate = rate_plus;
	GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate = rate_plus;
	GAMESTATE->m_SongOptions.GetPreferred().m_fMusicRate = rate_plus;

	// If using loop region, also consider the loop params
	if (loopStart != loopEnd) {
		p.m_StartSecond = loopStart - 2.F;
		p.m_fFadeOutSeconds = 1.F;
		p.m_LengthSeconds = loopEnd + 5.F - loopStart;
		p.StopMode = RageSoundParams::M_LOOP;
	} else {
		p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
		p.m_LengthSeconds = fSecondsToStartFadingOutMusic +
							MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
		p.StopMode = RageSoundParams::M_CONTINUE;
	}
	p.m_bAccurateSync = true;

	// Apply param (rate) changes to the running Music
	m_pSoundMusic->SetParams(p);
	GAMESTATE->m_Position.m_fMusicSeconds = fSeconds;

	// Tell the theme we changed the rate
	MESSAGEMAN->Broadcast("CurrentRateChanged");
	return static_cast<float>(rate_plus);
}

void
ScreenGameplayPractice::SetLoopRegion(float start, float end)
{
	// Don't allow a loop region that is too negative.
	// Some songs actually do start in negative time, so be lenient.
	if (start < -2 || end < -2) {
		return;
	}

	loopStart = start;
	loopEnd = end;

	// Tell the Music that it should loop on a given region instead
	// 2 seconds are removed from the start for "intro"
	// 5 seconds are added to the end for "outro"
	// No notedata will occupy that space.
	auto p = m_pSoundMusic->GetParams();
	p.m_StartSecond = start - 2.F;
	p.m_fFadeOutSeconds = 1.F;
	p.m_LengthSeconds = end + 5.F - start;
	p.StopMode = RageSoundParams::M_LOOP;

	// We dont reset notedata here because that could get repetitive and also be
	// annoying to users or cause other slowdowns

	m_pSoundMusic->SetParams(p);
}

void
ScreenGameplayPractice::ResetLoopRegion()
{
	// magic number defaults for loop region bounds
	loopStart = ARBITRARY_MIN_GAMEPLAY_NUMBER;
	loopEnd = ARBITRARY_MIN_GAMEPLAY_NUMBER;

	// Reload notedata for the entire file starting at current row
	RageTimer tm;
	const auto fSeconds = m_pSoundMusic->GetPositionSeconds(nullptr, &tm);
	auto* const td = GAMESTATE->m_pCurSteps->GetTimingData();
	const auto startBeat = td->GetBeatFromElapsedTime(fSeconds);
	const auto rowNow = BeatToNoteRow(startBeat);
	SetupNoteDataFromRow(GAMESTATE->m_pCurSteps, rowNow);

	float fSecondsToStartFadingOutMusic;
	float fSecondsToStartTransitioningOut;
	GetMusicEndTiming(fSecondsToStartFadingOutMusic,
					  fSecondsToStartTransitioningOut);

	// Reapply the standard Music parameters
	auto p = m_pSoundMusic->GetParams();
	p.m_StartSecond = GAMESTATE->m_pCurSong->GetFirstSecond() *
					  GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	p.m_fFadeOutSeconds = MUSIC_FADE_OUT_SECONDS;
	p.m_LengthSeconds =
	  fSecondsToStartFadingOutMusic + MUSIC_FADE_OUT_SECONDS - p.m_StartSecond;
	p.StopMode = RageSoundParams::M_CONTINUE;

	m_pSoundMusic->SetParams(p);
}

class LunaScreenGameplayPractice : public Luna<ScreenGameplayPractice>
{
  public:
	static auto SetSongPosition(T* p, lua_State* L) -> int
	{
		const auto position = FArg(1);
		const auto delay = FArg(2);
		const auto hardseek = BArg(3);
		p->SetSongPosition(position, delay, hardseek);
		return 0;
	}

	static auto SetSongPositionAndUnpause(T* p, lua_State* L) -> int
	{
		const auto position = FArg(1);
		const auto delay = FArg(2);
		const auto hardseek = BArg(3);
		p->SetSongPosition(position, delay, hardseek, true);
		return 0;
	}

	static auto AddToRate(T* p, lua_State* L) -> int
	{
		const auto rate = FArg(1);
		lua_pushnumber(L, p->AddToRate(rate));
		return 1;
	}

	static auto TogglePause(T* p, lua_State * /*L*/) -> int
	{
		p->TogglePause();
		return 0;
	}

	static auto SetLoopRegion(T* p, lua_State* L) -> int
	{
		const auto begin = FArg(1);
		const auto end = FArg(2);
		p->SetLoopRegion(begin, end);
		return 0;
	}

	static auto ResetLoopRegion(T* p, lua_State * /*L*/) -> int
	{
		p->ResetLoopRegion();
		return 0;
	}

	LunaScreenGameplayPractice()
	{
		ADD_METHOD(SetSongPosition);
		ADD_METHOD(SetSongPositionAndUnpause);
		ADD_METHOD(AddToRate);
		ADD_METHOD(TogglePause);
		ADD_METHOD(SetLoopRegion);
		ADD_METHOD(ResetLoopRegion);
	}
};

LUA_REGISTER_DERIVED_CLASS(ScreenGameplayPractice, ScreenGameplay)
