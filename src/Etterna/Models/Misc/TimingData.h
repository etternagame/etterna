#ifndef TIMING_DATA_H
#define TIMING_DATA_H

#include "NoteTypes.h"
#include "TimingSegments.h"
#include "Core/Services/Locator.hpp"
#include <cfloat> // max float

struct lua_State;

/** @brief Compare a TimingData segment's properties with one another. */
#define COMPARE(x)                                                             \
	if (this->x != other.x)                                                    \
		return false;

/* convenience functions to handle static casting */
template<class T>
auto
ToDerived(const TimingSegment* t, TimingSegmentType tst) -> T
{
	return static_cast<T>(t);
}

#define TimingSegmentToXWithName(Seg, SegName, SegType)                        \
	inline const Seg* To##SegName(const TimingSegment* t)                      \
	{                                                                          \
		return static_cast<const Seg*>(t);                                     \
	}                                                                          \
	inline Seg* To##SegName(TimingSegment* t) { return static_cast<Seg*>(t); }

#define TimingSegmentToX(Seg, SegType)                                         \
	TimingSegmentToXWithName(Seg##Segment, Seg, SEGMENT_##SegType)

/* ToBPM(TimingSegment*), ToTimeSignature(TimingSegment*), etc. */
TimingSegmentToX(BPM, BPM);
TimingSegmentToX(Stop, STOP);
TimingSegmentToX(Delay, DELAY);
TimingSegmentToX(TimeSignature, TIME_SIG);
TimingSegmentToX(Warp, WARP);
TimingSegmentToX(Label, LABEL);
TimingSegmentToX(Tickcount, TICKCOUNT);
TimingSegmentToX(Combo, COMBO);
TimingSegmentToX(Speed, SPEED);
TimingSegmentToX(Scroll, SCROLL);
TimingSegmentToX(Fake, FAKE);

#undef TimingSegmentToXWithName
#undef TimingSegmentToX

/**
 * @brief Holds data for translating beats<->seconds.
 */
class TimingData
{
  public:
	/**
	 * @brief Sets up initial timing data with a defined offset.
	 * @param fOffset the offset from the 0th beat. */
	TimingData(float fOffset = 0.F);
	~TimingData();

	void Copy(const TimingData& other);
	void Clear();

	TimingData(const TimingData& cpy) { Copy(cpy); }
	auto operator=(const TimingData& cpy) -> TimingData&
	{
		Copy(cpy);
		return *this;
	}
	auto operator=(TimingData&& other) noexcept -> TimingData&
	{
		std::swap(m_beat_start_lookup, other.m_beat_start_lookup);
		std::swap(m_time_start_lookup, other.m_time_start_lookup);
		std::swap(m_avpTimingSegments, other.m_avpTimingSegments);
		std::swap(m_sFile, other.m_sFile);
		std::swap(m_fBeat0OffsetInSeconds, other.m_fBeat0OffsetInSeconds);
		std::swap(ElapsedTimesAtAllRows, other.ElapsedTimesAtAllRows);
		std::swap(ElapsedTimesAtNonEmptyRows, other.ElapsedTimesAtNonEmptyRows);
		std::swap(ValidSequentialAssumption, other.ValidSequentialAssumption);

		return *this;
	}

	// GetBeatArgs, GetBeatStarts, m_beat_start_lookup, m_time_start_lookup,
	// PrepareLookup, and ReleaseLookup form a system for speeding up finding
	// the current beat and bps from the time, or finding the time from the
	// current beat.
	// The lookup tables contain indices for the beat and time finding
	// functions to start at so they don't have to walk through all the timing
	// segments.
	// PrepareLookup should be called before gameplay starts, so that the lookup
	// tables are populated.  ReleaseLookup should be called after gameplay
	// finishes so that memory isn't wasted.
	// -Kyz
	struct GetBeatArgs
	{
		float elapsed_time{ 0 };
		float beat{ 0 };
		float bps_out{ 0 };
		float warp_dest_out{ 0 };
		int warp_begin_out{ -1 };
		bool freeze_out{ false };
		bool delay_out{ false };
		GetBeatArgs() = default;
	};
	struct GetBeatStarts
	{
		unsigned int bpm{ 0 };
		unsigned int warp{ 0 };
		unsigned int stop{ 0 };
		unsigned int delay{ 0 };
		int last_row{ 0 };
		float last_time{ 0 };
		float warp_destination{ 0 };
		bool is_warping{ false };
		GetBeatStarts() = default;
	};
	// map can't be used for the lookup table because its find or *_bound
	// functions would return the wrong entry.
	// In a map<int, int> with three entries, [-1]= 3, [6]= 1, [8]= 2,
	// lower_bound(0) and upper_bound(0) both returned the entry at [6]= 1.
	// So the lookup table is a  std::vector of entries and FindEntryInLookup
	// does a binary search. -Kyz
	struct lookup_item_t
	{
		float first;
		GetBeatStarts second;
		lookup_item_t(float f, GetBeatStarts& s)
		  : first(f)
		  , second(s)
		{
		}
	};
	using beat_start_lookup_t = std::vector<lookup_item_t>;
	beat_start_lookup_t m_beat_start_lookup;
	beat_start_lookup_t m_time_start_lookup;

	void PrepareLookup();
	void ReleaseLookup();

	[[nodiscard]] auto GetSegmentIndexAtRow(TimingSegmentType tst,
											int row) const -> int;

	[[nodiscard]] auto GetSegmentIndexAtBeat(TimingSegmentType tst,
											 float beat) const -> int
	{
		return GetSegmentIndexAtRow(tst, BeatToNoteRow(beat));
	}

	[[nodiscard]] auto GetNextSegmentBeatAtRow(TimingSegmentType tst,
											   int row) const -> float;

	[[nodiscard]] auto GetNextSegmentBeatAtBeat(TimingSegmentType tst,
												float beat) const -> float
	{
		return GetNextSegmentBeatAtRow(tst, BeatToNoteRow(beat));
	}

	[[nodiscard]] auto GetPreviousSegmentBeatAtRow(TimingSegmentType tst,
												   int row) const -> float;

	[[nodiscard]] auto GetPreviousSegmentBeatAtBeat(TimingSegmentType tst,
													float beat) const -> float
	{
		return GetPreviousSegmentBeatAtRow(tst, BeatToNoteRow(beat));
	}

	[[nodiscard]] auto empty() const -> bool;

	void CopyRange(int start_row,
				   int end_row,
				   TimingSegmentType copy_type,
				   int dest_row,
				   TimingData& dest) const;
	void ShiftRange(int start_row,
					int end_row,
					TimingSegmentType shift_type,
					int shift_amount);
	void ClearRange(int start_row, int end_row, TimingSegmentType clear_type);
	/**
	 * @brief Gets the actual BPM of the song,
	 * while respecting a limit.
	 *
	 * The high limit is due to the implementation of mMods.
	 * @param fMinBPMOut the minimium specified BPM.
	 * @param fMaxBPMOut the maximum specified BPM.
	 * @param highest the highest allowed max BPM.
	 */
	void GetActualBPM(float& fMinBPMOut,
					  float& fMaxBPMOut,
					  float highest = FLT_MAX) const;

	/**
	 * @brief Retrieve the TimingSegment at the specified row.
	 * @param iNoteRow the row that has a TimingSegment.
	 * @param tst the TimingSegmentType requested.
	 * @return the segment in question.
	 */
	[[nodiscard]] auto GetSegmentAtRow(int iNoteRow,
									   TimingSegmentType tst) const
	  -> const TimingSegment*;
	auto GetSegmentAtRow(int iNoteRow, TimingSegmentType tst) -> TimingSegment*;

	/**
	 * @brief Retrieve the TimingSegment at the given beat.
	 * @param fBeat the beat that has a TimingSegment.
	 * @param tst the TimingSegmentType requested.
	 * @return the segment in question.
	 */
	[[nodiscard]] auto GetSegmentAtBeat(float fBeat,
										TimingSegmentType tst) const
	  -> const TimingSegment*
	{
		return GetSegmentAtRow(BeatToNoteRow(fBeat), tst);
	}

#define DefineSegmentWithName(Seg, SegName, SegType)                           \
	const Seg* Get##Seg##AtRow(int iNoteRow) const                             \
	{                                                                          \
		const TimingSegment* t = GetSegmentAtRow(iNoteRow, SegType);           \
		return To##SegName(t);                                                 \
	}                                                                          \
	Seg* Get##Seg##AtRow(int iNoteRow)                                         \
	{                                                                          \
		return const_cast<Seg*>(                                               \
		  ((const TimingData*)this)->Get##Seg##AtRow(iNoteRow));               \
	}                                                                          \
	const Seg* Get##Seg##AtBeat(float fBeat) const                             \
	{                                                                          \
		return Get##Seg##AtRow(BeatToNoteRow(fBeat));                          \
	}                                                                          \
	Seg* Get##Seg##AtBeat(float fBeat)                                         \
	{                                                                          \
		return const_cast<Seg*>(                                               \
		  ((const TimingData*)this)->Get##Seg##AtBeat(fBeat));                 \
	}                                                                          \
	void AddSegment(const Seg& seg) { AddSegment(&seg); }

// "XXX: this comment (and quote mark) exists so nano won't
// display the rest of this file as one giant string

// (TimeSignature,TIME_SIG) -> (TimeSignatureSegment,SEGMENT_TIME_SIG)
#define DefineSegment(Seg, SegType)                                            \
	DefineSegmentWithName(Seg##Segment, Seg, SEGMENT_##SegType)

	DefineSegment(BPM, BPM);
	DefineSegment(Stop, STOP);
	DefineSegment(Delay, DELAY);
	DefineSegment(Warp, WARP);
	DefineSegment(Label, LABEL);
	DefineSegment(Tickcount, TICKCOUNT);
	DefineSegment(Combo, COMBO);
	DefineSegment(Speed, SPEED);
	DefineSegment(Scroll, SCROLL);
	DefineSegment(Fake, FAKE);
	DefineSegment(TimeSignature, TIME_SIG);

#undef DefineSegmentWithName
#undef DefineSegment

	/* convenience aliases (Set functions are deprecated) */
	[[nodiscard]] auto GetBPMAtRow(int iNoteRow) const -> float
	{
		return GetBPMSegmentAtRow(iNoteRow)->GetBPM();
	}

	[[nodiscard]] auto GetBPMAtBeat(float fBeat) const -> float
	{
		return GetBPMAtRow(BeatToNoteRow(fBeat));
	}
	void SetBPMAtRow(int iNoteRow, float fBPM)
	{
		AddSegment(BPMSegment(iNoteRow, fBPM));
	}
	void SetBPMAtBeat(float fBeat, float fBPM)
	{
		SetBPMAtRow(BeatToNoteRow(fBeat), fBPM);
	}

	[[nodiscard]] auto GetStopAtRow(int iNoteRow) const -> float
	{
		return GetStopSegmentAtRow(iNoteRow)->GetPause();
	}

	[[nodiscard]] auto GetStopAtBeat(float fBeat) const -> float
	{
		return GetStopAtRow(BeatToNoteRow(fBeat));
	}
	void SetStopAtRow(int iNoteRow, float fSeconds)
	{
		AddSegment(StopSegment(iNoteRow, fSeconds));
	}
	void SetStopAtBeat(float fBeat, float fSeconds)
	{
		SetStopAtRow(BeatToNoteRow(fBeat), fSeconds);
	}

	[[nodiscard]] auto GetDelayAtRow(int iNoteRow) const -> float
	{
		return GetDelaySegmentAtRow(iNoteRow)->GetPause();
	}

	[[nodiscard]] auto GetDelayAtBeat(float fBeat) const -> float
	{
		return GetDelayAtRow(BeatToNoteRow(fBeat));
	}
	void SetDelayAtRow(int iNoteRow, float fSeconds)
	{
		AddSegment(DelaySegment(iNoteRow, fSeconds));
	}
	void SetDelayAtBeat(float fBeat, float fSeconds)
	{
		SetDelayAtRow(BeatToNoteRow(fBeat), fSeconds);
	}

	void SetTimeSignatureAtRow(int iNoteRow, int iNum, int iDen)
	{
		AddSegment(TimeSignatureSegment(iNoteRow, iNum, iDen));
	}

	void SetTimeSignatureAtBeat(float fBeat, int iNum, int iDen)
	{
		SetTimeSignatureAtRow(BeatToNoteRow(fBeat), iNum, iDen);
	}

	[[nodiscard]] auto GetWarpAtRow(int iNoteRow) const -> float
	{
		return GetWarpSegmentAtRow(iNoteRow)->GetLength();
	}

	[[nodiscard]] auto GetWarpAtBeat(float fBeat) const -> float
	{
		return GetWarpAtRow(BeatToNoteRow(fBeat));
	}
	/* Note: fLength is in beats, not rows */
	void SetWarpAtRow(int iRow, float fLength)
	{
		AddSegment(WarpSegment(iRow, fLength));
	}
	void SetWarpAtBeat(float fBeat, float fLength)
	{
		AddSegment(WarpSegment(BeatToNoteRow(fBeat), fLength));
	}

	[[nodiscard]] auto GetTickcountAtRow(int iNoteRow) const -> int
	{
		return GetTickcountSegmentAtRow(iNoteRow)->GetTicks();
	}

	[[nodiscard]] auto GetTickcountAtBeat(float fBeat) const -> int
	{
		return GetTickcountAtRow(BeatToNoteRow(fBeat));
	}
	void SetTickcountAtRow(int iNoteRow, int iTicks)
	{
		AddSegment(TickcountSegment(iNoteRow, iTicks));
	}
	void SetTickcountAtBeat(float fBeat, int iTicks)
	{
		SetTickcountAtRow(BeatToNoteRow(fBeat), iTicks);
	}

	[[nodiscard]] auto GetComboAtRow(int iNoteRow) const -> int
	{
		return GetComboSegmentAtRow(iNoteRow)->GetCombo();
	}

	[[nodiscard]] auto GetComboAtBeat(float fBeat) const -> int
	{
		return GetComboAtRow(BeatToNoteRow(fBeat));
	}

	[[nodiscard]] auto GetMissComboAtRow(int iNoteRow) const -> int
	{
		return GetComboSegmentAtRow(iNoteRow)->GetMissCombo();
	}

	[[nodiscard]] auto GetMissComboAtBeat(float fBeat) const -> int
	{
		return GetMissComboAtRow(BeatToNoteRow(fBeat));
	}

	[[nodiscard]] auto GetLabelAtRow(int iNoteRow) const -> const std::string&
	{
		return GetLabelSegmentAtRow(iNoteRow)->GetLabel();
	}

	[[nodiscard]] auto GetLabelAtBeat(float fBeat) const -> const std::string&
	{
		return GetLabelAtRow(BeatToNoteRow(fBeat));
	}
	void SetLabelAtRow(int iNoteRow, const std::string& sLabel)
	{
		AddSegment(LabelSegment(iNoteRow, sLabel));
	}
	void SetLabelAtBeat(float fBeat, const std::string& sLabel)
	{
		SetLabelAtRow(BeatToNoteRow(fBeat), sLabel);
	}

	[[nodiscard]] auto DoesLabelExist(const std::string& sLabel) const -> bool;

	[[nodiscard]] auto GetSpeedPercentAtRow(int iNoteRow) const -> float
	{
		return GetSpeedSegmentAtRow(iNoteRow)->GetRatio();
	}

	[[nodiscard]] auto GetSpeedPercentAtBeat(float fBeat) const -> float
	{
		return GetSpeedPercentAtRow(BeatToNoteRow(fBeat));
	}

	[[nodiscard]] auto GetSpeedWaitAtRow(int iNoteRow) const -> float
	{
		return GetSpeedSegmentAtRow(iNoteRow)->GetDelay();
	}

	[[nodiscard]] auto GetSpeedWaitAtBeat(float fBeat) const -> float
	{
		return GetSpeedWaitAtRow(BeatToNoteRow(fBeat));
	}

	// XXX: is there any point to having specific unit types?
	[[nodiscard]] auto GetSpeedModeAtRow(int iNoteRow) const
	  -> SpeedSegment::BaseUnit
	{
		return GetSpeedSegmentAtRow(iNoteRow)->GetUnit();
	}

	[[nodiscard]] auto GetSpeedModeAtBeat(float fBeat) const
	  -> SpeedSegment::BaseUnit
	{
		return GetSpeedModeAtRow(BeatToNoteRow(fBeat));
	}

	void SetSpeedAtRow(int iNoteRow,
					   float fPercent,
					   float fWait,
					   SpeedSegment::BaseUnit unit)
	{
		AddSegment(SpeedSegment(iNoteRow, fPercent, fWait, unit));
	}

	void SetSpeedAtBeat(float fBeat,
						float fPercent,
						float fWait,
						SpeedSegment::BaseUnit unit)
	{
		SetSpeedAtRow(BeatToNoteRow(fBeat), fPercent, fWait, unit);
	}

	void SetSpeedPercentAtRow(int iNoteRow, float fPercent)
	{
		const SpeedSegment* seg = GetSpeedSegmentAtRow(iNoteRow);
		SetSpeedAtRow(iNoteRow, fPercent, seg->GetDelay(), seg->GetUnit());
	}

	void SetSpeedWaitAtRow(int iNoteRow, float fWait)
	{
		const SpeedSegment* seg = GetSpeedSegmentAtRow(iNoteRow);
		SetSpeedAtRow(iNoteRow, seg->GetRatio(), fWait, seg->GetUnit());
	}

	void SetSpeedModeAtRow(int iNoteRow, SpeedSegment::BaseUnit unit)
	{
		const SpeedSegment* seg = GetSpeedSegmentAtRow(iNoteRow);
		SetSpeedAtRow(iNoteRow, seg->GetRatio(), seg->GetDelay(), unit);
	}

	void SetSpeedPercentAtBeat(float fBeat, float fPercent)
	{
		SetSpeedPercentAtRow(BeatToNoteRow(fBeat), fPercent);
	}
	void SetSpeedWaitAtBeat(float fBeat, float fWait)
	{
		SetSpeedWaitAtRow(BeatToNoteRow(fBeat), fWait);
	}
	void SetSpeedModeAtBeat(float fBeat, SpeedSegment::BaseUnit unit)
	{
		SetSpeedModeAtRow(BeatToNoteRow(fBeat), unit);
	}

	[[nodiscard]] auto GetDisplayedSpeedPercent(float fBeat,
												float fMusicSeconds) const
	  -> float;

	[[nodiscard]] auto GetScrollAtRow(int iNoteRow) const -> float
	{
		return GetScrollSegmentAtRow(iNoteRow)->GetRatio();
	}

	[[nodiscard]] auto GetScrollAtBeat(float fBeat) const -> float
	{
		return GetScrollAtRow(BeatToNoteRow(fBeat));
	}

	void SetScrollAtRow(int iNoteRow, float fPercent)
	{
		AddSegment(ScrollSegment(iNoteRow, fPercent));
	}
	void SetScrollAtBeat(float fBeat, float fPercent)
	{
		SetScrollAtRow(BeatToNoteRow(fBeat), fPercent);
	}

	[[nodiscard]] auto GetFakeAtRow(int iRow) const -> float
	{
		return GetFakeSegmentAtRow(iRow)->GetLength();
	}

	[[nodiscard]] auto GetFakeAtBeat(float fBeat) const -> float
	{
		return GetFakeAtRow(BeatToNoteRow(fBeat));
	}

	[[nodiscard]] auto IsWarpAtRow(int iRow) const -> bool;

	[[nodiscard]] auto IsWarpAtBeat(float fBeat) const -> bool
	{
		return IsWarpAtRow(BeatToNoteRow(fBeat));
	}

	[[nodiscard]] auto IsFakeAtRow(int iRow) const -> bool;

	[[nodiscard]] auto IsFakeAtBeat(float fBeat) const -> bool
	{
		return IsFakeAtRow(BeatToNoteRow(fBeat));
	}

	/**
	 * @brief Determine if this notes on this row can be judged.
	 * @param row the row to focus on.
	 * @return true if the row can be judged, false otherwise. */
	[[nodiscard]] auto IsJudgableAtRow(int row) const -> bool
	{
		return !IsWarpAtRow(row) && !IsFakeAtRow(row);
	}

	[[nodiscard]] auto IsJudgableAtBeat(float beat) const -> bool
	{
		return IsJudgableAtRow(BeatToNoteRow(beat));
	}

	void MultiplyBPMInBeatRange(int iStartIndex, int iEndIndex, float fFactor);

	void NoteRowToMeasureAndBeat(int iNoteRow,
								 int& iMeasureIndexOut,
								 int& iBeatIndexOut,
								 int& iRowsRemainder) const;

	void GetBeatInternal(GetBeatStarts& start,
						 GetBeatArgs& args,
						 unsigned int max_segment) const;
	auto GetElapsedTimeInternal(GetBeatStarts& start,
								float beat,
								unsigned int max_segment) const -> float;
	void GetBeatAndBPSFromElapsedTime(GetBeatArgs& args) const;

	[[nodiscard]] auto GetBeatFromElapsedTime(float elapsed_time) const
	  -> float // shortcut for places that care only about the beat
	{
		GetBeatArgs args;
		args.elapsed_time = elapsed_time;
		GetBeatAndBPSFromElapsedTime(args);
		return args.beat;
	}

	[[nodiscard]] auto GetElapsedTimeFromBeat(float fBeat) const -> float;

	void GetBeatAndBPSFromElapsedTimeNoOffset(GetBeatArgs& args) const;

	[[nodiscard]] auto GetBeatFromElapsedTimeNoOffset(float elapsed_time) const
	  -> float // shortcut for places that care only about the beat
	{
		GetBeatArgs args;
		args.elapsed_time = elapsed_time;
		GetBeatAndBPSFromElapsedTimeNoOffset(args);
		return args.beat;
	}

	[[nodiscard]] auto GetElapsedTimeFromBeatNoOffset(float fBeat) const
	  -> float;
	[[nodiscard]] auto GetDisplayedBeat(float fBeat) const -> float;

	[[nodiscard]] auto HasBpmChanges() const -> bool
	{
		return GetTimingSegments(SEGMENT_BPM).size() > 1;
	}

	[[nodiscard]] auto HasStops() const -> bool
	{
		return !GetTimingSegments(SEGMENT_STOP).empty();
	}
	[[nodiscard]] auto HasDelays() const -> bool
	{
		return !GetTimingSegments(SEGMENT_DELAY).empty();
	}
	[[nodiscard]] auto HasWarps() const -> bool
	{
		return !GetTimingSegments(SEGMENT_WARP).empty();
	}
	[[nodiscard]] auto HasFakes() const -> bool
	{
		return !GetTimingSegments(SEGMENT_FAKE).empty();
	}

	[[nodiscard]] auto HasSpeedChanges() const -> bool;
	[[nodiscard]] auto HasScrollChanges() const -> bool;

	/**
	 * @brief Compare two sets of timing data to see if they are equal.
	 * @param other the other TimingData.
	 * @return the equality or lack thereof of the two TimingData.
	 */
	auto operator==(const TimingData& other) const -> bool
	{
		FOREACH_ENUM(TimingSegmentType, tst)
		{
			const auto& us = m_avpTimingSegments[tst];
			const auto& them = other.m_avpTimingSegments[tst];

			// optimization: check  std::vector sizes before contents
			if (us.size() != them.size()) {
				return false;
			}

			for (unsigned i = 0; i < us.size(); ++i) {
				/* UGLY: since TimingSegment's comparison compares base data,
				 * and the derived versions only compare derived data, we must
				 * manually call each. */
				if (!(*us[i]).TimingSegment::operator==(*them[i])) {
					return false;
				}
				if (!(*us[i]).operator==(*them[i])) {
					return false;
				}
			}
		}

		COMPARE(m_fBeat0OffsetInSeconds);
		return true;
	}

	/**
	 * @brief Compare two sets of timing data to see if they are not equal.
	 * @param other the other TimingData.
	 * @return the inequality or lack thereof of the two TimingData.
	 */
	auto operator!=(const TimingData& other) const -> bool
	{
		return !operator==(other);
	}

	void ScaleRegion(float fScale = 1,
					 int iStartRow = 0,
					 int iEndRow = MAX_NOTE_ROW,
					 bool bAdjustBPM = false);
	void InsertRows(int iStartRow, int iRowsToAdd);
	void DeleteRows(int iStartRow, int iRowsToDelete);

	void SortSegments(TimingSegmentType tst);

	[[nodiscard]] auto GetTimingSegments(TimingSegmentType tst) const
	  -> const std::vector<TimingSegment*>&
	{
		return const_cast<TimingData*>(this)->GetTimingSegments(tst);
	}
	auto GetTimingSegments(TimingSegmentType tst)
	  -> std::vector<TimingSegment*>&
	{
		return m_avpTimingSegments[tst];
	}

	/**
	 * @brief Tidy up the timing data, e.g. provide default BPMs, labels,
	 * tickcounts.
	 * @param allowEmpty true if completely empty TimingData should be left
	 *                   alone, false if it should be changed
	 */
	void TidyUpData(bool allowEmpty);

	// Lua
	void PushSelf(lua_State* L);

	/**
	 * @brief The file of the song/steps that use this TimingData.
	 *
	 * This is for informational purposes only.
	 */
	std::string m_sFile;

	/** @brief The initial offset of a song. */
	float m_fBeat0OffsetInSeconds{};

	// XXX: this breaks encapsulation. get rid of it ASAP
	[[nodiscard]] auto ToVectorString(TimingSegmentType tst, int dec = 6) const
	  -> std::vector<std::string>;

	/*	Wow it's almost like this should have been done a decade ago.
	Essentially what's happening here is the results of getelapsedtimeat(row)
	are pre-calculated and stored in a  std::vector that can be simply subset
	rather than values being recalculated millions of times per file. This only
	applies however to files for which there can be made an assumption of
	sequential execution I don't actually know for sure if any of negative
	bpms/stops/warps do this, or if mod maps have the power to fundamentally
	change timing data. If they don't then I suppose all of these checks aren't
	needed at all :/. Not my responsibility to investigate, though. - Mina.*/

	std::vector<float> ElapsedTimesAtAllRows;
	std::vector<float> ElapsedTimesAtNonEmptyRows;
	auto BuildAndGetEtaner(const std::vector<int>& nerv)
	  -> const std::vector<float>&;
	auto BuildAndGetEtar(int lastrow) -> const std::vector<float>&;
	void SetElapsedTimesAtAllRows(std::vector<float>& etar)
	{
		ElapsedTimesAtAllRows = etar;
	}

	[[nodiscard]] auto GetElapsedTimesAtAllRows() const -> std::vector<float>
	{
		return ElapsedTimesAtAllRows;
	}
	void UnsetElapsedTimesAtAllRows()
	{
		ElapsedTimesAtAllRows.clear();
		ElapsedTimesAtAllRows.shrink_to_fit();
	};
	void UnsetEtaner()
	{
		ElapsedTimesAtNonEmptyRows.clear();
		ElapsedTimesAtNonEmptyRows.shrink_to_fit();
	}

	[[nodiscard]] auto WhereUAtBro(float beat) const -> float;
	auto WhereUAtBro(float beat) -> float;
	[[nodiscard]] auto WhereUAtBroNoOffset(float beat) const -> float;
	auto WhereUAtBroNoOffset(float beat) -> float;
	auto WhereUAtBro(int row) -> float;

	auto ConvertReplayNoteRowsToTimestamps(const std::vector<int>& nrv,
										   float rate) -> std::vector<float>;

	bool ValidSequentialAssumption = true;
	void InvalidateSequentialAssmption() { ValidSequentialAssumption = false; }

	[[nodiscard]] auto IsSequentialAssumptionValid() const -> bool
	{
		return ValidSequentialAssumption;
	}

	void NegStopAndBPMCheck()
	{
		if (HasWarps()) {
			ValidSequentialAssumption = false;
			return;
		}

		auto& bpms = m_avpTimingSegments[SEGMENT_BPM];
		auto& stops = m_avpTimingSegments[SEGMENT_STOP];

		for (auto& i : bpms) {
			auto bpm = ToBPM(i);
			if (0 > bpm->GetBPM()) {
				Locator::getLogger()->warn("Sequential Assumption Invalidated.");
				ValidSequentialAssumption = false;
				return;
			}
		}

		for (auto& stop : stops) {
			auto s = ToStop(stop);
			if (0 > s->GetPause()) {
				Locator::getLogger()->warn("Sequential Assumption Invalidated.");
				ValidSequentialAssumption = false;
				return;
			}
		}
	}

  protected:
	// don't call this directly; use the derived-type overloads.
	void AddSegment(const TimingSegment* seg);

	// All of the following vectors must be sorted before gameplay.
	std::vector<TimingSegment*> m_avpTimingSegments[NUM_TimingSegmentType];
};

#undef COMPARE

#endif
