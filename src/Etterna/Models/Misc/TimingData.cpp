#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "NoteTypes.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "TimingData.h"

#include <cfloat>
#include <algorithm>
#include <cmath>

#include "AdjustSync.h"

static void
EraseSegment(std::vector<TimingSegment*>& vSegs, int index, TimingSegment* cur);
static const int INVALID_INDEX = -1;

TimingSegment*
GetSegmentAtRow(int iNoteRow, TimingSegmentType tst);

TimingData::TimingData(float fOffset)
  : m_fBeat0OffsetInSeconds(fOffset)
{
}

void
TimingData::Copy(const TimingData& cpy)
{
	/* de-allocate any old pointers we had */
	Clear();

	m_fBeat0OffsetInSeconds = cpy.m_fBeat0OffsetInSeconds;
	m_sFile = cpy.m_sFile;

	FOREACH_TimingSegmentType(tst)
	{
		const auto& vpSegs = cpy.m_avpTimingSegments[tst];

		for (auto* vpSeg : vpSegs)
			AddSegment(vpSeg);
	}
}

void
TimingData::Clear()
{
	/* Delete all pointers owned by this TimingData. */
	FOREACH_TimingSegmentType(tst)
	{
		auto& vSegs = m_avpTimingSegments[tst];
		for (auto& i : vSegs) {
			SAFE_DELETE(i);
		}

		vSegs.clear();
	}
	UnsetElapsedTimesAtAllRows();
	UnsetEtaner();
}

TimingData::~TimingData()
{
	Clear();
}

void
TimingData::PrepareLookup()
{
	// If multiple players have the same timing data, then adding to the
	// lookups would probably cause FindEntryInLookup to return the wrong
	// thing.  So release the lookups. -Kyz
	ReleaseLookup();
	const unsigned int segments_per_lookup = 16;
	const std::vector<TimingSegment*>* segs = m_avpTimingSegments;
	const auto& bpms = segs[SEGMENT_BPM];
	const auto& warps = segs[SEGMENT_WARP];
	const auto& stops = segs[SEGMENT_STOP];
	const auto& delays = segs[SEGMENT_DELAY];

	const unsigned int total_segments =
	  bpms.size() + warps.size() + stops.size() + delays.size();
	const auto lookup_entries = total_segments / segments_per_lookup;
	m_beat_start_lookup.reserve(lookup_entries);
	m_time_start_lookup.reserve(lookup_entries);
	for (auto curr_segment = segments_per_lookup; curr_segment < total_segments;
		 curr_segment += segments_per_lookup) {
		GetBeatStarts beat_start;
		beat_start.last_time = -m_fBeat0OffsetInSeconds;
		GetBeatArgs args;
		args.elapsed_time = FLT_MAX;
		GetBeatInternal(beat_start, args, curr_segment);
		m_beat_start_lookup.push_back(
		  lookup_item_t(args.elapsed_time, beat_start));

		GetBeatStarts time_start;
		time_start.last_time = -m_fBeat0OffsetInSeconds;
		m_time_start_lookup.push_back(
		  lookup_item_t(NoteRowToBeat(time_start.last_row), time_start));
	}
	// If there are less than two entries, then FindEntryInLookup in lookup
	// will always decide there's no appropriate entry.  So clear the table.
	// -Kyz
	if (m_beat_start_lookup.size() < 2) {
		ReleaseLookup();
	}
	// DumpLookupTables();
}

void
TimingData::ReleaseLookup()
{
	m_beat_start_lookup.clear();
	m_beat_start_lookup.shrink_to_fit();
	m_time_start_lookup.clear();
	m_time_start_lookup.shrink_to_fit();
}

std::string
SegInfoStr(const std::vector<TimingSegment*>& segs,
		   unsigned int index,
		   const std::string& name)
{
	if (index < segs.size()) {
		return ssprintf(
		  "%s: %d at %d", name.c_str(), index, segs[index]->GetRow());
	}
	return ssprintf("%s: %d at end", name.c_str(), index);
}

TimingData::beat_start_lookup_t::const_iterator
FindEntryInLookup(const TimingData::beat_start_lookup_t& lookup, float entry)
{
	if (lookup.empty()) {
		return lookup.end();
	}
	long lower = 0;
	long upper = static_cast<long>(lookup.size() - 1);
	if (lookup[lower].first > entry) {
		return lookup.end();
	}
	if (lookup[upper].first < entry) {
		// See explanation at the end of this function. -Kyz
		return lookup.begin() + upper - 1;
	}
	while (upper - lower > 1) {
		const auto next = (upper + lower) / 2;
		if (lookup[next].first > entry) {
			upper = next;
		} else if (lookup[next].first < entry) {
			lower = next;
		} else {
			lower = next;
			break;
		}
	}
	// If the time or beat being looked up is close enough to the starting
	// point that is returned, such as putting the time inside a stop or delay,
	// then it can make arrows unhittable.  So always return the entry before
	// the closest one to prevent that. -Kyz
	if (lower == 0) {
		return lookup.end();
	}
	return lookup.begin() + lower - 1;
}

bool
TimingData::empty() const
{
	FOREACH_TimingSegmentType(
	  tst) if (!GetTimingSegments(tst).empty()) return false;

	return true;
}

void
TimingData::CopyRange(int start_row,
					  int end_row,
					  TimingSegmentType copy_type,
					  int dest_row,
					  TimingData& dest) const
{
	const auto row_offset = dest_row - start_row;
	FOREACH_TimingSegmentType(seg_type)
	{
		if (seg_type == copy_type || copy_type == TimingSegmentType_Invalid) {
			const auto& segs = GetTimingSegments(seg_type);
			for (auto* seg : segs) {
				if (seg->GetRow() >= start_row && seg->GetRow() <= end_row) {
					auto* copy = seg->Copy();
					copy->SetRow(seg->GetRow() + row_offset);
					dest.AddSegment(copy);
					// TimingSegment::Copy creates a new segment with new, and
					// AddSegment copies it again, so delete the temp. -Kyz
					delete copy;
				}
			}
		}
	}
}

