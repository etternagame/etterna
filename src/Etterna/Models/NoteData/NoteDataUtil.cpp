#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/GameState.h"
#include "NoteData.h"
#include "NoteDataUtil.h"
#include "Etterna/Models/Misc/PlayerOptions.h"
#include "Etterna/Models/Misc/RadarValues.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Models/Songs/Song.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "Etterna/Globals/rngthing.h"

#include <utility>
#include <numeric>
#include <algorithm>
#include <set>
#include <unordered_map>

using std::pair;
using std::set;
using std::string;
using std::tuple;

// TODO: Remove these constants that aren't time signature-aware
static const int BEATS_PER_MEASURE = 4;
static const int ROWS_PER_MEASURE = ROWS_PER_BEAT * BEATS_PER_MEASURE;

NoteType
NoteDataUtil::GetSmallestNoteTypeForMeasure(const NoteData& nd,
											int iMeasureIndex)
{
	const auto iMeasureStartIndex = iMeasureIndex * ROWS_PER_MEASURE;
	const auto iMeasureEndIndex = (iMeasureIndex + 1) * ROWS_PER_MEASURE;

	return GetSmallestNoteTypeInRange(nd, iMeasureStartIndex, iMeasureEndIndex);
}

NoteType
NoteDataUtil::GetSmallestNoteTypeInRange(const NoteData& n,
										 int iStartIndex,
										 int iEndIndex)
{
	// probe to find the smallest note type
	FOREACH_ENUM(NoteType, nt)
	{
		const auto fBeatSpacing = NoteTypeToBeat(nt);
		const int iRowSpacing = lround(fBeatSpacing * ROWS_PER_BEAT);

		auto bFoundSmallerNote = false;
		// for each index in this measure
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(n, i, iStartIndex, iEndIndex)
		{
			if (i % iRowSpacing == 0)
				continue; // skip

			if (!n.IsRowEmpty(i)) {
				bFoundSmallerNote = true;
				break;
			}
		}

		if (bFoundSmallerNote)
			continue; // searching the next NoteType

		return nt; // stop searching. We found the smallest NoteType
	}
	return NoteType_Invalid; // well-formed notes created in the editor should
							 // never get here
}

static void
LoadFromSMNoteDataStringWithPlayer(NoteData& out,
								   const std::string& sSMNoteData,
								   int start,
								   int len,
								   int iNumTracks)
{
	/* Don't allocate memory for the entire string, nor per measure. Instead,
	 * use the in-place partial string split twice. By maintaining begin and end
	 * pointers to each measure line we can perform this without copying the
	 * string at all. */
	auto size = -1;
	const auto end = start + len;
	std::vector<pair<const char*, const char*>> aMeasureLines;
	for (unsigned m = 0; true; ++m) {
		/* XXX Ignoring empty seems wrong for measures. It means that ",,," is
		 * treated as
		 * "," where I would expect most people would want 2 empty measures.
		 * ",\n,\n," would do as I would expect. */
		split(sSMNoteData,
			  ",",
			  start,
			  size,
			  end,
			  true); // Ignore empty is important.
		if (start == end) {
			break;
		}
		// Partial string split.
		auto measureLineStart = start, measureLineSize = -1;
		const auto measureEnd = start + size;

		aMeasureLines.clear();
		for (;;) {
			// Ignore empty is clearly important here.
			split(sSMNoteData,
				  "\n",
				  measureLineStart,
				  measureLineSize,
				  measureEnd,
				  true);
			if (measureLineStart == measureEnd) {
				break;
			}
			// std::string &line = sSMNoteData.substr( measureLineStart,
			// measureLineSize );
			const auto* beginLine = sSMNoteData.data() + measureLineStart;
			const auto* endLine = beginLine + measureLineSize;

			while (beginLine < endLine && strchr("\r\n\t ", *beginLine))
				++beginLine;
			while (endLine > beginLine && strchr("\r\n\t ", *(endLine - 1)))
				--endLine;
			if (beginLine < endLine) // nonempty
				aMeasureLines.emplace_back(
				  pair<const char*, const char*>(beginLine, endLine));
		}

		for (unsigned l = 0; l < aMeasureLines.size(); l++) {
			const auto* p = aMeasureLines[l].first;
			const auto* const endLine = aMeasureLines[l].second;

			const auto fPercentIntoMeasure =
			  l / static_cast<float>(aMeasureLines.size());
			const auto fBeat = (m + fPercentIntoMeasure) * BEATS_PER_MEASURE;
			const auto iIndex = BeatToNoteRow(fBeat);

			auto iTrack = 0;
			while (iTrack < iNumTracks && p < endLine) {
				TapNote tn;
				auto ch = *p;

				switch (ch) {
					case '0':
						break; // massive waste of processor time to initialize
							   // a tapnote with type empty only to do nothing
							   // with it -mina
					case '1':
						tn = TAP_ORIGINAL_TAP;
						break;
					case '2':
					case '4':
						// case 'N': // minefield
						tn = ch == '2' ? TAP_ORIGINAL_HOLD_HEAD
									   : TAP_ORIGINAL_ROLL_HEAD;
						/*
						// upcoming code for minefields -aj
						switch(ch)
						{
						case '2': tn = TAP_ORIGINAL_HOLD_HEAD; break;
						case '4': tn = TAP_ORIGINAL_ROLL_HEAD; break;
						case 'N': tn = TAP_ORIGINAL_MINE_HEAD; break;
						}
						*/

						/* Set the hold note to have infinite length. We'll
						 * clamp it when we hit the tail. */
						tn.iDuration = MAX_NOTE_ROW;
						break;
					case '3': {
						// This is the end of a hold. Search for the beginning.
						int iHeadRow;
						if (!out.IsHoldNoteAtRow(iTrack, iIndex, &iHeadRow)) {
							//int n = intptr_t(endLine) - intptr_t(beginLine);
						} else {
							out.FindTapNote(iTrack, iHeadRow)
							  ->second.iDuration = iIndex - iHeadRow;
						}

						// This won't write tn, but keep parsing normally
						// anyway.
						break;
					}
					//				case 'm':
					// Don't be loose with the definition.  Use only 'M' since
					// that's what we've been writing to disk.  -Chris
					case 'M':
						tn = TAP_ORIGINAL_MINE;
						break;
					// case 'A': tn = TAP_ORIGINAL_ATTACK;			break;
					case 'K':
						tn = TAP_ORIGINAL_AUTO_KEYSOUND;
						break;
					case 'L':
						tn = TAP_ORIGINAL_LIFT;
						break;
					case 'F':
						tn = TAP_ORIGINAL_FAKE;
						break;
					// case 'I': tn = TAP_ORIGINAL_ITEM;			break;
					default:
						/* Invalid data. We don't want to assert, since there
						 * might simply be invalid data in an .SM, and we don't
						 * want to die due to invalid data. We should probably
						 * check for this when we load SM data for the first
						 * time ... */
						// FAIL_M("Invalid data in SM");

						// massive waste of processor time -mina
						// tn = TAP_EMPTY;
						break;
				}

				p++;
				// We won't scan past the end of the line so these are safe to
				// do.

				// look for optional keysound index (e.g. "[123]")
				if (*p == '[') {
					p++;
					auto iKeysoundIndex = 0;
					if (1 == sscanf(p,
									"%d]",
									&iKeysoundIndex)) // not fatal if this fails
													  // due to malformed data
						tn.iKeysoundIndex = iKeysoundIndex;

					// skip past the ']'
					while (p < endLine) {
						if (*p++ == ']')
							break;
					}
				}

				/* Optimization: if we pass TAP_EMPTY, NoteData will do a search
				 * to remove anything in this position.  We know that there's
				 * nothing there, so avoid the search. */
				if (tn.type != TapNoteType_Empty && ch != '3') {
					out.SetTapNote(iTrack, iIndex, tn);
				}

				iTrack++;
			}
		}
	}

	// Make sure we don't have any hold notes that didn't find a tail.
	for (auto t = 0; t < out.GetNumTracks(); t++) {
		auto begin = out.begin(t);
		auto lEnd = out.end(t);
		while (begin != lEnd) {
			auto next = Increment(begin);
			const auto& tn = begin->second;
			if (tn.type == TapNoteType_HoldHead &&
				tn.iDuration == MAX_NOTE_ROW) {
				auto iRow = begin->first;
				Locator::getLogger()->debug(
				  "While loading .sm/.ssc note data, there was an unmatched 2 "
				  "at beat {}",
				  NoteRowToBeat(iRow));
				out.RemoveTapNote(t, begin);
			}

			begin = next;
		}
	}
	out.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::LoadFromETTNoteDataString(NoteData& out,
										const std::string& sSMNoteData)
{
	size_t pos = 0;

	auto m = 0;
	auto at = 0;

	int lasthead[4];
	for (;;) {
		auto mend = sSMNoteData.find(',', pos);
		int nt = mend - pos - 1;
		if (mend == -1)
			nt = sSMNoteData.size() - pos - 1;

		const auto cnt = sSMNoteData.at(at);
		++at;

		if (cnt == '0')
			nt = 16;
		else if (cnt == '1')
			nt = 32;
		else if (cnt == '2')
			nt = 48;
		else if (cnt == '3')
			nt = 64;
		else if (cnt == '4')
			nt = 96;
		else if (cnt == '5')
			nt = 128;
		else if (cnt == '6')
			nt = 192;
		else if (cnt == '7')
			nt = 256;
		else if (cnt == '8')
			nt = 768;

		auto tps = 0;
		auto tr = 0;
		auto r = 0;
		while (tps < nt) {
			const auto fPercentIntoMeasure = r / static_cast<float>(nt) * 4;
			const auto fBeat = (m + fPercentIntoMeasure) * BEATS_PER_MEASURE;
			const auto iIndex = BeatToNoteRow(fBeat);

			const auto c = sSMNoteData.at(at);
			++at;
			if (c == '.') {
				tps += 256;
				r += 64;
				continue;
			}
			if (c == '|') {
				tps += 16;
				r += 4;
				continue;
			}
			if (c == '~') {
				tps += 8;
				r += 2;
				continue;
			}
			if (c == '?') {
				tps += 4;
				++r;
				continue;
			}
			if (c == 'M') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'W') {
				tps += 4;
				++r;
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'L') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'D') {
				tps += 4;
				++r;
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'U') {
				tps += 4;
				++r;
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'Y') {
				tps += 4;
				++r;
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'B') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'Z') {
				tps += 4;
				++r;
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'G') {
				tps += 4;
				++r;
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'K') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'U') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'I') {
				tps += 4;
				++r;
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'O') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(2, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == 'P') {
				tps += 4;
				++r;
				out.SetTapNote(0, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(1, iIndex, TAP_ORIGINAL_TAP);
				out.SetTapNote(3, iIndex, TAP_ORIGINAL_TAP);
				continue;
			}
			if (c == '`') { // compressed notes this far are pretty rare
				tps += 128;
				r += 32;
				continue;
			}
			if (c == '-') {
				tps += 64;
				r += 16;
				continue;
			}
			if (c == '!') {
				tps += 32;
				r += 8;
				continue;
			}
			if (c == 'E') {
				++tps;
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == 'T') {
				++tps;
				out.SetTapNote(tr, iIndex, TAP_ORIGINAL_TAP);
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == 'H') {
				++tps;
				lasthead[tr] = iIndex;
				auto tn = TAP_ORIGINAL_HOLD_HEAD;
				tn.iDuration = 1;
				out.SetTapNote(tr, iIndex, TAP_ORIGINAL_HOLD_HEAD);
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == 'R') {
				++tps;
				lasthead[tr] = iIndex;
				auto tn = TAP_ORIGINAL_HOLD_HEAD;
				tn.iDuration = 1;
				out.SetTapNote(tr, iIndex, TAP_ORIGINAL_ROLL_HEAD);
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == 'J') {
				++tps;
				lasthead[tr] = iIndex;
				auto tn = TAP_ORIGINAL_HOLD_HEAD;
				tn.iDuration = 1;
				out.FindTapNote(tr, lasthead[tr])->second.iDuration =
				  iIndex - lasthead[tr];
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == '*') {
				++tps;
				out.SetTapNote(tr, iIndex, TAP_ORIGINAL_MINE);
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == '^') {
				++tps;
				out.SetTapNote(tr, iIndex, TAP_ORIGINAL_LIFT);
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
			if (c == 'V') {
				++tps;
				out.SetTapNote(tr, iIndex, TAP_ORIGINAL_FAKE);
				++tr;
				if (tr == 4) {
					++r;
					tr = 0;
				}
				continue;
			}
		}

		if (mend == -1)
			break;

		at += 2;
		pos += nt + 2;
		++m;
	}
	out.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::LoadFromSMNoteDataString(NoteData& out,
									   const std::string& sSMNoteData_)
{
	// Load note data
	std::string sSMNoteData;
	std::string::size_type iIndexCommentStart = 0;
	std::string::size_type iIndexCommentEnd = 0;
	const auto origSize = sSMNoteData_.size();
	const auto* p = sSMNoteData_.data();

	sSMNoteData.reserve(origSize);
	while ((iIndexCommentStart = sSMNoteData_.find("//", iIndexCommentEnd)) !=
		   std::string::npos) {
		sSMNoteData.append(p, iIndexCommentStart - iIndexCommentEnd);
		p += iIndexCommentStart - iIndexCommentEnd;
		iIndexCommentEnd = sSMNoteData_.find('\n', iIndexCommentStart);
		iIndexCommentEnd = iIndexCommentEnd == std::string::npos
							 ? origSize
							 : iIndexCommentEnd + 1;
		p += iIndexCommentEnd - iIndexCommentStart;
	}
	sSMNoteData.append(p, origSize - iIndexCommentEnd);

	// Clear notes, but keep the same number of tracks.
	const auto iNumTracks = out.GetNumTracks();
	out.Init();
	out.SetNumTracks(iNumTracks);
	LoadFromSMNoteDataStringWithPlayer(
	  out, sSMNoteData, 0, sSMNoteData.size(), iNumTracks);
}

