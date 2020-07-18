#include "Etterna/Globals/global.h"
#include "ArrowEffects.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Singletons/GameState.h"
#include "NoteDisplay.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Misc/RageTimer.h"
#include "Etterna/Models/Misc/ScreenDimensions.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Models/StepsAndStyles/Style.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
#include "Etterna/Models/Songs/SongOptions.h"

#include <algorithm>

static ThemeMetric<float> ARROW_SPACING("ArrowEffects", "ArrowSpacing");
static ThemeMetric<bool> QUANTIZE_ARROW_Y("ArrowEffects",
										  "QuantizeArrowYPosition");
static ThemeMetric<bool> HIDDEN_SUDDEN_PAST_RECEPTOR(
  "ArrowEffects",
  "DrawHiddenNotesAfterReceptor");

/* For better or for worse, allow the themes to modify the various mod
 * effects for the different mods. In general, it is recommended to not
 * edit the default values and instead use percentage mods when changes
 * are wanted. Still, the option is available for those that want it.
 *
 * Is this a good idea? We'll find out. -aj & Wolfman2000 */
static ThemeMetric<float> BLINK_MOD_FREQUENCY("ArrowEffects",
											  "BlinkModFrequency");
static ThemeMetric<float> BOOST_MOD_MIN_CLAMP("ArrowEffects",
											  "BoostModMinClamp");
static ThemeMetric<float> BOOST_MOD_MAX_CLAMP("ArrowEffects",
											  "BoostModMaxClamp");
static ThemeMetric<float> BRAKE_MOD_MIN_CLAMP("ArrowEffects",
											  "BrakeModMinClamp");
static ThemeMetric<float> BRAKE_MOD_MAX_CLAMP("ArrowEffects",
											  "BrakeModMaxClamp");
static ThemeMetric<float> WAVE_MOD_MAGNITUDE("ArrowEffects",
											 "WaveModMagnitude");
static ThemeMetric<float> WAVE_MOD_HEIGHT("ArrowEffects", "WaveModHeight");
static ThemeMetric<float> BOOMERANG_PEAK_PERCENTAGE("ArrowEffects",
													"BoomerangPeakPercentage");
static ThemeMetric<float> EXPAND_MULTIPLIER_FREQUENCY(
  "ArrowEffects",
  "ExpandMultiplierFrequency");
static ThemeMetric<float> EXPAND_MULTIPLIER_SCALE_FROM_LOW(
  "ArrowEffects",
  "ExpandMultiplierScaleFromLow");
static ThemeMetric<float> EXPAND_MULTIPLIER_SCALE_FROM_HIGH(
  "ArrowEffects",
  "ExpandMultiplierScaleFromHigh");
static ThemeMetric<float> EXPAND_MULTIPLIER_SCALE_TO_LOW(
  "ArrowEffects",
  "ExpandMultiplierScaleToLow");
static ThemeMetric<float> EXPAND_MULTIPLIER_SCALE_TO_HIGH(
  "ArrowEffects",
  "ExpandMultiplierScaleToHigh");
static ThemeMetric<float> EXPAND_SPEED_SCALE_FROM_LOW(
  "ArrowEffects",
  "ExpandSpeedScaleFromLow");
static ThemeMetric<float> EXPAND_SPEED_SCALE_FROM_HIGH(
  "ArrowEffects",
  "ExpandSpeedScaleFromHigh");
static ThemeMetric<float> EXPAND_SPEED_SCALE_TO_LOW("ArrowEffects",
													"ExpandSpeedScaleToLow");
static ThemeMetric<float> TIPSY_TIMER_FREQUENCY("ArrowEffects",
												"TipsyTimerFrequency");
static ThemeMetric<float> TIPSY_COLUMN_FREQUENCY("ArrowEffects",
												 "TipsyColumnFrequency");
static ThemeMetric<float> TIPSY_ARROW_MAGNITUDE("ArrowEffects",
												"TipsyArrowMagnitude");
static ThemeMetric<float> TIPSY_OFFSET_TIMER_FREQUENCY(
  "ArrowEffects",
  "TipsyOffsetTimerFrequency");
static ThemeMetric<float> TIPSY_OFFSET_COLUMN_FREQUENCY(
  "ArrowEffects",
  "TipsyOffsetColumnFrequency");
static ThemeMetric<float> TIPSY_OFFSET_ARROW_MAGNITUDE(
  "ArrowEffects",
  "TipsyOffsetArrowMagnitude");
static ThemeMetric<float> TORNADO_POSITION_SCALE_TO_LOW(
  "ArrowEffects",
  "TornadoPositionScaleToLow");
static ThemeMetric<float> TORNADO_POSITION_SCALE_TO_HIGH(
  "ArrowEffects",
  "TornadoPositionScaleToHigh");
static ThemeMetric<float> TORNADO_OFFSET_FREQUENCY("ArrowEffects",
												   "TornadoOffsetFrequency");
static ThemeMetric<float> TORNADO_OFFSET_SCALE_FROM_LOW(
  "ArrowEffects",
  "TornadoOffsetScaleFromLow");
static ThemeMetric<float> TORNADO_OFFSET_SCALE_FROM_HIGH(
  "ArrowEffects",
  "TornadoOffsetScaleFromHigh");
static ThemeMetric<float> DRUNK_COLUMN_FREQUENCY("ArrowEffects",
												 "DrunkColumnFrequency");
static ThemeMetric<float> DRUNK_OFFSET_FREQUENCY("ArrowEffects",
												 "DrunkOffsetFrequency");
static ThemeMetric<float> DRUNK_ARROW_MAGNITUDE("ArrowEffects",
												"DrunkArrowMagnitude");
static ThemeMetric<float> BEAT_OFFSET_HEIGHT("ArrowEffects",
											 "BeatOffsetHeight");
static ThemeMetric<float> BEAT_PI_HEIGHT("ArrowEffects", "BeatPIHeight");
static ThemeMetric<float> TINY_PERCENT_BASE("ArrowEffects", "TinyPercentBase");
static ThemeMetric<float> TINY_PERCENT_GATE("ArrowEffects", "TinyPercentGate");
static ThemeMetric<bool> DIZZY_HOLD_HEADS("ArrowEffects", "DizzyHoldHeads");

static const PlayerOptions* curr_options = nullptr;

float
ArrowGetPercentVisible(float fYPosWithoutReverse);

static float
GetNoteFieldHeight()
{
	return SCREEN_HEIGHT + fabsf(curr_options->m_fPerspectiveTilt) * 200;
}

