#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h" // blame radar calculations.
#include "NoteData.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/FileTypes/XmlFile.h"

#include <unordered_map>
#include <algorithm>

using std::map;
using std::vector;

void
NoteData::Init()
{
	UnsetNerv();
	UnsetSerializedNoteData();
	m_TapNotes = std::vector<TrackMap>(); // ensure that the memory is freed
}

void
NoteData::SetNumTracks(int iNewNumTracks)
{
	ASSERT(iNewNumTracks > 0);

	m_TapNotes.resize(iNewNumTracks);
	CalcNumTracksLCD();
}

// Clear (rowBegin,rowEnd).
void
NoteData::ClearRangeForTrack(int rowBegin, int rowEnd, int iTrack)
{
	// Optimization: if the range encloses everything, just clear the whole
	// maps.
	if (rowBegin == 0 && rowEnd == MAX_NOTE_ROW) {
		m_TapNotes[iTrack].clear();
		return;
	}

	/* If the range is empty, don't do anything. Otherwise, an empty range will
	 * cause hold notes to be split when they shouldn't be. */
	if (rowBegin == rowEnd) {
		return;
	}

	TrackMap::iterator lBegin;
	TrackMap::iterator lEnd;
	GetTapNoteRangeInclusive(iTrack, rowBegin, rowEnd, lBegin, lEnd);

	if (lBegin != lEnd && lBegin->first < rowBegin &&
		lBegin->first + lBegin->second.iDuration > rowEnd) {
		/* A hold note overlaps the whole range. Truncate it, and add the
		 * remainder to the end. */
		auto tn1 = lBegin->second;
		auto tn2 = tn1;

		const auto iEndRow = lBegin->first + tn1.iDuration;
		const auto iRow = lBegin->first;

		tn1.iDuration = rowBegin - iRow;
		tn2.iDuration = iEndRow - rowEnd;

		SetTapNote(iTrack, iRow, tn1);
		SetTapNote(iTrack, rowEnd, tn2);

		// We may have invalidated our iterators.
		GetTapNoteRangeInclusive(iTrack, rowBegin, rowEnd, lBegin, lEnd);
	} else if (lBegin != lEnd && lBegin->first < rowBegin) {
		// A hold note overlaps the beginning of the range.  Truncate it.
		auto& tn1 = lBegin->second;
		const auto iRow = lBegin->first;
		tn1.iDuration = rowBegin - iRow;

		++lBegin;
	}

	if (lBegin != lEnd) {
		auto prev = lEnd;
		--prev;
		auto tn = prev->second;
		auto iRow = prev->first;
		if (tn.type == TapNoteType_HoldHead && iRow + tn.iDuration > rowEnd) {
			// A hold note overlaps the end of the range.  Separate it.
			SetTapNote(iTrack, iRow, TAP_EMPTY);

			const auto iAdd = rowEnd - iRow;
			tn.iDuration -= iAdd;
			iRow += iAdd;
			SetTapNote(iTrack, iRow, tn);
			lEnd = prev;
		}

		// We may have invalidated our iterators.
		GetTapNoteRangeInclusive(iTrack, rowBegin, rowEnd, lBegin, lEnd);
	}

	m_TapNotes[iTrack].erase(lBegin, lEnd);
}

void
NoteData::ClearRange(int rowBegin, int rowEnd)
{
	for (size_t t = 0; t < m_TapNotes.size(); ++t) {
		ClearRangeForTrack(rowBegin, rowEnd, t);
	}
}

void
NoteData::ClearAll()
{
	for (auto& m_TapNote : m_TapNotes) {
		m_TapNote.clear();
	}
}

/* Copy [rowFromBegin,rowFromEnd) from pFrom to this. (Note that this does
 * *not* overlay; all data in the range is overwritten.) */
void
NoteData::CopyRange(const NoteData& from,
					int rowFromBegin,
					int rowFromEnd,
					int rowToBegin)
{
	ASSERT(from.GetNumTracks() == GetNumTracks());

	if (rowFromBegin > rowFromEnd) {
		return; // empty range
	}

	const auto rowToEnd = rowFromEnd - rowFromBegin + rowToBegin;
	const auto iMoveBy = rowToBegin - rowFromBegin;

	// Clear the region.
	ClearRange(rowToBegin, rowToEnd);

	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		TrackMap::const_iterator lBegin;
		TrackMap::const_iterator lEnd;
		from.GetTapNoteRangeInclusive(
		  t, rowFromBegin, rowFromEnd, lBegin, lEnd);
		for (; lBegin != lEnd; ++lBegin) {
			auto head = lBegin->second;
			if (head.type == TapNoteType_Empty) {
				continue;
			}

			if (head.type == TapNoteType_HoldHead) {
				auto iStartRow = lBegin->first + iMoveBy;
				auto iEndRow = iStartRow + head.iDuration;

				iStartRow = std::clamp(iStartRow, rowToBegin, rowToEnd);
				iEndRow = std::clamp(iEndRow, rowToBegin, rowToEnd);

				this->AddHoldNote(t, iStartRow, iEndRow, head);
			} else {
				const auto iTo = lBegin->first + iMoveBy;
				if (iTo >= rowToBegin && iTo <= rowToEnd) {
					this->SetTapNote(t, iTo, head);
				}
			}
		}
	}
}

void
NoteData::CopyAll(const NoteData& from)
{
	*this = from;
}

auto
NoteData::WifeTotalScoreCalc(TimingData* td,
							 int /*iStartIndex*/,
							 int /*iEndIndex*/) const -> int
{
	auto taps = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(*this, r)
	{
		for (size_t t = 0; t < m_TapNotes.size(); t++) {
			const auto& tn = GetTapNote(t, r);
			if (tn.type != TapNoteType_Empty && tn.type != TapNoteType_Mine &&
				tn.type != TapNoteType_Fake && td->IsJudgableAtRow(r)) {
				taps++;
			}
		}
	}

	return taps * 2;
}

