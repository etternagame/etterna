/*
 * AdjustSync defines a method for fixing the sync.
 *
 * The first method adjusts either the song or the machine by the
 * average offset of the user's steps.  In other words, if the user
 * averages to step early by 10 ms, either the song or the global
 * offset is adjusted by 10 ms to compensate for that.  These
 * adjustments only require a small set of data, so this method
 * updates the offset while the song is playing.
 *
 */

#include "Etterna/Globals/global.h"
#include "AdjustSync.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Etterna/Singletons/ScreenManager.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Songs/SongOptions.h"
#include "Etterna/Singletons/SongManager.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"

std::vector<TimingData> AdjustSync::s_vpTimingDataOriginal;
float AdjustSync::s_fGlobalOffsetSecondsOriginal = 0.0f;
int AdjustSync::s_iAutosyncOffsetSample = 0;
float AdjustSync::s_fAutosyncOffset[AdjustSync::OFFSET_SAMPLE_COUNT];
float AdjustSync::s_fStandardDeviation = 0.0f;

void
AdjustSync::ResetOriginalSyncData()
{
	s_vpTimingDataOriginal.clear();

	if (GAMESTATE->m_pCurSong) {
		s_vpTimingDataOriginal.push_back(GAMESTATE->m_pCurSong->m_SongTiming);
		const auto& vpSteps = GAMESTATE->m_pCurSong->GetAllSteps();
		for (auto& s : const_cast<std::vector<Steps*>&>(vpSteps)) {
			s_vpTimingDataOriginal.push_back(s->m_Timing);
		}
	} else {
		s_vpTimingDataOriginal.push_back(TimingData());
	}
	s_fGlobalOffsetSecondsOriginal = PREFSMAN->m_fGlobalOffsetSeconds;

	ResetAutosync();
}

void
AdjustSync::ResetAutosync()
{
	s_iAutosyncOffsetSample = 0;
}

bool
AdjustSync::IsSyncDataChanged()
{
	// Can't sync in course mode :joy_cat: -mina
	if (GAMESTATE->IsPlaylistCourse())
		return false;

	bool syncing = false;

	// global offset
	auto fOld = Quantize(AdjustSync::s_fGlobalOffsetSecondsOriginal, 0.001f);
	auto fNew = Quantize(PREFSMAN->m_fGlobalOffsetSeconds, 0.001f);
	auto fDelta = fNew - fOld;
	syncing |= fabsf(fDelta) > 0.0001f;

	// song offset
	if (GAMESTATE->m_pCurSong != nullptr) {
		auto& original = s_vpTimingDataOriginal[0];
		auto& testing = GAMESTATE->m_pCurSong->m_SongTiming;

		// the files should match. typically this is the case but sometimes that
		// just isnt true and we really dont want to let it happen
		if (original.m_sFile == testing.m_sFile) {
			auto fOld = Quantize(original.m_fBeat0OffsetInSeconds, 0.001f);
			auto fNew = Quantize(testing.m_fBeat0OffsetInSeconds, 0.001f);
			auto fDelta = fNew - fOld;
			syncing |= fabsf(fDelta) > 0.0001f;
		}
	}

	return syncing;
}

void
AdjustSync::SaveSyncChanges()
{
	/* TODO: Save all of the timing data changes.
	 * Luckily, only the song timing data needs comparing here. */
	if (GAMESTATE->m_pCurSong &&
		s_vpTimingDataOriginal[0] != GAMESTATE->m_pCurSong->m_SongTiming) {

		// Hack: Otherwise it doesnt work (files created are called /.sm and
		// /.ssc)
		auto tmp = GAMESTATE->m_pCurSong->m_SongTiming;
		GAMESTATE->m_pCurSong->ReloadFromSongDir();
		GAMESTATE->m_pCurSong->m_SongTiming = tmp;

		GAMESTATE->m_pCurSong->Save();
		GAMESTATE->m_pCurSong->ReloadFromSongDir();
	}
	if (s_fGlobalOffsetSecondsOriginal != PREFSMAN->m_fGlobalOffsetSeconds)
		PREFSMAN->SavePrefsToDisk();
	ResetOriginalSyncData();
	s_fStandardDeviation = 0.0f;
}

void
AdjustSync::RevertSyncChanges()
{
	PREFSMAN->m_fGlobalOffsetSeconds.Set(s_fGlobalOffsetSecondsOriginal);

	// The first one is ALWAYS the song timing.
	GAMESTATE->m_pCurSong->m_SongTiming = s_vpTimingDataOriginal[0];

	unsigned location = 1;
	const auto& vpSteps = GAMESTATE->m_pCurSong->GetAllSteps();
	for (auto& s : const_cast<std::vector<Steps*>&>(vpSteps)) {
		s->m_Timing = s_vpTimingDataOriginal[location];
		location++;
	}

	ResetOriginalSyncData();
	s_fStandardDeviation = 0.0f;
}

static LocalizedString AUTOSYNC_CORRECTION_APPLIED(
  "AdjustSync",
  "Autosync: Correction applied.");
static LocalizedString AUTOSYNC_CORRECTION_NOT_APPLIED(
  "AdjustSync",
  "Autosync: Correction NOT applied. Deviation too high.");