void
TimingData::ShiftRange(int start_row,
					   int end_row,
					   TimingSegmentType shift_type,
					   int shift_amount)
{
	FOREACH_TimingSegmentType(seg_type)
	{
		if (seg_type == shift_type || shift_type == TimingSegmentType_Invalid) {
			auto& segs = GetTimingSegments(seg_type);
			const auto first_row =
			  std::min(start_row, start_row + shift_amount);
			const auto last_row = std::max(end_row, end_row + shift_amount);
			const auto first_affected =
			  GetSegmentIndexAtRow(seg_type, first_row);
			size_t last_affected = GetSegmentIndexAtRow(seg_type, last_row);
			if (first_affected == INVALID_INDEX) {
				continue;
			}
			// Prance through the affected area twice.  The first time, changing
			// the rows of the segments, the second time removing segments that
			// have been run over by a segment being moved.  Attempts to combine
			// both operations into a single loop were error prone. -Kyz
			for (size_t i = first_affected;
				 i <= last_affected && i < segs.size();
				 ++i) {
				const auto seg_row = segs[i]->GetRow();
				if (seg_row > 0 && seg_row >= start_row && seg_row <= end_row) {
					const auto dest_row = std::max(seg_row + shift_amount, 0);
					segs[i]->SetRow(dest_row);
				}
			}
#define ERASE_SEG(s)                                                           \
	if (segs.size() > 1) {                                                     \
		EraseSegment(segs, s, segs[s]);                                        \
		--i;                                                                   \
		--last_affected;                                                       \
		erased = true;                                                         \
	}
			for (size_t i = first_affected;
				 i <= last_affected && i < segs.size();
				 ++i) {
				auto erased = false;
				auto seg_row = segs[i]->GetRow();
				if (i < segs.size() - 1) {
					const auto next_row = segs[i + 1]->GetRow();
					// This is a loop so that it will go back through and remove
					// all segments that were run over. -Kyz
					while (seg_row >= next_row && seg_row < start_row) {
						ERASE_SEG(i);
						if (i < segs.size()) {
							seg_row = segs[i]->GetRow();
						} else {
							seg_row = -1;
						}
					}
				}
				if (!erased && i > 0) {
					const auto prev_row = segs[i - 1]->GetRow();
					if (prev_row >= seg_row) {
						ERASE_SEG(i);
					}
				}
			}
#undef ERASE_SEG
		}
	}
}

void
TimingData::ClearRange(int start_row, int end_row, TimingSegmentType clear_type)
{
	FOREACH_TimingSegmentType(seg_type)
	{
		if (seg_type == clear_type || clear_type == TimingSegmentType_Invalid) {
			auto& segs = GetTimingSegments(seg_type);
			const auto first_affected =
			  GetSegmentIndexAtRow(seg_type, start_row);
			const auto last_affected = GetSegmentIndexAtRow(seg_type, end_row);
			if (first_affected == INVALID_INDEX) {
				continue;
			}
			for (auto index = last_affected; index >= first_affected; --index) {
				const auto seg_row = segs[index]->GetRow();
				if (segs.size() > 1 && seg_row > 0 && seg_row >= start_row &&
					seg_row <= end_row) {
					EraseSegment(segs, index, segs[index]);
				}
			}
		}
	}
}

void
TimingData::GetActualBPM(float& fMinBPMOut,
						 float& fMaxBPMOut,
						 float highest) const
{
	fMinBPMOut = FLT_MAX;
	fMaxBPMOut = 0;
	const auto& bpms = GetTimingSegments(SEGMENT_BPM);

	for (auto* bpm : bpms) {
		const auto fBPM = ToBPM(bpm)->GetBPM();
		fMaxBPMOut = std::clamp(std::max(fBPM, fMaxBPMOut), 0.F, highest);
		fMinBPMOut = std::min(fBPM, fMinBPMOut);
	}
}

float
TimingData::GetNextSegmentBeatAtRow(TimingSegmentType tst, int row) const
{
	const auto segs = GetTimingSegments(tst);
	for (auto* seg : segs) {
		if (seg->GetRow() <= row) {
			continue;
		}
		return seg->GetBeat();
	}
	return NoteRowToBeat(row);
}

float
TimingData::GetPreviousSegmentBeatAtRow(TimingSegmentType tst, int row) const
{
	float backup = -1;
	const auto segs = GetTimingSegments(tst);
	for (auto* seg : segs) {
		if (seg->GetRow() >= row) {
			break;
		}
		backup = seg->GetBeat();
	}
	return (backup > -1) ? backup : NoteRowToBeat(row);
}

int
TimingData::GetSegmentIndexAtRow(TimingSegmentType tst, int iRow) const
{
	const auto& vSegs = GetTimingSegments(tst);

	if (vSegs.empty())
		return INVALID_INDEX;

	const int min = 0;
	const int max = static_cast<int>(vSegs.size() - 1);
	auto l = min;
	auto r = max;
	while (l <= r) {
		const auto m = (l + r) / 2;
		if ((m == min || vSegs[m]->GetRow() <= iRow) &&
			(m == max || iRow < vSegs[m + 1]->GetRow())) {
			return m;
		}
		if (vSegs[m]->GetRow() <= iRow) {
			l = m + 1;
		} else {
			r = m - 1;
		}
	}

	// iRow is before the first segment of type tst
	return INVALID_INDEX;
}

struct ts_less
{
	bool operator()(const TimingSegment* x, const TimingSegment* y) const
	{
		return (*x) < (*y);
	}
};

// Multiply the BPM in the range [fStartBeat,fEndBeat) by fFactor.
void
TimingData::MultiplyBPMInBeatRange(int iStartIndex,
								   int iEndIndex,
								   float fFactor)
{
	// Change all other BPM segments in this range.
	auto& bpms = m_avpTimingSegments[SEGMENT_BPM];
	for (unsigned i = 0; i < bpms.size(); i++) {
		auto* bs = ToBPM(bpms[i]);
		const auto iStartIndexThisSegment = bs->GetRow();
		const auto bIsLastBPMSegment = i == bpms.size() - 1;
		const auto iStartIndexNextSegment =
		  bIsLastBPMSegment ? INT_MAX : bpms[i + 1]->GetRow();

		if (iStartIndexThisSegment <= iStartIndex &&
			iStartIndexNextSegment <= iStartIndex)
			continue;

		/* If this BPM segment crosses the beginning of the range,
		 * split it into two. */
		if (iStartIndexThisSegment < iStartIndex &&
			iStartIndexNextSegment > iStartIndex) {
			auto* b = new BPMSegment(iStartIndexNextSegment, bs->GetBPS());
			bpms.insert(bpms.begin() + i + 1, b);

			/* Don't apply the BPM change to the first half of the segment we
			 * just split, since it lies outside the range. */
			continue;
		}

		// If this BPM segment crosses the end of the range, split it into two.
		if (iStartIndexThisSegment < iEndIndex &&
			iStartIndexNextSegment > iEndIndex) {
			auto* b = new BPMSegment(iEndIndex, bs->GetBPS());
			bpms.insert(bpms.begin() + i + 1, b);
		} else if (iStartIndexNextSegment > iEndIndex)
			continue;

		bs->SetBPM(bs->GetBPM() * fFactor);
	}
}

bool
TimingData::IsWarpAtRow(int iRow) const
{
	const auto& warps = GetTimingSegments(SEGMENT_WARP);
	if (warps.empty())
		return false;

	const auto i = GetSegmentIndexAtRow(SEGMENT_WARP, iRow);
	if (i == -1) {
		return false;
	}
	const WarpSegment* s = ToWarp(warps[i]);
	const auto beatRow = NoteRowToBeat(iRow);
	if (s->GetBeat() <= beatRow && beatRow < (s->GetBeat() + s->GetLength())) {
		// Allow stops inside warps to allow things like stop, warp, stop, warp,
		// stop, and so on.
		if (GetTimingSegments(SEGMENT_STOP).empty() &&
			GetTimingSegments(SEGMENT_DELAY).empty()) {
			return true;
		}
		return !(GetStopAtRow(iRow) != 0.f || GetDelayAtRow(iRow) != 0.f);
	}
	return false;
}