namespace {
struct PerPlayerData
{
	float m_fMinTornadoX[MAX_COLS_PER_PLAYER];
	float m_fMaxTornadoX[MAX_COLS_PER_PLAYER];
	float m_fInvertDistance[MAX_COLS_PER_PLAYER];
	float m_tipsy_result[MAX_COLS_PER_PLAYER];
	float m_tipsy_offset_result[MAX_COLS_PER_PLAYER];
	float m_fBeatFactor;
	float m_fExpandSeconds;
};
PerPlayerData g_EffectData;
} // namespace;

void
ArrowEffects::Update()
{
	static float fLastTime = 0;
	const auto fTime = RageTimer::GetTimeSinceStart();

	const auto* const pStyle = GAMESTATE->GetCurrentStyle(PLAYER_1);
	const auto* const pCols = pStyle->m_ColumnInfo;
	const auto& position = GAMESTATE->m_Position;
	const auto field_zoom = GAMESTATE->m_pPlayerState->m_NotefieldZoom;
	const float* effects =
	  GAMESTATE->m_pPlayerState->m_PlayerOptions.GetCurrent().m_fEffects;

	auto& data = g_EffectData;

	if (!position.m_bFreeze || !position.m_bDelay) {
		data.m_fExpandSeconds += fTime - fLastTime;
		data.m_fExpandSeconds = fmodf(data.m_fExpandSeconds, PI * 2);
	}

	// Update Tornado
	if (effects[PlayerOptions::EFFECT_TORNADO] != 0) {
		for (auto iColNum = 0; iColNum < MAX_COLS_PER_PLAYER; ++iColNum) {
			// TRICKY: Tornado is very unplayable in doubles, so use a
			// smaller tornado width if there are many columns

			/* the below makes an assumption for dance mode.
			 * perhaps check if we are actually playing on singles without,
			 * say more than 6 columns. That would exclude IIDX, pop'n, and
			 * techno-8, all of which would be very hectic.
			 * certain non-singles modes (like halfdoubles 6cols)
			 * could possibly have tornado enabled.
			 * let's also take default resolution (640x480) into mind. -aj
			 */
			const auto bWideField = pStyle->m_iColsPerPlayer > 4;
			const auto iTornadoWidth = bWideField ? 2 : 3;

			auto iStartCol = iColNum - iTornadoWidth;
			auto iEndCol = iColNum + iTornadoWidth;
			CLAMP(iStartCol, 0, pStyle->m_iColsPerPlayer - 1);
			CLAMP(iEndCol, 0, pStyle->m_iColsPerPlayer - 1);

			data.m_fMinTornadoX[iColNum] = FLT_MAX;
			data.m_fMaxTornadoX[iColNum] = FLT_MIN;

			for (auto i = iStartCol; i <= iEndCol; i++) {
				data.m_fMinTornadoX[iColNum] = std::min(
				  data.m_fMinTornadoX[iColNum], pCols[i].fXOffset * field_zoom);
				data.m_fMaxTornadoX[iColNum] = std::max(
				  data.m_fMaxTornadoX[iColNum], pCols[i].fXOffset * field_zoom);
			}
		}
	}

	// Update Invert
	if (effects[PlayerOptions::EFFECT_INVERT] != 0) {
		for (auto iColNum = 0; iColNum < MAX_COLS_PER_PLAYER; ++iColNum) {
			const auto iNumCols = pStyle->m_iColsPerPlayer;
			const auto iNumSides = 1;
			const auto iNumColsPerSide = iNumCols / iNumSides;
			const auto iSideIndex = iColNum / iNumColsPerSide;
			const auto iColOnSide = iColNum % iNumColsPerSide;

			const auto iColLeftOfMiddle = (iNumColsPerSide - 1) / 2;
			const auto iColRightOfMiddle = (iNumColsPerSide + 1) / 2;

			auto iFirstColOnSide = -1;
			auto iLastColOnSide = -1;
			if (iColOnSide <= iColLeftOfMiddle) {
				iFirstColOnSide = 0;
				iLastColOnSide = iColLeftOfMiddle;
			} else if (iColOnSide >= iColRightOfMiddle) {
				iFirstColOnSide = iColRightOfMiddle;
				iLastColOnSide = iNumColsPerSide - 1;
			} else {
				iFirstColOnSide = iColOnSide / 2;
				iLastColOnSide = iColOnSide / 2;
			}

			// mirror
			int iNewColOnSide;
			if (iFirstColOnSide == iLastColOnSide)
				iNewColOnSide = 0;
			else
				iNewColOnSide = SCALE(iColOnSide,
									  iFirstColOnSide,
									  iLastColOnSide,
									  iLastColOnSide,
									  iFirstColOnSide);
			const auto iNewCol = iSideIndex * iNumColsPerSide + iNewColOnSide;

			const auto fOldPixelOffset = pCols[iColNum].fXOffset;
			const auto fNewPixelOffset = pCols[iNewCol].fXOffset;
			data.m_fInvertDistance[iColNum] = fNewPixelOffset - fOldPixelOffset;
		}
	}

	// Update Tipsy
	if (effects[PlayerOptions::EFFECT_TIPSY] != 0) {
		const auto time = RageTimer::GetTimeSinceStart();
		const auto time_times_timer = time * TIPSY_TIMER_FREQUENCY;
		const auto arrow_times_mag = ARROW_SIZE * TIPSY_ARROW_MAGNITUDE;
		const auto time_times_offset_timer =
		  time * TIPSY_OFFSET_TIMER_FREQUENCY;
		const auto arrow_times_offset_mag =
		  ARROW_SIZE * TIPSY_OFFSET_ARROW_MAGNITUDE;
		for (auto col = 0; col < MAX_COLS_PER_PLAYER; ++col) {
			data.m_tipsy_result[col] =
			  RageFastCos(time_times_timer + (col * TIPSY_COLUMN_FREQUENCY)) *
			  arrow_times_mag;
			data.m_tipsy_offset_result[col] =
			  RageFastCos(time_times_offset_timer +
						  (col * TIPSY_OFFSET_COLUMN_FREQUENCY)) *
			  arrow_times_offset_mag;
		}
	} else {
		for (auto& col : data.m_tipsy_result) {
			col = 0;
		}
	}

	// Update Beat
	if (effects[PlayerOptions::EFFECT_BEAT] != 0) {
		do {
			const auto fAccelTime = 0.2f, fTotalTime = 0.5f;
			auto fBeat = position.m_fSongBeatVisible + fAccelTime;

			const auto bEvenBeat = (static_cast<int>(fBeat) % 2) != 0;

			data.m_fBeatFactor = 0;
			if (fBeat < 0)
				break;

			// -100.2 -> -0.2 -> 0.2
			fBeat -= truncf(fBeat);
			fBeat += 1;
			fBeat -= truncf(fBeat);

			if (fBeat >= fTotalTime)
				break;

			if (fBeat < fAccelTime) {
				data.m_fBeatFactor = SCALE(fBeat, 0.0f, fAccelTime, 0.0f, 1.0f);
				data.m_fBeatFactor *= data.m_fBeatFactor;
			} else /* fBeat < fTotalTime */ {
				data.m_fBeatFactor =
				  SCALE(fBeat, fAccelTime, fTotalTime, 1.0f, 0.0f);
				data.m_fBeatFactor =
				  1 - (1 - data.m_fBeatFactor) * (1 - data.m_fBeatFactor);
			}

			if (bEvenBeat)
				data.m_fBeatFactor *= -1;
			data.m_fBeatFactor *= 20.0f;
		} while (false);
	}
	fLastTime = fTime;
}