auto
NoteData::SerializeNoteData(const std::vector<float>& etaner)
  -> const std::vector<NoteInfo>&
{
	SerializedNoteData.reserve(NonEmptyRowVector.size());

	const auto tracks = m_TapNotes.size();
	for (size_t i = 0; i < NonEmptyRowVector.size(); i++) {
		auto rowNotes = 0;
		for (size_t q = 0; q < tracks; q++) {
			if (GetTapNote(q, NonEmptyRowVector[i]).IsNote()) {
				rowNotes |= 1 << q;
			}
		}

		if (rowNotes != 0) {
			// see note in serialize 2 about zeroing out element 0
			NoteInfo rowOutput{ static_cast<unsigned int>(rowNotes),
								etaner[i] - etaner[0] };
			SerializedNoteData.emplace_back(rowOutput);
		}
	}
	return SerializedNoteData;
}

// about twice as fast as above and more flexible about when to release
// data that we might want to persist for a bit (still room for optimization)
// in the functions that call it
auto
NoteData::SerializeNoteData2(TimingData* ts,
							 bool unset_nerv_when_done,
							 bool unset_etaner_when_done)
  -> const std::vector<NoteInfo>&
{
	SerializedNoteData.clear();
	SerializedNoteData.reserve(NonEmptyRowVector.size());
	const auto tracks = m_TapNotes.size();

	std::unordered_map<int, int> lal;
	// iterate over tracks so we can avoid the lookup in gettapnote
	for (size_t t = 0; t < tracks; ++t) {
		const auto& tm = m_TapNotes[t];
		for (const auto& r : tm) {
			if (ts->IsJudgableAtRow(r.first)) {
				if (r.second.IsNote()) {
					// initialize in map
					const auto res = lal.emplace(r.first, 1 << t);
					if (!res.second) {
						// already added, but last column wasn't
						// actually a tap, start here
						if (lal.at(r.first) == 128) {
							lal.at(r.first) = 1 << t;
						} else {
							// already added and is a tap, update info
							lal.at(r.first) |= 1 << t;
						}
					}
				} else {
					// we need to keep track of more than just taps fo
					// if we want to use this for key generation
					// this won't alter tap values if there's something like
					// 11MM
					lal.emplace(r.first, 128);
				}
			}
		}
	}
	NonEmptyRowVector.clear();
	NonEmptyRowVector.reserve(lal.size());
	for (auto& soup : lal) {
		NonEmptyRowVector.push_back(soup.first);
	}
	std::sort(NonEmptyRowVector.begin(), NonEmptyRowVector.end());

	const auto& etaner = ts->BuildAndGetEtaner(NonEmptyRowVector);
	size_t idx = 0;
	for (auto r : NonEmptyRowVector) {
		// only send tap data to calc
		if (lal.at(r) != 128) {
			// zeroing out the first line makes debug output more annoying to
			// deal with, since we have to add back the file offset to make the
			// graphs line up with the cdgraphs, however, if we don't do this
			// offset will affect msd which is extremely stupid practically and
			// conceptually
			NoteInfo rowOutput{ static_cast<unsigned int>(lal.at(r)),
								etaner[idx] - etaner[0] };
			SerializedNoteData.emplace_back(rowOutput);
		}
		++idx;
	}
	if (unset_etaner_when_done) {
		ts->UnsetEtaner();
	}
	if (unset_nerv_when_done) {
		UnsetNerv();
	}
	return SerializedNoteData;
}

void
NoteData::LogNonEmptyRows(TimingData* ts)
{
	NonEmptyRowVector.clear();
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(*this, row)
	if (ts->IsJudgableAtRow(row)) {
		NonEmptyRowVector.emplace_back(row);
	}
}

auto
NoteData::IsRowEmpty(int row) const -> bool
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		if (GetTapNote(t, row).type != TapNoteType_Empty) {
			return false;
		}
	}
	return true;
}

auto
NoteData::IsRangeEmpty(int track, int rowBegin, int rowEnd) const -> bool
{
	ASSERT(track < GetNumTracks());

	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(*this, track, r, rowBegin, rowEnd)
	if (GetTapNote(track, r).type != TapNoteType_Empty) {
		return false;
	}
	return true;
}

auto
NoteData::GetNumTapNonEmptyTracks(int row) const -> int
{
	auto iNum = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		if (GetTapNote(t, row).type != TapNoteType_Empty) {
			iNum++;
		}
	}
	return iNum;
}

void
NoteData::GetTapNonEmptyTracks(int row, std::set<int>& addTo) const
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		if (GetTapNote(t, row).type != TapNoteType_Empty) {
			addTo.insert(t);
		}
	}
}

auto
NoteData::GetTapFirstNonEmptyTrack(int row, int& iNonEmptyTrackOut) const
  -> bool
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		if (GetTapNote(t, row).type != TapNoteType_Empty) {
			iNonEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

auto
NoteData::GetTapFirstEmptyTrack(int row, int& iEmptyTrackOut) const -> bool
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		if (GetTapNote(t, row).type == TapNoteType_Empty) {
			iEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

auto
NoteData::GetTapLastEmptyTrack(int row, int& iEmptyTrackOut) const -> bool
{
	auto num_tracks = GetNumTracks();
	for (size_t t = num_tracks - 1; t < num_tracks; t--) {
		if (GetTapNote(t, row).type == TapNoteType_Empty) {
			iEmptyTrackOut = t;
			return true;
		}
	}
	return false;
}

auto
NoteData::GetNumTracksWithTap(int row) const -> int
{
	auto iNum = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		const auto& tn = GetTapNote(t, row);
		if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_Lift) {
			iNum++;
		}
	}
	return iNum;
}