bool
TimingData::IsFakeAtRow(int iRow) const
{
	const auto& fakes = GetTimingSegments(SEGMENT_FAKE);
	if (fakes.empty())
		return false;

	const auto i = GetSegmentIndexAtRow(SEGMENT_FAKE, iRow);
	if (i == -1) {
		return false;
	}
	const FakeSegment* s = ToFake(fakes[i]);
	const auto beatRow = NoteRowToBeat(iRow);
	return s->GetBeat() <= beatRow && beatRow < (s->GetBeat() + s->GetLength());
}

/* DummySegments: since our model relies on being able to get a segment at will,
 * whether one exists or not, we have a bunch of dummies to return if there is
 * no segment. It's kind of kludgy, but when we have functions making
 * indiscriminate calls to get segments at arbitrary rows, I think it's the
 * best solution we've got for now.
 *
 * Note that types whose SegmentEffectAreas are "Indefinite" are NULL here,
 * because they should never need to be used; we always have at least one such
 * segment in the TimingData, and if not, we'll crash anyway. -- vyhd */
static const TimingSegment* DummySegments[NUM_TimingSegmentType] = {
	nullptr, // BPMSegment
	new StopSegment, new DelaySegment,
	nullptr, // TimeSignatureSegment
	new WarpSegment,
	nullptr, // LabelSegment
	nullptr, // TickcountSegment
	nullptr, // ComboSegment
	nullptr, // SpeedSegment
	nullptr, // ScrollSegment
	new FakeSegment
};

const TimingSegment*
TimingData::GetSegmentAtRow(int iNoteRow, TimingSegmentType tst) const
{
	const auto& vSegments = GetTimingSegments(tst);

	if (vSegments.empty())
		return DummySegments[tst];

	const auto index = GetSegmentIndexAtRow(tst, iNoteRow);
	if (index < 0)
		return DummySegments[tst];
	const TimingSegment* seg = vSegments[index];

	switch (seg->GetEffectType()) {
		case SegmentEffectType_Indefinite: {
			// this segment is in effect at this row
			return seg;
		}
		default: {
			// if the returned segment isn't exactly on this row,
			// we don't want it, return a dummy instead
			if (seg->GetRow() == iNoteRow)
				return seg;

			return DummySegments[tst];
		}
	}

	FAIL_M("Could not find timing segment for row");
}

TimingSegment*
TimingData::GetSegmentAtRow(int iNoteRow, TimingSegmentType tst)
{
	return const_cast<TimingSegment*>(
	  static_cast<const TimingData*>(this)->GetSegmentAtRow(iNoteRow, tst));
}

static void
EraseSegment(std::vector<TimingSegment*>& vSegs, int index, TimingSegment* cur)
{
#ifdef WITH_LOGGING_TIMING_DATA
	Locator::getLogger()->trace("EraseSegment({}, {})", index, cur);
	cur->DebugPrint();
#endif

	vSegs.erase(vSegs.begin() + index);
	SAFE_DELETE(cur);
}

// NOTE: the pointer we're passed is a reference to a temporary,
// so we must deep-copy it (with ::Copy) for new allocations.
void
TimingData::AddSegment(const TimingSegment* seg)
{
#ifdef WITH_LOGGING_TIMING_DATA
	Locator::getLogger()->trace("AddSegment({})",
			   TimingSegmentTypeToString(seg->GetType()).c_str());
	seg->DebugPrint();
#endif

	const auto tst = seg->GetType();
	auto& vSegs = m_avpTimingSegments[tst];

	// OPTIMIZATION: if this is our first segment, push and return.
	if (vSegs.empty()) {
		vSegs.push_back(seg->Copy());
		return;
	}

	const auto index = GetSegmentIndexAtRow(tst, seg->GetRow());
	ASSERT(index != INVALID_INDEX);
	auto* cur = vSegs[index];

	const auto bIsNotable = seg->IsNotable();
	const auto bOnSameRow = seg->GetRow() == cur->GetRow();

	// ignore changes that are zero and don't overwrite an existing segment
	if (!bIsNotable && !bOnSameRow)
		return;

	switch (seg->GetEffectType()) {
		case SegmentEffectType_Row:
		case SegmentEffectType_Range: {
			// if we're overwriting a change with a non-notable
			// one, take it to mean deleting the existing segment
			if (bOnSameRow && !bIsNotable) {
				EraseSegment(vSegs, index, cur);
				return;
			}

			break;
		}
		case SegmentEffectType_Indefinite: {
			auto* prev = cur;

			// get the segment before last; if we're on the same
			// row, get the segment in effect before 'cur'
			if (bOnSameRow && index > 0) {
				prev = vSegs[index - 1];
			}

			// If there is another segment after this one, it might become
			// redundant when this one is inserted.
			// If the next segment is redundant, we want to move its starting
			// row to the row the new segment is being added at instead of
			// erasing it and adding the new segment. If the new segment is also
			// redundant, erase the next segment because that effectively moves
			// it back to the prev segment. -Kyz
			if (static_cast<size_t>(index) < vSegs.size() - 1) {
				auto* next = vSegs[index + 1];
				if ((*seg) == (*next)) {
					// The segment after this new one is redundant.
					if ((*seg) == (*prev)) {
						// This new segment is redundant.  Erase the next
						// segment and ignore this new one.
						EraseSegment(vSegs, index + 1, next);
						if (prev != cur) {
							EraseSegment(vSegs, index, cur);
						}
						return;
					}
					// Move the next segment's start back to this row.
					next->SetRow(seg->GetRow());
					if (prev != cur) {
						EraseSegment(vSegs, index, cur);
					}
					return;
				}
				// if true, this is redundant segment change
				if ((*prev) == (*seg)) {
					if (prev != cur) {
						EraseSegment(vSegs, index, cur);
					}
					return;
				}
			} else {
				// if true, this is redundant segment change
				if ((*prev) == (*seg)) {
					if (prev != cur) {
						EraseSegment(vSegs, index, cur);
					}
					return;
				}
			}
			break;
		}
		default:
			break;
	}

	// the segment at or before this row is equal to the new one; ignore it
	if (bOnSameRow && (*cur) == (*seg)) {
#ifdef WITH_LOGGING_TIMING_DATA
		Locator::getLogger()->trace("equals previous segment, ignoring");
#endif
		return;
	}

	// Copy() the segment (which allocates a new segment), assign it
	// to the position of the old one, then delete the old pointer.
	auto* cpy = seg->Copy();

	if (bOnSameRow) {
		// delete the existing pointer and replace it
		SAFE_DELETE(cur);
		vSegs[index] = cpy;
	} else {
		// copy and insert a new segment
		std::vector<TimingSegment*>::iterator it;
		it = upper_bound(vSegs.begin(), vSegs.end(), cpy, ts_less());
		vSegs.insert(it, cpy);
	}
}