void
ArrowEffects::SetCurrentOptions(const PlayerOptions* options)
{
	curr_options = options;
}

static float
GetDisplayedBeat(const PlayerState* pPlayerState, float beat)
{
	// do a binary search here
	const auto& data = pPlayerState->m_CacheDisplayedBeat;
	const int max = data.size() - 1;
	auto l = 0, r = max;
	while (l <= r) {
		const auto m = (l + r) / 2;
		if ((m == 0 || data[m].beat <= beat) &&
			(m == max || beat < data[m + 1].beat)) {
			return data[m].displayedBeat +
				   data[m].velocity * (beat - data[m].beat);
		}
		if (data[m].beat <= beat) {
			l = m + 1;
		} else {
			r = m - 1;
		}
	}
	return beat;
}

/* For visibility testing: if bAbsolute is false, random modifiers must return
 * the minimum possible scroll speed. */
float
ArrowEffects::GetYOffset(const PlayerState* pPlayerState,
						 int iCol,
						 float fNoteBeat,
						 float& fPeakYOffsetOut,
						 bool& bIsPastPeakOut,
						 bool bAbsolute)
{
	// Default values that are returned if boomerang is off.
	fPeakYOffsetOut = FLT_MAX;
	bIsPastPeakOut = true;

	float fYOffset = 0;
	const auto& position = GAMESTATE->m_Position;

	const auto fSongBeat = position.m_fSongBeatVisible;
	Steps* pCurSteps = GAMESTATE->m_pCurSteps;

	/* Usually, fTimeSpacing is 0 or 1, in which case we use entirely beat
	 * spacing or entirely time spacing (respectively). Occasionally, we tween
	 * between them. */
	if (curr_options->m_fTimeSpacing != 1.0f) {
		fYOffset = GetDisplayedBeat(pPlayerState, fNoteBeat) -
				   GetDisplayedBeat(pPlayerState, fSongBeat);
		fYOffset *= pCurSteps->GetTimingData()->GetDisplayedSpeedPercent(
		  position.m_fSongBeatVisible, position.m_fMusicSecondsVisible);
		fYOffset *= 1 - curr_options->m_fTimeSpacing;
	}

	if (curr_options->m_fTimeSpacing != 0.0f) {
		const auto fSongSeconds = GAMESTATE->m_Position.m_fMusicSecondsVisible;
		const auto fNoteSeconds =
		  pCurSteps->GetTimingData()->WhereUAtBro(fNoteBeat);
		const auto fSecondsUntilStep = fNoteSeconds - fSongSeconds;
		const auto fBPM = curr_options->m_fScrollBPM;
		const auto fBPS =
		  fBPM / 60.f / GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
		const auto fYOffsetTimeSpacing = fSecondsUntilStep * fBPS;
		fYOffset += fYOffsetTimeSpacing * curr_options->m_fTimeSpacing;
	}

	// TODO: If we allow noteskins to have metricable row spacing
	// (per issue 24), edit this to reflect that. -aj
	fYOffset *= ARROW_SPACING;

	// Factor in scroll speed
	auto fScrollSpeed = curr_options->m_fScrollSpeed;
	if (curr_options->m_fMaxScrollBPM != 0) {
		fScrollSpeed =
		  curr_options->m_fMaxScrollBPM / (pPlayerState->m_fReadBPM);
	}

	// don't mess with the arrows after they've crossed 0
	if (fYOffset < 0) {
		return fYOffset * fScrollSpeed;
	}

	const auto* const fAccels = curr_options->m_fAccels;

	float fYAdjust = 0; // fill this in depending on PlayerOptions

	if (fAccels[PlayerOptions::ACCEL_BOOST] != 0) {
		const auto fEffectHeight = GetNoteFieldHeight();
		const auto fNewYOffset =
		  fYOffset * 1.5f / ((fYOffset + fEffectHeight / 1.2f) / fEffectHeight);
		auto fAccelYAdjust =
		  fAccels[PlayerOptions::ACCEL_BOOST] * (fNewYOffset - fYOffset);
		// TRICKY: Clamp this value, or else BOOST+BOOMERANG will draw a ton of
		// arrows on the screen.
		CLAMP(fAccelYAdjust, BOOST_MOD_MIN_CLAMP, BOOST_MOD_MAX_CLAMP);
		fYAdjust += fAccelYAdjust;
	}
	if (fAccels[PlayerOptions::ACCEL_BRAKE] != 0) {
		const auto fEffectHeight = GetNoteFieldHeight();
		const auto fScale = SCALE(fYOffset, 0.f, fEffectHeight, 0, 1.f);
		const auto fNewYOffset = fYOffset * fScale;
		auto fBrakeYAdjust =
		  fAccels[PlayerOptions::ACCEL_BRAKE] * (fNewYOffset - fYOffset);
		// TRICKY: Clamp this value the same way as BOOST so that in
		// BOOST+BRAKE, BRAKE doesn't overpower BOOST
		CLAMP(fBrakeYAdjust, BRAKE_MOD_MIN_CLAMP, BRAKE_MOD_MAX_CLAMP);
		fYAdjust += fBrakeYAdjust;
	}
	if (fAccels[PlayerOptions::ACCEL_WAVE] != 0)
		fYAdjust += fAccels[PlayerOptions::ACCEL_WAVE] * WAVE_MOD_MAGNITUDE *
					RageFastSin(fYOffset / WAVE_MOD_HEIGHT);

	fYOffset += fYAdjust;

	// Factor in boomerang
	if (fAccels[PlayerOptions::ACCEL_BOOMERANG] != 0) {
		const auto fPeakAtYOffset =
		  SCREEN_HEIGHT *
		  BOOMERANG_PEAK_PERCENTAGE; // zero point of boomerang function
		fPeakYOffsetOut =
		  (-1 * fPeakAtYOffset * fPeakAtYOffset / SCREEN_HEIGHT) +
		  1.5f * fPeakAtYOffset;
		bIsPastPeakOut = fYOffset < fPeakAtYOffset;

		fYOffset = (-1 * fYOffset * fYOffset / SCREEN_HEIGHT) + 1.5f * fYOffset;
	}

	if (curr_options->m_fRandomSpeed > 0 && !bAbsolute) {
		// Generate a deterministically "random" speed for each arrow.
		unsigned seed = GAMESTATE->m_iStageSeed +
						(BeatToNoteRow(fNoteBeat) << 8) + (iCol * 100);

		for (auto i = 0; i < 3; ++i)
			seed = ((seed * 1664525u) + 1013904223u) & 0xFFFFFFFF;
		const auto fRandom = seed / 4294967296.0f;

		/* Random speed always increases speed: a random speed of 10 indicates
		 * [1,11]. This keeps it consistent with other mods: 0 means no effect.
		 */
		fScrollSpeed *=
		  SCALE(fRandom, 0.0f, 1.0f, 1.0f, curr_options->m_fRandomSpeed + 1.0f);
	}

	if (fAccels[PlayerOptions::ACCEL_EXPAND] != 0) {
		auto& data = g_EffectData;

		const auto fExpandMultiplier = SCALE(
		  RageFastCos(data.m_fExpandSeconds * EXPAND_MULTIPLIER_FREQUENCY),
		  EXPAND_MULTIPLIER_SCALE_FROM_LOW,
		  EXPAND_MULTIPLIER_SCALE_FROM_HIGH,
		  EXPAND_MULTIPLIER_SCALE_TO_LOW,
		  EXPAND_MULTIPLIER_SCALE_TO_HIGH);
		fScrollSpeed *= SCALE(fAccels[PlayerOptions::ACCEL_EXPAND],
							  EXPAND_SPEED_SCALE_FROM_LOW,
							  EXPAND_SPEED_SCALE_FROM_HIGH,
							  EXPAND_SPEED_SCALE_TO_LOW,
							  fExpandMultiplier);
	}

	fYOffset *= fScrollSpeed;
	fPeakYOffsetOut *= fScrollSpeed;

	return fYOffset;
}