auto
NoteData::GetNumTracksWithTapOrHoldHead(int row) const -> int
{
	auto iNum = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		const auto& tn = GetTapNote(t, row);
		if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_Lift ||
			tn.type == TapNoteType_HoldHead) {
			iNum++;
		}
	}
	return iNum;
}

auto
NoteData::GetFirstTrackWithTap(int row) const -> int
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		const auto& tn = GetTapNote(t, row);
		if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_Lift) {
			return t;
		}
	}
	return -1;
}

auto
NoteData::GetFirstTrackWithTapOrHoldHead(int row) const -> int
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		const auto& tn = GetTapNote(t, row);
		if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_Lift ||
			tn.type == TapNoteType_HoldHead) {
			return t;
		}
	}
	return -1;
}

auto
NoteData::GetLastTrackWithTapOrHoldHead(int row) const -> int
{
	for (auto t = GetNumTracks() - 1; t >= 0; t--) {
		const auto& tn = GetTapNote(t, row);
		if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_Lift ||
			tn.type == TapNoteType_HoldHead) {
			return t;
		}
	}
	return -1;
}

void
NoteData::AddHoldNote(int iTrack, int iStartRow, int iEndRow, TapNote tn)
{
	ASSERT(iStartRow >= 0 && iEndRow >= 0);
	ASSERT_M(iEndRow >= iStartRow,
			 ssprintf("EndRow %d < StartRow %d", iEndRow, iStartRow));

	/* Include adjacent (non-overlapping) hold notes, since we need to merge
	 * with them. */
	TrackMap::iterator lBegin;
	TrackMap::iterator lEnd;
	GetTapNoteRangeInclusive(iTrack, iStartRow, iEndRow, lBegin, lEnd, true);

	// Look for other hold notes that overlap and merge them into add.
	for (auto it = lBegin; it != lEnd; ++it) {
		auto iOtherRow = it->first;
		const auto& tnOther = it->second;
		if (tnOther.type == TapNoteType_HoldHead) {
			iStartRow = std::min(iStartRow, iOtherRow);
			iEndRow = std::max(iEndRow, iOtherRow + tnOther.iDuration);
		}
	}

	tn.iDuration = iEndRow - iStartRow;

	// Remove everything in the range.
	while (lBegin != lEnd) {
		auto next = lBegin;
		++next;

		RemoveTapNote(iTrack, lBegin);

		lBegin = next;
	}

	/* Additionally, if there's a tap note lying at the end of our range,
	 * remove it too. */
	SetTapNote(iTrack, iEndRow, TAP_EMPTY);

	// add a tap note at the start of this hold
	SetTapNote(iTrack, iStartRow, tn);
}

/* Determine if a hold note lies on the given spot.  Return true if so.  If
 * pHeadRow is non-NULL, return the row of the head. */
auto
NoteData::IsHoldHeadOrBodyAtRow(int iTrack, int iRow, int* pHeadRow) const
  -> bool
{
	const auto& tn = GetTapNote(iTrack, iRow);
	if (tn.type == TapNoteType_HoldHead) {
		if (pHeadRow != nullptr) {
			*pHeadRow = iRow;
		}
		return true;
	}

	return IsHoldNoteAtRow(iTrack, iRow, pHeadRow);
}

/* Determine if a hold note lies on the given spot. Return true if so.  If
 * pHeadRow is non-NULL, return the row of the head. (Note that this returns
 * false if a hold head lies on iRow itself.) */
/* XXX: rename this to IsHoldBodyAtRow */
auto
NoteData::IsHoldNoteAtRow(int iTrack, int iRow, int* pHeadRow) const -> bool
{
	int iDummy;
	if (pHeadRow == nullptr) {
		pHeadRow = &iDummy;
	}

	/* Starting at iRow, search upwards. If we find a TapNoteType_HoldHead,
	 * we're within a hold. If we find a tap, mine or attack, we're not--those
	 * never lie within hold notes. Ignore autoKeysound. */
	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE_REVERSE(*this, iTrack, r, 0, iRow)
	{
		const auto& tn = GetTapNote(iTrack, r);
		switch (tn.type) {
			case TapNoteType_HoldHead:
				if (tn.iDuration + r < iRow) {
					return false;
				}
				*pHeadRow = r;
				return true;

			case TapNoteType_Tap:
			case TapNoteType_Mine:
			case TapNoteType_Lift:
			case TapNoteType_Fake:
				return false;

			case TapNoteType_Empty:
			case TapNoteType_AutoKeysound:
				// ignore
				continue;
				DEFAULT_FAIL(tn.type);
		}
	}

	return false;
}

auto
NoteData::IsEmpty() const -> bool
{
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		auto iRow = -1;
		if (!GetNextTapNoteRowForTrack(t, iRow)) {
			continue;
		}

		return false;
	}

	return true;
}

auto
NoteData::GetFirstRow() const -> int
{
	auto iEarliestRowFoundSoFar = -1;

	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		auto iRow = -1;
		if (!GetNextTapNoteRowForTrack(t, iRow, true)) {
			continue;
		}

		if (iEarliestRowFoundSoFar == -1) {
			iEarliestRowFoundSoFar = iRow;
		} else {
			iEarliestRowFoundSoFar = std::min(iEarliestRowFoundSoFar, iRow);
		}
	}

	if (iEarliestRowFoundSoFar == -1) { // there are no notes
		return 0;
	}

	return iEarliestRowFoundSoFar;
}