bool
TimingData::DoesLabelExist(const std::string& sLabel) const
{
	const auto& labels = GetTimingSegments(SEGMENT_LABEL);
	for (auto* label : labels) {
		if (ToLabel(label)->GetLabel() == sLabel)
			return true;
	}
	return false;
}

void
TimingData::GetBeatAndBPSFromElapsedTime(GetBeatArgs& args) const
{
	args.elapsed_time += GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate *
						 PREFSMAN->m_fGlobalOffsetSeconds;
	GetBeatAndBPSFromElapsedTimeNoOffset(args);
}

enum
{
	FOUND_WARP,
	FOUND_WARP_DESTINATION,
	FOUND_BPM_CHANGE,
	FOUND_STOP,
	FOUND_DELAY,
	FOUND_STOP_DELAY,
	// we have these two on the same row.
	FOUND_MARKER,
	NOT_FOUND
};

void
FindEvent(int& event_row,
		  int& event_type,
		  TimingData::GetBeatStarts& start,
		  float beat,
		  bool find_marker,
		  const std::vector<TimingSegment*>& bpms,
		  const std::vector<TimingSegment*>& warps,
		  const std::vector<TimingSegment*>& stops,
		  const std::vector<TimingSegment*>& delays)
{
	if (start.is_warping) {
		const auto eventBeat = BeatToNoteRow(start.warp_destination);
		if (eventBeat < event_row) {
			event_row = eventBeat;
			event_type = FOUND_WARP_DESTINATION;
		}
	}
	if (start.bpm < bpms.size() && bpms[start.bpm]->GetRow() < event_row) {
		event_row = bpms[start.bpm]->GetRow();
		event_type = FOUND_BPM_CHANGE;
	}
	if (start.delay < delays.size() &&
		delays[start.delay]->GetRow() < event_row) {
		event_row = delays[start.delay]->GetRow();
		event_type = FOUND_DELAY;
	}
	if (find_marker) {
		const auto eventBeat = BeatToNoteRow(beat);
		if (eventBeat < event_row) {
			event_row = eventBeat;
			event_type = FOUND_MARKER;
		}
	}
	if (start.stop < stops.size() && stops[start.stop]->GetRow() < event_row) {
		const auto tmp_row = event_row;
		event_row = stops[start.stop]->GetRow();
		event_type = (tmp_row == event_row) ? FOUND_STOP_DELAY : FOUND_STOP;
	}
	if (start.warp < warps.size() && warps[start.warp]->GetRow() < event_row) {
		event_row = warps[start.warp]->GetRow();
		event_type = FOUND_WARP;
	}
}

void
TimingData::GetBeatInternal(GetBeatStarts& start,
							GetBeatArgs& args,
							unsigned int max_segment) const
{
	const auto *segs = m_avpTimingSegments;
	const auto& bpms = segs[SEGMENT_BPM];
	const auto& warps = segs[SEGMENT_WARP];
	const auto& stops = segs[SEGMENT_STOP];
	const auto& delays = segs[SEGMENT_DELAY];
	auto curr_segment = start.bpm + start.warp + start.stop + start.delay;

	auto bps = GetBPMAtRow(start.last_row) / 60.0f;
#define INC_INDEX(index)                                                       \
	++curr_segment;                                                            \
	++(index);

	while (curr_segment < max_segment) {
		auto event_row = INT_MAX;
		int event_type = NOT_FOUND;
		FindEvent(
		  event_row, event_type, start, 0, false, bpms, warps, stops, delays);
		if (event_type == NOT_FOUND) {
			break;
		}
		auto time_to_next_event =
		  start.is_warping ? 0
						   : NoteRowToBeat(event_row - start.last_row) / bps;
		auto next_event_time = start.last_time + time_to_next_event;
		if (args.elapsed_time < next_event_time) {
			break;
		}
		start.last_time = next_event_time;
		switch (event_type) {
			case FOUND_WARP_DESTINATION:
				start.is_warping = false;
				break;
			case FOUND_BPM_CHANGE:
				bps = ToBPM(bpms[start.bpm])->GetBPS();
				INC_INDEX(start.bpm);
				break;
			case FOUND_DELAY:
			case FOUND_STOP_DELAY: {
				const DelaySegment* ss = ToDelay(delays[start.delay]);
				time_to_next_event = ss->GetPause();
				next_event_time = start.last_time + time_to_next_event;
				if (args.elapsed_time < next_event_time) {
					args.freeze_out = false;
					args.delay_out = true;
					args.beat = ss->GetBeat();
					args.bps_out = bps;
					return;
				}
				start.last_time = next_event_time;
				INC_INDEX(start.delay);
				if (event_type == FOUND_DELAY) {
					break;
				}
			}
			case FOUND_STOP: {
				const StopSegment* ss = ToStop(stops[start.stop]);
				time_to_next_event = ss->GetPause();
				next_event_time = start.last_time + time_to_next_event;
				if (args.elapsed_time < next_event_time) {
					args.freeze_out = true;
					args.delay_out = false;
					args.beat = ss->GetBeat();
					args.bps_out = bps;
					return;
				}
				start.last_time = next_event_time;
				INC_INDEX(start.stop);
				break;
			}
			case FOUND_WARP: {
				start.is_warping = true;
				const WarpSegment* ws = ToWarp(warps[start.warp]);
				const auto warp_sum = ws->GetLength() + ws->GetBeat();
				if (warp_sum > start.warp_destination) {
					start.warp_destination = warp_sum;
				}
				args.warp_begin_out = event_row;
				args.warp_dest_out = start.warp_destination;
				INC_INDEX(start.warp);
				break;
			}
			default:
				break;
		}
		start.last_row = event_row;
	}
#undef INC_INDEX
	if (args.elapsed_time == FLT_MAX) {
		args.elapsed_time = start.last_time;
	}
	args.beat = NoteRowToBeat(start.last_row) +
				(args.elapsed_time - start.last_time) * bps;
	args.bps_out = bps;
}

void
TimingData::GetBeatAndBPSFromElapsedTimeNoOffset(GetBeatArgs& args) const
{
	GetBeatStarts start;
	start.last_time = -m_fBeat0OffsetInSeconds;
	const auto looked_up_start =
	  FindEntryInLookup(m_beat_start_lookup, args.elapsed_time);
	if (looked_up_start != m_beat_start_lookup.end()) {
		start = looked_up_start->second;
	}
	GetBeatInternal(start, args, INT_MAX);
}