static void
ArrowGetReverseShiftAndScale(int iCol,
							 float fYReverseOffsetPixels,
							 float& fShiftOut,
							 float& fScaleOut)
{
	// XXX: Hack: we need to scale the reverse shift by the zoom.
	const auto fMiniPercent =
	  curr_options->m_fEffects[PlayerOptions::EFFECT_MINI];
	auto fZoom = 1 - fMiniPercent * 0.5f;

	// don't divide by 0
	if (fabsf(fZoom) < 0.01)
		fZoom = 0.01f;

	const auto fPercentReverse = curr_options->GetReversePercentForColumn(iCol);
	fShiftOut = SCALE(fPercentReverse,
					  0.f,
					  1.f,
					  -fYReverseOffsetPixels / fZoom / 2,
					  fYReverseOffsetPixels / fZoom / 2);
	const auto fPercentCentered =
	  curr_options->m_fScrolls[PlayerOptions::SCROLL_CENTERED];
	fShiftOut = SCALE(fPercentCentered, 0.f, 1.f, fShiftOut, 0.0f);

	fScaleOut = SCALE(fPercentReverse, 0.f, 1.f, 1.f, -1.f);
}

float
ArrowEffects::GetYPos(int iCol,
					  float fYOffset,
					  float fYReverseOffsetPixels,
					  bool WithReverse)
{
	auto f = fYOffset;

	if (WithReverse) {
		float fShift, fScale;
		ArrowGetReverseShiftAndScale(
		  iCol, fYReverseOffsetPixels, fShift, fScale);

		f *= fScale;
		f += fShift;
	}

	const auto* const fEffects = curr_options->m_fEffects;
	// Doing the math with a precalculated result of 0 should be faster than
	// checking whether tipsy is on. -Kyz
	auto& data = g_EffectData;
	f += fEffects[PlayerOptions::EFFECT_TIPSY] * data.m_tipsy_result[iCol];

	// In beware's DDR Extreme-focused fork of StepMania 3.9, this value is
	// floored, making arrows show on integer Y coordinates. Supposedly it makes
	// the arrows look better, but testing needs to be done.
	// todo: make this a noteskin metric instead of a theme metric? -aj
	return QUANTIZE_ARROW_Y ? floor(f) : f;
}

float
ArrowEffects::GetYOffsetFromYPos(int iCol,
								 float YPos,
								 float fYReverseOffsetPixels)
{
	auto f = YPos;

	const auto* const fEffects = curr_options->m_fEffects;
	// Doing the math with a precalculated result of 0 should be faster than
	// checking whether tipsy is on. -Kyz
	auto& data = g_EffectData;
	f +=
	  fEffects[PlayerOptions::EFFECT_TIPSY] * data.m_tipsy_offset_result[iCol];

	float fShift, fScale;
	ArrowGetReverseShiftAndScale(iCol, fYReverseOffsetPixels, fShift, fScale);

	f -= fShift;
	if (fScale != 0.0f)
		f /= fScale;

	return f;
}