void
NoteDataUtil::InsertHoldTails(NoteData& inout)
{
	for (auto t = 0; t < inout.GetNumTracks(); t++) {
		auto begin = inout.begin(t), end = inout.end(t);

		for (; begin != end; ++begin) {
			const auto iRow = begin->first;
			const auto& tn = begin->second;
			if (tn.type != TapNoteType_HoldHead)
				continue;

			auto tail = tn;
			tail.type = TapNoteType_HoldTail;

			/* If iDuration is 0, we'd end up overwriting the head with the tail
			 * (and invalidating our iterator). Empty hold notes aren't valid.
			 */
			ASSERT(tn.iDuration != 0);

			inout.SetTapNote(t, iRow + tn.iDuration, tail);
		}
	}
}

void
NoteDataUtil::GetSMNoteDataString(const NoteData& in, std::string& sRet)
{
	// Get note data
	auto nd = in;
	auto fLastBeat = -1.0f;

	InsertHoldTails(nd);
	fLastBeat = std::max(fLastBeat, nd.GetLastBeat());

	const auto iLastMeasure = static_cast<int>(fLastBeat / BEATS_PER_MEASURE);

	sRet = "";
	for (auto m = 0; m <= iLastMeasure; ++m) // foreach measure
	{
		if (m)
			sRet.append(1, ',');
		sRet += ssprintf("  // measure %d\n", m);

		const auto nt = GetSmallestNoteTypeForMeasure(nd, m);
		int iRowSpacing;
		if (nt == NoteType_Invalid)
			iRowSpacing = 1;
		else
			iRowSpacing = lround(NoteTypeToBeat(nt) * ROWS_PER_BEAT);
		// (verify first)
		// iRowSpacing = BeatToNoteRow( NoteTypeToBeat(nt) );

		const auto iMeasureStartRow = m * ROWS_PER_MEASURE;
		const auto iMeasureLastRow = (m + 1) * ROWS_PER_MEASURE - 1;

		for (auto r = iMeasureStartRow; r <= iMeasureLastRow;
			 r += iRowSpacing) {
			for (auto t = 0; t < nd.GetNumTracks(); ++t) {
				const auto& tn = nd.GetTapNote(t, r);
				char c;
				switch (tn.type) {
					case TapNoteType_Empty:
						c = '0';
						break;
					case TapNoteType_Tap:
						c = '1';
						break;
					case TapNoteType_HoldHead:
						switch (tn.subType) {
							case TapNoteSubType_Hold:
								c = '2';
								break;
							case TapNoteSubType_Roll:
								c = '4';
								break;
							// case TapNoteSubType_Mine:	c = 'N'; break;
							default:
								FAIL_M(ssprintf("Invalid tap note subtype: %i",
												tn.subType));
						}
						break;
					case TapNoteType_HoldTail:
						c = '3';
						break;
					case TapNoteType_Mine:
						c = 'M';
						break;
					case TapNoteType_AutoKeysound:
						c = 'K';
						break;
					case TapNoteType_Lift:
						c = 'L';
						break;
					case TapNoteType_Fake:
						c = 'F';
						break;
					default:
						c = '\0';
						FAIL_M(ssprintf("Invalid tap note type: %i", tn.type));
				}
				sRet.append(1, c);

				// hey maybe if we have TapNoteType_Item we can do things
				// here.
				if (tn.iKeysoundIndex >= 0)
					sRet.append(ssprintf("[%d]", tn.iKeysoundIndex));
			}

			sRet.append(1, '\n');
		}
	}
}

void
NoteDataUtil::GetETTNoteDataString(const NoteData& in, std::string& sRet)
{
	// Get note data
	auto nd = in;
	auto fLastBeat = -1.f;

	fLastBeat = std::max(fLastBeat, nd.GetLastBeat());

	auto iLastMeasure = static_cast<int>(fLastBeat / BEATS_PER_MEASURE);

	sRet = "";

	if (in.GetNumTracks() != 4) {
		sRet.shrink_to_fit();
		return;
	}

	for (auto m = 0; m <= iLastMeasure; ++m) {
		if (m)
			sRet.append(1, ',');
		auto nt = GetSmallestNoteTypeForMeasure(nd, m);
		int iRowSpacing;
		if (nt == NoteType_Invalid)
			iRowSpacing = 1;
		else
			iRowSpacing = lround(NoteTypeToBeat(nt) * ROWS_PER_BEAT);

		const auto iMeasureStartRow = m * ROWS_PER_MEASURE;
		const auto iMeasureLastRow = (m + 1) * ROWS_PER_MEASURE - 1;

		sRet.append(std::to_string(nt));
		for (auto r = iMeasureStartRow; r <= iMeasureLastRow;
			 r += iRowSpacing) {
			string halp;
			for (auto t = 0; t < nd.GetNumTracks(); ++t) {
				const auto& tn = nd.GetTapNote(t, r);
				if (tn.type == TapNoteType_Empty) {
					halp.append("E");
					continue;
				}
				if (tn.type == TapNoteType_Tap) {
					halp.append("T");
					continue;
				}
				if (tn.type == TapNoteType_HoldHead) {
					if (tn.subType == TapNoteSubType_Hold) {
						halp.append("H");
						continue;
					}
					if (tn.subType == TapNoteSubType_Roll) {
						halp.append("R");
						continue;
					}
				} else if (tn.type == TapNoteType_HoldTail) {
					halp.append("J");
					continue;
				} else if (tn.type == TapNoteType_Mine) {
					halp.append("*");
					continue;
				} else if (tn.type == TapNoteType_Lift) {
					halp.append("^");
					continue;
				} else if (tn.type == TapNoteType_Fake) {
					halp.append("V");
					continue;
				}
			}

			if (halp == "EEEE")
				halp = "?";
			else if (halp == "TTEE")
				halp = "M";
			else if (halp == "EETT")
				halp = "W";
			else if (halp == "TEEE")
				halp = "L";
			else if (halp == "ETEE")
				halp = "D";
			else if (halp == "EETE")
				halp = "U";
			else if (halp == "EEET")
				halp = "Y";
			else if (halp == "TEET")
				halp = "B";
			else if (halp == "ETTE")
				halp = "Z";
			else if (halp == "ETET")
				halp = "G";
			else if (halp == "TETE")
				halp = "K";
			else if (halp == "TTTE")
				halp = "U";
			else if (halp == "ETTT")
				halp = "I";
			else if (halp == "TETT")
				halp = "O";
			else if (halp == "TTET")
				halp = "P";

			sRet.append(halp);
		}
		sRet.append("\n");
	}
	size_t oop = 1;
	for (;;) {
		oop = sRet.find("??", oop - 1);
		if (oop == string::npos)
			break;
		sRet.replace(sRet.begin() + oop, sRet.begin() + oop + 2, "~");
	}
	oop = 1;
	for (;;) {
		oop = sRet.find("~~", oop - 1);
		if (oop == string::npos)
			break;
		sRet.replace(sRet.begin() + oop, sRet.begin() + oop + 2, "|");
	}
	oop = 1;
	for (;;) {
		oop = sRet.find("||", oop - 1);
		if (oop == string::npos)
			break;
		sRet.replace(sRet.begin() + oop, sRet.begin() + oop + 2, "!");
	}
	oop = 1;
	for (;;) {
		oop = sRet.find("!!", oop - 1);
		if (oop == string::npos)
			break;
		sRet.replace(sRet.begin() + oop, sRet.begin() + oop + 2, "-");
	}
	oop = 1;
	for (;;) {
		oop = sRet.find("--", oop - 1);
		if (oop == string::npos)
			break;
		sRet.replace(sRet.begin() + oop, sRet.begin() + oop + 2, "`");
	}
	oop = 1;
	for (;;) {
		oop = sRet.find("``", oop - 1);
		if (oop == string::npos)
			break;
		sRet.replace(sRet.begin() + oop, sRet.begin() + oop + 2, ".");
	}
	sRet.shrink_to_fit();
}

void
NoteDataUtil::LoadTransformedSlidingWindow(const NoteData& in,
										   NoteData& out,
										   int iNewNumTracks)
{
	// reset all notes
	out.Init();

	if (in.GetNumTracks() > iNewNumTracks) {
		// Use a different algorithm for reducing tracks.
		LoadOverlapped(in, out, iNewNumTracks);
		return;
	}

	out.SetNumTracks(iNewNumTracks);

	if (in.GetNumTracks() == 0)
		return; // nothing to do and don't AV below

	auto iCurTrackOffset = 0;
	const auto iTrackOffsetMin = 0;
	const auto iTrackOffsetMax = abs(iNewNumTracks - in.GetNumTracks());
	auto bOffsetIncreasing = 1;

	auto iLastMeasure = 0;
	auto iMeasuresSinceChange = 0;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(in, r)
	{
		const auto iMeasure = r / ROWS_PER_MEASURE;
		if (iMeasure != iLastMeasure)
			++iMeasuresSinceChange;

		if (iMeasure != iLastMeasure &&
			iMeasuresSinceChange >=
			  4) // adjust sliding window every 4 measures at most
		{
			// See if there is a hold crossing the beginning of this measure
			auto bHoldCrossesThisMeasure = false;

			for (auto t = 0; t < in.GetNumTracks(); t++) {
				if (in.IsHoldNoteAtRow(t, r - 1) && in.IsHoldNoteAtRow(t, r)) {
					bHoldCrossesThisMeasure = true;
					break;
				}
			}

			// adjust offset
			if (!bHoldCrossesThisMeasure) {
				iMeasuresSinceChange = 0;
				iCurTrackOffset += bOffsetIncreasing != 0 ? 1 : -1;
				if (iCurTrackOffset == iTrackOffsetMin ||
					iCurTrackOffset == iTrackOffsetMax)
					bOffsetIncreasing ^= 1;
				CLAMP(iCurTrackOffset, iTrackOffsetMin, iTrackOffsetMax);
			}
		}

		iLastMeasure = iMeasure;

		// copy notes in this measure
		for (auto t = 0; t < in.GetNumTracks(); t++) {
			const auto iOldTrack = t;
			const auto iNewTrack =
			  (iOldTrack + iCurTrackOffset) % iNewNumTracks;
			auto tn = in.GetTapNote(iOldTrack, r);
			out.SetTapNote(iNewTrack, r, tn);
		}
	}
	out.RevalidateATIs(std::vector<int>(), false);
}

void
PlaceAutoKeysound(NoteData& out, int row, TapNote akTap)
{
	auto iEmptyTrack = -1;
	auto iEmptyRow = row;
	const auto iNewNumTracks = out.GetNumTracks();
	auto bFoundEmptyTrack = false;
	int iRowsToLook[3] = { 0, -1, 1 };

	for (auto j : iRowsToLook) {
		auto r = j + row;
		if (r < 0)
			continue;
		for (auto i = 0; i < iNewNumTracks; ++i) {
			if (out.GetTapNote(i, r) == TAP_EMPTY &&
				!out.IsHoldNoteAtRow(i, r)) {
				iEmptyTrack = i;
				iEmptyRow = r;
				bFoundEmptyTrack = true;
				break;
			}
		}
		if (bFoundEmptyTrack)
			break;
	}

	if (iEmptyTrack != -1) {
		akTap.type = TapNoteType_AutoKeysound;
		out.SetTapNote(iEmptyTrack, iEmptyRow, akTap);
	}
}

