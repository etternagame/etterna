#ifndef NOTE_DATA_H
#define NOTE_DATA_H

#include "NoteDataStructures.h"
#include "Etterna/Models/Misc/NoteTypes.h"

#include <iterator>
#include <map>
#include <set>
#include <vector>

class TimingData;

/** @brief Act on each non empty row in the specific track. */
#define FOREACH_NONEMPTY_ROW_IN_TRACK(nd, track, row)                          \
	for (int(row) = -1; (nd).GetNextTapNoteRowForTrack(track, row);)
/** @brief Act on each non empty row in the specified track within the specified
 * range. */
#define FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(nd, track, row, start, last)       \
	for (int(row) = (start)-1;                                                 \
		 (nd).GetNextTapNoteRowForTrack(track, row) && (row) < (last);)
/** @brief Act on each non empty row in the specified track within the specified
 range, going in reverse order. */
#define FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE_REVERSE(                           \
  nd, track, row, start, last)                                                 \
	for (int(row) = last;                                                      \
		 (nd).GetPrevTapNoteRowForTrack(track, row) && (row) >= (start);)
/** @brief Act on each non empty row for all of the tracks. */
#define FOREACH_NONEMPTY_ROW_ALL_TRACKS(nd, row)                               \
	for (int(row) = -1; (nd).GetNextTapNoteRowForAllTracks(row);)
/** @brief Act on each non empty row for all of the tracks within the specified
 * range. */
#define FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(nd, row, start, last)            \
	for (int(row) = (start)-1;                                                 \
		 (nd).GetNextTapNoteRowForAllTracks(row) && (row) < (last);)

#define FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE_REVERSE(nd, row, start, last)    \
	for (int(row) = last;													   \
		(nd).GetPrevTapNoteRowForAllTracks(row) && (row) >= (start);)

/** @brief Holds data about the notes that the player is supposed to hit. */
class NoteData
{
  public:
	using TrackMap = std::map<int, TapNote>;
	using iterator = std::map<int, TapNote>::iterator;
	using const_iterator = std::map<int, TapNote>::const_iterator;
	using reverse_iterator = std::map<int, TapNote>::reverse_iterator;
	using const_reverse_iterator =
	  std::map<int, TapNote>::const_reverse_iterator;

	NoteData() { m_numTracksLCD = 0; }

	auto begin(int iTrack) -> iterator { return m_TapNotes[iTrack].begin(); }
	auto begin(int iTrack) const -> const_iterator
	{
		return m_TapNotes[iTrack].begin();
	}
	auto rbegin(int iTrack) -> reverse_iterator
	{
		return m_TapNotes[iTrack].rbegin();
	}
	auto rbegin(int iTrack) const -> const_reverse_iterator
	{
		return m_TapNotes[iTrack].rbegin();
	}
	auto end(int iTrack) -> iterator { return m_TapNotes[iTrack].end(); }
	auto end(int iTrack) const -> const_iterator
	{
		return m_TapNotes[iTrack].end();
	}
	auto rend(int iTrack) -> reverse_iterator
	{
		return m_TapNotes[iTrack].rend();
	}
	auto rend(int iTrack) const -> const_reverse_iterator
	{
		return m_TapNotes[iTrack].rend();
	}
	auto lower_bound(int iTrack, int iRow) -> iterator
	{
		return m_TapNotes[iTrack].lower_bound(iRow);
	}
	auto lower_bound(int iTrack, int iRow) const -> const_iterator
	{
		return m_TapNotes[iTrack].lower_bound(iRow);
	}
	auto upper_bound(int iTrack, int iRow) -> iterator
	{
		return m_TapNotes[iTrack].upper_bound(iRow);
	}
	auto upper_bound(int iTrack, int iRow) const -> const_iterator
	{
		return m_TapNotes[iTrack].upper_bound(iRow);
	}
	void swap(NoteData& nd)
	{
		m_TapNotes.swap(nd.m_TapNotes);
		m_atis.swap(nd.m_atis);
		m_const_atis.swap(nd.m_const_atis);
	}

	// This is ugly to make it templated but I don't want to have to write the
	// same class twice.
	template<typename ND, typename iter, typename TN>
	class _all_tracks_iterator
	{
		ND* m_pNoteData;
		std::vector<iter> m_vBeginIters;