float
ArrowEffects::GetXPos(const PlayerState* pPlayerState,
					  int iColNum,
					  float fYOffset)
{
	float fPixelOffsetFromCenter = 0; // fill this in below

	auto pStyle = GAMESTATE->GetCurrentStyle(pPlayerState->m_PlayerNumber);
	const auto* const fEffects = curr_options->m_fEffects;

	const auto* const pCols = pStyle->m_ColumnInfo;
	auto& data = g_EffectData;

	if (fEffects[PlayerOptions::EFFECT_TORNADO] != 0) {
		const auto fRealPixelOffset =
		  pCols[iColNum].fXOffset * pPlayerState->m_NotefieldZoom;
		const auto fPositionBetween = SCALE(fRealPixelOffset,
											data.m_fMinTornadoX[iColNum],
											data.m_fMaxTornadoX[iColNum],
											TORNADO_POSITION_SCALE_TO_LOW,
											TORNADO_POSITION_SCALE_TO_HIGH);
		auto fRads = acosf(fPositionBetween);
		fRads += fYOffset * TORNADO_OFFSET_FREQUENCY / SCREEN_HEIGHT;

		const auto fAdjustedPixelOffset = SCALE(RageFastCos(fRads),
												TORNADO_OFFSET_SCALE_FROM_LOW,
												TORNADO_OFFSET_SCALE_FROM_HIGH,
												data.m_fMinTornadoX[iColNum],
												data.m_fMaxTornadoX[iColNum]);

		fPixelOffsetFromCenter += (fAdjustedPixelOffset - fRealPixelOffset) *
								  fEffects[PlayerOptions::EFFECT_TORNADO];
	}

	if (fEffects[PlayerOptions::EFFECT_DRUNK] != 0)
		fPixelOffsetFromCenter +=
		  fEffects[PlayerOptions::EFFECT_DRUNK] *
		  (RageFastCos(RageTimer::GetTimeSinceStart() +
					   iColNum * DRUNK_COLUMN_FREQUENCY +
					   fYOffset * DRUNK_OFFSET_FREQUENCY / SCREEN_HEIGHT) *
		   ARROW_SIZE * DRUNK_ARROW_MAGNITUDE);
	if (fEffects[PlayerOptions::EFFECT_FLIP] != 0) {
		const auto iFirstCol = 0;
		const auto iLastCol = pStyle->m_iColsPerPlayer - 1;
		const auto iNewCol =
		  SCALE(iColNum, iFirstCol, iLastCol, iLastCol, iFirstCol);
		const auto fOldPixelOffset =
		  pCols[iColNum].fXOffset * pPlayerState->m_NotefieldZoom;
		const auto fNewPixelOffset =
		  pCols[iNewCol].fXOffset * pPlayerState->m_NotefieldZoom;
		const auto fDistance = fNewPixelOffset - fOldPixelOffset;
		fPixelOffsetFromCenter +=
		  fDistance * fEffects[PlayerOptions::EFFECT_FLIP];
	}
	if (fEffects[PlayerOptions::EFFECT_INVERT] != 0)
		fPixelOffsetFromCenter += data.m_fInvertDistance[iColNum] *
								  fEffects[PlayerOptions::EFFECT_INVERT];

	if (fEffects[PlayerOptions::EFFECT_BEAT] != 0) {
		const auto fShift =
		  data.m_fBeatFactor *
		  RageFastSin(fYOffset / BEAT_OFFSET_HEIGHT + PI / BEAT_PI_HEIGHT);
		fPixelOffsetFromCenter += fEffects[PlayerOptions::EFFECT_BEAT] * fShift;
	}

	if (fEffects[PlayerOptions::EFFECT_XMODE] != 0) {
		// XMODE is dead -- depends on P2 positioning
	}

	fPixelOffsetFromCenter +=
	  pCols[iColNum].fXOffset * pPlayerState->m_NotefieldZoom;

	if (fEffects[PlayerOptions::EFFECT_TINY] != 0) {
		// Allow Tiny to pull tracks together, but not to push them apart.
		auto fTinyPercent = fEffects[PlayerOptions::EFFECT_TINY];
		fTinyPercent = std::min(powf(TINY_PERCENT_BASE, fTinyPercent),
								static_cast<float>(TINY_PERCENT_GATE));
		fPixelOffsetFromCenter *= fTinyPercent;
	}

	return fPixelOffsetFromCenter;
}

float
ArrowEffects::GetRotationX(float fYOffset)
{
	const auto* const fEffects = curr_options->m_fEffects;
	float fRotation = 0;
	if (fEffects[PlayerOptions::EFFECT_ROLL] != 0) {
		fRotation = fEffects[PlayerOptions::EFFECT_ROLL] * fYOffset / 2;
	}
	return fRotation;
}

float
ArrowEffects::GetRotationY(float fYOffset)
{
	const auto* const fEffects = curr_options->m_fEffects;
	float fRotation = 0;
	if (fEffects[PlayerOptions::EFFECT_TWIRL] != 0) {
		fRotation = fEffects[PlayerOptions::EFFECT_TWIRL] * fYOffset / 2;
	}
	return fRotation;
}

float
ArrowEffects::GetRotationZ(const PlayerState* pPlayerState,
						   float fNoteBeat,
						   bool bIsHoldHead)
{
	const auto* const fEffects = curr_options->m_fEffects;
	float fRotation = 0;
	if (fEffects[PlayerOptions::EFFECT_CONFUSION] != 0)
		fRotation += ReceptorGetRotationZ(pPlayerState);

	// As usual, enable dizzy hold heads at your own risk. -Wolfman2000
	if (fEffects[PlayerOptions::EFFECT_DIZZY] != 0 &&
		(DIZZY_HOLD_HEADS || !bIsHoldHead)) {
		const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeatVisible;
		auto fDizzyRotation = fNoteBeat - fSongBeat;
		fDizzyRotation *= fEffects[PlayerOptions::EFFECT_DIZZY];
		fDizzyRotation = fmodf(fDizzyRotation, 2 * PI);
		fDizzyRotation *= 180 / PI;
		fRotation += fDizzyRotation;
	}
	return fRotation;
}

float
ArrowEffects::ReceptorGetRotationZ(const PlayerState* pPlayerState)
{
	const auto* const fEffects = curr_options->m_fEffects;
	float fRotation = 0;

	if (fEffects[PlayerOptions::EFFECT_CONFUSION] != 0) {
		auto fConfRotation = GAMESTATE->m_Position.m_fSongBeatVisible;
		fConfRotation *= fEffects[PlayerOptions::EFFECT_CONFUSION];
		fConfRotation = fmodf(fConfRotation, 2 * PI);
		fConfRotation *= -180 / PI;
		fRotation += fConfRotation;
	}
	return fRotation;
}

constexpr auto CENTER_LINE_Y = 160; // from fYOffset == 0;
constexpr auto FADE_DIST_Y = 40;

static float
GetCenterLine()
{
	/* Another mini hack: if EFFECT_MINI is on, then our center line is at
	 * eg. 320, not 160. */
	const auto fMiniPercent =
	  curr_options->m_fEffects[PlayerOptions::EFFECT_MINI];
	const auto fZoom = 1 - fMiniPercent * 0.5f;
	return CENTER_LINE_Y / fZoom;
}