void
AdjustSync::HandleAutosync(float fNoteOffBySeconds, float fStepTime)
{
	auto type = GAMESTATE->m_SongOptions.GetCurrent().m_AutosyncType;
	switch (type) {
		case AutosyncType_Off:
			return;
		case AutosyncType_Machine:
		case AutosyncType_Song: {
			s_fAutosyncOffset[s_iAutosyncOffsetSample] = fNoteOffBySeconds;
			++s_iAutosyncOffsetSample;

			if (s_iAutosyncOffsetSample < OFFSET_SAMPLE_COUNT)
				break; // need more

			AutosyncOffset();
			break;
		}
		default:
			FAIL_M(ssprintf("Invalid autosync type: %i", type));
	}
}

void
AdjustSync::AutosyncOffset()
{
	const auto mean =
	  calc_mean(s_fAutosyncOffset, s_fAutosyncOffset + OFFSET_SAMPLE_COUNT);
	const auto stddev =
	  calc_stddev(s_fAutosyncOffset, s_fAutosyncOffset + OFFSET_SAMPLE_COUNT);

	auto type = GAMESTATE->m_SongOptions.GetCurrent().m_AutosyncType;

	if (stddev < .03f) // If they stepped with less than .03 error
	{
		switch (type) {
			case AutosyncType_Song: {
				GAMESTATE->m_pCurSong->m_SongTiming.m_fBeat0OffsetInSeconds +=
				  mean;
				GAMESTATE->m_pCurSong->m_SongTiming.PrepareLookup();
				const auto& vpSteps = GAMESTATE->m_pCurSong->GetAllSteps();
				for (auto& s : const_cast<std::vector<Steps*>&>(vpSteps)) {
					// Empty TimingData means it's inherited
					// from the song and is already changed.
					if (s->m_Timing.empty())
						continue;
					s->m_Timing.m_fBeat0OffsetInSeconds += mean;
					s->m_Timing.PrepareLookup();
				}
				break;
			}
			case AutosyncType_Machine:
				// Step timing is not needed for this operation.
				PREFSMAN->m_fGlobalOffsetSeconds.Set(
				  PREFSMAN->m_fGlobalOffsetSeconds + mean);
				break;
			default:
				FAIL_M(ssprintf("Invalid autosync type: %i", type));
		}

		SCREENMAN->SystemMessage(AUTOSYNC_CORRECTION_APPLIED.GetValue());
	} else {
		SCREENMAN->SystemMessage(AUTOSYNC_CORRECTION_NOT_APPLIED.GetValue());
	}

	s_iAutosyncOffsetSample = 0;
	s_fStandardDeviation = stddev;
}

static LocalizedString EARLIER("AdjustSync", "earlier");
static LocalizedString LATER("AdjustSync", "later");
static LocalizedString GLOBAL_OFFSET_FROM(
  "AdjustSync",
  "Global Offset from %+.3f to %+.3f (notes %s)");
// We need to limit the length of lines so each one fits on one line of the SM
// console. The tempo and stop change message can get very long in a complicated
// song, and at a low resolution, the keep/revert menu would be pushed off the
// bottom of the screen if we didn't limit the length of the message.  Keeping
// the lines short lets us fit more information on the screen.
static LocalizedString SONG_OFFSET_FROM(
  "AdjustSync",
  "Song offset from %+.3f to %+.3f (notes %s)");

void
AdjustSync::GetSyncChangeTextGlobal(std::vector<std::string>& vsAddTo)
{
	{
		auto fOld =
		  Quantize(AdjustSync::s_fGlobalOffsetSecondsOriginal, 0.001f);
		auto fNew = Quantize(PREFSMAN->m_fGlobalOffsetSeconds, 0.001f);
		auto fDelta = fNew - fOld;

		if (fabsf(fDelta) > 0.0001f) {
			vsAddTo.push_back(
			  ssprintf(GLOBAL_OFFSET_FROM.GetValue(),
					   fOld,
					   fNew,
					   (fDelta > 0 ? EARLIER : LATER).GetValue().c_str()));
		}
	}
}

void
AdjustSync::GetSyncChangeTextSong(std::vector<std::string>& vsAddTo)
{
	if (!GAMESTATE->isplaylistcourse && GAMESTATE->m_pCurSong.Get()) {
		auto& original = s_vpTimingDataOriginal[0];
		auto& testing = GAMESTATE->m_pCurSong->m_SongTiming;

		// the files should match. typically this is the case but sometimes that
		// just isnt true and we really dont want to let it happen
		if (original.m_sFile != testing.m_sFile)
			return;

		{
			auto fOld = Quantize(original.m_fBeat0OffsetInSeconds, 0.001f);
			auto fNew = Quantize(testing.m_fBeat0OffsetInSeconds, 0.001f);
			auto fDelta = fNew - fOld;

			if (fabsf(fDelta) > 0.0001f) {
				vsAddTo.push_back(
				  ssprintf(SONG_OFFSET_FROM.GetValue(),
						   fOld,
						   fNew,
						   (fDelta > 0 ? EARLIER : LATER).GetValue().c_str()));
			}
		}
	}
}