auto
NoteData::GetLastRow() const -> int
{
	auto iOldestRowFoundSoFar = 0;

	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		auto iRow = MAX_NOTE_ROW;
		if (!GetPrevTapNoteRowForTrack(t, iRow)) {
			continue;
		}

		/* XXX: We might have a hold note near the end with autoplay sounds
		 * after it.  Do something else with autoplay sounds ... */
		const auto& tn = GetTapNote(t, iRow);
		if (tn.type == TapNoteType_HoldHead) {
			iRow += tn.iDuration;
		}

		iOldestRowFoundSoFar = std::max(iOldestRowFoundSoFar, iRow);
	}

	return iOldestRowFoundSoFar;
}

auto
NoteData::IsTap(const TapNote& tn, const int row) -> bool
{
	return tn.type != TapNoteType_Empty && tn.type != TapNoteType_Mine &&
		   tn.type != TapNoteType_Lift && tn.type != TapNoteType_Fake &&
		   tn.type != TapNoteType_AutoKeysound &&
		   GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(row);
}

auto
NoteData::IsMine(const TapNote& tn, const int row) -> bool
{
	return tn.type == TapNoteType_Mine &&
		   GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(row);
}

auto
NoteData::IsLift(const TapNote& tn, const int row) -> bool
{
	return tn.type == TapNoteType_Lift &&
		   GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(row);
}

auto
NoteData::IsFake(const TapNote& tn, const int row) -> bool
{
	return tn.type == TapNoteType_Fake ||
		   !GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(row);
}

auto
NoteData::GetNumTapNotes(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumNotes = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(*this, t, r, iStartIndex, iEndIndex)
		{
			if (this->IsTap(GetTapNote(t, r), r)) {
				iNumNotes++;
			}
		}
	}

	return iNumNotes;
}

auto
NoteData::GetNumTapNotesNoTiming(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumNotes = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(*this, t, r, iStartIndex, iEndIndex)
		{
			if (GetTapNote(t, r).type != TapNoteType_Empty) {
				iNumNotes++;
			}
		}
	}

	return iNumNotes;
}

auto
NoteData::GetNumTapNotesInRow(int iRow) const -> int
{
	auto iNumNotes = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		if (this->IsTap(GetTapNote(t, iRow), iRow)) {
			iNumNotes++;
		}
	}

	return iNumNotes;
}

auto
NoteData::GetNumRowsWithTap(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumNotes = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(*this, r, iStartIndex, iEndIndex)
	if (IsThereATapAtRow(r) &&
		GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(r)) {
		iNumNotes++;
	}

	return iNumNotes;
}

auto
NoteData::GetNumMines(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumMines = 0;

	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(*this, t, r, iStartIndex, iEndIndex)
		if (this->IsMine(GetTapNote(t, r), r)) {
			iNumMines++;
		}
	}

	return iNumMines;
}

auto
NoteData::GetNumRowsWithTapOrHoldHead(int iStartIndex, int iEndIndex) const
  -> int
{
	auto iNumNotes = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(*this, r, iStartIndex, iEndIndex)
	if (IsThereATapOrHoldHeadAtRow(r) &&
		GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(r)) {
		iNumNotes++;
	}

	return iNumNotes;
}

auto
NoteData::RowNeedsAtLeastSimultaneousPresses(int iMinSimultaneousPresses,
											 const int row) const -> bool
{
	auto iNumNotesThisIndex = 0;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		const auto& tn = GetTapNote(t, row);
		switch (tn.type) {
			case TapNoteType_Mine:
			case TapNoteType_Empty:
			case TapNoteType_Fake:
			case TapNoteType_Lift: // you don't "press" on a lift.
			case TapNoteType_AutoKeysound:
				continue; // skip these types - they don't count
			default:
				break;
		}
		++iNumNotesThisIndex;
	}

	/* We must have at least one tap or hold head at this row to count it. */
	if (iNumNotesThisIndex == 0) {
		return false;
	}

	if (iNumNotesThisIndex < iMinSimultaneousPresses) {
		/* We have at least one, but not enough.  Count holds.  Do count
		 * adjacent holds. */
		for (size_t t = 0; t < m_TapNotes.size(); ++t) {
			if (IsHoldNoteAtRow(t, row)) {
				++iNumNotesThisIndex;
			}
		}
	}

	return iNumNotesThisIndex >= iMinSimultaneousPresses;
}

auto
NoteData::GetNumRowsWithSimultaneousPresses(int iMinSimultaneousPresses,
											int iStartIndex,
											int iEndIndex) const -> int
{
	/* Count the number of times you have to use your hands.  This includes
	 * three taps at the same time, a tap while two hold notes are being held,
	 * etc.  Only count rows that have at least one tap note (hold heads count).
	 * Otherwise, every row of hold notes counts, so three simultaneous hold
	 * notes will count as hundreds of "hands". */
	auto iNum = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(*this, r, iStartIndex, iEndIndex)
	{
		if (!RowNeedsAtLeastSimultaneousPresses(iMinSimultaneousPresses, r)) {
			continue;
		}
		if (!GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(r)) {
			continue;
		}
		iNum++;
	}

	return iNum;
}

auto
NoteData::GetNumRowsWithSimultaneousTaps(int iMinTaps,
										 int iStartIndex,
										 int iEndIndex) const -> int
{
	auto iNum = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(*this, r, iStartIndex, iEndIndex)
	{
		if (!GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(r)) {
			continue;
		}
		auto iNumNotesThisIndex = 0;
		for (size_t t = 0; t < m_TapNotes.size(); t++) {
			const auto& tn = GetTapNote(t, r);
			if (tn.type != TapNoteType_Mine && // mines don't count.
				tn.type != TapNoteType_Empty && tn.type != TapNoteType_Fake &&
				tn.type != TapNoteType_AutoKeysound) {
				iNumNotesThisIndex++;
			}
		}
		if (iNumNotesThisIndex >= iMinTaps) {
			iNum++;
		}
	}

	return iNum;
}

