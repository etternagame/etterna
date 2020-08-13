#include "Etterna/Globals/global.h"
#include "CommonMetrics.h"
#include "Etterna/Singletons/GameState.h"
#include "GameplayAssist.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "PlayerState.h"
#include "RageUtil/Sound/RageSoundManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Singletons/ThemeManager.h"

#include <algorithm>

void
GameplayAssist::Init()
{
	m_soundAssistClap.Load(THEME->GetPathS("GameplayAssist", "clap"), true);
	m_soundAssistMetronomeMeasure.Load(
	  THEME->GetPathS("GameplayAssist", "metronome measure"), true);
	m_soundAssistMetronomeBeat.Load(
	  THEME->GetPathS("GameplayAssist", "metronome beat"), true);
}

void
GameplayAssist::PlayTicks(const NoteData& nd, const PlayerState* ps)
{
	auto bClap = GAMESTATE->m_SongOptions.GetCurrent().m_bAssistClap;
	auto bMetronome = GAMESTATE->m_SongOptions.GetCurrent().m_bAssistMetronome;
	if (!bClap && !bMetronome)
		return;

	// don't play sounds for dead players
	if (ps->m_HealthState == HealthState_Dead)
		return;

	/* Sound cards have a latency between when a sample is Play()ed and when the
	 * sound will start coming out the speaker.  Compensate for this by boosting
	 * fPositionSeconds ahead.  This is just to make sure that we request the
	 * sound early enough for it to come out on time; the actual precise timing
	 * is handled by SetStartTime. */
	auto& position = GAMESTATE->m_Position;
	auto fPositionSeconds = position.m_fMusicSeconds;

	// float fPositionSeconds = GAMESTATE->m_Position.m_fMusicSeconds;
	fPositionSeconds += SOUNDMAN->GetPlayLatency() +
						static_cast<float>(CommonMetrics::TICK_EARLY_SECONDS) +
						0.250f;
	const auto& timing = *GAMESTATE->m_pCurSteps->GetTimingData();
	const auto fSongBeat =
	  timing.GetBeatFromElapsedTimeNoOffset(fPositionSeconds);

	const auto iSongRow = std::max(0, BeatToNoteRow(fSongBeat));
	static auto iRowLastCrossed = -1;
	if (iSongRow < iRowLastCrossed)
		iRowLastCrossed = iSongRow;

	if (bClap) {
		auto iClapRow = -1;
		// for each index we crossed since the last update:
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
		  nd, r, iRowLastCrossed + 1, iSongRow + 1)
		if (nd.IsThereATapOrHoldHeadAtRow(r))
			iClapRow = r;

		if (iClapRow != -1 && timing.IsJudgableAtRow(iClapRow)) {
			const auto fTickBeat = NoteRowToBeat(iClapRow);
			const auto fTickSecond = timing.WhereUAtBroNoOffset(fTickBeat);
			auto fSecondsUntil = fTickSecond - position.m_fMusicSeconds;
			fSecondsUntil /= GAMESTATE->m_SongOptions.GetCurrent()
							   .m_fMusicRate; /* 2x music rate means the time
												 until the tick is halved */

			RageSoundParams p;
			p.m_StartTime =
			  position.m_LastBeatUpdate +
			  (fSecondsUntil -
			   static_cast<float>(CommonMetrics::TICK_EARLY_SECONDS));
			m_soundAssistClap.Play(false, &p);
		}
	}

	if (bMetronome && iRowLastCrossed != -1) {
		// iRowLastCrossed+1, iSongRow+1

		int iLastCrossedMeasureIndex;
		int iLastCrossedBeatIndex;
		int iLastCrossedRowsRemainder;
		timing.NoteRowToMeasureAndBeat(iRowLastCrossed,
									   iLastCrossedMeasureIndex,
									   iLastCrossedBeatIndex,
									   iLastCrossedRowsRemainder);

		int iCurrentMeasureIndex;
		int iCurrentBeatIndex;
		int iCurrentRowsRemainder;
		timing.NoteRowToMeasureAndBeat(iSongRow,
									   iCurrentMeasureIndex,
									   iCurrentBeatIndex,
									   iCurrentRowsRemainder);

		auto iMetronomeRow = -1;
		auto bIsMeasure = false;

		if (iLastCrossedMeasureIndex != iCurrentMeasureIndex ||
			iLastCrossedBeatIndex != iCurrentBeatIndex) {
			iMetronomeRow = iSongRow - iCurrentRowsRemainder;
			bIsMeasure = iCurrentBeatIndex == 0 && iCurrentRowsRemainder == 0;
		}

		if (iMetronomeRow != -1) {
			const auto fTickBeat = NoteRowToBeat(iMetronomeRow);
			const auto fTickSecond = timing.WhereUAtBroNoOffset(fTickBeat);
			auto fSecondsUntil = fTickSecond - position.m_fMusicSeconds;
			fSecondsUntil /= GAMESTATE->m_SongOptions.GetCurrent()
							   .m_fMusicRate; /* 2x music rate means the time
												 until the tick is halved */

			RageSoundParams p;
			p.m_StartTime =
			  position.m_LastBeatUpdate +
			  (fSecondsUntil -
			   static_cast<float>(CommonMetrics::TICK_EARLY_SECONDS));
			if (bIsMeasure)
				m_soundAssistMetronomeMeasure.Play(false, &p);
			else
				m_soundAssistMetronomeBeat.Play(false, &p);
		}
	}

	iRowLastCrossed = iSongRow;
}

void
GameplayAssist::StopPlaying()
{
	m_soundAssistClap.StopPlaying();
	m_soundAssistMetronomeMeasure.StopPlaying();
	m_soundAssistMetronomeBeat.StopPlaying();
}
