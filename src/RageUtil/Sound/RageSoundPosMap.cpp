#include "Etterna/Globals/global.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "Core/Services/Locator.hpp"
#include "RageSoundPosMap.h"
#include "RageUtil/Misc/RageTimer.h"

#include <climits>
#include <vector>

/* The number of frames we should keep pos_map data for.  This being too high
 * is mostly harmless; the data is small. */
const int pos_map_backlog_frames = 100000;

struct pos_map_t
{
	int64_t m_iSourceFrame;
	int64_t m_iDestFrame;
	int m_iFrames;
	float m_fSourceToDestRatio;

	pos_map_t()
	{
		m_iSourceFrame = 0;
		m_iDestFrame = 0;
		m_iFrames = 0;
		m_fSourceToDestRatio = 1.0f;
	}
};

struct pos_map_impl
{
	std::vector<pos_map_t> m_Queue;
	void Cleanup();
};

pos_map_queue::pos_map_queue()
{
	m_pImpl = new pos_map_impl;
}

pos_map_queue::~pos_map_queue()
{
	delete m_pImpl;
}

pos_map_queue::pos_map_queue(const pos_map_queue& cpy)
{
	*this = cpy;
	m_pImpl = new pos_map_impl(*cpy.m_pImpl);
}

pos_map_queue&
pos_map_queue::operator=(const pos_map_queue& rhs)
{
	if (this != &rhs) {
		delete m_pImpl;
		m_pImpl = new pos_map_impl(*rhs.m_pImpl);
	}
	return *this;
}

void
pos_map_queue::Insert(int64_t iSourceFrame,
					  int iFrames,
					  int64_t iDestFrame,
					  float fSourceToDestRatio)
{
	if (!m_pImpl->m_Queue.empty()) {
		/* Optimization: If the last entry lines up with this new entry, just
		 * merge them. */
		pos_map_t& last = m_pImpl->m_Queue.back();
		if (last.m_iSourceFrame + last.m_iFrames == iSourceFrame &&
			last.m_fSourceToDestRatio == fSourceToDestRatio &&
			llabs(last.m_iDestFrame +
				  lround(last.m_iFrames * last.m_fSourceToDestRatio) -
				  iDestFrame) <= 1) {
			last.m_iFrames += iFrames;

			/* Make sure that m_Frames doesn't grow too large and overflow an
			 * int. */
			if (!m_pImpl->m_Queue.empty() &&
				last.m_iFrames > pos_map_backlog_frames * 2) {
				/*
				 * Split this entry into two smaller entries.  This will cause
				 * up to one sample of rounding error in m_iDestFrame.  This
				 * will not accumulate; if we split again and it becomes two
				 * frames of error, the next Insert() will be beyond the
				 * tolerance of the above iDestFrame check, and new data will be
				 * added to a new entry.
				 */
				int iDeleteFrames = last.m_iFrames - pos_map_backlog_frames;

				pos_map_t next(last);

				last.m_iFrames = iDeleteFrames;

				next.m_iSourceFrame += iDeleteFrames;
				next.m_iFrames -= iDeleteFrames;
				next.m_iDestFrame +=
				  lround(iDeleteFrames * next.m_fSourceToDestRatio);

				m_pImpl->m_Queue.push_back(next);
			}

			m_pImpl->Cleanup();

			return;
		}
	}

	auto m = pos_map_t();
	m.m_iSourceFrame = iSourceFrame;
	m.m_iDestFrame = iDestFrame;
	m.m_iFrames = iFrames;
	m.m_fSourceToDestRatio = fSourceToDestRatio;
	m_pImpl->m_Queue.push_back(m);

	m_pImpl->Cleanup();
}

void
pos_map_impl::Cleanup()
{
	/* Scan backwards until we have at least pos_map_backlog_frames. */
	auto it = m_Queue.end();
	int iTotalFrames = 0;
	while (iTotalFrames < pos_map_backlog_frames) {
		if (it == m_Queue.begin())
			break;
		--it;
		iTotalFrames += it->m_iFrames;
	}

	m_Queue.erase(m_Queue.begin(), it);
}

int64_t
pos_map_queue::Search(int64_t iSourceFrame, bool* bApproximate) const
{
	if (bApproximate)
		*bApproximate = false;

	if (IsEmpty()) {
		if (bApproximate)
			*bApproximate = true;
		return 0;
	}

	/* iSourceFrame is probably in pos_map.  Search to figure out what position
	 * it maps to. */
	int64_t iClosestPosition = 0, iClosestPositionDist = INT_MAX;
	const pos_map_t* pClosestBlock =
	  &*m_pImpl->m_Queue.begin(); /* print only */
	for (auto& pm : m_pImpl->m_Queue) {
		if (iSourceFrame >= pm.m_iSourceFrame &&
			iSourceFrame < pm.m_iSourceFrame + pm.m_iFrames) {
			/* iSourceFrame lies in this block; it's an exact match.  Figure
			 * out the exact position. */
			int iDiff = int(iSourceFrame - pm.m_iSourceFrame);
			iDiff = lround(iDiff * pm.m_fSourceToDestRatio);
			return pm.m_iDestFrame + iDiff;
		}

		/* See if the current position is close to the beginning of this block.
		 */
		int64_t dist = llabs(pm.m_iSourceFrame - iSourceFrame);
		if (dist < iClosestPositionDist) {
			iClosestPositionDist = dist;
			pClosestBlock = &pm;
			iClosestPosition = pm.m_iDestFrame;
		}

		/* See if the current position is close to the end of this block. */
		dist = llabs(pm.m_iSourceFrame + pm.m_iFrames - iSourceFrame);
		if (dist < iClosestPositionDist) {
			iClosestPositionDist = dist;
			pClosestBlock = &pm;
			iClosestPosition =
			  pm.m_iDestFrame + lround(pm.m_iFrames * pm.m_fSourceToDestRatio);
		}
	}

	/*
	 * The frame is out of the range of data we've actually sent.
	 * Return the closest position.
	 *
	 * There are three cases when this happens:
	 * 1. Before the first CommitPlayingPosition call.
	 * 2. After GetDataToPlay returns EOF and the sound has flushed, but
	 * before SoundStopped has been called.
	 * 3. Underflow; we'll be given a larger frame number than we know
	 * about.
	 */

	static RageTimer last;
	if (last.PeekDeltaTime() >= 1.0f) {
		last.GetDeltaTime();
		Locator::getLogger()->trace("Approximate sound time: driver frame {}, m_pImpl->m_Queue frame {}..{} (dist {}), closest position is {}",
					iSourceFrame, pClosestBlock->m_iDestFrame,
					pClosestBlock->m_iDestFrame + pClosestBlock->m_iFrames,
					iClosestPositionDist, iClosestPosition);
	}

	if (bApproximate)
		*bApproximate = true;
	return iClosestPosition;
}

void
pos_map_queue::Clear()
{
	m_pImpl->m_Queue.clear();
}

bool
pos_map_queue::IsEmpty() const
{
	return m_pImpl->m_Queue.empty();
}