auto
NoteData::GetNumHoldNotes(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumHolds = 0;
	for (size_t t = 0; t < m_TapNotes.size(); ++t) {
		TrackMap::const_iterator lBegin;
		TrackMap::const_iterator lEnd;
		GetTapNoteRangeExclusive(t, iStartIndex, iEndIndex, lBegin, lEnd);
		for (; lBegin != lEnd; ++lBegin) {
			if (lBegin->second.type != TapNoteType_HoldHead ||
				lBegin->second.subType != TapNoteSubType_Hold) {
				continue;
			}
			if (!GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(
				  lBegin->first)) {
				continue;
			}
			iNumHolds++;
		}
	}
	return iNumHolds;
}

auto
NoteData::GetNumRolls(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumRolls = 0;
	for (size_t t = 0; t < m_TapNotes.size(); ++t) {
		TrackMap::const_iterator lBegin;
		TrackMap::const_iterator lEnd;
		GetTapNoteRangeExclusive(t, iStartIndex, iEndIndex, lBegin, lEnd);
		for (; lBegin != lEnd; ++lBegin) {
			if (lBegin->second.type != TapNoteType_HoldHead ||
				lBegin->second.subType != TapNoteSubType_Roll) {
				continue;
			}
			if (!GAMESTATE->GetProcessedTimingData()->IsJudgableAtRow(
				  lBegin->first)) {
				continue;
			}
			iNumRolls++;
		}
	}
	return iNumRolls;
}

auto
NoteData::GetNumLifts(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumLifts = 0;

	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(*this, t, r, iStartIndex, iEndIndex)
		if (this->IsLift(GetTapNote(t, r), r)) {
			iNumLifts++;
		}
	}

	return iNumLifts;
}

auto
NoteData::GetNumFakes(int iStartIndex, int iEndIndex) const -> int
{
	auto iNumFakes = 0;

	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(*this, t, r, iStartIndex, iEndIndex)
		if (this->IsFake(GetTapNote(t, r), r)) {
			iNumFakes++;
		}
	}

	return iNumFakes;
}

// This is a fast but inaccurate LCD calculator.
// Used for generating accurate DP scores when
// chord cohesion is disabled.
void
NoteData::CalcNumTracksLCD()
{
	const auto numTracks = this->m_TapNotes.size();
	std::vector<int> nums;
	auto lcd = 1;

	for (size_t i = 1; i < numTracks + 1; i++) {
		lcd *= i;
		nums.push_back(i);
	}

	auto stillValid = true;
	while (stillValid) {
		const auto tmpLCD = lcd / 2;
		for (size_t i = 0; i < numTracks; i++) {
			if ((tmpLCD % nums[i]) != 0) {
				stillValid = false;
				break;
			}
		}

		if (stillValid) {
			lcd = tmpLCD;
		}
	}

	m_numTracksLCD = lcd;
}

auto
NoteData::GetNumTracksLCD() const -> int
{
	return m_numTracksLCD;
}

// -1 for iOriginalTracksToTakeFrom means no track
void
NoteData::LoadTransformed(const NoteData& in,
						  int iNewNumTracks,
						  const int iOriginalTrackToTakeFrom[])
{
	// reset all notes
	Init();

	SetNumTracks(iNewNumTracks);

	// copy tracks
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		const auto iOriginalTrack = iOriginalTrackToTakeFrom[t];
		ASSERT_M(
		  iOriginalTrack < in.GetNumTracks(),
		  ssprintf("from OriginalTrack %i >= %i (#tracks) (taking from %i)",
				   iOriginalTrack,
				   in.GetNumTracks(),
				   iOriginalTrackToTakeFrom[t]));

		if (iOriginalTrack == -1) {
			continue;
		}
		m_TapNotes[t] = in.m_TapNotes[iOriginalTrack];
	}
}

void
NoteData::MoveTapNoteTrack(int dest, int src)
{
	if (dest == src) {
		return;
	}
	m_TapNotes[dest] = m_TapNotes[src];
	m_TapNotes[src].clear();
}

void
NoteData::SetTapNote(int track, int row, const TapNote& t)
{
	DEBUG_ASSERT(track >= 0 && track < GetNumTracks());

	if (row < 0) {
		return;
	}

	// There's no point in inserting empty notes into the map.
	// Any blank space in the map is defined to be empty.
	// If we're trying to insert an empty at a spot where another note
	// already exists, then we're really deleting from the map.
	if (t == TAP_EMPTY) {
		auto& trackMap = m_TapNotes[track];
		// remove the element at this position (if any).
		// This will return either 0 or 1.
		trackMap.erase(row);
	} else {
		m_TapNotes[track][row] = t;
	}
}

void
NoteData::GetTracksHeldAtRow(int row, std::set<int>& addTo) const
{
	for (size_t t = 0; t < m_TapNotes.size(); ++t) {
		if (IsHoldNoteAtRow(t, row)) {
			addTo.insert(t);
		}
	}
}

auto
NoteData::GetNumTracksHeldAtRow(int row) -> int
{
	static std::set<int> viTracks;
	viTracks.clear();
	GetTracksHeldAtRow(row, viTracks);
	return viTracks.size();
}