		/* There isn't a "past the beginning" iterator so this is hard to make a
		 * true bidirectional iterator. Use the "past the end" iterator in place
		 * of the "past the beginning" iterator when in reverse. */
		std::vector<iter> m_vCurrentIters;

		std::vector<iter> m_vEndIters;
		int m_iTrack;
		bool m_bReverse;

		// These exist so that the iterator can be revalidated if the NoteData
		// is transformed during this iterator's lifetime.
		std::vector<int> m_PrevCurrentRows;
		bool m_Inclusive;
		int m_StartRow;
		int m_EndRow;

		void Find(bool bReverse);

	  public:
		_all_tracks_iterator(ND& nd,
							 int iStartRow,
							 int iEndRow,
							 bool bReverse,
							 bool bInclusive);
		_all_tracks_iterator(const _all_tracks_iterator& other);
		~_all_tracks_iterator();
		auto operator++() -> _all_tracks_iterator&;			// preincrement
		auto operator++(int dummy) -> _all_tracks_iterator; // postincrement
		//_all_tracks_iterator &operator--();		// predecrement
		//_all_tracks_iterator operator--( int dummy );	// postdecrement
		[[nodiscard]] auto Track() const -> int { return m_iTrack; }
		[[nodiscard]] auto Row() const -> int
		{
			return m_vCurrentIters[m_iTrack]->first;
		}
		[[nodiscard]] auto IsAtEnd() const -> bool { return m_iTrack == -1; }
		[[nodiscard]] auto GetIter(int iTrack) const -> iter
		{
			return m_vCurrentIters[iTrack];
		}

		auto operator*() -> TN&
		{
			DEBUG_ASSERT(!IsAtEnd());
			return m_vCurrentIters[m_iTrack]->second;
		}

		auto operator->() -> TN*
		{
			DEBUG_ASSERT(!IsAtEnd());
			return &m_vCurrentIters[m_iTrack]->second;
		}

		auto operator*() const -> const TN&
		{
			DEBUG_ASSERT(!IsAtEnd());
			return m_vCurrentIters[m_iTrack]->second;
		}

		auto operator->() const -> const TN*
		{
			DEBUG_ASSERT(!IsAtEnd());
			return &m_vCurrentIters[m_iTrack]->second;
		}
		// Use when transforming the NoteData.
		void Revalidate(ND* notedata,
						std::vector<int> const& added_or_removed_tracks,
						bool added);
	};
	using all_tracks_iterator =
	  _all_tracks_iterator<NoteData, iterator, TapNote>;
	using all_tracks_const_iterator =
	  _all_tracks_iterator<const NoteData, const_iterator, const TapNote>;
	using all_tracks_reverse_iterator = all_tracks_iterator;
	using all_tracks_const_reverse_iterator = all_tracks_const_iterator;
	friend class _all_tracks_iterator<NoteData, iterator, TapNote>;
	friend class _all_tracks_iterator<const NoteData,
									  const_iterator,
									  const TapNote>;

  private:
	// There's no point in inserting empty notes into the map.
	// Any blank space in the map is defined to be empty.
	std::vector<TrackMap> m_TapNotes;
	int m_numTracksLCD;

	void CalcNumTracksLCD();

	/**
	 * @brief Determine if the note in question should be counted as a tap.
	 * @param tn the note in question.
	 * @param row the row it lives in.
	 * @return true if it's a tap, false otherwise. */
	static auto IsTap(const TapNote& tn, int row) -> bool;

	/**
	 * @brief Determine if the note in question should be counted as a mine.
	 * @param tn the note in question.
	 * @param row the row it lives in.
	 * @return true if it's a mine, false otherwise. */
	static auto IsMine(const TapNote& tn, int row) -> bool;

	/**
	 * @brief Determine if the note in question should be counted as a lift.
	 * @param tn the note in question.
	 * @param row the row it lives in.
	 * @return true if it's a lift, false otherwise. */
	static auto IsLift(const TapNote& tn, int row) -> bool;

	/**
	 * @brief Determine if the note in question should be counted as a fake.
	 * @param tn the note in question.
	 * @param row the row it lives in.
	 * @return true if it's a fake, false otherwise. */
	static auto IsFake(const TapNote& tn, int row) -> bool;

	// These exist so that they can be revalidated when something that
	// transforms the NoteData occurs. -Kyz
	mutable std::set<all_tracks_iterator*> m_atis;
	mutable std::set<all_tracks_const_iterator*> m_const_atis;