float
TimingData::GetElapsedTimeInternal(GetBeatStarts& start,
								   float beat,
								   unsigned int max_segment) const
{
	const auto *segs = m_avpTimingSegments;
	const auto& bpms = segs[SEGMENT_BPM];
	const auto& warps = segs[SEGMENT_WARP];
	const auto& stops = segs[SEGMENT_STOP];
	const auto& delays = segs[SEGMENT_DELAY];
	auto curr_segment = start.bpm + start.warp + start.stop + start.delay;

	auto bps = GetBPMAtRow(start.last_row) / 60.0f;
#define INC_INDEX(index)                                                       \
	++curr_segment;                                                            \
	++(index);
	const auto find_marker = beat < FLT_MAX;

	while (curr_segment < max_segment) {
		auto event_row = INT_MAX;
		int event_type = NOT_FOUND;
		FindEvent(event_row,
				  event_type,
				  start,
				  beat,
				  find_marker,
				  bpms,
				  warps,
				  stops,
				  delays);
		auto time_to_next_event =
		  start.is_warping ? 0
						   : NoteRowToBeat(event_row - start.last_row) / bps;
		auto next_event_time = start.last_time + time_to_next_event;
		start.last_time = next_event_time;
		switch (event_type) {
			case FOUND_WARP_DESTINATION:
				start.is_warping = false;
				break;
			case FOUND_BPM_CHANGE:
				bps = ToBPM(bpms[start.bpm])->GetBPS();
				INC_INDEX(start.bpm);
				break;
			case FOUND_STOP:
			case FOUND_STOP_DELAY:
				time_to_next_event = ToStop(stops[start.stop])->GetPause();
				next_event_time = start.last_time + time_to_next_event;
				start.last_time = next_event_time;
				INC_INDEX(start.stop);
				break;
			case FOUND_DELAY:
				time_to_next_event = ToDelay(delays[start.delay])->GetPause();
				next_event_time = start.last_time + time_to_next_event;
				start.last_time = next_event_time;
				INC_INDEX(start.delay);
				break;
			case FOUND_MARKER:
				return start.last_time;
			case FOUND_WARP: {
				start.is_warping = true;
				auto* ws = ToWarp(warps[start.warp]);
				const auto warp_sum = ws->GetLength() + ws->GetBeat();
				if (warp_sum > start.warp_destination) {
					start.warp_destination = warp_sum;
				}
				INC_INDEX(start.warp);
				break;
			}
			default:
				break;
		}
		start.last_row = event_row;
	}
#undef INC_INDEX
	return start.last_time;
}

float
TimingData::GetElapsedTimeFromBeat(float fBeat) const
{
	return GetElapsedTimeFromBeatNoOffset(fBeat) -
		   GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate *
			 PREFSMAN->m_fGlobalOffsetSeconds;
}

float
TimingData::GetElapsedTimeFromBeatNoOffset(float fBeat) const
{
	GetBeatStarts start;
	start.last_time = -m_fBeat0OffsetInSeconds;
	const auto looked_up_start = FindEntryInLookup(m_time_start_lookup, fBeat);
	if (looked_up_start != m_time_start_lookup.end()) {
		start = looked_up_start->second;
	}
	GetElapsedTimeInternal(start, fBeat, INT_MAX);
	return start.last_time;
}

float
TimingData::GetDisplayedBeat(float fBeat) const
{
	float fOutBeat = 0;
	unsigned i = 0;
	const auto& scrolls = m_avpTimingSegments[SEGMENT_SCROLL];
	for (i = 0; i < scrolls.size() - 1; i++) {
		if (scrolls[i + 1]->GetBeat() > fBeat)
			break;
		fOutBeat += (scrolls[i + 1]->GetBeat() - scrolls[i]->GetBeat()) *
					ToScroll(scrolls[i])->GetRatio();
	}
	fOutBeat +=
	  (fBeat - scrolls[i]->GetBeat()) * ToScroll(scrolls[i])->GetRatio();
	return fOutBeat;
}

void
TimingData::ScaleRegion(float fScale,
						int iStartRow,
						int iEndRow,
						bool bAdjustBPM)
{
	ASSERT(fScale > 0);
	ASSERT(iStartRow >= 0);
	ASSERT(iStartRow < iEndRow);

	const auto length = iEndRow - iStartRow;
	const int newLength = static_cast<int>(std::lround(fScale * static_cast<float>(length)));

	FOREACH_TimingSegmentType(tst) for (auto& j : m_avpTimingSegments[tst])
	  j->Scale(iStartRow, length, newLength);

	// adjust BPM changes to preserve timing
	if (bAdjustBPM) {
		const auto iNewEndIndex = iStartRow + newLength;
		const auto fEndBPMBeforeScaling = GetBPMAtRow(iNewEndIndex);
		auto& bpms = m_avpTimingSegments[SEGMENT_BPM];

		// adjust BPM changes "between" iStartIndex and iNewEndIndex
		for (auto& i : bpms) {
			auto* bpm = ToBPM(i);
			const auto iSegStart = bpm->GetRow();
			if (iSegStart <= iStartRow)
				continue;
			if (iSegStart >= iNewEndIndex)
				continue;
			bpm->SetBPM(bpm->GetBPM() * fScale);
		}

		// set BPM at iStartIndex and iNewEndIndex.
		SetBPMAtRow(iStartRow, GetBPMAtRow(iStartRow) * fScale);
		SetBPMAtRow(iNewEndIndex, fEndBPMBeforeScaling);
	}
}

void
TimingData::InsertRows(int iStartRow, int iRowsToAdd)
{
	FOREACH_TimingSegmentType(tst)
	{
		auto& segs = m_avpTimingSegments[tst];
		for (auto* seg : segs) {
			if (seg->GetRow() < iStartRow)
				continue;
			seg->SetRow(seg->GetRow() + iRowsToAdd);
		}
	}

	if (iStartRow == 0) {
		/* If we're shifting up at the beginning, we just shifted up the first
		 * BPMSegment. That segment must always begin at 0. */
		auto& bpms = m_avpTimingSegments[SEGMENT_BPM];
		ASSERT_M(!bpms.empty(),
				 "There must be at least one BPM Segment in the chart!");
		bpms[0]->SetRow(0);
	}
}

// Delete timing changes in [iStartRow, iStartRow + iRowsToDelete) and shift up.
void
TimingData::DeleteRows(int iStartRow, int iRowsToDelete)
{
	FOREACH_TimingSegmentType(tst)
	{
		// Don't delete the indefinite segments that are still in effect
		// at the end row; rather, shift them so they start there.
		auto* tsEnd = GetSegmentAtRow(iStartRow + iRowsToDelete, tst);
		if (tsEnd != nullptr &&
			tsEnd->GetEffectType() == SegmentEffectType_Indefinite &&
			iStartRow <= tsEnd->GetRow() &&
			tsEnd->GetRow() < iStartRow + iRowsToDelete) {
			// The iRowsToDelete will eventually be subtracted out
			Locator::getLogger()->trace("Segment at row {} shifted to {}",
					   tsEnd->GetRow(),
					   iStartRow + iRowsToDelete);
			tsEnd->SetRow(iStartRow + iRowsToDelete);
		}

		// Now delete and shift up
		auto& segs = m_avpTimingSegments[tst];
		for (unsigned j = 0; j < segs.size(); j++) {
			auto* seg = segs[j];
			// Before deleted region:
			if (seg->GetRow() < iStartRow)
				continue;
			// Inside deleted region:
			if (seg->GetRow() < iStartRow + iRowsToDelete) {
				segs.erase(segs.begin() + j, segs.begin() + j + 1);
				--j;
				continue;
			}

			// After deleted regions:
			seg->SetRow(seg->GetRow() - iRowsToDelete);
		}
	}
}