void
NoteDataUtil::LoadOverlapped(const NoteData& in,
							 NoteData& out,
							 int iNewNumTracks)
{
	out.SetNumTracks(iNewNumTracks);

	/* Keep track of the last source track that put a tap into each destination
	 * track, and the row of that tap. Then, if two rows are trying to put taps
	 * into the same row within the shift threshold, shift the newcomer source
	 * row. */
	int LastSourceTrack[MAX_NOTE_TRACKS];
	int LastSourceRow[MAX_NOTE_TRACKS];
	int DestRow[MAX_NOTE_TRACKS];

	for (auto tr = 0; tr < MAX_NOTE_TRACKS; ++tr) {
		LastSourceTrack[tr] = -1;
		LastSourceRow[tr] = -MAX_NOTE_ROW;
		DestRow[tr] = tr;
		wrap(DestRow[tr], iNewNumTracks);
	}

	const auto ShiftThreshold = BeatToNoteRow(1);

	FOREACH_NONEMPTY_ROW_ALL_TRACKS(in, row)
	{
		for (auto iTrackFrom = 0; iTrackFrom < in.GetNumTracks();
			 ++iTrackFrom) {
			auto tnFrom = in.GetTapNote(iTrackFrom, row);
			if (tnFrom.type == TapNoteType_Empty ||
				tnFrom.type == TapNoteType_AutoKeysound)
				continue;

			// If this is a hold note, find the end.
			auto iEndIndex = row;
			if (tnFrom.type == TapNoteType_HoldHead)
				iEndIndex = row + tnFrom.iDuration;

			auto& iTrackTo = DestRow[iTrackFrom];
			if (LastSourceTrack[iTrackTo] != iTrackFrom) {
				if (iEndIndex - LastSourceRow[iTrackTo] < ShiftThreshold) {
					/* This destination track is in use by a different source
					 * track. Use the least-recently-used track. */
					for (auto DestTrack = 0; DestTrack < iNewNumTracks;
						 ++DestTrack)
						if (LastSourceRow[DestTrack] < LastSourceRow[iTrackTo])
							iTrackTo = DestTrack;
				}

				// If it's still in use, then we just don't have an available
				// track.
				if (iEndIndex - LastSourceRow[iTrackTo] < ShiftThreshold) {
					// If it has a keysound, put it in autokeysound track.
					if (tnFrom.iKeysoundIndex >= 0) {
						const auto akTap = tnFrom;
						PlaceAutoKeysound(out, row, akTap);
					}
					continue;
				}
			}

			LastSourceTrack[iTrackTo] = iTrackFrom;
			LastSourceRow[iTrackTo] = iEndIndex;
			out.SetTapNote(iTrackTo, row, tnFrom);
			if (tnFrom.type == TapNoteType_HoldHead) {
				const auto& tnTail = in.GetTapNote(iTrackFrom, iEndIndex);
				out.SetTapNote(iTrackTo, iEndIndex, tnTail);
			}
		}

		// find empty track for autokeysounds in 2 next rows, so you can hear
		// most autokeysounds
		for (auto iTrackFrom = 0; iTrackFrom < in.GetNumTracks();
			 ++iTrackFrom) {
			const auto& tnFrom = in.GetTapNote(iTrackFrom, row);
			if (tnFrom.type != TapNoteType_AutoKeysound)
				continue;

			PlaceAutoKeysound(out, row, tnFrom);
		}
	}
	out.RevalidateATIs(std::vector<int>(), false);
}

int
FindLongestOverlappingHoldNoteForAnyTrack(const NoteData& in, int iRow)
{
	auto iMaxTailRow = -1;
	for (auto t = 0; t < in.GetNumTracks(); t++) {
		const auto& tn = in.GetTapNote(t, iRow);
		if (tn.type == TapNoteType_HoldHead)
			iMaxTailRow = std::max(iMaxTailRow, iRow + tn.iDuration);
	}

	return iMaxTailRow;
}

struct recent_note
{
	int row{ 0 };
	int track{ 0 };
	recent_note() = default;
	recent_note(int r, int t)
	  : row(r)
	  , track(t)
	{
	}
};

// CalculateRadarValues has to delay some stuff until a row ends, but can
// only detect a row ending when it hits the next note.  There isn't a note
// after the last row, so it also has to do the delayed stuff after exiting
// its loop.  So this state structure exists to be passed to a function that
// can be called from both places to do the work.  If this were Lua,
// DoRowEndRadarCalc would be a nested function. -Kyz
struct crv_state
{
	bool judgable{ false };
	// hold_ends tracks where currently active holds will end, which is used
	// to count the number of hands. -Kyz
	std::vector<int> hold_ends;
	// num_holds_on_curr_row saves us the work of tracking where holds started
	// just to keep a jump of two holds from counting as a hand.
	int num_holds_on_curr_row{ 0 };
	int num_notes_on_curr_row{ 0 };

	crv_state() = default;
};

static void
DoRowEndRadarCalc(crv_state& state, RadarValues& out)
{
	if (state.judgable) {
		if (state.num_notes_on_curr_row +
			  (state.hold_ends.size() - state.num_holds_on_curr_row) >=
			3) {
			++out[RadarCategory_Hands];
		}
	}
}

void
NoteDataUtil::CalculateRadarValues(const NoteData& in,
								   RadarValues& out,
								   TimingData* td)
{
	// Anybody editing this function should also examine
	// NoteDataWithScoring::GetActualRadarValues to make sure it handles things
	// the same way.
	out.Zero();
	auto curr_row = -1;
	// recent_notes is used to calculate the voltage.  Each element is the row
	// and track number of a tap note.  When the pair at the beginning is too
	// old, it's deleted.  This provides a way to have a rolling window
	// that scans for the peak step density. -Kyz
	std::vector<recent_note> recent_notes;
	auto curr_note = in.GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW);
	auto* timing = td != nullptr ? td : GAMESTATE->GetProcessedTimingData();
	// total_taps exists because the stream calculation needs GetNumTapNotes,
	// but TapsAndHolds + Jumps + Hands would be inaccurate. -Kyz
	float total_taps = 0;
	crv_state state;

	while (!curr_note.IsAtEnd()) {
		if (curr_note.Row() != curr_row) {
			DoRowEndRadarCalc(state, out);
			curr_row = curr_note.Row();
			state.num_notes_on_curr_row = 0;
			state.num_holds_on_curr_row = 0;
			state.judgable = timing->IsJudgableAtRow(curr_row);
			for (size_t n = 0; n < state.hold_ends.size(); ++n) {
				if (state.hold_ends[n] < curr_row) {
					state.hold_ends.erase(state.hold_ends.begin() + n);
					--n;
				}
			}
		}
		if (state.judgable) {
			switch (curr_note->type) {
				case TapNoteType_Tap:
				case TapNoteType_HoldHead:
					// Lifts have to be counted with taps for them to be added
					// to max dp correctly. -Kyz
				case TapNoteType_Lift:
					// HoldTails and Attacks are counted by IsTap.  But it
					// doesn't make sense to count HoldTails as hittable notes.
					// -Kyz
					++out[RadarCategory_Notes];
					++state.num_notes_on_curr_row;
					++total_taps;
					// If there is one hold active, and one tap on this row, it
					// does not count as a jump.  Hands do need to count the
					// number of holds active though. -Kyz
					switch (state.num_notes_on_curr_row) {
						case 1:
							++out[RadarCategory_TapsAndHolds];
							break;
						case 2:
							++out[RadarCategory_Jumps];
							break;
						default:
							break;
					}
					if (curr_note->type == TapNoteType_HoldHead) {
						state.hold_ends.emplace_back(curr_row +
													 curr_note->iDuration);
						++state.num_holds_on_curr_row;
						switch (curr_note->subType) {
							case TapNoteSubType_Hold:
								++out[RadarCategory_Holds];
								break;
							case TapNoteSubType_Roll:
								++out[RadarCategory_Rolls];
								break;
							default:
								break;
						}
					} else if (curr_note->type == TapNoteType_Lift) {
						++out[RadarCategory_Lifts];
					}
					break;
				case TapNoteType_Mine:
					++out[RadarCategory_Mines];
					break;
				case TapNoteType_Fake:
					++out[RadarCategory_Fakes];
					break;
				default:
					break;
			}
		} else {
			++out[RadarCategory_Fakes];
		}
		++curr_note;
	}
	DoRowEndRadarCalc(state, out);
}

