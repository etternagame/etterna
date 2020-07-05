#include "Etterna/Globals/global.h"
#include "Etterna/Models/NoteData/NoteData.h"
#include "Etterna/Models/NoteData/NoteDataWithScoring.h"
#include "Etterna/Models/Misc/PlayerState.h"
#include "ScoreKeeper.h"

ScoreKeeper::ScoreKeeper(PlayerState* pPlayerState,
						 PlayerStageStats* pPlayerStageStats)
{
	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
}

void
ScoreKeeper::GetScoreOfLastTapInRow(const NoteData& nd,
									int iRow,
									TapNoteScore& tnsOut,
									int& iNumTapsInRowOut)
{
	int iNum = 0;

	for (int track = 0; track < nd.GetNumTracks(); ++track) {
		const TapNote& tn = nd.GetTapNote(track, iRow);

		if (tn.type != TapNoteType_Tap && tn.type != TapNoteType_HoldHead)
			continue;
		++iNum;
	}
	tnsOut = NoteDataWithScoring::LastTapNoteWithResult(nd, iRow).result.tns;
	iNumTapsInRowOut = iNum;
}

#include "ScoreKeeperNormal.h"

ScoreKeeper*
ScoreKeeper::MakeScoreKeeper(const std::string& sClassName,
							 PlayerState* pPlayerState,
							 PlayerStageStats* pPlayerStageStats)
{
	if (sClassName == "ScoreKeeperNormal")
		return new ScoreKeeperNormal(pPlayerState, pPlayerStageStats);
	FAIL_M(ssprintf("Invalid ScoreKeeper named %s!", sClassName.c_str()));
}