static float
GetHiddenSudden()
{
	const auto* const fAppearances = curr_options->m_fAppearances;
	return fAppearances[PlayerOptions::APPEARANCE_HIDDEN] *
		   fAppearances[PlayerOptions::APPEARANCE_SUDDEN];
}

//
//  -gray arrows-
//
//  ...invisible...
//  -hidden end line-
//  -hidden start line-
//  ...visible...
//  -sudden end line-
//  -sudden start line-
//  ...invisible...
//
// TRICKY:  We fudge hidden and sudden to be farther apart if they're both on.
static float
GetHiddenEndLine()
{
	return GetCenterLine() +
		   FADE_DIST_Y * SCALE(GetHiddenSudden(), 0.f, 1.f, -1.0f, -1.25f) +
		   GetCenterLine() *
			 curr_options
			   ->m_fAppearances[PlayerOptions::APPEARANCE_HIDDEN_OFFSET];
}

static float
GetHiddenStartLine()
{
	return GetCenterLine() +
		   FADE_DIST_Y * SCALE(GetHiddenSudden(), 0.f, 1.f, +0.0f, -0.25f) +
		   GetCenterLine() *
			 curr_options
			   ->m_fAppearances[PlayerOptions::APPEARANCE_HIDDEN_OFFSET];
}

static float
GetSuddenEndLine()
{
	return GetCenterLine() +
		   FADE_DIST_Y * SCALE(GetHiddenSudden(), 0.f, 1.f, -0.0f, +0.25f) +
		   GetCenterLine() *
			 curr_options
			   ->m_fAppearances[PlayerOptions::APPEARANCE_SUDDEN_OFFSET];
}

static float
GetSuddenStartLine()
{
	return GetCenterLine() +
		   FADE_DIST_Y * SCALE(GetHiddenSudden(), 0.f, 1.f, +1.0f, +1.25f) +
		   GetCenterLine() *
			 curr_options
			   ->m_fAppearances[PlayerOptions::APPEARANCE_SUDDEN_OFFSET];
}

// used by ArrowGetAlpha and ArrowGetGlow below
float
ArrowGetPercentVisible(float fYPosWithoutReverse)
{
	const auto fDistFromCenterLine = fYPosWithoutReverse - GetCenterLine();

	if (fYPosWithoutReverse < 0 &&
		HIDDEN_SUDDEN_PAST_RECEPTOR) // past Gray Arrows
		return 1;					 // totally visible

	const auto* const fAppearances = curr_options->m_fAppearances;

	float fVisibleAdjust = 0;

	if (fAppearances[PlayerOptions::APPEARANCE_HIDDEN] != 0) {
		auto fHiddenVisibleAdjust = SCALE(
		  fYPosWithoutReverse, GetHiddenStartLine(), GetHiddenEndLine(), 0, -1);
		CLAMP(fHiddenVisibleAdjust, -1, 0);
		fVisibleAdjust +=
		  fAppearances[PlayerOptions::APPEARANCE_HIDDEN] * fHiddenVisibleAdjust;
	}
	if (fAppearances[PlayerOptions::APPEARANCE_SUDDEN] != 0) {
		auto fSuddenVisibleAdjust = SCALE(
		  fYPosWithoutReverse, GetSuddenStartLine(), GetSuddenEndLine(), -1, 0);
		CLAMP(fSuddenVisibleAdjust, -1, 0);
		fVisibleAdjust +=
		  fAppearances[PlayerOptions::APPEARANCE_SUDDEN] * fSuddenVisibleAdjust;
	}

	if (fAppearances[PlayerOptions::APPEARANCE_STEALTH] != 0)
		fVisibleAdjust -= fAppearances[PlayerOptions::APPEARANCE_STEALTH];
	if (fAppearances[PlayerOptions::APPEARANCE_BLINK] != 0) {
		auto f = RageFastSin(RageTimer::GetTimeSinceStart() * 10);
		f = Quantize(f, BLINK_MOD_FREQUENCY);
		fVisibleAdjust += SCALE(f, 0, 1, -1, 0);
	}
	if (fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH] != 0) {
		const float fRealFadeDist = 80;
		fVisibleAdjust += SCALE(fabsf(fDistFromCenterLine),
								fRealFadeDist,
								2 * fRealFadeDist,
								-1,
								0) *
						  fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH];
	}

	return std::clamp(1.F + fVisibleAdjust, 0.F, 1.F);
}

float
ArrowEffects::GetAlpha(int iCol,
					   float fYOffset,
					   float fPercentFadeToFail,
					   float fYReverseOffsetPixels,
					   float fDrawDistanceBeforeTargetsPixels,
					   float fFadeInPercentOfDrawFar)
{
	// Get the YPos without reverse (that is, factor in EFFECT_TIPSY).
	const auto fYPosWithoutReverse =
	  GetYPos(iCol, fYOffset, fYReverseOffsetPixels, false);

	auto fPercentVisible = ArrowGetPercentVisible(fYPosWithoutReverse);

	if (fPercentFadeToFail != -1)
		fPercentVisible = 1 - fPercentFadeToFail;

	const auto fFullAlphaY =
	  fDrawDistanceBeforeTargetsPixels * (1 - fFadeInPercentOfDrawFar);
	if (fYPosWithoutReverse > fFullAlphaY) {
		const auto f = SCALE(fYPosWithoutReverse,
							 fFullAlphaY,
							 fDrawDistanceBeforeTargetsPixels,
							 1.0f,
							 0.0f);
		return f;
	}
	return fPercentVisible;
}

float
ArrowEffects::GetGlow(int iCol,
					  float fYOffset,
					  float fPercentFadeToFail,
					  float fYReverseOffsetPixels,
					  float fDrawDistanceBeforeTargetsPixels,
					  float fFadeInPercentOfDrawFar)
{
	// Get the YPos without reverse (that is, factor in EFFECT_TIPSY).
	const auto fYPosWithoutReverse =
	  GetYPos(iCol, fYOffset, fYReverseOffsetPixels, false);

	auto fPercentVisible = ArrowGetPercentVisible(fYPosWithoutReverse);

	if (fPercentFadeToFail != -1)
		fPercentVisible = 1 - fPercentFadeToFail;

	const auto fDistFromHalf = fabsf(fPercentVisible - 0.5f);

	if (!PREFSMAN->m_bNoGlow) {
		return SCALE(fDistFromHalf, 0, 0.5f, 1.3f, 0);
	}
	return 0;
}