	void AddATIToList(all_tracks_iterator* iter) const;
	void AddATIToList(all_tracks_const_iterator* iter) const;
	void RemoveATIFromList(all_tracks_iterator* iter) const;
	void RemoveATIFromList(all_tracks_const_iterator* iter) const;

	// Mina stuf
	std::vector<int> NonEmptyRowVector;
	std::vector<NoteInfo> SerializedNoteData;

  public:
	void Init();

	// Mina stuf
	void LogNonEmptyRows(TimingData* ts);
	void UnsetNerv()
	{
		NonEmptyRowVector.clear();
		NonEmptyRowVector.shrink_to_fit();
	}
	void UnsetSerializedNoteData()
	{
		SerializedNoteData.clear();
		SerializedNoteData.shrink_to_fit();
	}
	auto BuildAndGetNerv(TimingData* ts) -> const std::vector<int>&
	{
		LogNonEmptyRows(ts);
		return NonEmptyRowVector;
	}
	auto WifeTotalScoreCalc(TimingData* td,
							int iStartIndex = 0,
							int iEndIndex = MAX_NOTE_ROW) const -> int;
	auto GetNonEmptyRowVector() -> std::vector<int>&
	{
		return NonEmptyRowVector;
	};
	auto SerializeNoteData(const std::vector<float>& etaner)
	  -> const std::vector<NoteInfo>&;
	// faster than the above and gives us more control over stuff like nerv
	// generation
	auto SerializeNoteData2(TimingData* ts,
							bool unset_nerv_when_done = true,
							bool unset_etaner_when_done = true)
	  -> const std::vector<NoteInfo>&;

	auto GetNumTracks() const -> int
	{
		return static_cast<int>(m_TapNotes.size());
	}
	void SetNumTracks(int iNewNumTracks);
	auto operator==(const NoteData& nd) const -> bool
	{
		return m_TapNotes == nd.m_TapNotes;
	}
	auto operator!=(const NoteData& nd) const -> bool
	{
		return m_TapNotes != nd.m_TapNotes;
	}

	/* Return the note at the given track and row.  Row may be out of
	 * range; pretend the song goes on with TAP_EMPTYs indefinitely. */
	auto GetTapNote(const unsigned& track, const int& row) const
	  -> const TapNote&
	{
		const auto& mapTrack = m_TapNotes[track];
		const auto iter = mapTrack.find(row);
		if (iter != mapTrack.end()) {
			return iter->second;
		}
		{
			return TAP_EMPTY;
		}
	}

	auto FindTapNote(unsigned iTrack, int iRow) -> iterator
	{
		return m_TapNotes[iTrack].find(iRow);
	}

	auto FindTapNote(unsigned iTrack, int iRow) const -> const_iterator
	{
		return m_TapNotes[iTrack].find(iRow);
	}
	void RemoveTapNote(unsigned iTrack, iterator it)
	{
		m_TapNotes[iTrack].erase(it);
	}

	/**
	 * @brief Return an iterator range for [rowBegin,rowEnd).
	 *
	 * This can be used to efficiently iterate trackwise over a range of notes.
	 * It's like FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE, except it only requires
	 * two map searches (iterating is constant time), but the iterators will
	 * become invalid if the notes they represent disappear, so you need to
	 * pay attention to how you modify the data.
	 * @param iTrack the column to use.
	 * @param iStartRow the starting point.
	 * @param iEndRow the ending point.
	 * @param begin the eventual beginning point of the range.
	 * @param end the eventual end point of the range. */
	void GetTapNoteRange(int iTrack,
						 int iStartRow,
						 int iEndRow,
						 TrackMap::const_iterator& begin,
						 TrackMap::const_iterator& end) const;
	/**
	 * @brief Return a constant iterator range for [rowBegin,rowEnd).
	 * @param iTrack the column to use.
	 * @param iStartRow the starting point.
	 * @param iEndRow the ending point.
	 * @param begin the eventual beginning point of the range.
	 * @param end the eventual end point of the range. */
	void GetTapNoteRange(int iTrack,
						 int iStartRow,
						 int iEndRow,
						 TrackMap::iterator& begin,
						 TrackMap::iterator& end);
	auto GetTapNoteRangeAllTracks(int iStartRow,
								  int iEndRow,
								  bool bInclusive = false)
	  -> all_tracks_iterator
	{
		return all_tracks_iterator(
		  *this, iStartRow, iEndRow, false, bInclusive);
	}
	auto GetTapNoteRangeAllTracks(int iStartRow,
								  int iEndRow,
								  bool bInclusive = false) const
	  -> all_tracks_const_iterator
	{
		return all_tracks_const_iterator(
		  *this, iStartRow, iEndRow, false, bInclusive);
	}
	auto GetTapNoteRangeAllTracksReverse(int iStartRow,
										 int iEndRow,
										 bool bInclusive = false)
	  -> all_tracks_reverse_iterator
	{
		return all_tracks_iterator(*this, iStartRow, iEndRow, true, bInclusive);
	}
	auto GetTapNoteRangeAllTracksReverse(int iStartRow,
										 int iEndRow,
										 bool bInclusive = false) const
	  -> all_tracks_const_reverse_iterator
	{
		return all_tracks_const_iterator(
		  *this, iStartRow, iEndRow, true, bInclusive);
	}