float
TimingData::GetDisplayedSpeedPercent(float fBeat, float fMusicSeconds) const
{

	const auto& speeds = GetTimingSegments(SEGMENT_SPEED);
	if (speeds.empty()) {
#ifdef DEBUG
		Locator::getLogger()->trace("No speed segments found: using default value.");
#endif
		return 1.0f;
	}

	const auto index = GetSegmentIndexAtBeat(SEGMENT_SPEED, fBeat);

	if (index < 0) {
#ifdef DEBUG
		Locator::getLogger()->trace("Speed segment negative index: using default value");
#endif
		return 1.0f;
	}

	const SpeedSegment* seg = ToSpeed(speeds[index]);
	const auto fStartBeat = seg->GetBeat();
	const auto fStartTime =
	  WhereUAtBro(fStartBeat) - GetDelayAtBeat(fStartBeat);
	float fEndTime = 0.f;
	const auto fCurTime = fMusicSeconds;

	if (seg->GetUnit() == SpeedSegment::UNIT_SECONDS) {
		fEndTime = fStartTime + seg->GetDelay();
	} else {
		fEndTime = WhereUAtBro(fStartBeat + seg->GetDelay()) -
				   GetDelayAtBeat(fStartBeat + seg->GetDelay());
	}

	auto* first = ToSpeed(speeds[0]);

	if ((index == 0 && first->GetDelay() > 0.0) && fCurTime < fStartTime) {
		return 1.0f;
	}
	if (fEndTime >= fCurTime && (index > 0 || first->GetDelay() > 0.0)) {
		const auto fPriorSpeed =
		  (index == 0) ? 1 : ToSpeed(speeds[index - 1])->GetRatio();

		const auto fTimeUsed = fCurTime - fStartTime;
		const auto fDuration = fEndTime - fStartTime;
		const auto fRatioUsed = fDuration == 0.0 ? 1 : fTimeUsed / fDuration;

		const auto fDistance = fPriorSpeed - seg->GetRatio();
		const auto fRatioNeed = fRatioUsed * -fDistance;
		return (fPriorSpeed + fRatioNeed);
	}

	return seg->GetRatio();
}

void
TimingData::TidyUpData(bool allowEmpty)
{
	// Empty TimingData is used to implement steps with no timing of their
	// own.  Don't override this.
	if (allowEmpty && empty())
		return;

	// If there are no BPM segments, provide a default.
	auto* segs = m_avpTimingSegments;
	if (segs[SEGMENT_BPM].empty()) {
        Locator::getLogger()->info("Song file {} has no BPM segments, default provided.", m_sFile);
		AddSegment(BPMSegment(0, 60));
	}

	// Make sure the first BPM segment starts at beat 0.
	if (segs[SEGMENT_BPM][0]->GetRow() != 0)
		segs[SEGMENT_BPM][0]->SetRow(0);

	// If no time signature specified, assume default time for the whole song.
	if (segs[SEGMENT_TIME_SIG].empty())
		AddSegment(TimeSignatureSegment(0));

	// Likewise, if no tickcount signature is specified, assume 4 ticks
	// per beat for the entire song. The default of 4 is chosen more
	// for compatibility with the main Pump series than anything else.
	// (TickcountSegment's constructor handles that now. -- vyhd)
	if (segs[SEGMENT_TICKCOUNT].empty())
		AddSegment(TickcountSegment(0));

	// Have a default combo segment of one just in case.
	if (segs[SEGMENT_COMBO].empty())
		AddSegment(ComboSegment(0));

	// Have a default label segment just in case.
	if (segs[SEGMENT_LABEL].empty())
		AddSegment(LabelSegment(0, "Song Start"));

	// Always be sure there is a starting speed.
	if (segs[SEGMENT_SPEED].empty())
		AddSegment(SpeedSegment(0));

	// Always be sure there is a starting scrolling factor.
	if (segs[SEGMENT_SCROLL].empty())
		AddSegment(ScrollSegment(0));
}

void
TimingData::SortSegments(TimingSegmentType tst)
{
	auto& vSegments = m_avpTimingSegments[tst];
	sort(vSegments.begin(), vSegments.end());
}

bool
TimingData::HasSpeedChanges() const
{
	const auto& speeds = GetTimingSegments(SEGMENT_SPEED);
	return (speeds.size() > 1 || ToSpeed(speeds[0])->GetRatio() != 1);
}

bool
TimingData::HasScrollChanges() const
{
	const auto& scrolls = GetTimingSegments(SEGMENT_SCROLL);
	return (scrolls.size() > 1 || ToScroll(scrolls[0])->GetRatio() != 1);
}

void
TimingData::NoteRowToMeasureAndBeat(int iNoteRow,
									int& iMeasureIndexOut,
									int& iBeatIndexOut,
									int& iRowsRemainder) const
{
	iMeasureIndexOut = 0;
	const auto& tSigs = GetTimingSegments(SEGMENT_TIME_SIG);
	for (unsigned i = 0; i < tSigs.size(); i++) {
		auto* curSig = ToTimeSignature(tSigs[i]);
		const auto iSegmentEndRow =
		  (i + 1 == tSigs.size()) ? INT_MAX : curSig->GetRow();

		const auto iRowsPerMeasureThisSegment = curSig->GetNoteRowsPerMeasure();

		if (iNoteRow >= curSig->GetRow()) {
			// iNoteRow lands in this segment
			const auto iNumRowsThisSegment = iNoteRow - curSig->GetRow();
			const auto iNumMeasuresThisSegment =
			  (iNumRowsThisSegment) /
			  iRowsPerMeasureThisSegment; // don't round up
			iMeasureIndexOut += iNumMeasuresThisSegment;
			iBeatIndexOut = iNumRowsThisSegment / iRowsPerMeasureThisSegment;
			iRowsRemainder = iNumRowsThisSegment % iRowsPerMeasureThisSegment;
			return;
		}
		// iNoteRow lands after this segment
		const auto iNumRowsThisSegment = iSegmentEndRow - curSig->GetRow();
		const auto iNumMeasuresThisSegment =
		  (iNumRowsThisSegment + iRowsPerMeasureThisSegment - 1) /
		  iRowsPerMeasureThisSegment; // round up
		iMeasureIndexOut += iNumMeasuresThisSegment;
	}

	FAIL_M("Failed to get measure and beat for note row");
}