void
NoteDataUtil::RemoveHoldNotes(NoteData& in, int iStartIndex, int iEndIndex)
{
	// turn all the HoldNotes into TapNotes
	for (auto t = 0; t < in.GetNumTracks(); ++t) {
		NoteData::TrackMap::iterator begin, end;
		in.GetTapNoteRangeInclusive(t, iStartIndex, iEndIndex, begin, end);
		for (; begin != end; ++begin) {
			if (begin->second.type != TapNoteType_HoldHead ||
				begin->second.subType != TapNoteSubType_Hold)
				continue;
			begin->second.type = TapNoteType_Tap;
		}
	}
	in.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::ChangeRollsToHolds(NoteData& in, int iStartIndex, int iEndIndex)
{
	for (auto t = 0; t < in.GetNumTracks(); ++t) {
		NoteData::TrackMap::iterator begin, end;
		in.GetTapNoteRangeInclusive(t, iStartIndex, iEndIndex, begin, end);
		for (; begin != end; ++begin) {
			if (begin->second.type != TapNoteType_HoldHead ||
				begin->second.subType != TapNoteSubType_Roll)
				continue;
			begin->second.subType = TapNoteSubType_Hold;
		}
	}
	in.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::ChangeHoldsToRolls(NoteData& in, int iStartIndex, int iEndIndex)
{
	for (auto t = 0; t < in.GetNumTracks(); ++t) {
		NoteData::TrackMap::iterator begin, end;
		in.GetTapNoteRangeInclusive(t, iStartIndex, iEndIndex, begin, end);
		for (; begin != end; ++begin) {
			if (begin->second.type != TapNoteType_HoldHead ||
				begin->second.subType != TapNoteSubType_Hold)
				continue;
			begin->second.subType = TapNoteSubType_Roll;
		}
	}
	in.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveSimultaneousNotes(NoteData& in,
									  int iMaxSimultaneous,
									  int iStartIndex,
									  int iEndIndex)
{
	// Remove tap and hold notes so no more than iMaxSimultaneous buttons are
	// being held at any given time.  Never touch data outside of the range
	// given; if many hold notes are overlapping iStartIndex, and we'd have to
	// change those holds to obey iMaxSimultaneous, just do the best we can
	// without doing so.

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(in, r, iStartIndex, iEndIndex)
	{
		set<int> viTracksHeld;
		in.GetTracksHeldAtRow(r, viTracksHeld);

		// remove the first tap note or the first hold note that starts on this
		// row
		const int iTotalTracksPressed =
		  in.GetNumTracksWithTapOrHoldHead(r) + viTracksHeld.size();
		auto iTracksToRemove =
		  std::max(0, iTotalTracksPressed - iMaxSimultaneous);
		for (auto t = 0; iTracksToRemove > 0 && t < in.GetNumTracks(); t++) {
			const auto& tn = in.GetTapNote(t, r);
			if (tn.type == TapNoteType_Tap || tn.type == TapNoteType_HoldHead) {
				in.SetTapNote(t, r, TAP_EMPTY);
				iTracksToRemove--;
			}
		}
	}
	in.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveJumps(NoteData& inout, int iStartIndex, int iEndIndex)
{
	RemoveSimultaneousNotes(inout, 1, iStartIndex, iEndIndex);
}

void
NoteDataUtil::RemoveHands(NoteData& inout, int iStartIndex, int iEndIndex)
{
	RemoveSimultaneousNotes(inout, 2, iStartIndex, iEndIndex);
}

void
NoteDataUtil::RemoveQuads(NoteData& inout, int iStartIndex, int iEndIndex)
{
	RemoveSimultaneousNotes(inout, 3, iStartIndex, iEndIndex);
}

void
NoteDataUtil::RemoveSpecificTapNotes(NoteData& inout,
									 TapNoteType tn,
									 int iStartIndex,
									 int iEndIndex)
{
	for (auto t = 0; t < inout.GetNumTracks(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(inout, t, r, iStartIndex, iEndIndex)
		{
			if (inout.GetTapNote(t, r).type == tn) {
				inout.SetTapNote(t, r, TAP_EMPTY);
			}
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveMines(NoteData& inout, int iStartIndex, int iEndIndex)
{
	RemoveSpecificTapNotes(inout, TapNoteType_Mine, iStartIndex, iEndIndex);
}

void
NoteDataUtil::RemoveLifts(NoteData& inout, int iStartIndex, int iEndIndex)
{
	RemoveSpecificTapNotes(inout, TapNoteType_Lift, iStartIndex, iEndIndex);
}

void
NoteDataUtil::RemoveFakes(NoteData& inout,
						  TimingData const& timing_data,
						  int iStartIndex,
						  int iEndIndex)
{
	RemoveSpecificTapNotes(inout, TapNoteType_Fake, iStartIndex, iEndIndex);
	for (auto t = 0; t < inout.GetNumTracks(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(inout, t, r, iStartIndex, iEndIndex)
		{
			if (!timing_data.IsJudgableAtRow(r)) {
				inout.SetTapNote(t, r, TAP_EMPTY);
			}
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveAllButOneTap(NoteData& inout, int row)
{
	if (row < 0)
		return;

	int track;
	for (track = 0; track < inout.GetNumTracks(); ++track) {
		if (inout.GetTapNote(track, row).type == TapNoteType_Tap)
			break;
	}

	track++;

	for (; track < inout.GetNumTracks(); ++track) {
		auto iter = inout.FindTapNote(track, row);
		if (iter != inout.end(track) && iter->second.type == TapNoteType_Tap)
			inout.RemoveTapNote(track, iter);
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

// TODO: Perform appropriate matrix calculations for everything instead.
static void
GetTrackMapping(StepsType st,
				NoteDataUtil::TrackMapping tt,
				int NumTracks,
				int* iTakeFromTrack)
{
	// Identity transform for cases not handled below.
	for (auto t = 0; t < MAX_NOTE_TRACKS; ++t)
		iTakeFromTrack[t] = t;

	switch (tt) {
		case NoteDataUtil::left:
		case NoteDataUtil::right:
			// Is there a way to do this without handling each StepsType? -Chris
			switch (st) {
				case StepsType_dance_single:
				case StepsType_dance_double:
					iTakeFromTrack[0] = 2;
					iTakeFromTrack[1] = 0;
					iTakeFromTrack[2] = 3;
					iTakeFromTrack[3] = 1;
					iTakeFromTrack[4] = 6;
					iTakeFromTrack[5] = 4;
					iTakeFromTrack[6] = 7;
					iTakeFromTrack[7] = 5;
					break;
				case StepsType_dance_solo:
					iTakeFromTrack[0] = 5;
					iTakeFromTrack[1] = 4;
					iTakeFromTrack[2] = 0;
					iTakeFromTrack[3] = 3;
					iTakeFromTrack[4] = 1;
					iTakeFromTrack[5] = 2;
					break;
				case StepsType_pump_single:
					iTakeFromTrack[0] = 1;
					iTakeFromTrack[1] = 3;
					iTakeFromTrack[2] = 2;
					iTakeFromTrack[3] = 4;
					iTakeFromTrack[4] = 0;
					iTakeFromTrack[5] = 6;
					iTakeFromTrack[6] = 8;
					iTakeFromTrack[7] = 7;
					iTakeFromTrack[8] = 9;
					iTakeFromTrack[9] = 5;
					break;
				case StepsType_pump_halfdouble:
					iTakeFromTrack[0] = 2;
					iTakeFromTrack[1] = 0;
					iTakeFromTrack[2] = 1;
					iTakeFromTrack[3] = 3;
					iTakeFromTrack[4] = 4;
					iTakeFromTrack[5] = 5;
					break;
				case StepsType_pump_double:
					iTakeFromTrack[0] = 8;
					iTakeFromTrack[1] = 9;
					iTakeFromTrack[2] = 7;
					iTakeFromTrack[3] = 5;
					iTakeFromTrack[4] = 6;
					iTakeFromTrack[5] = 3;
					iTakeFromTrack[6] = 4;
					iTakeFromTrack[7] = 2;
					iTakeFromTrack[8] = 0;
					iTakeFromTrack[9] = 1;
					break;
				default:
					break;
			}

			if (tt == NoteDataUtil::right) {
				// Invert.
				int iTrack[MAX_NOTE_TRACKS];
				memcpy(iTrack, iTakeFromTrack, sizeof iTrack);
				for (auto t = 0; t < MAX_NOTE_TRACKS; ++t) {
					const auto to = iTrack[t];
					iTakeFromTrack[to] = t;
				}
			}

			break;
		case NoteDataUtil::backwards: {
			// If a Pump game type, treat differently. Otherwise, send to
			// mirror.
			auto needsBackwards = true;
			switch (st) {
				case StepsType_pump_single: {
					iTakeFromTrack[0] = 3;
					iTakeFromTrack[1] = 4;
					iTakeFromTrack[2] = 2;
					iTakeFromTrack[3] = 0;
					iTakeFromTrack[4] = 1;
					iTakeFromTrack[5] = 8;
					iTakeFromTrack[6] = 9;
					iTakeFromTrack[7] = 2;
					iTakeFromTrack[8] = 5;
					iTakeFromTrack[9] = 6;
					break;
				}
				case StepsType_pump_double: {
					iTakeFromTrack[0] = 8;
					iTakeFromTrack[1] = 9;
					iTakeFromTrack[2] = 7;
					iTakeFromTrack[3] = 5;
					iTakeFromTrack[4] = 6;
					iTakeFromTrack[5] = 3;
					iTakeFromTrack[6] = 4;
					iTakeFromTrack[7] = 2;
					iTakeFromTrack[8] = 0;
					iTakeFromTrack[9] = 1;
					break;
				}
				case StepsType_pump_halfdouble: {
					iTakeFromTrack[0] = 5;
					iTakeFromTrack[1] = 3;
					iTakeFromTrack[2] = 4;
					iTakeFromTrack[3] = 1;
					iTakeFromTrack[4] = 2;
					iTakeFromTrack[5] = 0;
					break;
				}
				case StepsType_beat_single5: {
					// scratch is on right (cols 5 and 11) for 5-key
					iTakeFromTrack[0] = 4;
					iTakeFromTrack[1] = 3;
					iTakeFromTrack[2] = 2;
					iTakeFromTrack[3] = 1;
					iTakeFromTrack[4] = 0;
					iTakeFromTrack[5] = 5;
					iTakeFromTrack[6] = 10;
					iTakeFromTrack[7] = 9;
					iTakeFromTrack[8] = 8;
					iTakeFromTrack[9] = 7;
					iTakeFromTrack[10] = 6;
					iTakeFromTrack[11] = 11;
					break;
				}
				case StepsType_beat_single7: {
					// scratch is on left (cols 0 and 8) for 7-key
					iTakeFromTrack[0] = 0;
					iTakeFromTrack[1] = 7;
					iTakeFromTrack[2] = 6;
					iTakeFromTrack[3] = 5;
					iTakeFromTrack[4] = 4;
					iTakeFromTrack[5] = 3;
					iTakeFromTrack[6] = 2;
					iTakeFromTrack[7] = 1;
					iTakeFromTrack[8] = 8;
					iTakeFromTrack[9] = 15;
					iTakeFromTrack[10] = 14;
					iTakeFromTrack[11] = 13;
					iTakeFromTrack[12] = 12;
					iTakeFromTrack[13] = 11;
					iTakeFromTrack[14] = 10;
					iTakeFromTrack[15] = 9;
					break;
				}
				default:
					needsBackwards = false;
			}
			if (needsBackwards)
				break;
		}
		case NoteDataUtil::mirror: {
			for (auto t = 0; t < NumTracks; t++)
				iTakeFromTrack[t] = NumTracks - t - 1;
			break;
		}
		case NoteDataUtil::hran_shuffle:
		case NoteDataUtil::shuffle:
		case NoteDataUtil::super_shuffle: // use shuffle code to mix up
										  // HoldNotes without creating
										  // impossible patterns
		{
			// TRICKY: Shuffle so that both player get the same shuffle mapping
			// in the same round. This is already achieved in beat mode.
			int iOrig[MAX_NOTE_TRACKS];
			memcpy(iOrig, iTakeFromTrack, sizeof iOrig);

			auto f = [&iTakeFromTrack, &st, &NumTracks]() {
				RandomGen rnd(GAMESTATE->m_iStageSeed);
				// ignore turntable in beat mode
				switch (st) {
					case StepsType_beat_single5: {
						std::shuffle(
						  &iTakeFromTrack[0], &iTakeFromTrack[5], rnd);
						std::shuffle(
						  &iTakeFromTrack[6], &iTakeFromTrack[11], rnd);
						break;
					}
					case StepsType_beat_single7: {
						std::shuffle(
						  &iTakeFromTrack[1], &iTakeFromTrack[8], rnd);
						std::shuffle(
						  &iTakeFromTrack[9], &iTakeFromTrack[16], rnd);
						break;
					}
					default: {
						std::shuffle(
						  &iTakeFromTrack[0], &iTakeFromTrack[NumTracks], rnd);
						break;
					}
				}
			};

			f();
			// shuffle again if shuffle managed to
			// shuffle them in the same order
			while (!memcmp(iOrig, iTakeFromTrack, sizeof iOrig)) {
				GAMESTATE->SetNewStageSeed();
				f();
			}

		} break;
		case NoteDataUtil::soft_shuffle: {
			// soft shuffle, as described at
			// http://www.stepmania.com/forums/showthread.php?t=19469

			/* one of the following at random:
			 *
			 * 0. No columns changed
			 * 1. Left and right columns swapped
			 * 2. Down and up columns swapped
			 * 3. Mirror (left and right swapped, down and up swapped)
			 * ----------------------------------------------------------------
			 * To extend it to handle all game types, it would pick each axis
			 * of symmetry the game type has and either flip it or not flip it.
			 *
			 * For instance, PIU singles has four axes:
			 * horizontal, vertical,
			 * diagonally top left to bottom right,
			 * diagonally bottom left to top right.
			 * (above text from forums) */

			RandomGen rnd(GAMESTATE->m_iStageSeed);
			const int iRandChoice = rnd() % 4;

			// XXX: cases 1 and 2 only implemented for dance_*
			switch (iRandChoice) {
				case 1: // left and right mirror
				case 2: // up and down mirror
					switch (st) {
						case StepsType_dance_single:
							if (iRandChoice == 1) {
								// left and right
								iTakeFromTrack[0] = 3;
								iTakeFromTrack[3] = 0;
							}
							if (iRandChoice == 2) {
								// up and down
								iTakeFromTrack[1] = 2;
								iTakeFromTrack[2] = 1;
							}
							break;
						case StepsType_dance_double:
							if (iRandChoice == 1) {
								// left and right
								iTakeFromTrack[0] = 3;
								iTakeFromTrack[3] = 0;
								iTakeFromTrack[4] = 7;
								iTakeFromTrack[7] = 4;
							}
							if (iRandChoice == 2) {
								// up and down
								iTakeFromTrack[1] = 2;
								iTakeFromTrack[2] = 1;
								iTakeFromTrack[5] = 6;
								iTakeFromTrack[6] = 5;
							}
							break;
						// here be dragons (unchanged code)
						case StepsType_dance_solo:
							iTakeFromTrack[0] = 5;
							iTakeFromTrack[1] = 4;
							iTakeFromTrack[2] = 0;
							iTakeFromTrack[3] = 3;
							iTakeFromTrack[4] = 1;
							iTakeFromTrack[5] = 2;
							break;
						case StepsType_pump_single:
							iTakeFromTrack[0] = 3;
							iTakeFromTrack[1] = 4;
							iTakeFromTrack[2] = 2;
							iTakeFromTrack[3] = 0;
							iTakeFromTrack[4] = 1;
							iTakeFromTrack[5] = 8;
							iTakeFromTrack[6] = 9;
							iTakeFromTrack[7] = 7;
							iTakeFromTrack[8] = 5;
							iTakeFromTrack[9] = 6;
							break;
						case StepsType_pump_halfdouble:
							iTakeFromTrack[0] = 2;
							iTakeFromTrack[1] = 0;
							iTakeFromTrack[2] = 1;
							iTakeFromTrack[3] = 3;
							iTakeFromTrack[4] = 4;
							iTakeFromTrack[5] = 5;
							break;
						case StepsType_pump_double:
							iTakeFromTrack[0] = 8;
							iTakeFromTrack[1] = 9;
							iTakeFromTrack[2] = 7;
							iTakeFromTrack[3] = 5;
							iTakeFromTrack[4] = 6;
							iTakeFromTrack[5] = 3;
							iTakeFromTrack[6] = 4;
							iTakeFromTrack[7] = 2;
							iTakeFromTrack[8] = 0;
							iTakeFromTrack[9] = 1;
							break;
						default:
							break;
					}
					break;
				case 3: // full mirror
					GetTrackMapping(
					  st, NoteDataUtil::mirror, NumTracks, iTakeFromTrack);
					break;
				case 0:
				default:
					// case 0 and default are set by identity matrix above
					break;
			}
		} break;
		case NoteDataUtil::stomp:
			switch (st) {
				case StepsType_dance_single:
					iTakeFromTrack[0] = 3;
					iTakeFromTrack[1] = 2;
					iTakeFromTrack[2] = 1;
					iTakeFromTrack[3] = 0;
					iTakeFromTrack[4] = 7;
					iTakeFromTrack[5] = 6;
					iTakeFromTrack[6] = 5;
					iTakeFromTrack[7] = 4;
					break;
				case StepsType_dance_double:
					iTakeFromTrack[0] = 1;
					iTakeFromTrack[1] = 0;
					iTakeFromTrack[2] = 3;
					iTakeFromTrack[3] = 2;
					iTakeFromTrack[4] = 5;
					iTakeFromTrack[5] = 4;
					iTakeFromTrack[6] = 7;
					iTakeFromTrack[7] = 6;
					break;
				default:
					break;
			}
			break;
		case NoteDataUtil::swap_up_down:
			switch (st) {
				case StepsType_dance_single:
				case StepsType_dance_double:
					iTakeFromTrack[0] = 0;
					iTakeFromTrack[1] = 2;
					iTakeFromTrack[2] = 1;
					iTakeFromTrack[3] = 3;
					iTakeFromTrack[4] = 4;
					iTakeFromTrack[5] = 6;
					iTakeFromTrack[6] = 5;
					iTakeFromTrack[7] = 7;
					break;
				case StepsType_pump_single:
				case StepsType_pump_double:
					iTakeFromTrack[0] = 1;
					iTakeFromTrack[1] = 0;
					iTakeFromTrack[2] = 2;
					iTakeFromTrack[3] = 4;
					iTakeFromTrack[4] = 3;
					iTakeFromTrack[5] = 6;
					iTakeFromTrack[6] = 5;
					iTakeFromTrack[7] = 7;
					iTakeFromTrack[8] = 9;
					iTakeFromTrack[9] = 8;
					break;
				case StepsType_pump_halfdouble:
					iTakeFromTrack[0] = 0;
					iTakeFromTrack[1] = 2;
					iTakeFromTrack[2] = 1;
					iTakeFromTrack[3] = 4;
					iTakeFromTrack[4] = 3;
					iTakeFromTrack[5] = 5;
					break;
				default:
					break;
			}
			break;
		default:
			ASSERT(0);
	}
}

static void
HRanShuffleTaps(NoteData& input, int startIndex, int endIndex)
{
	// shuffle like supershuffle but do it in a way that doesnt create new jacks
	// - still allow jacks if forced to
	const auto columns = input.GetNumTracks();

	RandomGen rnd(GAMESTATE->m_iStageSeed);

	auto shfl = [&rnd](std::vector<int>& stuff) {
		std::shuffle(stuff.begin(), stuff.end(), rnd);
	};

	std::set<int> last_row_cols;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE_REVERSE(input, row, startIndex, endIndex) {

		std::set<int> garbage_columns;
		std::set<int> note_columns;

		for (auto c = 0; c < columns; c++) {
			const auto& t = input.GetTapNote(c, row);
			switch (t.type) {
				case TapNoteType_Empty:
					// nothing is here
					break;
				case TapNoteType_HoldHead:
				case TapNoteType_HoldTail:
				case TapNoteType_AutoKeysound:
					// garbage
					garbage_columns.insert(c);
					break;
				case TapNoteType_Tap:
				case TapNoteType_Mine:
				case TapNoteType_Lift:
				case TapNoteType_Fake:
					// something is here
					note_columns.insert(c);
					break;
				default:
					FAIL_M(
					  "Unexpected TapNoteType when processing HRanShuffleTaps");
					break;
			}

			if (input.IsHoldNoteAtRow(c, row))
				garbage_columns.insert(c);
		}

		// the step before running this function already applied a soft shuffle
		// the columns are already shuffled, so we can stop here
		// there is no way to avoid a jack if every row has a note
		if (note_columns.size() == columns) {
			last_row_cols.clear();
			for (auto i = 0; i < columns; i++) {
				last_row_cols.insert(i);
			}
			continue;
		}

		std::vector<int> column_choices;
		// if this row has a valid permutation that fits against the previous row
		// (also cant be first row)
		if ((last_row_cols.size() > 0) &&
			static_cast<int>(note_columns.size()) <=
			  static_cast<int>(
				columns - (last_row_cols.size() + garbage_columns.size()))) {
			// then we can avoid forming a jack. do that
			for (int i = 0; i < columns; i++) {
				if (last_row_cols.find(i) == last_row_cols.end() &&
					garbage_columns.find(i) == garbage_columns.end()) {
					column_choices.push_back(i);
				}
			}
		} else {
			// otherwise a jack is forced to form. dont care
			// just shuffle randomly
			for (int i = 0; i < columns; i++) {
				if (garbage_columns.find(i) == garbage_columns.end()) {
					column_choices.push_back(i);
				}
			}
		}

		ASSERT(column_choices.size() >= note_columns.size());

		std::unordered_map<int, TapNote> rownotes;
		for (auto c : note_columns) {
			rownotes.emplace(c, input.GetTapNote(c, row));

			auto iter = input.FindTapNote(c, row);
			if (iter != input.end(c))
				input.RemoveTapNote(c, iter);
		}

		shfl(column_choices);
		std::set<int> new_columns;
		for (auto c : note_columns) {
			auto c2 = column_choices.back();
			column_choices.pop_back();
			new_columns.insert(c2);
			const auto& tn1 = rownotes.at(c);

			const auto tmp = tn1;
			input.SetTapNote(c2, row, tn1);
		}
		for (auto c : garbage_columns) {
			new_columns.insert(c);
		}
		last_row_cols = new_columns;
	}

}

static void
SuperShuffleTaps(NoteData& inout, int iStartIndex, int iEndIndex)
{
	/*
	 * We already did the normal shuffling code above, which did a good job
	 * of shuffling HoldNotes without creating impossible patterns.
	 * Now, go in and shuffle the TapNotes per-row.
	 *
	 * This is only called by NoteDataUtil::Turn.
	 */

	RandomGen rnd(GAMESTATE->m_iStageSeed);

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		std::vector<int> doot(inout.GetNumTracks());
		iota(std::begin(doot), std::end(doot), 0);

		std::shuffle(doot.begin(), doot.end(), rnd);
		for (auto tdoot = 0; tdoot < inout.GetNumTracks(); tdoot++) {
			const auto t1 = doot[tdoot];
			const auto& tn1 = inout.GetTapNote(t1, r);
			switch (tn1.type) {
				case TapNoteType_Empty:
				case TapNoteType_HoldHead:
				case TapNoteType_HoldTail:
				case TapNoteType_AutoKeysound:
					continue; // skip
				case TapNoteType_Tap:
				case TapNoteType_Mine:
				case TapNoteType_Lift:
				case TapNoteType_Fake:
					break; // shuffle this
					DEFAULT_FAIL(tn1.type);
			}

			DEBUG_ASSERT_M(
			  !inout.IsHoldNoteAtRow(t1, r),
			  ssprintf("There is a tap.type = %d inside of a hold at row %d",
					   tn1.type,
					   r));

			// Probe for a spot to swap with.
			set<int> vTriedTracks;
			for (auto i = 0; i < 4; i++) // probe max 4 times
			{
				auto t2 = RandomInt(rnd, inout.GetNumTracks());
				if (vTriedTracks.find(t2) !=
					vTriedTracks.end()) // already tried this track
					continue;			// skip
				vTriedTracks.insert(t2);

				// swapping with ourself is a no-op
				if (t1 == t2)
					break; // done swapping

				const auto& tn2 = inout.GetTapNote(t2, r);
				switch (tn2.type) {
					case TapNoteType_HoldHead:
					case TapNoteType_HoldTail:
					case TapNoteType_AutoKeysound:
						continue; // don't swap with these
					case TapNoteType_Empty:
					case TapNoteType_Tap:
					case TapNoteType_Mine:
					case TapNoteType_Lift:
					case TapNoteType_Fake:
						break; // ok to swap with this
						DEFAULT_FAIL(tn2.type);
				}

				// don't swap into the middle of a hold note
				if (inout.IsHoldNoteAtRow(t2, r))
					continue;

				// do the swap
				const auto tnTemp = tn1;
				inout.SetTapNote(t1, r, tn2);
				inout.SetTapNote(t2, r, tnTemp);

				break; // done swapping
			}
		}
	}
}

void
NoteDataUtil::Turn(NoteData& inout,
				   StepsType st,
				   TrackMapping tt,
				   int iStartIndex,
				   int iEndIndex)
{
	int iTakeFromTrack[MAX_NOTE_TRACKS]; // New track "t" will take from old
										 // track iTakeFromTrack[t]
	GetTrackMapping(st, tt, inout.GetNumTracks(), iTakeFromTrack);

	NoteData tempNoteData;
	tempNoteData.LoadTransformed(inout, inout.GetNumTracks(), iTakeFromTrack);

	switch (tt) {
		case super_shuffle:
			SuperShuffleTaps(tempNoteData, iStartIndex, iEndIndex);
			break;
		case hran_shuffle:
			HRanShuffleTaps(tempNoteData, iStartIndex, iEndIndex);
			break;
		default:
			break;
	}

	inout.CopyAll(tempNoteData);
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Backwards(NoteData& inout)
{
	NoteData out;
	out.SetNumTracks(inout.GetNumTracks());

	const auto max_row = inout.GetLastRow();
	for (auto t = 0; t < inout.GetNumTracks(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(inout, t, r, 0, max_row)
		{
			auto iRowEarlier = r;
			auto iRowLater = max_row - r;

			const auto& tnEarlier = inout.GetTapNote(t, iRowEarlier);
			if (tnEarlier.type == TapNoteType_HoldHead)
				iRowLater -= tnEarlier.iDuration;

			out.SetTapNote(t, iRowLater, tnEarlier);
		}
	}

	inout.swap(out);
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::SwapSides(NoteData& inout)
{
	int iOriginalTrackToTakeFrom[MAX_NOTE_TRACKS];
	for (auto i = 0; i < MAX_NOTE_TRACKS; ++i)
		iOriginalTrackToTakeFrom[i] = i;
	for (auto t = 0; t < inout.GetNumTracks() / 2; ++t) {
		const auto iTrackEarlier = t;
		const auto iTrackLater =
		  t + inout.GetNumTracks() / 2 + inout.GetNumTracks() % 2;
		iOriginalTrackToTakeFrom[iTrackEarlier] = iTrackLater;
		iOriginalTrackToTakeFrom[iTrackLater] = iTrackEarlier;
	}

	const auto orig(inout);
	inout.LoadTransformed(orig, orig.GetNumTracks(), iOriginalTrackToTakeFrom);
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Little(NoteData& inout, int iStartIndex, int iEndIndex)
{
	// filter out all non-quarter notes
	for (auto t = 0; t < inout.GetNumTracks(); t++) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(inout, t, i, iStartIndex, iEndIndex)
		{
			if (i % ROWS_PER_BEAT == 0)
				continue;
			inout.SetTapNote(t, i, TAP_EMPTY);
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

// Make all quarter notes into jumps.
void
NoteDataUtil::Wide(NoteData& inout, int iStartIndex, int iEndIndex)
{
	/* Start on an even beat. */
	iStartIndex = Quantize(iStartIndex, BeatToNoteRow(2.0f));

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, i, iStartIndex, iEndIndex)
	{
		if (i % BeatToNoteRow(2.0f) != 0)
			continue; // even beats only

		auto bHoldNoteAtBeat = false;
		for (auto t = 0; !bHoldNoteAtBeat && t < inout.GetNumTracks(); ++t)
			if (inout.IsHoldNoteAtRow(t, i))
				bHoldNoteAtBeat = true;
		if (bHoldNoteAtBeat)
			continue; // skip.  Don't place during holds

		if (inout.GetNumTracksWithTap(i) != 1)
			continue; // skip

		auto bSpaceAroundIsEmpty =
		  true; // no other notes with a 1/8th of this row
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
		  inout, j, i - ROWS_PER_BEAT / 2 + 1, i + ROWS_PER_BEAT / 2)
		if (j != i && inout.GetNumTapNonEmptyTracks(j) > 0) {
			bSpaceAroundIsEmpty = false;
			break;
		}

		if (!bSpaceAroundIsEmpty)
			continue; // skip

		// add a note determinitsitcally
		const int iBeat = lround(NoteRowToBeat(i));
		const auto iTrackOfNote = inout.GetFirstTrackWithTap(i);
		auto iTrackToAdd =
		  iTrackOfNote + iBeat % 5 -
		  2; // won't be more than 2 tracks away from the existing note
		CLAMP(iTrackToAdd, 0, inout.GetNumTracks() - 1);
		if (iTrackToAdd == iTrackOfNote)
			iTrackToAdd++;
		CLAMP(iTrackToAdd, 0, inout.GetNumTracks() - 1);
		if (iTrackToAdd == iTrackOfNote)
			iTrackToAdd--;
		CLAMP(iTrackToAdd, 0, inout.GetNumTracks() - 1);

		if (inout.GetTapNote(iTrackToAdd, i).type != TapNoteType_Empty &&
			inout.GetTapNote(iTrackToAdd, i).type != TapNoteType_Fake) {
			iTrackToAdd = (iTrackToAdd + 1) % inout.GetNumTracks();
		}
		inout.SetTapNote(iTrackToAdd, i, TAP_ADDITION_TAP);
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Big(NoteData& inout, int iStartIndex, int iEndIndex)
{
	// add 8ths between 4ths
	InsertIntelligentTaps(inout,
						  BeatToNoteRow(1.0f),
						  BeatToNoteRow(0.5f),
						  BeatToNoteRow(1.0f),
						  false,
						  iStartIndex,
						  iEndIndex);
}

void
NoteDataUtil::Quick(NoteData& inout, int iStartIndex, int iEndIndex)
{
	// add 16ths between 8ths
	InsertIntelligentTaps(inout,
						  BeatToNoteRow(0.5f),
						  BeatToNoteRow(0.25f),
						  BeatToNoteRow(1.0f),
						  false,
						  iStartIndex,
						  iEndIndex);
}

// Due to popular request by people annoyed with the "new" implementation of
// Quick, we now have this BMR-izer for your steps.  Use with caution.
void
NoteDataUtil::BMRize(NoteData& inout, int iStartIndex, int iEndIndex)
{
	Big(inout, iStartIndex, iEndIndex);
	Quick(inout, iStartIndex, iEndIndex);
}

void
NoteDataUtil::Skippy(NoteData& inout, int iStartIndex, int iEndIndex)
{
	// add 16ths between 4ths
	InsertIntelligentTaps(inout,
						  BeatToNoteRow(1.0f),
						  BeatToNoteRow(0.75f),
						  BeatToNoteRow(1.0f),
						  true,
						  iStartIndex,
						  iEndIndex);
}

void
NoteDataUtil::InsertIntelligentTaps(NoteData& inout,
									int iWindowSizeRows,
									int iInsertOffsetRows,
									int iWindowStrideRows,
									bool bSkippy,
									int iStartIndex,
									int iEndIndex)
{
	ASSERT(iInsertOffsetRows <= iWindowSizeRows);
	ASSERT(iWindowSizeRows <= iWindowStrideRows);

	const auto bRequireNoteAtBeginningOfWindow = !bSkippy;
	const auto bRequireNoteAtEndOfWindow = true;

	/* Start on a multiple of fBeatInterval. */
	iStartIndex = Quantize(iStartIndex, iWindowStrideRows);

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, i, iStartIndex, iEndIndex)
	{
		// Insert a beat in the middle of every fBeatInterval.
		if (i % iWindowStrideRows != 0)
			continue; // even beats only

		const auto iRowEarlier = i;
		const auto iRowLater = i + iWindowSizeRows;
		const auto iRowToAdd = i + iInsertOffsetRows;
		// following two lines have been changed because the behavior of
		// treating hold-heads as different from taps doesn't feel right, and
		// because we need to check against TAP_ADDITION with the BMRize mod.
		if (bRequireNoteAtBeginningOfWindow)
			if (inout.GetNumTapNonEmptyTracks(iRowEarlier) != 1 ||
				inout.GetNumTracksWithTapOrHoldHead(iRowEarlier) != 1)
				continue;
		if (bRequireNoteAtEndOfWindow)
			if (inout.GetNumTapNonEmptyTracks(iRowLater) != 1 ||
				inout.GetNumTracksWithTapOrHoldHead(iRowLater) != 1)
				continue;
		// there is a 4th and 8th note surrounding iRowBetween

		// don't insert a new note if there's already one within this interval
		auto bNoteInMiddle = false;
		for (auto t = 0; t < inout.GetNumTracks(); ++t)
			if (inout.IsHoldNoteAtRow(t, iRowEarlier + 1))
				bNoteInMiddle = true;
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
		  inout, j, iRowEarlier + 1, iRowLater - 1)
		bNoteInMiddle = true;
		if (bNoteInMiddle)
			continue;

		// add a note deterministically somewhere on a track different from the
		// two surrounding notes
		auto iTrackOfNoteEarlier = -1;
		const auto bEarlierHasNonEmptyTrack =
		  inout.GetTapFirstNonEmptyTrack(iRowEarlier, iTrackOfNoteEarlier);
		auto iTrackOfNoteLater = -1;
		inout.GetTapFirstNonEmptyTrack(iRowLater, iTrackOfNoteLater);
		auto iTrackOfNoteToAdd = 0;
		if (bSkippy &&
			iTrackOfNoteEarlier !=
			  iTrackOfNoteLater && // Don't make skips on the same note
			bEarlierHasNonEmptyTrack) {
			iTrackOfNoteToAdd = iTrackOfNoteEarlier;
		} else if (abs(iTrackOfNoteEarlier - iTrackOfNoteLater) >= 2) {
			// try to choose a track between the earlier and later notes
			iTrackOfNoteToAdd =
			  std::min(iTrackOfNoteEarlier, iTrackOfNoteLater) + 1;
		} else if (std::min(iTrackOfNoteEarlier, iTrackOfNoteLater) - 1 >= 0) {
			// try to choose a track just to the left
			iTrackOfNoteToAdd =
			  std::min(iTrackOfNoteEarlier, iTrackOfNoteLater) - 1;
		} else if (std::max(iTrackOfNoteEarlier, iTrackOfNoteLater) + 1 <
				   inout.GetNumTracks()) {
			// try to choose a track just to the right
			iTrackOfNoteToAdd =
			  std::max(iTrackOfNoteEarlier, iTrackOfNoteLater) + 1;
		}

		inout.SetTapNote(iTrackOfNoteToAdd, iRowToAdd, TAP_ADDITION_TAP);
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}
#if 0
class TrackIterator
{
public:
	TrackIterator();

	/* If called, iterate only over [iStart,iEnd]. */
	void SetRange( int iStart, int iEnd )
	{
	}

	/* If called, pay attention to iTrack only. */
	void SetTrack( iTrack );

	/* Extend iStart and iEnd to include hold notes overlapping the boundaries.  Call SetRange()
	 * and SetTrack() first. */
	void HoldInclusive();

	/* Reduce iStart and iEnd to exclude hold notes overlapping the boundaries.  Call SetRange()
	 * and SetTrack() first. */
	void HoldExclusive();

	/* If called, keep the iterator around.  This results in much faster iteration.  If used,
	 * ensure that the current row will always remain valid.  SetTrack() must be called first. */
	void Fast();

	/* Retrieve an iterator for the current row.  SetTrack() must be called first (but Fast()
	 * does not). */
	TapNote::iterator Get();

	int GetRow() const { return m_iCurrentRow; }
	bool Prev();
	bool Next();

private:
	int m_iStart, m_iEnd;
	int m_iTrack;

	bool m_bFast;

	int m_iCurrentRow;

	NoteData::iterator m_Iterator;

	/* m_bFast only: */
	NoteData::iterator m_Begin, m_End;
};

bool TrackIterator::Next()
{
	if( m_bFast )
	{
		if( m_Iterator == XXX )
			;

	}

}

TrackIterator::TrackIterator()
{
	m_iStart = 0;
	m_iEnd = MAX_NOTE_ROW;
	m_iTrack = -1;
}
#endif

void
NoteDataUtil::AddMines(NoteData& inout, int iStartIndex, int iEndIndex)
{
	// Change whole rows at a time to be tap notes.  Otherwise, it causes
	// major problems for our scoring system. -Chris

	auto iRowCount = 0;
	auto iPlaceEveryRows = 6;
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		iRowCount++;

		// place every 6 or 7 rows
		// XXX: What is "6 or 7" derived from?  Can we calculate that in a way
		// that won't break if ROWS_PER_MEASURE changes?
		if (iRowCount >= iPlaceEveryRows) {
			for (auto t = 0; t < inout.GetNumTracks(); t++)
				if (inout.GetTapNote(t, r).type == TapNoteType_Tap)
					inout.SetTapNote(t, r, TAP_ADDITION_MINE);

			iRowCount = 0;
			if (iPlaceEveryRows == 6)
				iPlaceEveryRows = 7;
			else
				iPlaceEveryRows = 6;
		}
	}

	// Place mines right after hold so player must lift their foot.
	for (auto iTrack = 0; iTrack < inout.GetNumTracks(); ++iTrack) {
		FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(
		  inout, iTrack, r, iStartIndex, iEndIndex)
		{
			const auto& tn = inout.GetTapNote(iTrack, r);
			if (tn.type != TapNoteType_HoldHead)
				continue;

			auto iMineRow = r + tn.iDuration + BeatToNoteRow(0.5f);
			if (iMineRow < iStartIndex || iMineRow > iEndIndex)
				continue;

			// Only place a mines if there's not another step nearby
			const auto iMineRangeBegin = iMineRow - BeatToNoteRow(0.5f) + 1;
			const auto iMineRangeEnd = iMineRow + BeatToNoteRow(0.5f) - 1;
			if (!inout.IsRangeEmpty(iTrack, iMineRangeBegin, iMineRangeEnd))
				continue;

			// Add a mine right after the hold end.
			inout.SetTapNote(iTrack, iMineRow, TAP_ADDITION_MINE);

			// Convert all notes in this row to mines.
			for (auto t = 0; t < inout.GetNumTracks(); t++)
				if (inout.GetTapNote(t, iMineRow).type == TapNoteType_Tap)
					inout.SetTapNote(t, iMineRow, TAP_ADDITION_MINE);

			iRowCount = 0;
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Echo(NoteData& inout, int iStartIndex, int iEndIndex)
{
	// add 8th note tap "echos" after all taps
	auto iEchoTrack = -1;

	const auto rows_per_interval = BeatToNoteRow(0.5f);
	iStartIndex = Quantize(iStartIndex, rows_per_interval);

	/* Clamp iEndIndex to the last real tap note.  Otherwise, we'll keep adding
	 * echos of our echos all the way up to MAX_TAP_ROW. */
	iEndIndex = std::min(iEndIndex, inout.GetLastRow()) + 1;

	// window is one beat wide and slides 1/2 a beat at a time
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		if (r % rows_per_interval != 0)
			continue; // 8th notes only

		const auto iRowWindowBegin = r;
		const auto iRowWindowEnd = r + rows_per_interval * 2;

		const auto iFirstTapInRow = inout.GetFirstTrackWithTap(iRowWindowBegin);
		if (iFirstTapInRow != -1)
			iEchoTrack = iFirstTapInRow;

		if (iEchoTrack == -1)
			continue; // don't lay

		// don't insert a new note if there's already a tap within this interval
		auto bTapInMiddle = false;
		FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
		  inout, r2, iRowWindowBegin + 1, iRowWindowEnd - 1)
		bTapInMiddle = true;
		if (bTapInMiddle)
			continue; // don't lay

		const auto iRowEcho = r + rows_per_interval;
		{
			set<int> viTracks;
			inout.GetTracksHeldAtRow(iRowEcho, viTracks);

			// don't lay if holding 2 already
			if (viTracks.size() >= 2)
				continue; // don't lay

			// don't lay echos on top of a HoldNote
			if (find(viTracks.begin(), viTracks.end(), iEchoTrack) !=
				viTracks.end())
				continue; // don't lay
		}

		inout.SetTapNote(iEchoTrack, iRowEcho, TAP_ADDITION_TAP);
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Planted(NoteData& inout, int iStartIndex, int iEndIndex)
{
	ConvertTapsToHolds(inout, 1, iStartIndex, iEndIndex);
}
void
NoteDataUtil::Floored(NoteData& inout, int iStartIndex, int iEndIndex)
{
	ConvertTapsToHolds(inout, 2, iStartIndex, iEndIndex);
}
void
NoteDataUtil::Twister(NoteData& inout, int iStartIndex, int iEndIndex)
{
	ConvertTapsToHolds(inout, 3, iStartIndex, iEndIndex);
}
void
NoteDataUtil::ConvertTapsToHolds(NoteData& inout,
								 int iSimultaneousHolds,
								 int iStartIndex,
								 int iEndIndex)
{
	// Convert all taps to freezes.
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		auto iTrackAddedThisRow = 0;
		for (auto t = 0; t < inout.GetNumTracks(); t++) {
			if (iTrackAddedThisRow > iSimultaneousHolds)
				break;

			if (inout.GetTapNote(t, r).type == TapNoteType_Tap) {
				// Find the ending row for this hold
				auto iTapsLeft = iSimultaneousHolds;

				auto r2 = r + 1;
				auto addHold = true;
				FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
				  inout, next_row, r + 1, iEndIndex)
				{
					r2 = next_row;

					// If there are two taps in a row on the same track,
					// don't convert the earlier one to a hold.
					if (inout.GetTapNote(t, r2).type != TapNoteType_Empty) {
						addHold = false;
						break;
					}

					set<int> tracksDown;
					inout.GetTracksHeldAtRow(r2, tracksDown);
					inout.GetTapNonEmptyTracks(r2, tracksDown);
					iTapsLeft -= tracksDown.size();
					if (iTapsLeft == 0)
						break; // we found the ending row for this hold
					if (iTapsLeft < 0) {
						addHold = false;
						break;
					}
				}

				if (!addHold) {
					continue;
				}

				// If the steps end in a tap, convert that tap
				// to a hold that lasts for at least one beat.
				if (r2 == r + 1)
					r2 = r + BeatToNoteRow(1);

				inout.AddHoldNote(t, r, r2, TAP_ORIGINAL_HOLD_HEAD);
				iTrackAddedThisRow++;
			}
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

/* Need to redo parsing to handle multiple notes on a row
 skippping breaks it  (Jacks generated)
 due to searching left/right it generates a lot of Left or right arrows which
 can also be jump jacks need creative solution or actual math */
void
NoteDataUtil::IcyWorld(NoteData& inout,
					   StepsType st,
					   TimingData const& timing_data,
					   int iStartIndex,
					   int iEndIndex)
{
	// Row, tap column
	std::vector<tuple<int, int>> rowsWithNotes;
	auto currentTap = -1;

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		currentTap = inout.GetNumTracksWithTap(r);
		if (currentTap != 1)
			continue; // skip

		for (auto q = 0; q < 4; q++) {
			const auto getTap = inout.GetTapNote(q, r);
			if (getTap.type == TapNoteType_Tap) {
				currentTap = q;
				break;
			}
		}
		rowsWithNotes.emplace_back(tuple<int, int>(r, currentTap));
	}

	auto lastTap = -1;
	auto flipStartSide = false;
	auto skipLine = true;
	size_t i = 0;
	for (auto iterator : rowsWithNotes) {
		// Every second row with note, insert a note which doesn't collide with
		// the previous
		if (!skipLine) {
			const auto iNumTracksHeld =
			  inout.GetNumTracksHeldAtRow(std::get<0>(iterator));
			if (iNumTracksHeld >= 1) {
				i++;
				continue;
			}

			const auto tempI = i + 1 < rowsWithNotes.size() ? i + 1 : i;
			if (flipStartSide) {
				for (auto c = 3; c > -1; c--) {
					if (c != lastTap && c != std::get<1>(iterator) &&
						c != std::get<1>(rowsWithNotes.at(tempI))) {
						inout.SetTapNote(
						  c, std::get<0>(iterator), TAP_ADDITION_TAP);
						break;
					}
				}
			} else {
				for (auto c = 0; c < 4; c++) {
					if (c != lastTap && c != std::get<1>(iterator) &&
						c != std::get<1>(rowsWithNotes.at(tempI))) {
						inout.SetTapNote(
						  c, std::get<0>(iterator), TAP_ADDITION_TAP);
						break;
					}
				}
			}
			flipStartSide = !flipStartSide;
		} else {
			lastTap = std::get<1>(iterator);
		}
		skipLine = !skipLine;
		i++;
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::AnchorJS(NoteData& inout,
					   StepsType st,
					   TimingData const& timing_data,
					   int iStartIndex,
					   int iEndIndex)
{
	// Row, tap column
	std::vector<tuple<int, int>> rowsWithNotes;
	auto currentTap = -1;

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		currentTap = inout.GetNumTracksWithTap(r);
		if (currentTap != 1)
			continue; // skip

		for (auto q = 0; q < 4; q++) {
			const auto getTap = inout.GetTapNote(q, r);
			if (getTap.type == TapNoteType_Tap) {
				currentTap = q;
				break;
			}
		}
		rowsWithNotes.emplace_back(tuple<int, int>(r, currentTap));
	}

	auto lastTap = -1;
	auto flipStartSide = false;
	auto skipLine = true;
	size_t i = 0;
	for (auto iterator : rowsWithNotes) {
		// Every second row with note, insert a note which doesn't collide with
		// the previous
		if (!skipLine) {
			const auto iNumTracksHeld =
			  inout.GetNumTracksHeldAtRow(std::get<0>(iterator));
			if (iNumTracksHeld >= 1) {
				i++;
				continue;
			}

			const auto tempI = i + 1 < rowsWithNotes.size() ? i + 1 : i;
			if (flipStartSide) {
				for (auto c = 3; c > -1; c--) {
					if (c != lastTap && c != std::get<1>(iterator) &&
						c != std::get<1>(rowsWithNotes.at(tempI))) {
						inout.SetTapNote(
						  c, std::get<0>(iterator), TAP_ADDITION_TAP);
						break;
					}
				}
			} else {
				for (auto c = 0; c < 4; c++) {
					if (c != lastTap && c != std::get<1>(iterator) &&
						c != std::get<1>(rowsWithNotes.at(tempI))) {
						inout.SetTapNote(
						  c, std::get<0>(iterator), TAP_ADDITION_TAP);
						break;
					}
				}
			}
			flipStartSide = !flipStartSide;
		} else {
			lastTap = std::get<1>(iterator);
		}
		skipLine = !skipLine;
		i++;
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

/* Same as AnchorJS, but it doesn't check if the next row will generate a jack.
 This causes mini jacks to be formed, with JS. c: */
void
NoteDataUtil::JackJS(NoteData& inout,
					 StepsType st,
					 TimingData const& timing_data,
					 int iStartIndex,
					 int iEndIndex)
{
	// Row, tap column
	std::vector<tuple<int, int>> rowsWithNotes;
	auto currentTap = -1;

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		currentTap = inout.GetNumTracksWithTap(r);
		if (currentTap != 1)
			continue; // skip

		for (auto q = 0; q < 4; q++) {
			const auto getTap = inout.GetTapNote(q, r);
			if (getTap.type == TapNoteType_Tap) {
				currentTap = q;
				break;
			}
		}
		rowsWithNotes.emplace_back(tuple<int, int>(r, currentTap));
	}

	auto lastTap = -1;
	auto flipStartSide = false;
	auto skipLine = true;
	auto i = 0;
	for (auto iterator : rowsWithNotes) {
		// Every second row with note, insert a note which doesn't collide with
		// the previous
		if (!skipLine) {
			const auto iNumTracksHeld =
			  inout.GetNumTracksHeldAtRow(std::get<0>(iterator));
			if (iNumTracksHeld >= 1) {
				i++;
				continue;
			}

			if (flipStartSide) {
				for (auto c = 3; c > -1; c--) {
					if (c != lastTap && c != std::get<1>(iterator)) {
						inout.SetTapNote(
						  c, std::get<0>(iterator), TAP_ADDITION_TAP);
						break;
					}
				}
			} else {
				for (auto c = 0; c < 4; c++) {
					if (c != lastTap && c != std::get<1>(iterator)) {
						inout.SetTapNote(
						  c, std::get<0>(iterator), TAP_ADDITION_TAP);
						break;
					}
				}
			}
			flipStartSide = !flipStartSide;
		} else {
			lastTap = std::get<1>(iterator);
		}
		skipLine = !skipLine;
		i++;
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Stomp(NoteData& inout,
					StepsType st,
					int iStartIndex,
					int iEndIndex)
{
	// Make all non jumps with ample space around them into jumps.
	int iTrackMapping[MAX_NOTE_TRACKS];
	GetTrackMapping(st, stomp, inout.GetNumTracks(), iTrackMapping);

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		if (inout.GetNumTracksWithTap(r) != 1)
			continue; // skip

		for (auto t = 0; t < inout.GetNumTracks(); t++) {
			if (inout.GetTapNote(t, r).type ==
				TapNoteType_Tap) // there is a tap here
			{
				// Look to see if there is enough empty space on either side of
				// the note to turn this into a jump.
				const auto iRowWindowBegin = r - BeatToNoteRow(0.5f); // 0.5
				const auto iRowWindowEnd = r + BeatToNoteRow(0.5f);

				auto bTapInMiddle = false;
				FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
				  inout, r2, iRowWindowBegin + 1, iRowWindowEnd - 1)
				if (inout.IsThereATapAtRow(r2) &&
					r2 != r) // don't count the note we're looking around
				{
					bTapInMiddle = true;
					break;
				}
				if (bTapInMiddle)
					continue;

				// don't convert to jump if there's a hold here
				const auto iNumTracksHeld = inout.GetNumTracksHeldAtRow(r);
				if (iNumTracksHeld >= 1)
					continue;

				const auto iOppositeTrack = iTrackMapping[t];
				inout.SetTapNote(iOppositeTrack, r, TAP_ADDITION_TAP);
			}
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::SnapToNearestNoteType(NoteData& inout,
									NoteType nt1,
									NoteType nt2,
									int iStartIndex,
									int iEndIndex)
{
	// nt2 is optional and should be NoteType_Invalid if it is not used

	const auto fSnapInterval1 = NoteTypeToBeat(nt1);
	float fSnapInterval2 =
	  10000; // nothing will ever snap to this.  That's what we want!
	if (nt2 != NoteType_Invalid)
		fSnapInterval2 = NoteTypeToBeat(nt2);

	// iterate over all TapNotes in the interval and snap them
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(
	  inout, iOldIndex, iStartIndex, iEndIndex)
	{
		auto iNewIndex1 = Quantize(iOldIndex, BeatToNoteRow(fSnapInterval1));
		auto iNewIndex2 = Quantize(iOldIndex, BeatToNoteRow(fSnapInterval2));

		const auto bNewBeat1IsCloser =
		  abs(iNewIndex1 - iOldIndex) < abs(iNewIndex2 - iOldIndex);
		const auto iNewIndex = bNewBeat1IsCloser ? iNewIndex1 : iNewIndex2;

		for (auto c = 0; c < inout.GetNumTracks(); c++) {
			auto tnNew = inout.GetTapNote(c, iOldIndex);
			if (tnNew.type == TapNoteType_Empty)
				continue;

			inout.SetTapNote(c, iOldIndex, TAP_EMPTY);

			if (tnNew.type == TapNoteType_Tap &&
				inout.IsHoldNoteAtRow(c, iNewIndex))
				continue; // HoldNotes override TapNotes

			if (tnNew.type == TapNoteType_HoldHead) {
				/* Quantize the duration.  If the result is empty, just discard
				 * the hold. */
				tnNew.iDuration =
				  Quantize(tnNew.iDuration, BeatToNoteRow(fSnapInterval1));
				if (tnNew.iDuration == 0)
					continue;

				/* We might be moving a hold note downwards, or extending its
				 * duration downwards.  Make sure there isn't anything else in
				 * the new range. */
				inout.ClearRangeForTrack(
				  iNewIndex, iNewIndex + tnNew.iDuration + 1, c);
			}

			inout.SetTapNote(c, iNewIndex, tnNew);
		}
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::CopyLeftToRight(NoteData& inout)
{
	/* XXX
	inout.ConvertHoldNotesTo4s();
	for( int t=0; t<inout.GetNumTracks()/2; t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK( inout, t, r )
		{
			int iTrackEarlier = t;
			int iTrackLater = inout.GetNumTracks()-1-t;

			const TapNote &tnEarlier = inout.GetTapNote(iTrackEarlier, r);
			inout.SetTapNote(iTrackLater, r, tnEarlier);
		}
	}
	inout.Convert4sToHoldNotes();
*/
}

void
NoteDataUtil::CopyRightToLeft(NoteData& inout)
{
	/* XXX
	inout.ConvertHoldNotesTo4s();
	for( int t=0; t<inout.GetNumTracks()/2; t++ )
	{
		FOREACH_NONEMPTY_ROW_IN_TRACK( inout, t, r )
		{
			int iTrackEarlier = t;
			int iTrackLater = inout.GetNumTracks()-1-t;

			TapNote tnLater = inout.GetTapNote(iTrackLater, r);
			inout.SetTapNote(iTrackEarlier, r, tnLater);
		}
	}
	inout.Convert4sToHoldNotes();
*/
}

void
NoteDataUtil::ClearLeft(NoteData& inout)
{
	for (auto t = 0; t < inout.GetNumTracks() / 2; t++)
		inout.ClearRangeForTrack(0, MAX_NOTE_ROW, t);
}

void
NoteDataUtil::ClearRight(NoteData& inout)
{
	for (auto t = (inout.GetNumTracks() + 1) / 2; t < inout.GetNumTracks(); t++)
		inout.ClearRangeForTrack(0, MAX_NOTE_ROW, t);
}

void
NoteDataUtil::CollapseToOne(NoteData& inout)
{
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(inout, r)
	for (auto t = 1; t < inout.GetNumTracks(); t++) {
		auto iter = inout.FindTapNote(t, r);
		if (iter == inout.end(t))
			continue;
		inout.SetTapNote(0, r, iter->second);
		inout.RemoveTapNote(t, iter);
	}
}

void
NoteDataUtil::CollapseLeft(NoteData& inout)
{
	FOREACH_NONEMPTY_ROW_ALL_TRACKS(inout, r)
	{
		auto iNumTracksFilled = 0;
		for (auto t = 0; t < inout.GetNumTracks(); t++) {
			if (inout.GetTapNote(t, r).type != TapNoteType_Empty) {
				auto tn = inout.GetTapNote(t, r);
				inout.SetTapNote(t, r, TAP_EMPTY);
				if (iNumTracksFilled < inout.GetNumTracks()) {
					inout.SetTapNote(iNumTracksFilled, r, tn);
					++iNumTracksFilled;
				}
			}
		}
	}
}

void
NoteDataUtil::ShiftTracks(NoteData& inout, int iShiftBy)
{
	int iOriginalTrackToTakeFrom[MAX_NOTE_TRACKS];
	for (auto i = 0; i < inout.GetNumTracks(); ++i) {
		auto iFrom = i - iShiftBy;
		wrap(iFrom, inout.GetNumTracks());
		iOriginalTrackToTakeFrom[i] = iFrom;
	}

	const auto orig(inout);
	inout.LoadTransformed(orig, orig.GetNumTracks(), iOriginalTrackToTakeFrom);
}

void
NoteDataUtil::ShiftLeft(NoteData& inout)
{
	ShiftTracks(inout, -1);
}

void
NoteDataUtil::ShiftRight(NoteData& inout)
{
	ShiftTracks(inout, +1);
}

void
NoteDataUtil::SwapUpDown(NoteData& inout, StepsType st)
{
	int TakeFrom[MAX_NOTE_TRACKS];
	GetTrackMapping(st, swap_up_down, inout.GetNumTracks(), TakeFrom);
	NoteData tempND;
	tempND.LoadTransformed(inout, inout.GetNumTracks(), TakeFrom);
	inout.CopyAll(tempND);
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::ArbitraryRemap(NoteData& inout, int* mapping)
{
	NoteData tempND;
	tempND.LoadTransformed(inout, inout.GetNumTracks(), mapping);
	inout.CopyAll(tempND);
	inout.RevalidateATIs(std::vector<int>(), false);
}

struct ValidRow
{
	StepsType st;
	bool bValidMask[MAX_NOTE_TRACKS];
};
#define T true
#define f false
const ValidRow g_ValidRows[] = {
	{ StepsType_dance_double, { T, T, T, T, f, f, f, f } },
	{ StepsType_dance_double, { f, T, T, T, T, f, f, f } },
	{ StepsType_dance_double, { f, f, f, T, T, T, T, f } },
	{ StepsType_dance_double, { f, f, f, f, T, T, T, T } },
	{ StepsType_pump_double, { T, T, T, T, T, f, f, f, f, f } },
	{ StepsType_pump_double, { f, f, T, T, T, T, T, T, f, f } },
	{ StepsType_pump_double, { f, f, f, f, f, T, T, T, T, T } },
};
#undef T
#undef f

void
NoteDataUtil::RemoveStretch(NoteData& inout,
							StepsType st,
							int iStartIndex,
							int iEndIndex)
{
	std::vector<const ValidRow*> vpValidRowsToCheck;
	for (const auto& g_ValidRow : g_ValidRows) {
		if (g_ValidRow.st == st)
			vpValidRowsToCheck.push_back(&g_ValidRow);
	}

	// bail early if there's nothing to validate against
	if (vpValidRowsToCheck.empty())
		return;

	// each row must pass at least one valid mask
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE(inout, r, iStartIndex, iEndIndex)
	{
		// only check rows with jumps
		if (inout.GetNumTapNonEmptyTracks(r) < 2)
			continue;

		auto bPassedOneMask = false;
		for (auto& i : vpValidRowsToCheck) {
			const auto& vr = *i;
			if (RowPassesValidMask(inout, r, vr.bValidMask)) {
				bPassedOneMask = true;
				break;
			}
		}

		if (!bPassedOneMask)
			RemoveAllButOneTap(inout, r);
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

bool
NoteDataUtil::RowPassesValidMask(NoteData& inout,
								 int row,
								 const bool bValidMask[])
{
	for (auto t = 0; t < inout.GetNumTracks(); t++) {
		if (!bValidMask[t] &&
			inout.GetTapNote(t, row).type != TapNoteType_Empty)
			return false;
	}

	return true;
}

void
NoteDataUtil::ConvertAdditionsToRegular(NoteData& inout)
{
	for (auto t = 0; t < inout.GetNumTracks(); t++)
		FOREACH_NONEMPTY_ROW_IN_TRACK(inout, t, r)
	if (inout.GetTapNote(t, r).source == TapNoteSource_Addition) {
		auto tn = inout.GetTapNote(t, r);
		tn.source = TapNoteSource_Original;
		inout.SetTapNote(t, r, tn);
	}
	inout.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::TransformNoteData(NoteData& nd,
								TimingData const& timing_data,
								const PlayerOptions& po,
								StepsType st,
								int iStartIndex,
								int iEndIndex)
{
	// Apply remove transforms before others so that we don't go removing
	// notes we just inserted.  Apply TRANSFORM_NOROLLS before
	// TRANSFORM_NOHOLDS, since NOROLLS creates holds.
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_LITTLE])
		Little(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOROLLS])
		ChangeRollsToHolds(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOHOLDS])
		RemoveHoldNotes(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOMINES])
		RemoveMines(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOJUMPS])
		RemoveJumps(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOLIFTS])
		RemoveLifts(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOFAKES])
		RemoveFakes(nd, timing_data, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOHANDS])
		RemoveHands(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOQUADS])
		RemoveQuads(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_NOSTRETCH])
		RemoveStretch(nd, st, iStartIndex, iEndIndex);

	// Apply inserts.
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_BIG])
		Big(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_QUICK])
		Quick(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_BMRIZE])
		BMRize(nd, iStartIndex, iEndIndex);

	// Skippy will still add taps to places that the other
	// AddIntelligentTaps above won't.
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_SKIPPY])
		Skippy(nd, iStartIndex, iEndIndex);

	// These aren't affects by the above inserts very much.
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_MINES])
		AddMines(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_ECHO])
		Echo(nd, iStartIndex, iEndIndex);

	// Jump-adding transforms aren't much affected by additional taps.
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_WIDE])
		Wide(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_STOMP])
		Stomp(nd, st, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_JACKJS])
		JackJS(nd, st, timing_data, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_ANCHORJS])
		AnchorJS(nd, st, timing_data, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_ICYWORLD])
		IcyWorld(nd, st, timing_data, iStartIndex, iEndIndex);

	// Transforms that add holds go last.  If they went first, most tap-adding
	// transforms wouldn't do anything because tap-adding transforms skip areas
	// where there's a hold.
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_PLANTED])
		Planted(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_FLOORED])
		Floored(nd, iStartIndex, iEndIndex);
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_TWISTER])
		Twister(nd, iStartIndex, iEndIndex);

	// Do this here to turn any added holds into rolls
	if (po.m_bTransforms[PlayerOptions::TRANSFORM_HOLDROLLS])
		ChangeHoldsToRolls(nd, iStartIndex, iEndIndex);

	// Apply turns and shuffles last so that they affect inserts.
	if (po.m_bTurns[PlayerOptions::TURN_MIRROR])
		Turn(nd, st, mirror, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_BACKWARDS])
		Turn(nd, st, backwards, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_LEFT])
		Turn(nd, st, left, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_RIGHT])
		Turn(nd, st, right, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_SHUFFLE])
		Turn(nd, st, shuffle, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_SOFT_SHUFFLE])
		Turn(nd, st, soft_shuffle, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE])
		Turn(nd, st, super_shuffle, iStartIndex, iEndIndex);
	if (po.m_bTurns[PlayerOptions::TURN_HRAN_SHUFFLE])
		Turn(nd, st, hran_shuffle, iStartIndex, iEndIndex);

	nd.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::Scale(NoteData& nd, float fScale)
{
	ASSERT(fScale > 0);

	NoteData ndOut;
	ndOut.SetNumTracks(nd.GetNumTracks());

	for (auto t = 0; t < nd.GetNumTracks(); t++) {
		for (NoteData::const_iterator iter = nd.begin(t); iter != nd.end(t);
			 ++iter) {
			auto tn = iter->second;
			const int iNewRow = lround(fScale * iter->first);
			const int iNewDuration =
			  lround(fScale * (iter->first + tn.iDuration));
			tn.iDuration = iNewDuration;
			ndOut.SetTapNote(t, iNewRow, tn);
		}
	}

	nd.swap(ndOut);
	nd.RevalidateATIs(std::vector<int>(), false);
}