auto
NoteData::GetNextTapNoteRowForTrack(int track,
									int& rowInOut,
									bool ignoreAutoKeysounds) const -> bool
{
	const auto& mapTrack = m_TapNotes[track];

	// lower_bound and upper_bound have the same effect here because duplicate
	// keys aren't allowed.

	// lower_bound "finds the first element whose key is not less than k" (>=);
	// upper_bound "finds the first element whose key greater than k".  They
	// don't have the same effect, but lower_bound(row+1) should equal
	// upper_bound(row). -glenn
	auto iter = mapTrack.lower_bound(
	  rowInOut + 1); // "find the first note for which row+1 < key == false"
	if (iter == mapTrack.end()) {
		return false;
	}

	ASSERT(iter->first > rowInOut);

	// If we want to ignore autokeysounds, keep going until we find a real note.
	if (ignoreAutoKeysounds) {
		while (iter->second.type == TapNoteType_AutoKeysound) {
			++iter;
			if (iter == mapTrack.end()) {
				return false;
			}
		}
	}
	rowInOut = iter->first;
	return true;
}

auto
NoteData::GetPrevTapNoteRowForTrack(int track, int& rowInOut) const -> bool
{
	const auto& mapTrack = m_TapNotes[track];

	// Find the first note >= rowInOut.
	auto iter = mapTrack.lower_bound(rowInOut);

	// If we're at the beginning, we can't move back any more.
	if (iter == mapTrack.begin()) {
		return false;
	}

	// Move back by one.
	--iter;
	ASSERT(iter->first < rowInOut);
	rowInOut = iter->first;
	return true;
}

void
NoteData::GetTapNoteRange(int iTrack,
						  int iStartRow,
						  int iEndRow,
						  TrackMap::iterator& lBegin,
						  TrackMap::iterator& lEnd)
{
	ASSERT_M(iTrack < GetNumTracks(),
			 ssprintf("%i,%i", iTrack, GetNumTracks()));
	auto& mapTrack = m_TapNotes[iTrack];

	if (iStartRow > iEndRow) {
		lBegin = lEnd = mapTrack.end();
		return;
	}

	if (iStartRow <= 0) {
		lBegin = mapTrack.begin(); // optimization
	} else if (iStartRow >= MAX_NOTE_ROW) {
		lBegin = mapTrack.end(); // optimization
	} else {
		lBegin = mapTrack.lower_bound(iStartRow);
	}

	if (iEndRow <= 0) {
		lEnd = mapTrack.begin(); // optimization
	} else if (iEndRow >= MAX_NOTE_ROW) {
		lEnd = mapTrack.end(); // optimization
	} else {
		lEnd = mapTrack.lower_bound(iEndRow);
	}
}

/* Include hold notes that overlap the edges.  If a hold note completely
 * surrounds the given range, included it, too.  If bIncludeAdjacent is true,
 * also include hold notes adjacent to, but not overlapping, the edge. */
void
NoteData::GetTapNoteRangeInclusive(int iTrack,
								   int iStartRow,
								   int iEndRow,
								   TrackMap::iterator& lBegin,
								   TrackMap::iterator& lEnd,
								   bool bIncludeAdjacent)
{
	GetTapNoteRange(iTrack, iStartRow, iEndRow, lBegin, lEnd);

	if (lBegin != this->begin(iTrack)) {
		const auto prev = Decrement(lBegin);

		const auto& tn = prev->second;
		if (tn.type == TapNoteType_HoldHead) {
			const auto iHoldStartRow = prev->first;
			auto iHoldEndRow = iHoldStartRow + tn.iDuration;
			if (bIncludeAdjacent) {
				++iHoldEndRow;
			}
			if (iHoldEndRow > iStartRow) {
				// The previous note is a hold.
				lBegin = prev;
			}
		}
	}

	if (bIncludeAdjacent && lEnd != this->end(iTrack)) {
		// Include the next note if it's a hold and starts on iEndRow.
		const auto& tn = lEnd->second;
		const auto iHoldStartRow = lEnd->first;
		if (tn.type == TapNoteType_HoldHead && iHoldStartRow == iEndRow) {
			++lEnd;
		}
	}
}

void
NoteData::GetTapNoteRangeExclusive(int iTrack,
								   int iStartRow,
								   int iEndRow,
								   TrackMap::iterator& lBegin,
								   TrackMap::iterator& lEnd)
{
	GetTapNoteRange(iTrack, iStartRow, iEndRow, lBegin, lEnd);

	// If end-1 is a hold_head, and extends beyond iEndRow, exclude it.
	if (lBegin != lEnd && lEnd != this->begin(iTrack)) {
		auto prev = lEnd;
		--prev;
		if (prev->second.type == TapNoteType_HoldHead) {
			const auto localStartRow = prev->first;
			const auto& tn = prev->second;
			if (localStartRow + tn.iDuration >= iEndRow) {
				lEnd = prev;
			}
		}
	}
}

void
NoteData::GetTapNoteRange(int iTrack,
						  int iStartRow,
						  int iEndRow,
						  TrackMap::const_iterator& lBegin,
						  TrackMap::const_iterator& lEnd) const
{
	TrackMap::iterator const_begin;
	TrackMap::iterator const_end;
	const_cast<NoteData*>(this)->GetTapNoteRange(
	  iTrack, iStartRow, iEndRow, const_begin, const_end);
	lBegin = const_begin;
	lEnd = const_end;
}

void
NoteData::GetTapNoteRangeInclusive(int iTrack,
								   int iStartRow,
								   int iEndRow,
								   TrackMap::const_iterator& lBegin,
								   TrackMap::const_iterator& lEnd,
								   bool bIncludeAdjacent) const
{
	TrackMap::iterator const_begin;
	TrackMap::iterator const_end;
	const_cast<NoteData*>(this)->GetTapNoteRangeInclusive(
	  iTrack, iStartRow, iEndRow, const_begin, const_end, bIncludeAdjacent);
	lBegin = const_begin;
	lEnd = const_end;
}