float
ArrowEffects::GetBrightness(const PlayerState* pPlayerState, float fNoteBeat)
{
	const auto fSongBeat = GAMESTATE->m_Position.m_fSongBeatVisible;
	const auto fBeatsUntilStep = fNoteBeat - fSongBeat;

	auto fBrightness = SCALE(fBeatsUntilStep, 0, -1, 1.f, 0.f);
	CLAMP(fBrightness, 0, 1);
	return fBrightness;
}

float
ArrowEffects::GetZPos(int iCol, float fYOffset)
{
	float fZPos = 0;
	const auto* const fEffects = curr_options->m_fEffects;

	if (fEffects[PlayerOptions::EFFECT_BUMPY] != 0)
		fZPos += fEffects[PlayerOptions::EFFECT_BUMPY] * 40 *
				 RageFastSin(fYOffset / 16.0f);

	return fZPos;
}

bool
ArrowEffects::NeedZBuffer()
{
	const auto* const fEffects = curr_options->m_fEffects;
	// We also need to use the Z buffer if twirl is in play, because of
	// hold modulation. -vyhd (OpenITG r623)
	if (fEffects[PlayerOptions::EFFECT_BUMPY] != 0 ||
		fEffects[PlayerOptions::EFFECT_TWIRL] != 0) {
		return true;
	}
	return false;
}

float
ArrowEffects::GetZoom(const PlayerState* pPlayerState)
{
	auto fZoom = 1.0f;
	// Design change:  Instead of having a flag in the style that toggles a
	// fixed zoom (0.6) that is only applied to the columns, ScreenGameplay now
	// calculates a zoom factor to apply to the notefield and puts it in the
	// PlayerState. -Kyz
	fZoom *= pPlayerState->m_NotefieldZoom;

	auto fTinyPercent = curr_options->m_fEffects[PlayerOptions::EFFECT_TINY];
	if (fTinyPercent != 0) {
		fTinyPercent = powf(0.5f, fTinyPercent);
		fZoom *= fTinyPercent;
	}
	return fZoom;
}

static ThemeMetric<float> FRAME_WIDTH_EFFECTS_PIXELS_PER_SECOND(
  "ArrowEffects",
  "FrameWidthEffectsPixelsPerSecond");
static ThemeMetric<float> FRAME_WIDTH_EFFECTS_MIN_MULTIPLIER(
  "ArrowEffects",
  "FrameWidthEffectsMinMultiplier");
static ThemeMetric<float> FRAME_WIDTH_EFFECTS_MAX_MULTIPLIER(
  "ArrowEffects",
  "FrameWidthEffectsMaxMultiplier");
static ThemeMetric<bool> FRAME_WIDTH_LOCK_EFFECTS_TO_OVERLAPPING(
  "ArrowEffects",
  "FrameWidthLockEffectsToOverlapping");
static ThemeMetric<float> FRAME_WIDTH_LOCK_EFFECTS_TWEEN_PIXELS(
  "ArrowEffects",
  "FrameWidthLockEffectsTweenPixels");

float
ArrowEffects::GetFrameWidthScale(const PlayerState* pPlayerState,
								 float fYOffset,
								 float fOverlappedTime)
{
	auto fFrameWidthMultiplier = 1.0f;

	const float fPixelsPerSecond = FRAME_WIDTH_EFFECTS_PIXELS_PER_SECOND;
	const auto fSecond = fYOffset / fPixelsPerSecond;
	auto fWidthEffect = pPlayerState->m_EffectHistory.GetSample(fSecond);
	if (fWidthEffect != 0 && FRAME_WIDTH_LOCK_EFFECTS_TO_OVERLAPPING) {
		// Don't display effect data that happened before this hold overlapped
		// the top.
		const auto fFromEndOfOverlapped = fOverlappedTime - fSecond;
		const float fTrailingPixels = FRAME_WIDTH_LOCK_EFFECTS_TWEEN_PIXELS;
		const auto fTrailingSeconds = fTrailingPixels / fPixelsPerSecond;
		auto fScaleEffect =
		  SCALE(fFromEndOfOverlapped, 0.0f, fTrailingSeconds, 0.0f, 1.0f);
		CLAMP(fScaleEffect, 0.0f, 1.0f);
		fWidthEffect *= fScaleEffect;
	}

	if (fWidthEffect > 0)
		fFrameWidthMultiplier *= SCALE(
		  fWidthEffect, 0.0f, 1.0f, 1.0f, FRAME_WIDTH_EFFECTS_MAX_MULTIPLIER);
	else if (fWidthEffect < 0)
		fFrameWidthMultiplier *= SCALE(
		  fWidthEffect, 0.0f, -1.0f, 1.0f, FRAME_WIDTH_EFFECTS_MIN_MULTIPLIER);

	return fFrameWidthMultiplier;
}

// To provide reasonable defaults to methods below.
ThemeMetric<float> FADE_BEFORE_TARGETS_PERCENT("NoteField",
											   "FadeBeforeTargetsPercent");
ThemeMetric<float> DRAW_DISTANCE_BEFORE_TARGET_PIXELS(
  "Player",
  "DrawDistanceBeforeTargetsPixels");
ThemeMetric<float> GRAY_ARROWS_Y_STANDARD("Player", "ReceptorArrowsYStandard");
ThemeMetric<float> GRAY_ARROWS_Y_REVERSE("Player", "ReceptorArrowsYReverse");

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

namespace {
/* Update() need to be exposed to use ArrowEffects off ScreenGameplay. It is
 * harmless.	 */
int
Update(lua_State* L)
{
	ArrowEffects::Update();
	return 0;
}

// Provide a reasonable default value for fYReverseOffset
float
YReverseOffset(lua_State* L, int argnum)
{
	auto fYReverseOffsetPixels = GRAY_ARROWS_Y_REVERSE - GRAY_ARROWS_Y_STANDARD;
	if (lua_gettop(L) >= argnum && !lua_isnil(L, argnum)) {
		fYReverseOffsetPixels = FArg(argnum);
	}
	return fYReverseOffsetPixels;
}

// ( PlayerState ps, int iCol, float fNoteBeat )
int
GetYOffset(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	float fPeakYOffset;
	bool bIsPastPeak;

	lua_pushnumber(L,
				   ArrowEffects::GetYOffset(
					 ps, IArg(2) - 1, FArg(3), fPeakYOffset, bIsPastPeak));
	lua_pushnumber(L, fPeakYOffset);
	lua_pushboolean(L, static_cast<int>(bIsPastPeak));
	return 3;
}

// ( PlayerState ps, int iCol, float fYOffset, float fYReverseOffsetPixels )
int
GetYPos(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	const auto fYReverseOffsetPixels = YReverseOffset(L, 4);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(
	  L, ArrowEffects::GetYPos(IArg(2) - 1, FArg(3), fYReverseOffsetPixels));
	return 1;
}

// ( PlayerState ps, int iCol, float fYPos, float fYReverseOffsetPixels )
int
GetYOffsetFromYPos(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	const auto fYReverseOffsetPixels = YReverseOffset(L, 4);
	lua_pushnumber(L,
				   ArrowEffects::GetYOffsetFromYPos(
					 IArg(2) - 1, FArg(3), fYReverseOffsetPixels));
	return 1;
}

// ( PlayerState ps, int iCol, float fYOffset )
int
GetXPos(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::GetXPos(ps, IArg(2) - 1, FArg(3)));
	return 1;
}