/* XXX: move this to an appropriate place, same place as NoteRowToBeat perhaps?
 */
static inline int
GetScaledRow(float fScale, int iStartIndex, int iEndIndex, int iRow)
{
	if (iRow < iStartIndex)
		return iRow;
	if (iRow > iEndIndex)
		return iRow + lround((iEndIndex - iStartIndex) * (fScale - 1));
	return lround((iRow - iStartIndex) * fScale) + iStartIndex;
}

void
NoteDataUtil::ScaleRegion(NoteData& nd,
						  float fScale,
						  int iStartIndex,
						  int iEndIndex)
{
	ASSERT(fScale > 0);
	ASSERT(iStartIndex < iEndIndex);
	ASSERT(iStartIndex >= 0);

	NoteData ndOut;
	ndOut.SetNumTracks(nd.GetNumTracks());

	for (auto t = 0; t < nd.GetNumTracks(); t++) {
		for (NoteData::const_iterator iter = nd.begin(t); iter != nd.end(t);
			 ++iter) {
			auto tn = iter->second;
			const auto iNewRow =
			  GetScaledRow(fScale, iStartIndex, iEndIndex, iter->first);
			const auto iNewDuration =
			  GetScaledRow(
				fScale, iStartIndex, iEndIndex, iter->first + tn.iDuration) -
			  iNewRow;
			tn.iDuration = iNewDuration;
			ndOut.SetTapNote(t, iNewRow, tn);
		}
	}

	nd.swap(ndOut);
	nd.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::InsertRows(NoteData& nd, int iStartIndex, int iRowsToAdd)
{
	ASSERT(iRowsToAdd >= 0);

	NoteData temp;
	temp.SetNumTracks(nd.GetNumTracks());
	temp.CopyRange(nd, iStartIndex, MAX_NOTE_ROW);
	nd.ClearRange(iStartIndex, MAX_NOTE_ROW);
	nd.CopyRange(temp, 0, MAX_NOTE_ROW, iStartIndex + iRowsToAdd);
	nd.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::DeleteRows(NoteData& nd, int iStartIndex, int iRowsToDelete)
{
	ASSERT(iRowsToDelete >= 0);

	NoteData temp;
	temp.SetNumTracks(nd.GetNumTracks());
	temp.CopyRange(nd, iStartIndex + iRowsToDelete, MAX_NOTE_ROW);
	nd.ClearRange(iStartIndex, MAX_NOTE_ROW);
	nd.CopyRange(temp, 0, MAX_NOTE_ROW, iStartIndex);
	nd.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveAllButRange(NoteData& nd, int iStartIndex, int iEndIndex)
{
	ASSERT(abs(iStartIndex - iEndIndex) > 0);
	nd.ClearRange(0, iStartIndex - 1);
	nd.ClearRange(iEndIndex, MAX_NOTE_ROW);
	nd.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveAllTapsOfType(NoteData& ndInOut, TapNoteType typeToRemove)
{
	/* Be very careful when deleting the tap notes. Erasing elements from maps
	 * using iterators invalidates only the iterator that is being erased. To
	 * that end, increment the iterator before deleting the elment of the map.
	 */
	for (auto t = 0; t < ndInOut.GetNumTracks(); t++) {
		for (auto iter = ndInOut.begin(t); iter != ndInOut.end(t);) {
			if (iter->second.type == typeToRemove)
				ndInOut.RemoveTapNote(t, iter++);
			else
				++iter;
		}
	}
	ndInOut.RevalidateATIs(std::vector<int>(), false);
}

void
NoteDataUtil::RemoveAllTapsExceptForType(NoteData& ndInOut,
										 TapNoteType typeToKeep)
{
	/* Same as in RemoveAllTapsOfType(). */
	for (auto t = 0; t < ndInOut.GetNumTracks(); t++) {
		for (auto iter = ndInOut.begin(t); iter != ndInOut.end(t);) {
			if (iter->second.type != typeToKeep)
				ndInOut.RemoveTapNote(t, iter++);
			else
				++iter;
		}
	}
	ndInOut.RevalidateATIs(std::vector<int>(), false);
}

int
NoteDataUtil::GetMaxNonEmptyTrack(const NoteData& in)
{
	for (auto t = in.GetNumTracks() - 1; t >= 0; t--)
		if (!in.IsTrackEmpty(t))
			return t;
	return -1;
}

bool
NoteDataUtil::AnyTapsAndHoldsInTrackRange(const NoteData& in,
										  int iTrack,
										  int iStart,
										  int iEnd)
{
	if (iStart >= iEnd)
		return false;

	// for each index we crossed since the last update:
	FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE(in, iTrack, r, iStart, iEnd)
	{
		switch (in.GetTapNote(iTrack, r).type) {
			case TapNoteType_Empty:
			case TapNoteType_Mine:
				continue;
			default:
				return true;
		}
	}

	if (in.IsHoldNoteAtRow(iTrack, iEnd))
		return true;

	return false;
}

/* Find the next row that either starts a TapNote, or ends a previous one. */
bool
NoteDataUtil::GetNextEditorPosition(const NoteData& in, int& rowInOut)
{
	const auto iOriginalRow = rowInOut;
	auto bAnyHaveNextNote = in.GetNextTapNoteRowForAllTracks(rowInOut);

	auto iClosestNextRow = rowInOut;
	if (!bAnyHaveNextNote)
		iClosestNextRow = MAX_NOTE_ROW;

	for (auto t = 0; t < in.GetNumTracks(); t++) {
		int iHeadRow;
		if (!in.IsHoldHeadOrBodyAtRow(t, iOriginalRow, &iHeadRow))
			continue;

		const auto& tn = in.GetTapNote(t, iHeadRow);
		auto iEndRow = iHeadRow + tn.iDuration;
		if (iEndRow == iOriginalRow)
			continue;

		bAnyHaveNextNote = true;
		ASSERT(iEndRow < MAX_NOTE_ROW);
		iClosestNextRow = std::min(iClosestNextRow, iEndRow);
	}

	if (!bAnyHaveNextNote)
		return false;

	rowInOut = iClosestNextRow;
	return true;
}

bool
NoteDataUtil::GetPrevEditorPosition(const NoteData& in, int& rowInOut)
{
	const auto iOriginalRow = rowInOut;
	auto bAnyHavePrevNote = in.GetPrevTapNoteRowForAllTracks(rowInOut);

	auto iClosestPrevRow = rowInOut;
	for (auto t = 0; t < in.GetNumTracks(); t++) {
		auto iHeadRow = iOriginalRow;
		if (!in.GetPrevTapNoteRowForTrack(t, iHeadRow))
			continue;

		const auto& tn = in.GetTapNote(t, iHeadRow);
		if (tn.type != TapNoteType_HoldHead)
			continue;

		auto iEndRow = iHeadRow + tn.iDuration;
		if (iEndRow >= iOriginalRow)
			continue;

		bAnyHavePrevNote = true;
		ASSERT(iEndRow < MAX_NOTE_ROW);
		iClosestPrevRow = std::max(iClosestPrevRow, iEndRow);
	}

	if (!bAnyHavePrevNote)
		return false;

	rowInOut = iClosestPrevRow;
	return true;
}

unsigned int
NoteDataUtil::GetTotalHoldTicks(NoteData* nd, const TimingData* td)
{
	unsigned int ret = 0;
	// Last row must be included. -- Matt
	auto end = nd->GetLastRow() + 1;
	auto segments = td->GetTimingSegments(SEGMENT_TICKCOUNT);
	// We start with the LAST TimingSegment and work our way backwards.
	// This way we can continually update end instead of having to lookup when
	// the next segment starts.
	for (int i = segments.size() - 1; i >= 0; i--) {
		auto* ts = (TickcountSegment*)segments[i];
		if (ts->GetTicks() > 0) {
			// Jump to each point where holds would tick and add the number of
			// holds there to ret.
			for (auto j = ts->GetRow(); j < end;
				 j += ROWS_PER_BEAT / ts->GetTicks())
				// 1 tick per row.
				if (nd->GetNumTracksHeldAtRow(j) > 0)
					ret++;
		}
		end = ts->GetRow();
	}
	return ret;
}