void
NoteData::GetTapNoteRangeExclusive(int iTrack,
								   int iStartRow,
								   int iEndRow,
								   TrackMap::const_iterator& lBegin,
								   TrackMap::const_iterator& lEnd) const
{
	TrackMap::iterator const_begin;
	TrackMap::iterator const_end;
	const_cast<NoteData*>(this)->GetTapNoteRange(
	  iTrack, iStartRow, iEndRow, const_begin, const_end);
	lBegin = const_begin;
	lEnd = const_end;
}

auto
NoteData::GetNextTapNoteRowForAllTracks(int& rowInOut) const -> bool
{
	auto iClosestNextRow = MAX_NOTE_ROW;
	auto bAnyHaveNextNote = false;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		auto iNewRowThisTrack = rowInOut;
		if (GetNextTapNoteRowForTrack(t, iNewRowThisTrack)) {
			bAnyHaveNextNote = true;
			ASSERT(iNewRowThisTrack < MAX_NOTE_ROW);
			iClosestNextRow = std::min(iClosestNextRow, iNewRowThisTrack);
		}
	}

	if (bAnyHaveNextNote) {
		rowInOut = iClosestNextRow;
		return true;
	}
	return false;
}

auto
NoteData::GetPrevTapNoteRowForAllTracks(int& rowInOut) const -> bool
{
	auto iClosestPrevRow = 0;
	auto bAnyHavePrevNote = false;
	for (size_t t = 0; t < m_TapNotes.size(); t++) {
		auto iNewRowThisTrack = rowInOut;
		if (GetPrevTapNoteRowForTrack(t, iNewRowThisTrack)) {
			bAnyHavePrevNote = true;
			ASSERT(iNewRowThisTrack < MAX_NOTE_ROW);
			iClosestPrevRow = std::max(iClosestPrevRow, iNewRowThisTrack);
		}
	}

	if (bAnyHavePrevNote) {
		rowInOut = iClosestPrevRow;
		return true;
	}
	return false;
}

auto
NoteData::CreateNode() const -> XNode*
{
	auto* p = new XNode("NoteData");

	auto iter = GetTapNoteRangeAllTracks(0, GetLastRow());

	for (; !iter.IsAtEnd(); ++iter) {
		auto* p2 = iter->CreateNode();

		p2->AppendAttr("Track", iter.Track());
		p2->AppendAttr("Row", iter.Row());
		p->AppendChild(p2);
	}
	return p;
}

void
NoteData::LoadFromNode(const XNode* /*pNode*/)
{
	FAIL_M("NoteData::LoadFromNode() not implemented");
}

void
NoteData::AddATIToList(all_tracks_iterator* iter) const
{
	m_atis.insert(iter);
}

void
NoteData::AddATIToList(all_tracks_const_iterator* iter) const
{
	m_const_atis.insert(iter);
}

void
NoteData::RemoveATIFromList(all_tracks_iterator* iter) const
{
	const auto pos = m_atis.find(iter);
	if (pos != m_atis.end()) {
		m_atis.erase(pos);
	}
}

void
NoteData::RemoveATIFromList(all_tracks_const_iterator* iter) const
{
	const auto pos = m_const_atis.find(iter);
	if (pos != m_const_atis.end()) {
		m_const_atis.erase(pos);
	}
}

void
NoteData::RevalidateATIs(std::vector<int> const& added_or_removed_tracks, bool added)
{
	for (auto* m_ati : m_atis) {
		m_ati->Revalidate(this, added_or_removed_tracks, added);
	}
	for (auto* m_const_ati : m_const_atis) {
		m_const_ati->Revalidate(this, added_or_removed_tracks, added);
	}
}