std::vector<std::string>
TimingData::ToVectorString(TimingSegmentType tst, int dec) const
{
	const auto segs = GetTimingSegments(tst);
	std::vector<std::string> ret;

	ret.reserve(segs.size());
	for (auto* seg : segs) {
		ret.push_back(seg->ToString(dec));
	}
	return ret;
}

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

#define TIMING_DATA_RETURNS_NUMBERS                                            \
	THEME->GetMetricB("TimingData", "GetReturnsNumbers")

// This breaks encapsulation just as much as TimingData::ToVectorString
// does. But, it exists solely for the purpose of providing lua access, so it's
// as okay as all the other lua stuff that reaches past the encapsulation.
void
TimingSegmentSetToLuaTable(TimingData* td, TimingSegmentType tst, lua_State* L);

void
TimingSegmentSetToLuaTable(TimingData* td, TimingSegmentType tst, lua_State* L)
{
	const auto segs = td->GetTimingSegments(tst);
	lua_createtable(L, static_cast<int>(segs.size()), 0);
	if (tst == SEGMENT_LABEL) {
		for (int i = 0; i < segs.size(); ++i) {
			lua_createtable(L, 2, 0);
			lua_pushnumber(L, segs[i]->GetBeat());
			lua_rawseti(L, -2, 1);
			lua_pushstring(L, (ToLabel(segs[i]))->GetLabel().c_str());
			lua_rawseti(L, -2, 2);
			lua_rawseti(L, -2, i + 1);
		}
	} else {
		for (int i = 0; i < segs.size(); ++i) {
			auto values = segs[i]->GetValues();
			lua_createtable(L, static_cast<int>(values.size()) + 1, 0);
			lua_pushnumber(L, segs[i]->GetBeat());
			lua_rawseti(L, -2, 1);
			for (int v = 0; v < values.size(); ++v) {
				lua_pushnumber(L, values[v]);
				lua_rawseti(L, -2, v + 2);
			}
			lua_rawseti(L, -2, i + 1);
		}
	}
}

float
TimingData::WhereUAtBro(float beat)
{
	if (beat < 0)
		return 0;
	const size_t row = BeatToNoteRow(beat);

	if (ValidSequentialAssumption && row < ElapsedTimesAtAllRows.size() &&
		!AdjustSync::IsSyncDataChanged())
		return ElapsedTimesAtAllRows[row] -
			   GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate *
				 PREFSMAN->m_fGlobalOffsetSeconds;

	return GetElapsedTimeFromBeat(beat);
}

float
TimingData::WhereUAtBro(float beat) const
{
	if (beat < 0)
		return 0;
	const size_t row = BeatToNoteRow(beat);

	if (ValidSequentialAssumption && row < ElapsedTimesAtAllRows.size() &&
		!AdjustSync::IsSyncDataChanged())
		return ElapsedTimesAtAllRows[row] -
			   GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate *
				 PREFSMAN->m_fGlobalOffsetSeconds;

	return GetElapsedTimeFromBeat(beat);
}

float
TimingData::WhereUAtBro(int row)
{
	if (row < 0)
		return 0;

	if (ValidSequentialAssumption &&
		static_cast<size_t>(row) < ElapsedTimesAtAllRows.size() &&
		!AdjustSync::IsSyncDataChanged())
		return ElapsedTimesAtAllRows[row] -
			   GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate *
				 PREFSMAN->m_fGlobalOffsetSeconds;

	return GetElapsedTimeFromBeat(NoteRowToBeat(row));
}

float
TimingData::WhereUAtBroNoOffset(float beat)
{
	if (beat < 0)
		return 0;
	const size_t row = BeatToNoteRow(beat);

	if (ValidSequentialAssumption && row < ElapsedTimesAtAllRows.size() &&
		!AdjustSync::IsSyncDataChanged())
		return ElapsedTimesAtAllRows[row];

	return GetElapsedTimeFromBeatNoOffset(beat);
}

float
TimingData::WhereUAtBroNoOffset(float beat) const
{
	if (beat < 0)
		return 0;
	const size_t row = BeatToNoteRow(beat);

	if (ValidSequentialAssumption && row < ElapsedTimesAtAllRows.size() &&
		!AdjustSync::IsSyncDataChanged())
		return ElapsedTimesAtAllRows[row];

	return GetElapsedTimeFromBeatNoOffset(beat);
}

std::vector<float>
TimingData::ConvertReplayNoteRowsToTimestamps(const std::vector<int>& nrv,
											  float rate)
{
	std::vector<float> o;
	o.reserve(nrv.size());
	for (auto nr : nrv)
		o.emplace_back(WhereUAtBro(nr) / rate);
	return o;
}

const std::vector<float>&
TimingData::BuildAndGetEtaner(const std::vector<int>& nerv)
{
	ElapsedTimesAtNonEmptyRows.clear();

	const std::vector<TimingSegment*>* segs = m_avpTimingSegments;
	const auto& bpms = segs[SEGMENT_BPM];
	const auto& warps = segs[SEGMENT_WARP];
	const auto& stops = segs[SEGMENT_STOP];
	const auto& delays = segs[SEGMENT_DELAY];

	// use original functions and move on if time isn't linear -mina
	if (!stops.empty() || !warps.empty() || !delays.empty() || nerv.empty()) {
		for (const auto& n : nerv)
			ElapsedTimesAtNonEmptyRows.emplace_back(
			  GetElapsedTimeFromBeatNoOffset(NoteRowToBeat(n)));
		return ElapsedTimesAtNonEmptyRows;
	}

	if (nerv.empty())
		return ElapsedTimesAtAllRows;

	// handle simple single bpm case if applicable -mina
	if (bpms.size() == 1) {
		const auto bps = GetBPMAtRow(0) / 60.f;
		for (const auto& n : nerv) {
			ElapsedTimesAtNonEmptyRows.emplace_back(NoteRowToBeat(n) / bps -
													m_fBeat0OffsetInSeconds);
			// LOG->Trace("%f", abs(ElapsedTimesAtNonEmptyRows.back() -
			// GetElapsedTimeFromBeatNoOffset(NoteRowToBeat(n))));
		}
		return ElapsedTimesAtNonEmptyRows;
	}

	if (bpms.size() > 1) {
		auto last_time = 0.f;
		auto bps = GetBPMAtRow(0) / 60.f;
		auto lastbpmrow = 0;
		size_t idx = 0;
		auto event_row = 0;
		auto time_to_next_event = 0.f;

		// start at one because the initial bpm is already handled
		for (size_t i = 1; i < bpms.size(); ++i) {
			event_row = bpms[i]->GetRow();
			time_to_next_event = NoteRowToBeat(event_row - lastbpmrow) / bps;
			const auto next_event_time = last_time + time_to_next_event;
			if (bps <= 0)
				Locator::getLogger()->fatal("Found {} bps in file {} - Very likely to crash.", bps, m_sFile);
			while (idx < nerv.size() && nerv[idx] <= event_row) {
				const auto perc = static_cast<float>(nerv[idx] - lastbpmrow) /
								  static_cast<float>(event_row - lastbpmrow);
				ElapsedTimesAtNonEmptyRows.emplace_back(
				  last_time + time_to_next_event * perc -
				  m_fBeat0OffsetInSeconds);
				// LOG->Trace("%f", abs(ElapsedTimesAtNonEmptyRows.back() -
				// GetElapsedTimeFromBeatNoOffset(NoteRowToBeat(nerv[idx]))));
				++idx;
			}
			last_time = next_event_time;
			bps = ToBPM(bpms[i])->GetBPM() / 60.f;
			lastbpmrow = event_row;
		}

		// fill out any timestamps that lie beyond the last bpm change
		time_to_next_event = NoteRowToBeat(nerv.back() - lastbpmrow) / bps;
		while (idx < nerv.size()) {
			const auto perc = static_cast<float>(nerv[idx] - lastbpmrow) /
							  static_cast<float>(nerv.back() - lastbpmrow);
			ElapsedTimesAtNonEmptyRows.emplace_back(
			  last_time + time_to_next_event * perc - m_fBeat0OffsetInSeconds);
			// LOG->Trace("%f", abs(ElapsedTimesAtNonEmptyRows.back() -
			// GetElapsedTimeFromBeatNoOffset(NoteRowToBeat(nerv[idx]))));
			++idx;
		}
		return ElapsedTimesAtNonEmptyRows;
	}
	// should never get here but prevent compiler whining
	return ElapsedTimesAtAllRows;
}