	// Call this after using any transform that changes the NoteData.
	void RevalidateATIs(std::vector<int> const& added_or_removed_tracks,
						bool added);

	/* Return an iterator range include iStartRow to iEndRow.  Extend the range
	 * to include hold notes overlapping the boundary. */
	void GetTapNoteRangeInclusive(int iTrack,
								  int iStartRow,
								  int iEndRow,
								  TrackMap::const_iterator& begin,
								  TrackMap::const_iterator& end,
								  bool bIncludeAdjacent = false) const;
	void GetTapNoteRangeInclusive(int iTrack,
								  int iStartRow,
								  int iEndRow,
								  TrackMap::iterator& begin,
								  TrackMap::iterator& end,
								  bool bIncludeAdjacent = false);

	/* Return an iterator range include iStartRow to iEndRow.  Shrink the range
	 * to exclude hold notes overlapping the boundary. */
	void GetTapNoteRangeExclusive(int iTrack,
								  int iStartRow,
								  int iEndRow,
								  TrackMap::const_iterator& begin,
								  TrackMap::const_iterator& end) const;
	void GetTapNoteRangeExclusive(int iTrack,
								  int iStartRow,
								  int iEndRow,
								  TrackMap::iterator& begin,
								  TrackMap::iterator& end);

	/* Returns the row of the first TapNote on the track that has a row greater
	 * than rowInOut. */
	auto GetNextTapNoteRowForTrack(int track,
								   int& rowInOut,
								   bool ignoreAutoKeysounds = false) const
	  -> bool;
	auto GetNextTapNoteRowForAllTracks(int& rowInOut) const -> bool;
	auto GetPrevTapNoteRowForTrack(int track, int& rowInOut) const -> bool;
	auto GetPrevTapNoteRowForAllTracks(int& rowInOut) const -> bool;

	void MoveTapNoteTrack(int dest, int src);
	void SetTapNote(int track, int row, const TapNote& tn);
	/**
	 * @brief Add a hold note, merging other overlapping holds and destroying
	 * tap notes underneath.
	 * @param iTrack the column to work with.
	 * @param iStartRow the starting row.
	 * @param iEndRow the ending row.
	 * @param tn the tap note. */
	void AddHoldNote(int iTrack, int iStartRow, int iEndRow, TapNote tn);

	void ClearRangeForTrack(int rowBegin, int rowEnd, int iTrack);
	void ClearRange(int rowBegin, int rowEnd);
	void ClearAll();
	void CopyRange(const NoteData& from,
				   int rowFromBegin,
				   int rowFromEnd,
				   int rowToBegin = 0);
	void CopyAll(const NoteData& from);

	auto IsRowEmpty(int row) const -> bool;
	auto IsRangeEmpty(int track, int rowBegin, int rowEnd) const -> bool;
	auto GetNumTapNonEmptyTracks(int row) const -> int;
	void GetTapNonEmptyTracks(int row, std::set<int>& addTo) const;
	auto GetTapFirstNonEmptyTrack(int row, int& iNonEmptyTrackOut) const
	  -> bool; // return false if no non-empty tracks at row
	auto GetTapFirstEmptyTrack(int row, int& iEmptyTrackOut) const
	  -> bool; // return false if no non-empty tracks at row
	auto GetTapLastEmptyTrack(int row, int& iEmptyTrackOut) const
	  -> bool; // return false if no empty tracks at row
	auto GetNumTracksWithTap(int row) const -> int;
	auto GetNumTracksWithTapOrHoldHead(int row) const -> int;
	auto GetFirstTrackWithTap(int row) const -> int;
	auto GetFirstTrackWithTapOrHoldHead(int row) const -> int;
	auto GetLastTrackWithTapOrHoldHead(int row) const -> int;

