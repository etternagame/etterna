#include "global.h"
#include "GameState.h"
#include "IniFile.h"
#include "Player.h"
#include "PlayerAI.h"
#include "PlayerState.h"
#include "RageUtil.h"

#define AI_PATH "Data/AI.ini"

struct TapScoreDistribution
{
	float fPercent[NUM_TapNoteScore];

	void ChangeWeightsToPercents()
	{
		float sum = 0;
		for (float i : fPercent)
		{
			sum += i;
		}
		for (float& i : fPercent)
		{
			i /= sum;
		}
	}
	void SetDefaultWeights()
	{
		fPercent[TNS_None] = 0;
		fPercent[TNS_Miss] = 1;
		fPercent[TNS_W5] = 0;
		fPercent[TNS_W4] = 0;
		fPercent[TNS_W3] = 0;
		fPercent[TNS_W2] = 0;
		fPercent[TNS_W1] = 0;
	}

	TapNoteScore GetTapNoteScore()
	{
		float fRand = randomf(0,1);
		float fCumulativePercent = 0;
		for (int i = 0; i <= TNS_W1; i++)
		{
			fCumulativePercent += fPercent[i];
			if (fRand <= fCumulativePercent + 1e-4) // rounding error
				return static_cast<TapNoteScore>(i);
		}
		// the fCumulativePercents must sum to 1.0, so we should never get here!
		ASSERT_M(0, ssprintf("%f,%f", fRand, fCumulativePercent));
	}
};

static TapScoreDistribution g_Distributions[NUM_SKILL_LEVELS];

HighScore* PlayerAI::pScoreData = nullptr;
map<int, vector<TapReplayResult>> PlayerAI::m_ReplayTapMap;
map<int, vector<HoldReplayResult>> PlayerAI::m_ReplayHoldMap;

void PlayerAI::InitFromDisk()
{
	IniFile ini;
	bool bSuccess = ini.ReadFile( AI_PATH );
	if (!bSuccess)
	{
		LuaHelpers::ReportScriptErrorFmt("Error trying to read \"%s\" to load AI player skill settings.", AI_PATH);
		for(auto& g_Distribution : g_Distributions)
		{
			g_Distribution.SetDefaultWeights();
			g_Distribution.ChangeWeightsToPercents();
		}
	}
	else
	{
		for (int i = 0; i < NUM_SKILL_LEVELS; i++)
		{
			RString sKey = ssprintf("Skill%d", i);
			XNode* pNode = ini.GetChild(sKey);
			TapScoreDistribution& dist = g_Distributions[i];
			if (pNode == nullptr)
			{
				LuaHelpers::ReportScriptErrorFmt("AI.ini: \"%s\" section doesn't exist.", sKey.c_str());
				dist.SetDefaultWeights();
			}
			else
			{
			#define SET_MALF_IF(condition, tns) \
				if (condition) \
				{ \
					LuaHelpers::ReportScriptErrorFmt("AI weight for " #tns " in \"%s\" section not set.", sKey.c_str()); \
					dist.fPercent[tns]= 0; \
				}
				dist.fPercent[TNS_None] = 0;
				bSuccess = pNode->GetAttrValue( "WeightMiss", dist.fPercent[TNS_Miss] );
				SET_MALF_IF(!bSuccess, TNS_Miss);
				bSuccess = pNode->GetAttrValue( "WeightW5", dist.fPercent[TNS_W5] );
				SET_MALF_IF(!bSuccess, TNS_W5);
				bSuccess = pNode->GetAttrValue( "WeightW4", dist.fPercent[TNS_W4] );
				SET_MALF_IF(!bSuccess, TNS_W4);
				bSuccess = pNode->GetAttrValue( "WeightW3", dist.fPercent[TNS_W3] );
				SET_MALF_IF(!bSuccess, TNS_W3);
				bSuccess = pNode->GetAttrValue( "WeightW2", dist.fPercent[TNS_W2] );
				SET_MALF_IF(!bSuccess, TNS_W2);
				bSuccess = pNode->GetAttrValue( "WeightW1", dist.fPercent[TNS_W1] );
				SET_MALF_IF(!bSuccess, TNS_W1);
			#undef SET_MALF_IF
			}
			dist.ChangeWeightsToPercents();
		}
	}
}

TapNoteScore PlayerAI::GetTapNoteScore( const PlayerState* pPlayerState )
{
	if (pPlayerState->m_PlayerController == PC_AUTOPLAY)
		return TNS_W1;
	if (pPlayerState->m_PlayerController == PC_REPLAY)
		return TNS_Miss;

	const int iCpuSkill = pPlayerState->m_iCpuSkill;

	TapScoreDistribution& distribution = g_Distributions[iCpuSkill];

	return distribution.GetTapNoteScore();
}

TapNoteScore PlayerAI::GetTapNoteScoreForReplay(const PlayerState* pPlayerState, float fNoteOffset)
{
	// This code is basically a copy paste from somewhere in Player for grabbing scores.

	//LOG->Trace("Given number %f ", fNoteOffset);
	if (fNoteOffset <= -1.0f)
		return TNS_Miss;
	const float fSecondsFromExact = fabsf(fNoteOffset);
	//LOG->Trace("TapNoteScore For Replay Seconds From Exact: %f", fSecondsFromExact);

	if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W1))
		return TNS_W1;
	else if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W2))
		return TNS_W2;
	else if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W3))
		return TNS_W3;
	else if (fSecondsFromExact <= Player::GetWindowSeconds(TW_W4))
		return TNS_W4;
	else if (fSecondsFromExact <= max(Player::GetWindowSeconds(TW_W5), 0.18f))
		return TNS_W5;
	return TNS_None;
}