const std::vector<float>&
TimingData::BuildAndGetEtar(int lastrow)
{
	ElapsedTimesAtAllRows.clear();

	// default old until the below is thoroughly checked
	for (auto r = 0; r <= lastrow; ++r)
		ElapsedTimesAtAllRows.emplace_back(
		  GetElapsedTimeFromBeatNoOffset(NoteRowToBeat(r)));
	return ElapsedTimesAtAllRows;
}

/** @brief Allow Lua to have access to the TimingData. */
class LunaTimingData : public Luna<TimingData>
{
  public:
	static int HasStops(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasStops()));
		return 1;
	}

	static int HasDelays(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasDelays()));
		return 1;
	}

	static int HasBPMChanges(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasBpmChanges()));
		return 1;
	}

	static int HasWarps(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasWarps()));
		return 1;
	}

	static int HasFakes(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasFakes()));
		return 1;
	}

	static int HasSpeedChanges(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasSpeedChanges()));
		return 1;
	}

	static int HasScrollChanges(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasScrollChanges()));
		return 1;
	}

#define GET_FUNCTION(get_name, segment_name)                                   \
	static int get_name(T* p, lua_State* L)                                    \
	{                                                                          \
		if (lua_toboolean(L, 1)) {                                             \
			TimingSegmentSetToLuaTable(p, segment_name, L);                    \
		} else {                                                               \
			LuaHelpers::CreateTableFromArray(p->ToVectorString(segment_name),  \
											 L);                               \
		}                                                                      \
		return 1;                                                              \
	}

	GET_FUNCTION(GetWarps, SEGMENT_WARP);
	GET_FUNCTION(GetFakes, SEGMENT_FAKE);
	GET_FUNCTION(GetScrolls, SEGMENT_SCROLL);
	GET_FUNCTION(GetSpeeds, SEGMENT_SPEED);
	GET_FUNCTION(GetTimeSignatures, SEGMENT_TIME_SIG);
	GET_FUNCTION(GetCombos, SEGMENT_COMBO);
	GET_FUNCTION(GetTickcounts, SEGMENT_TICKCOUNT);
	GET_FUNCTION(GetStops, SEGMENT_STOP);
	GET_FUNCTION(GetDelays, SEGMENT_DELAY);
	GET_FUNCTION(GetLabels, SEGMENT_LABEL);
	GET_FUNCTION(GetBPMsAndTimes, SEGMENT_BPM);
#undef GET_FUNCTION
	static int GetBPMs(T* p, lua_State* L)
	{
		std::vector<float> vBPMs;
		const auto& bpms = p->GetTimingSegments(SEGMENT_BPM);

		vBPMs.reserve(bpms.size());
		for (auto* bpm : bpms)
			vBPMs.push_back(ToBPM(bpm)->GetBPM());

		LuaHelpers::CreateTableFromArray(vBPMs, L);
		return 1;
	}

	static int GetActualBPM(T* p, lua_State* L)
	{
		// certainly there's a better way to do it than this? -aj
		float fMinBPM = 0.f;
		float fMaxBPM = 0.f;
		p->GetActualBPM(fMinBPM, fMaxBPM);
		std::vector<float> fBPMs;
		fBPMs.push_back(fMinBPM);
		fBPMs.push_back(fMaxBPM);
		LuaHelpers::CreateTableFromArray(fBPMs, L);
		return 1;
	}

	static int HasNegativeBPMs(T* p, lua_State* L)
	{
		lua_pushboolean(L, static_cast<int>(p->HasWarps()));
		return 1;
	}

	// formerly in Song.cpp in sm-ssc private beta 1.x:
	static int GetBPMAtBeat(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetBPMAtBeat(FArg(1)));
		return 1;
	}

	static int GetBeatFromElapsedTime(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->GetBeatFromElapsedTime(FArg(1)));
		return 1;
	}

	static int GetElapsedTimeFromBeat(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->WhereUAtBro(FArg(1)));
		return 1;
	}

	static int GetElapsedTimeFromNoteRow(T* p, lua_State* L)
	{
		lua_pushnumber(L, p->WhereUAtBro(IArg(1)));
		return 1;
	}

	LunaTimingData()
	{
		ADD_METHOD(HasStops);
		ADD_METHOD(HasDelays);
		ADD_METHOD(HasBPMChanges);
		ADD_METHOD(HasWarps);
		ADD_METHOD(HasFakes);
		ADD_METHOD(HasSpeedChanges);
		ADD_METHOD(HasScrollChanges);
		ADD_METHOD(GetStops);
		ADD_METHOD(GetDelays);
		ADD_METHOD(GetBPMs);
		ADD_METHOD(GetWarps);
		ADD_METHOD(GetFakes);
		ADD_METHOD(GetTimeSignatures);
		ADD_METHOD(GetTickcounts);
		ADD_METHOD(GetSpeeds);
		ADD_METHOD(GetScrolls);
		ADD_METHOD(GetCombos);
		ADD_METHOD(GetLabels);
		ADD_METHOD(GetBPMsAndTimes);
		ADD_METHOD(GetActualBPM);
		ADD_METHOD(HasNegativeBPMs);
		// formerly in Song.cpp in sm-ssc private beta 1.x:
		ADD_METHOD(GetBPMAtBeat);
		ADD_METHOD(GetBeatFromElapsedTime);
		ADD_METHOD(GetElapsedTimeFromBeat);
		ADD_METHOD(GetElapsedTimeFromNoteRow);
	}
};

LUA_REGISTER_CLASS(TimingData)

// lua end