	auto IsThereATapAtRow(int row) const -> bool
	{
		return GetFirstTrackWithTap(row) != -1;
	}

	auto IsThereATapOrHoldHeadAtRow(int row) const -> bool
	{
		return GetFirstTrackWithTapOrHoldHead(row) != -1;
	}
	void GetTracksHeldAtRow(int row, std::set<int>& addTo) const;
	auto GetNumTracksHeldAtRow(int row) -> int;

	auto IsHoldNoteAtRow(int iTrack, int iRow, int* pHeadRow = nullptr) const
	  -> bool;
	auto IsHoldHeadOrBodyAtRow(int iTrack, int iRow, int* pHeadRow) const
	  -> bool;

	// statistics
	auto IsEmpty() const -> bool;
	auto IsTrackEmpty(int iTrack) const -> bool
	{
		return m_TapNotes[iTrack].empty();
	}
	auto GetFirstRow() const -> int; // return the beat number of the first note
	auto GetLastRow() const -> int;	 // return the beat number of the last note
	auto GetFirstBeat() const -> float { return NoteRowToBeat(GetFirstRow()); }
	auto GetLastBeat() const -> float { return NoteRowToBeat(GetLastRow()); }
	auto GetNumTapNotes(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int;
	auto GetNumTapNotesNoTiming(int iStartIndex = 0,
								int iEndIndex = MAX_NOTE_ROW) const -> int;
	auto GetNumTapNotesInRow(int iRow) const -> int;
	auto GetNumMines(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int;
	auto GetNumRowsWithTap(int iStartIndex = 0,
						   int iEndIndex = MAX_NOTE_ROW) const -> int;
	auto GetNumRowsWithTapOrHoldHead(int iStartIndex = 0,
									 int iEndIndex = MAX_NOTE_ROW) const -> int;
	/* Optimization: for the default of start to end, use the second (faster).
	 * XXX: Second what? -- Steve */
	auto GetNumHoldNotes(int iStartIndex = 0,
						 int iEndIndex = MAX_NOTE_ROW) const -> int;
	auto GetNumRolls(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int;

	// Count rows that contain iMinTaps or more taps.
	auto GetNumRowsWithSimultaneousTaps(int iMinTaps,
										int iStartIndex = 0,
										int iEndIndex = MAX_NOTE_ROW) const
	  -> int;
	auto GetNumJumps(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int
	{
		return GetNumRowsWithSimultaneousTaps(2, iStartIndex, iEndIndex);
	}

	// This row needs at least iMinSimultaneousPresses either tapped or held.
	auto RowNeedsAtLeastSimultaneousPresses(int iMinSimultaneousPresses,
											int row) const -> bool;
	auto RowNeedsHands(int row) const -> bool
	{
		return RowNeedsAtLeastSimultaneousPresses(3, row);
	}

	// Count rows that need iMinSimultaneousPresses either tapped or held.
	auto GetNumRowsWithSimultaneousPresses(int iMinSimultaneousPresses,
										   int iStartIndex = 0,
										   int iEndIndex = MAX_NOTE_ROW) const
	  -> int;
	auto GetNumHands(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int
	{
		return GetNumRowsWithSimultaneousPresses(3, iStartIndex, iEndIndex);
	}
	auto GetNumQuads(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int
	{
		return GetNumRowsWithSimultaneousPresses(4, iStartIndex, iEndIndex);
	}

	// and the other notetypes
	auto GetNumLifts(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int;
	auto GetNumFakes(int iStartIndex = 0, int iEndIndex = MAX_NOTE_ROW) const
	  -> int;

	auto GetNumTracksLCD() const -> int;

	// Transformations
	void LoadTransformed(
	  const NoteData& in,
	  int iNewNumTracks,
	  const int iOriginalTrackToTakeFrom[]); // -1 for iOriginalTracksToTakeFrom
											 // means no track

	// XML
	auto CreateNode() const -> XNode*;
	static void LoadFromNode(const XNode* pNode);
};
#endif