void PlayerAI::SetScoreData(HighScore* pHighScore)
{
	pHighScore->LoadReplayData();
	pScoreData = pHighScore;
	auto replayNoteRowVector = pHighScore->GetCopyOfNoteRowVector();
	auto replayOffsetVector = pHighScore->GetCopyOfOffsetVector();
	auto replayTapNoteTypeVector = pHighScore->GetCopyOfTapNoteTypeVector();
	auto replayTrackVector = pHighScore->GetCopyOfTrackVector();
	auto replayHoldVector = pHighScore->GetCopyOfHoldReplayDataVector();

	// Generate TapReplayResults to put into a vector referenced by the song row in a map
	for (int i = 0; i < replayNoteRowVector.size(); i++)
	{
		TapReplayResult trr;
		trr.row = replayNoteRowVector[i];
		trr.offset = replayOffsetVector[i];
		if (pScoreData->GetReplayType() == 2) // 2 means that this is a Full Replay
		{
			trr.track = replayTrackVector[i];
			trr.type = replayTapNoteTypeVector[i];
		}
		else // Anything else (and we got this far without crashing) means it's not a Full Replay
		{
			trr.track = NULL;
			trr.type = TapNoteType_Empty;
		}

		// Create or append to the vector
		if (m_ReplayTapMap.count(replayNoteRowVector[i]) != 0)
		{
			m_ReplayTapMap[replayNoteRowVector[i]].push_back(trr);
		}
		else
		{
			vector<TapReplayResult> trrVector = { trr };
			m_ReplayTapMap[replayNoteRowVector[i]] = trrVector;
		}
	}

	// Generate vectors made of pregenerated HoldReplayResults referenced by the song row in a map
	for (int i = 0; i < replayHoldVector.size(); i++)
	{
		// Create or append to the vector
		if (m_ReplayHoldMap.count(replayHoldVector[i].row) != 0)
		{
			m_ReplayHoldMap[replayHoldVector[i].row].push_back(replayHoldVector[i]);
		}
		else
		{
			vector<HoldReplayResult> hrrVector = { replayHoldVector[i] };
			m_ReplayHoldMap[replayHoldVector[i].row] = hrrVector;
		}
	}
}

bool PlayerAI::DetermineIfHoldDropped(int noteRow, int col)
{
	//LOG->Trace("Checking for hold.");
	// Is the given row/column in our dropped hold map?
	if (m_ReplayHoldMap.count(noteRow) != 0)
	{
		//LOG->Trace("Hold row exists in the data");
		// It is, so let's go over each column, assuming we may have dropped more than one hold at once.
		for (auto hrr : m_ReplayHoldMap[noteRow])
		{
			// We found the column we are looking for
			if (hrr.track == col)
			{
				//LOG->Trace("KILL IT NOW");
				return true;
			}
		}
	}
	return false;
}

float PlayerAI::GetTapNoteOffsetForReplay(TapNote* pTN, int noteRow, int col)
{
	/* Given the pTN coming from gameplay, we search for the matching note in the replay data.
	If it is not found, it is a miss. (1.f)
	*/
	if (pScoreData == nullptr) // possible cheat prevention
		return -1.f;

	// Current v0.60 Replay Data format: [noterow] [offset] [track] [optional: tap note type]
	// Current v0.60 Replay Data format (H section): H [noterow] [track] [optional: tap note subtype]
	//LOG->Trace("Note row %d", noteRow);
	if (m_ReplayTapMap.count(noteRow) != 0) // is the current row recorded?
	{
		// This replay has no column data or is considered Basic. (Pre-v0.60 Replays do this.)
		if (pScoreData->GetReplayType() == 1)
		{
			// mines are not preset in the old replay data, we just skip them
			// this gets caught by Player after it finds that the offset wasnt -2.f
			// (We check for an impossible offset of -2.f in Player to blow up a mine)
			if (pTN->type == TapNoteType_Mine)
				return -1.f;

			float offset = m_ReplayTapMap[noteRow].back().offset;

			// this is done to be able to judge simultaneous taps differently due to CC Off
			// this results in possibly incorrect precise per tap judges, but the correct
			// judgement ends up being made overall.
			m_ReplayTapMap[noteRow].pop_back();
			if (m_ReplayTapMap[noteRow].empty())
			{
				m_ReplayTapMap.erase(noteRow);
			}

			return -offset;
		}
		else
		{

			// This is only reached if we have column data.
			for (auto trr : m_ReplayTapMap[noteRow]) // go over all elements in the row
			{
				if (trr.track == col) // if the column expected is the actual note, use it
				{
					if (trr.type == TapNoteType_Mine) // hack for mines
						return -2.f;
					if (pTN->type == TapNoteType_Lift)
					{
						if (trr.type != TapNoteType_Lift)
							continue;
					}
					return -trr.offset;
				}
			}
		}
	}

	return -1.f;	// data missing or invalid, give them a miss
}

/*
 * (c) 2003-2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