// ( PlayerState ps, int iCol, float fYOffset )
int
GetZPos(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::GetZPos(IArg(2) - 1, FArg(3)));
	return 1;
}

// ( PlayerState ps, float fYOffset )
int
GetRotationX(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::GetRotationX(FArg(2)));
	return 1;
}

// ( PlayerState ps, float fYOffset )
int
GetRotationY(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::GetRotationY(FArg(2)));
	return 1;
}

// ( PlayerState ps, float fNoteBeat, bool bIsHoldHead )
int
GetRotationZ(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	// Make bIsHoldHead optional.
	auto bIsHoldHead = false;
	if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
		bIsHoldHead = BArg(3);
	}
	lua_pushnumber(L, ArrowEffects::GetRotationZ(ps, FArg(2), bIsHoldHead));
	return 1;
}

// ( PlayerState ps )
int
ReceptorGetRotationZ(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::ReceptorGetRotationZ(ps));
	return 1;
}

//( PlayerState ps, int iCol, float fYOffset, float fPercentFadeToFail, float
// fYReverseOffsetPixels, float fDrawDistanceBeforeTargetsPixels, float
// fFadeInPercentOfDrawFar )
int
GetAlpha(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	// Provide reasonable default values.
	float fPercentFadeToFail = -1;
	const auto fYReverseOffsetPixels = YReverseOffset(L, 5);
	float fDrawDistanceBeforeTargetsPixels = DRAW_DISTANCE_BEFORE_TARGET_PIXELS;
	float fFadeInPercentOfDrawFar = FADE_BEFORE_TARGETS_PERCENT;
	if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
		fPercentFadeToFail = FArg(4);
	}
	if (lua_gettop(L) >= 6 && !lua_isnil(L, 6)) {
		fDrawDistanceBeforeTargetsPixels = FArg(6);
	}
	if (lua_gettop(L) >= 7 && !lua_isnil(L, 7)) {
		fFadeInPercentOfDrawFar = FArg(7);
	}
	lua_pushnumber(L,
				   ArrowEffects::GetAlpha(IArg(2) - 1,
										  FArg(3),
										  fPercentFadeToFail,
										  fYReverseOffsetPixels,
										  fDrawDistanceBeforeTargetsPixels,
										  fFadeInPercentOfDrawFar));
	return 1;
}

//( PlayerState ps, int iCol, float fYOffset, float fPercentFadeToFail, float
// fYReverseOffsetPixels, float fDrawDistanceBeforeTargetsPixels, float
// fFadeInPercentOfDrawFar )
int
GetGlow(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	// Provide reasonable default values.
	float fPercentFadeToFail = -1; //
	const auto fYReverseOffsetPixels = YReverseOffset(L, 5);
	float fDrawDistanceBeforeTargetsPixels = DRAW_DISTANCE_BEFORE_TARGET_PIXELS;
	float fFadeInPercentOfDrawFar = FADE_BEFORE_TARGETS_PERCENT;
	if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
		fPercentFadeToFail = FArg(4);
	}
	if (lua_gettop(L) >= 6 && !lua_isnil(L, 6)) {
		fDrawDistanceBeforeTargetsPixels = FArg(6);
	}
	if (lua_gettop(L) >= 7 && !lua_isnil(L, 7)) {
		fFadeInPercentOfDrawFar = FArg(7);
	}
	lua_pushnumber(L,
				   ArrowEffects::GetGlow(IArg(2) - 1,
										 FArg(3),
										 fPercentFadeToFail,
										 fYReverseOffsetPixels,
										 fDrawDistanceBeforeTargetsPixels,
										 fFadeInPercentOfDrawFar));
	return 1;
}

// ( PlayerState ps, float fNoteBeat )
int
GetBrightness(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::GetBrightness(ps, FArg(2)));
	return 1;
}

// ( PlayerState ps )
int
NeedZBuffer(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushboolean(L, static_cast<int>(ArrowEffects::NeedZBuffer()));
	return 1;
}

// ( PlayerState ps )
int
GetZoom(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());
	lua_pushnumber(L, ArrowEffects::GetZoom(ps));
	return 1;
}

// ( PlayerState ps, float fYOffset, fOverlappedTime )
int
GetFrameWidthScale(lua_State* L)
{
	auto* ps = Luna<PlayerState>::check(L, 1);
	ArrowEffects::SetCurrentOptions(&ps->m_PlayerOptions.GetCurrent());

	// Make fOverlappedTime optional.
	float fOverlappedTime = 0;
	if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
		fOverlappedTime = FArg(3);
	}
	lua_pushnumber(
	  L, ArrowEffects::GetFrameWidthScale(ps, FArg(2), fOverlappedTime));
	return 1;
}

const luaL_Reg ArrowEffectsTable[] = {
	LIST_METHOD(Update),		LIST_METHOD(GetYOffset),
	LIST_METHOD(GetYPos),		LIST_METHOD(GetYOffsetFromYPos),
	LIST_METHOD(GetXPos),		LIST_METHOD(GetZPos),
	LIST_METHOD(GetRotationX),	LIST_METHOD(GetRotationY),
	LIST_METHOD(GetRotationZ),	LIST_METHOD(ReceptorGetRotationZ),
	LIST_METHOD(GetAlpha),		LIST_METHOD(GetGlow),
	LIST_METHOD(GetBrightness), LIST_METHOD(NeedZBuffer),
	LIST_METHOD(GetZoom),		LIST_METHOD(GetFrameWidthScale),
	{ nullptr, nullptr }
};
} // namespace

LUA_REGISTER_NAMESPACE(ArrowEffects)