template<typename ND, typename iter, typename TN>
void
NoteData::_all_tracks_iterator<ND, iter, TN>::Find(bool bReverse)
{
	// If no notes can be found in the range, m_iTrack will stay -1 and
	// IsAtEnd() will return true.
	m_iTrack = -1;
	if (bReverse) {
		auto iMaxRow = INT_MIN;
		for (int iTrack = m_pNoteData->GetNumTracks() - 1; iTrack >= 0;
			 --iTrack) {
			iter& i(m_vCurrentIters[iTrack]);
			const iter& end = m_vEndIters[iTrack];
			if (i != end && i->first > iMaxRow) {
				iMaxRow = i->first;
				m_iTrack = iTrack;
			}
		}
	} else {

		auto iMinRow = INT_MAX;
		for (size_t iTrack = 0; iTrack < m_pNoteData->m_TapNotes.size();
			 ++iTrack) {
			iter& i = m_vCurrentIters[iTrack];
			const iter& end = m_vEndIters[iTrack];
			if (i != end && i->first < iMinRow) {
				iMinRow = i->first;
				m_iTrack = iTrack;
			}
		}
	}
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN>::_all_tracks_iterator(
  ND& nd,
  int iStartRow,
  int iEndRow,
  bool bReverse,
  bool bInclusive)
  : m_pNoteData(&nd)
  , m_iTrack(0)
  , m_bReverse(bReverse)
{
	ASSERT(m_pNoteData->GetNumTracks() > 0);

	m_StartRow = iStartRow;
	m_EndRow = iEndRow;
	m_Inclusive = bInclusive;

	for (size_t iTrack = 0; iTrack < m_pNoteData->m_TapNotes.size(); ++iTrack) {
		iter begin;
		iter end;
		if (bInclusive) {
			m_pNoteData->GetTapNoteRangeInclusive(
			  iTrack, iStartRow, iEndRow, begin, end);
		} else {
			m_pNoteData->GetTapNoteRange(
			  iTrack, iStartRow, iEndRow, begin, end);
		}

		m_vBeginIters.push_back(begin);
		m_vEndIters.push_back(end);
		m_PrevCurrentRows.push_back(0);

		iter cur;
		if (m_bReverse) {
			cur = end;
			if (cur != begin) {
				--cur;
			}
		} else {
			cur = begin;
		}
		m_vCurrentIters.push_back(cur);
	}
	m_pNoteData->AddATIToList(this);

	Find(bReverse);
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN>::_all_tracks_iterator(
  const _all_tracks_iterator& other)
  :
#define COPY_OTHER(x) x(other.x)
  COPY_OTHER(m_pNoteData)
  , COPY_OTHER(m_vBeginIters)
  , COPY_OTHER(m_vCurrentIters)
  , COPY_OTHER(m_vEndIters)
  , COPY_OTHER(m_iTrack)
  , COPY_OTHER(m_bReverse)
  , COPY_OTHER(m_PrevCurrentRows)
  , COPY_OTHER(m_StartRow)
  , COPY_OTHER(m_EndRow)
#undef COPY_OTHER
{
	m_pNoteData->AddATIToList(this);
	m_Inclusive = false;
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN>::~_all_tracks_iterator()
{
	if (m_pNoteData != nullptr) {
		m_pNoteData->RemoveATIFromList(this);
	}
}

template<typename ND, typename iter, typename TN>
auto
NoteData::_all_tracks_iterator<ND, iter, TN>::operator++()
  -> NoteData::_all_tracks_iterator<ND, iter, TN>& // preincrement
{
	m_PrevCurrentRows[m_iTrack] = Row();
	if (m_bReverse) {
		if (m_vCurrentIters[m_iTrack] == m_vBeginIters[m_iTrack]) {
			m_vCurrentIters[m_iTrack] = m_vEndIters[m_iTrack];
		} else {
			--m_vCurrentIters[m_iTrack];
		}
	} else {
		++m_vCurrentIters[m_iTrack];
	}
	Find(m_bReverse);
	return *this;
}

template<typename ND, typename iter, typename TN>
auto
NoteData::_all_tracks_iterator<ND, iter, TN>::operator++(int)
  -> NoteData::_all_tracks_iterator<ND, iter, TN> // postincrement
{
	_all_tracks_iterator<ND, iter, TN> ret(*this);
	operator++();
	return ret;
}

template<typename ND, typename iter, typename TN>
void
NoteData::_all_tracks_iterator<ND, iter, TN>::Revalidate(
  ND* notedata,
  std::vector<int> const& added_or_removed_tracks,
  bool added)
{
	m_pNoteData = notedata;
	ASSERT(m_pNoteData->GetNumTracks() > 0);
	if (!added_or_removed_tracks.empty()) {
		if (added) {
			auto avg_row = 0;
			for (auto m_PrevCurrentRow : m_PrevCurrentRows) {
				avg_row += m_PrevCurrentRow;
			}
			avg_row /= m_PrevCurrentRows.size();
			for (auto track_id : added_or_removed_tracks) {
				m_PrevCurrentRows.insert(m_PrevCurrentRows.begin() + track_id,
										 avg_row);
			}
			m_vBeginIters.resize(m_pNoteData->GetNumTracks());
			m_vCurrentIters.resize(m_pNoteData->GetNumTracks());
			m_vEndIters.resize(m_pNoteData->GetNumTracks());
		} else {
			for (auto track_id : added_or_removed_tracks) {
				m_PrevCurrentRows.erase(m_PrevCurrentRows.begin() + track_id);
			}
			m_vBeginIters.resize(m_pNoteData->GetNumTracks());
			m_vCurrentIters.resize(m_pNoteData->GetNumTracks());
			m_vEndIters.resize(m_pNoteData->GetNumTracks());
		}
	}
	for (size_t track = 0; track < m_pNoteData->m_TapNotes.size(); ++track) {
		iter begin;
		iter end;
		if (m_Inclusive) {
			m_pNoteData->GetTapNoteRangeInclusive(
			  track, m_StartRow, m_EndRow, begin, end);
		} else {
			m_pNoteData->GetTapNoteRange(
			  track, m_StartRow, m_EndRow, begin, end);
		}
		m_vBeginIters[track] = begin;
		m_vEndIters[track] = end;
		iter cur;
		if (m_bReverse) {
			cur = m_pNoteData->upper_bound(track, m_PrevCurrentRows[track]);
		} else {
			cur = m_pNoteData->lower_bound(track, m_PrevCurrentRows[track]);
		}
		m_vCurrentIters[track] = cur;
	}
	Find(m_bReverse);
}

/* XXX: This doesn't satisfy the requirements that ++iter; --iter; is a no-op so
 * it cannot be bidirectional for now. */
#if 0
template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN> &NoteData::_all_tracks_iterator<ND, iter, TN>::operator--() // predecrement
{
	if( m_bReverse )
	{
		++m_vCurrentIters[m_iTrack];
	}
	else
	{
		if( m_vCurrentIters[m_iTrack] == m_vEndIters[m_iTrack] )
			m_vCurrentIters[m_iTrack] = m_vEndIters[m_iTrack];
		else
			--m_vCurrentIters[m_iTrack];
	}
	Find( !m_bReverse );
	return *this;
}

template<typename ND, typename iter, typename TN>
NoteData::_all_tracks_iterator<ND, iter, TN> NoteData::_all_tracks_iterator<ND, iter, TN>::operator--( int dummy ) // postdecrement
{
	_all_tracks_iterator<ND, iter, TN> ret( *this );
	operator--();
	return ret;
}
#endif

// Explicit instantiation.
template class NoteData::
  _all_tracks_iterator<NoteData, NoteData::iterator, TapNote>;
template class NoteData::
  _all_tracks_iterator<const NoteData, NoteData::const_iterator, const TapNote>;
