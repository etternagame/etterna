#include "Etterna/Globals/global.h"
#include "CommonMetrics.h"
#include "Etterna/Singletons/CryptManager.h"
#include "Foreach.h"
#include "Etterna/Singletons/GameState.h"
#include "Etterna/Singletons/LuaManager.h"
#include <MinaCalc/MinaCalc.h>
#include "Etterna/Models/NoteData/NoteData.h"
#include "PlayerStageStats.h"
#include "Etterna/Singletons/PrefsManager.h"
#include "RageUtil/Misc/RageLog.h"
#include "Etterna/Models/ScoreKeepers/ScoreKeeperNormal.h"
#include "Etterna/Models/StepsAndStyles/Steps.h"
#include "Etterna/Singletons/ThemeManager.h"

#define GRADE_PERCENT_TIER(i)                                                  \
	THEME->GetMetricF(                                                         \
	  "PlayerStageStats",                                                      \
	  ssprintf("GradePercent%s", GradeToString((Grade)(i)).c_str()))
// deprecated, but no solution to replace them exists yet:
#define GRADE_TIER02_IS_ALL_W2S                                                \
	THEME->GetMetricB("PlayerStageStats", "GradeTier02IsAllW2s")
#define GRADE_TIER01_IS_ALL_W2S                                                \
	THEME->GetMetricB("PlayerStageStats", "GradeTier01IsAllW2s")
#define GRADE_TIER02_IS_FULL_COMBO                                             \
	THEME->GetMetricB("PlayerStageStats", "GradeTier02IsFullCombo")

static ThemeMetric<TapNoteScore> g_MinScoreToMaintainCombo(
  "Gameplay",
  "MinScoreToMaintainCombo");
static ThemeMetric<bool> g_MineHitIncrementsMissCombo(
  "Gameplay",
  "MineHitIncrementsMissCombo");

const float LESSON_PASS_THRESHOLD = 0.8f;

Grade
GetGradeFromPercent(float fPercent);

void
PlayerStageStats::InternalInit()
{
	m_pStyle = nullptr;
	m_for_multiplayer = false;
	m_player_number = PLAYER_1;
	m_multiplayer_number = MultiPlayer_P1;
	m_bPlayerCanAchieveFullCombo = true;
	m_bJoined = false;
	m_vpPossibleSteps.clear();
	m_iStepsPlayed = 0;
	m_fAliveSeconds = 0;
	m_bFailed = false;
	m_iPossibleDancePoints = 0;
	m_iCurPossibleDancePoints = 0;
	m_iActualDancePoints = 0;
	m_fWifeScore = 0.f;
	CurWifeScore = 0.f;
	MaxWifeScore = 0.f;
	m_fTimingScale = 0.f;
	m_vOffsetVector.clear();
	m_vNoteRowVector.clear();
	m_vTrackVector.clear();
	m_vTapNoteTypeVector.clear();
	m_vHoldReplayData.clear();
	InputData.clear();
	m_iPossibleGradePoints = 0;
	m_iCurCombo = 0;
	m_iMaxCombo = 0;
	m_iCurMissCombo = 0;
	m_iCurScoreMultiplier = 1;
	m_iScore = 0;
	m_iMaxScore = 0;
	m_iCurMaxScore = 0;
	m_iSongsPassed = 0;
	m_iSongsPlayed = 0;
	m_fLifeRemainingSeconds = 0;
	m_iNumControllerSteps = 0;

	// this should probably be handled better-mina
	everusedautoplay = false;
	luascriptwasloaded = false;
	filehadnegbpms = false;
	filegotmines = false;
	gaveuplikeadumbass = false;
	filegotholds = false;

	ZERO(m_iTapNoteScores);
	ZERO(m_iHoldNoteScores);
	m_radarPossible.Zero();
	m_radarActual.Zero();

	m_fFirstSecond = FLT_MAX;
	m_fLastSecond = 0;
	m_iPersonalHighScoreIndex = -1;
	m_iMachineHighScoreIndex = -1;
	m_bDisqualified = false;
	m_HighScore = HighScore();
}

void
PlayerStageStats::Init(PlayerNumber pn)
{
	m_for_multiplayer = false;
	m_player_number = pn;
}

void
PlayerStageStats::Init(MultiPlayer pn)
{
	m_for_multiplayer = true;
	m_multiplayer_number = pn;
}

void
PlayerStageStats::AddStats(const PlayerStageStats& other)
{
	m_pStyle = other.m_pStyle;
	m_bJoined = other.m_bJoined;
	FOREACH_CONST(Steps*, other.m_vpPossibleSteps, s)
	m_vpPossibleSteps.push_back(*s);
	m_iStepsPlayed += other.m_iStepsPlayed;
	m_fAliveSeconds += other.m_fAliveSeconds;
	m_bFailed |= static_cast<int>(other.m_bFailed);
	m_iPossibleDancePoints += other.m_iPossibleDancePoints;
	m_iActualDancePoints += other.m_iActualDancePoints;
	m_iCurPossibleDancePoints += other.m_iCurPossibleDancePoints;
	m_iPossibleGradePoints += other.m_iPossibleGradePoints;

	for (int t = 0; t < NUM_TapNoteScore; t++)
		m_iTapNoteScores[t] += other.m_iTapNoteScores[t];
	for (int h = 0; h < NUM_HoldNoteScore; h++)
		m_iHoldNoteScores[h] += other.m_iHoldNoteScores[h];
	m_iCurCombo += other.m_iCurCombo;
	m_iMaxCombo += other.m_iMaxCombo;
	m_iCurMissCombo += other.m_iCurMissCombo;
	m_iScore += other.m_iScore;
	m_iMaxScore += other.m_iMaxScore;
	m_iCurMaxScore += other.m_iCurMaxScore;
	m_radarPossible += other.m_radarPossible;
	m_radarActual += other.m_radarActual;
	m_iSongsPassed += other.m_iSongsPassed;
	m_iSongsPlayed += other.m_iSongsPlayed;
	m_iNumControllerSteps += other.m_iNumControllerSteps;
	m_fLifeRemainingSeconds = other.m_fLifeRemainingSeconds; // don't accumulate
	m_bDisqualified |= static_cast<int>(other.m_bDisqualified);

	// FirstSecond is always 0, and last second is the time of the last step,
	// so add 1 second between the stages so that the last element of this
	// stage's record isn't overwritten by the first element of the other
	// stage's record. -Kyz
	const float fOtherFirstSecond = other.m_fFirstSecond + m_fLastSecond + 1.0f;
	const float fOtherLastSecond = other.m_fLastSecond + m_fLastSecond + 1.0f;
	m_fLastSecond = fOtherLastSecond;

	map<float, float>::const_iterator it;
	for (it = other.m_fLifeRecord.begin(); it != other.m_fLifeRecord.end();
		 ++it) {
		const float pos = it->first;
		const float life = it->second;
		m_fLifeRecord[fOtherFirstSecond + pos] = life;
	}

	for (unsigned i = 0; i < other.m_ComboList.size(); ++i) {
		const Combo_t& combo = other.m_ComboList[i];

		Combo_t newcombo(combo);
		newcombo.m_fStartSecond += fOtherFirstSecond;
		m_ComboList.push_back(newcombo);
	}

	/* Merge identical combos. This normally only happens in course mode, when
	 * a combo continues between songs. */
	for (unsigned i = 1; i < m_ComboList.size(); ++i) {
		Combo_t& prevcombo = m_ComboList[i - 1];
		Combo_t& combo = m_ComboList[i];
		const float PrevComboEnd =
		  prevcombo.m_fStartSecond + prevcombo.m_fSizeSeconds;
		const float ThisComboStart = combo.m_fStartSecond;
		if (fabsf(PrevComboEnd - ThisComboStart) > 0.001)
			continue;

		// These are really the same combo.
		prevcombo.m_fSizeSeconds += combo.m_fSizeSeconds;
		prevcombo.m_cnt += combo.m_cnt;
		m_ComboList.erase(m_ComboList.begin() + i);
		--i;
	}
}

// get appropriated (for when we have scores but no highscore object to get
// wifegrades) -mina
Grade
GetGradeFromPercent(float fPercent)
{
	if (fPercent >= 0.9997f)
		return Grade_Tier01;
	if (fPercent >= 0.9975f)
		return Grade_Tier02;
	if (fPercent >= 0.93f)
		return Grade_Tier03;
	if (fPercent >= 0.8f)
		return Grade_Tier04;
	if (fPercent >= 0.7f)
		return Grade_Tier05;
	if (fPercent >= 0.6f)
		return Grade_Tier06;
	return Grade_Tier07;
}

Grade
PlayerStageStats::GetWifeGrade()
{
	if (GetGrade() == Grade_Failed)
		return Grade_Failed;

	return GetGradeFromPercent(m_fWifeScore);
}

Grade
PlayerStageStats::GetGrade(float p)
{
	return GetGradeFromPercent(p);
}
Grade
PlayerStageStats::GetGrade() const
{
	if (m_bFailed)
		return Grade_Failed;

	/* XXX: This entire calculation should be in ScoreKeeper, but final
	 * evaluation is tricky since at that point the ScoreKeepers no longer
	 * exist. */
	float fActual = 0;

	bool bIsBeginner = false;
	if (m_iStepsPlayed > 0)
		bIsBeginner =
		  m_vpPossibleSteps[0]->GetDifficulty() == Difficulty_Beginner;

	FOREACH_ENUM(TapNoteScore, tns)
	{
		int iTapScoreValue =
		  ScoreKeeperNormal::TapNoteScoreToGradePoints(tns, bIsBeginner);
		fActual += m_iTapNoteScores[tns] * iTapScoreValue;
		// LOG->Trace( "GetGrade actual: %i * %i", m_iTapNoteScores[tns],
		// iTapScoreValue );
	}

	FOREACH_ENUM(HoldNoteScore, hns)
	{
		int iHoldScoreValue =
		  ScoreKeeperNormal::HoldNoteScoreToGradePoints(hns, bIsBeginner);
		fActual += m_iHoldNoteScores[hns] * iHoldScoreValue;
		// LOG->Trace( "GetGrade actual: %i * %i", m_iHoldNoteScores[hns],
		// iHoldScoreValue );
	}

	// LOG->Trace( "GetGrade: fActual: %f, fPossible: %d", fActual,
	// m_iPossibleGradePoints );

	float fPercent =
	  (m_iPossibleGradePoints == 0) ? 0 : fActual / m_iPossibleGradePoints;

	Grade grade = GetGradeFromPercent(fPercent);

	// LOG->Trace( "GetGrade: Grade: %s, %i", GradeToString(grade).c_str(),
	// GRADE_TIER02_IS_ALL_W2S );

	// TODO: Change these conditions to use Lua instead. -aj
	if (GRADE_TIER02_IS_ALL_W2S) {
		if (FullComboOfScore(TNS_W1))
			return Grade_Tier01;

		if (FullComboOfScore(TNS_W2))
			return Grade_Tier02;

		grade = max(grade, Grade_Tier03);
	}

	if (GRADE_TIER01_IS_ALL_W2S) {
		if (FullComboOfScore(TNS_W2))
			return Grade_Tier01;
		grade = max(grade, Grade_Tier02);
	}

	if (GRADE_TIER02_IS_FULL_COMBO) {
		if (FullComboOfScore(g_MinScoreToMaintainCombo))
			return Grade_Tier02;
		grade = max(grade, Grade_Tier03);
	}

	return grade;
}

float
PlayerStageStats::MakePercentScore(int iActual, int iPossible)
{
	if (iPossible == 0)
		return 0; // div/0

	if (iActual == iPossible)
		return 1; // correct for rounding error

	// This can happen in battle, with transform attacks.
	// ASSERT_M( iActual <= iPossible, ssprintf("%i/%i", iActual, iPossible) );

	float fPercent = iActual / static_cast<float>(iPossible);

	// don't allow negative
	fPercent = max(0, fPercent);

	int iPercentTotalDigits =
	  3 + CommonMetrics::PERCENT_SCORE_DECIMAL_PLACES; // "100" + "." + "00"

	// TRICKY: printf will round, but we want to truncate. Otherwise, we may
	// display a percent score that's too high and doesn't match up with the
	// calculated grade.
	float fTruncInterval =
	  powf(0.1f, static_cast<float>(iPercentTotalDigits) - 1);

	// TRICKY: ftruncf is rounding 1.0000000 to 0.99990004. Give a little boost
	// to fPercentDancePoints to correct for this.
	fPercent += 0.000001f;

	fPercent = ftruncf(fPercent, fTruncInterval);
	return fPercent;
}

RString
PlayerStageStats::FormatPercentScore(float fPercentDancePoints)
{
	int iPercentTotalDigits =
	  3 + CommonMetrics::PERCENT_SCORE_DECIMAL_PLACES; // "100" + "." + "00"

	RString s =
	  ssprintf("%*.*f%%",
			   iPercentTotalDigits,
			   static_cast<int>(CommonMetrics::PERCENT_SCORE_DECIMAL_PLACES),
			   fPercentDancePoints * 100);
	return s;
}

float
PlayerStageStats::GetPercentDancePoints() const
{
	return MakePercentScore(m_iActualDancePoints, m_iPossibleDancePoints);
}
float
PlayerStageStats::GetWifeScore() const
{
	return m_fWifeScore;
}
float
PlayerStageStats::GetCurWifeScore() const
{
	return CurWifeScore;
}
float
PlayerStageStats::GetMaxWifeScore() const
{
	return MaxWifeScore;
}
vector<float>
PlayerStageStats::CalcSSR(float ssrpercent) const
{
	Steps* steps = GAMESTATE->m_pCurSteps;
	float musicrate = GAMESTATE->m_SongOptions.GetCurrent().m_fMusicRate;
	return MinaSDCalc(serializednd,
					  steps->GetNoteData().GetNumTracks(),
					  musicrate,
					  ssrpercent,
					  1.f,
					  steps->GetTimingData()->HasWarps());
}

float
PlayerStageStats::GetTimingScale() const
{
	return m_fTimingScale;
}
vector<float>
PlayerStageStats::GetOffsetVector() const
{
	return m_vOffsetVector;
}
vector<int>
PlayerStageStats::GetNoteRowVector() const
{
	return m_vNoteRowVector;
}
vector<int>
PlayerStageStats::GetTrackVector() const
{
	return m_vTrackVector;
}
vector<TapNoteType>
PlayerStageStats::GetTapNoteTypeVector() const
{
	return m_vTapNoteTypeVector;
}
vector<HoldReplayResult>
PlayerStageStats::GetHoldReplayDataVector() const
{
	return m_vHoldReplayData;
}

float
PlayerStageStats::GetCurMaxPercentDancePoints() const
{
	if (m_iPossibleDancePoints == 0)
		return 0; // div/0

	if (m_iCurPossibleDancePoints == m_iPossibleDancePoints)
		return 1; // correct for rounding error

	auto fCurMaxPercentDancePoints =
	  static_cast<float>(m_iCurPossibleDancePoints / m_iPossibleDancePoints);

	return fCurMaxPercentDancePoints;
}

// TODO: Make this use lua. Let more judgments be possible. -Wolfman2000
int
PlayerStageStats::GetLessonScoreActual() const
{
	int iScore = 0;

	FOREACH_ENUM(TapNoteScore, tns)
	{
		switch (tns) {
			case TNS_AvoidMine:
			case TNS_W5:
			case TNS_W4:
			case TNS_W3:
			case TNS_W2:
			case TNS_W1:
				iScore += m_iTapNoteScores[tns];
			default:
				break;
		}
	}

	FOREACH_ENUM(HoldNoteScore, hns)
	{
		switch (hns) {
			case HNS_Held:
				iScore += m_iHoldNoteScores[hns];
			default:
				break;
		}
	}

	return iScore;
}

int
PlayerStageStats::GetLessonScoreNeeded() const
{
	float fScore = 0;

	FOREACH_CONST(Steps*, m_vpPossibleSteps, steps)
	{
		fScore += (*steps)->GetRadarValues()[RadarCategory_TapsAndHolds];
	}

	return lround(fScore * LESSON_PASS_THRESHOLD);
}

void
PlayerStageStats::ResetScoreForLesson()
{
	m_iCurPossibleDancePoints = 0;
	m_iActualDancePoints = 0;
	FOREACH_ENUM(TapNoteScore, tns)
	m_iTapNoteScores[tns] = 0;
	FOREACH_ENUM(HoldNoteScore, hns)
	m_iHoldNoteScores[hns] = 0;
	m_iCurCombo = 0;
	m_iMaxCombo = 0;
	m_iCurMissCombo = 0;
	m_iScore = 0;
	m_iCurMaxScore = 0;
	m_iMaxScore = 0;
}

void
PlayerStageStats::SetLifeRecordAt(float fLife, float fStepsSecond)
{
	if (fStepsSecond < 0)
		return;

	m_fFirstSecond = min(fStepsSecond, m_fFirstSecond);
	m_fLastSecond = max(fStepsSecond, m_fLastSecond);
	// LOG->Trace( "fLastSecond = %f", m_fLastSecond );

	// fStepsSecond will usually be greater than any value already in the map,
	// but if a tap and a hold both set the life on the same frame, it won't.
	// Check whether an entry already exists for the current time, and move it
	// back a tiny bit if it does and the new value is not the same as the old.
	// Otherwise, you get the rare bug where the life graph shows a gradual
	// decline when the lifebar was actually full up to a miss.  This occurs
	// because the first call has full life, and removes the previous full life
	// entry.  Then the second call of the frame occurs and sets the life for
	// the current time to a lower value.
	// -Kyz
	map<float, float>::iterator curr = m_fLifeRecord.find(fStepsSecond);
	if (curr != m_fLifeRecord.end()) {
		if (curr->second != fLife) {
			// 2^-8
			m_fLifeRecord[fStepsSecond - 0.00390625f] = curr->second;
		}
	}
	m_fLifeRecord[fStepsSecond] = fLife;

	Message msg(
	  static_cast<MessageID>(Message_LifeMeterChangedP1 + m_player_number));
	msg.SetParam("Life", fLife);
	msg.SetParam("StepsSecond", fStepsSecond);
	MESSAGEMAN->Broadcast(msg);

	// Memory optimization:
	// If we have three consecutive records A, B, and C all with the same fLife,
	// we can eliminate record B without losing data. Only check the last three
	// records in the map since we're only inserting at the end, and we know all
	// earlier redundant records have already been removed.
	map<float, float>::iterator C = m_fLifeRecord.end();
	--C;
	if (C == m_fLifeRecord.begin()) // no earlier records left
		return;

	map<float, float>::iterator B = C;
	--B;
	if (B == m_fLifeRecord.begin()) // no earlier records left
		return;

	map<float, float>::iterator A = B;
	--A;

	if (A->second == B->second && B->second == C->second)
		m_fLifeRecord.erase(B);
}

float
PlayerStageStats::GetLifeRecordAt(float fStepsSecond) const
{
	if (m_fLifeRecord.empty())
		return 0;

	// Find the first element whose key is greater than k.
	map<float, float>::const_iterator it =
	  m_fLifeRecord.upper_bound(fStepsSecond);

	// Find the last element whose key is less than or equal to k.
	if (it != m_fLifeRecord.begin())
		--it;

	return it->second;
}

float
PlayerStageStats::GetLifeRecordLerpAt(float fStepsSecond) const
{
	if (m_fLifeRecord.empty())
		return 0;

	// Find the first element whose key is greater than k.
	map<float, float>::const_iterator later =
	  m_fLifeRecord.upper_bound(fStepsSecond);

	// Find the last element whose key is less than or equal to k.
	map<float, float>::const_iterator earlier = later;
	if (earlier != m_fLifeRecord.begin())
		--earlier;

	if (later == m_fLifeRecord.end())
		return earlier->second;

	if (earlier->first == later->first) // two samples from the same time.
										// Don't divide by zero in SCALE
		return earlier->second;

	// earlier <= pos <= later
	return SCALE(fStepsSecond,
				 earlier->first,
				 later->first,
				 earlier->second,
				 later->second);
}

void
PlayerStageStats::GetLifeRecord(float* fLifeOut,
								int iNumSamples,
								float fStepsEndSecond) const
{
	for (int i = 0; i < iNumSamples; ++i) {
		float from =
		  SCALE(i, 0, static_cast<float>(iNumSamples), 0.0f, fStepsEndSecond);
		fLifeOut[i] = GetLifeRecordLerpAt(from);
	}
}

float
PlayerStageStats::GetCurrentLife() const
{
	if (m_fLifeRecord.empty())
		return 0;
	map<float, float>::const_iterator iter = m_fLifeRecord.end();
	--iter;
	return iter->second;
}

// copy pasta from above but for wife% -mina
void
PlayerStageStats::SetWifeRecordAt(float Wife, float fStepsSecond)
{
	if (fStepsSecond < 0)
		return;

	m_fFirstSecond = min(fStepsSecond, m_fFirstSecond);
	m_fLastSecond = max(fStepsSecond, m_fLastSecond);
	map<float, float>::iterator curr = WifeRecord.find(fStepsSecond);
	if (curr != WifeRecord.end()) {
		if (curr->second != Wife) {
			// 2^-8
			WifeRecord[fStepsSecond - 0.00390625f] = curr->second;
		}
	}
	WifeRecord[fStepsSecond] = Wife;

	map<float, float>::iterator C = WifeRecord.end();
	--C;
	if (C == WifeRecord.begin()) // no earlier records left
		return;

	map<float, float>::iterator B = C;
	--B;
	if (B == WifeRecord.begin()) // no earlier records left
		return;

	map<float, float>::iterator A = B;
	--A;

	if (A->second == B->second && B->second == C->second)
		WifeRecord.erase(B);
}

float
PlayerStageStats::GetWifeRecordAt(float fStepsSecond) const
{
	if (WifeRecord.empty())
		return 0;
	map<float, float>::const_iterator it = WifeRecord.upper_bound(fStepsSecond);
	if (it != WifeRecord.begin())
		--it;
	return it->second;
}

float
PlayerStageStats::GetWifeRecordLerpAt(float fStepsSecond) const
{
	if (WifeRecord.empty())
		return 0;

	map<float, float>::const_iterator later =
	  WifeRecord.upper_bound(fStepsSecond);
	map<float, float>::const_iterator earlier = later;
	if (earlier != WifeRecord.begin())
		--earlier;

	if (later == WifeRecord.end())
		return earlier->second;

	if (earlier->first == later->first) // two samples from the same time.
										// Don't divide by zero in SCALE
		return earlier->second;

	return SCALE(fStepsSecond,
				 earlier->first,
				 later->first,
				 earlier->second,
				 later->second);
}

void
PlayerStageStats::GetWifeRecord(float* WifeOut,
								int iNumSamples,
								float fStepsEndSecond) const
{
	for (int i = 0; i < iNumSamples; ++i) {
		float from =
		  SCALE(i, 0, static_cast<float>(iNumSamples), 0.0f, fStepsEndSecond);
		WifeOut[i] = GetLifeRecordLerpAt(from);
	}
}

/* If bRollover is true, we're being called before gameplay begins, so we can
 * record the amount of the first combo that comes from the previous song. */
void
PlayerStageStats::UpdateComboList(float fSecond, bool bRollover)
{
	if (fSecond < 0)
		return;

	if (!bRollover) {
		m_fFirstSecond = min(fSecond, m_fFirstSecond);
		m_fLastSecond = max(fSecond, m_fLastSecond);
		// LOG->Trace( "fLastSecond = %f", fLastSecond );
	}

	int cnt = m_iCurCombo;
	if (cnt == 0)
		return; // no combo

	if (m_ComboList.size() == 0 || m_ComboList.back().m_cnt >= cnt) {
		/* If the previous combo (if any) starts on -9999, then we rolled over
		 * some combo, but missed the first step. Remove it. */
		if (m_ComboList.size() && m_ComboList.back().m_fStartSecond == -9999)
			m_ComboList.erase(m_ComboList.begin() + m_ComboList.size() - 1,
							  m_ComboList.end());

		// This is a new combo.
		Combo_t NewCombo;
		/* "start" is the position that the combo started within this song.
		 * If we're recording rollover, the combo hasn't started yet (within
		 * this song), so put a placeholder in and set it on the next call.
		 * (Otherwise, start will be less than fFirstPos.) */
		if (bRollover)
			NewCombo.m_fStartSecond = -9999;
		else
			NewCombo.m_fStartSecond = fSecond;
		m_ComboList.push_back(NewCombo);
	}

	Combo_t& combo = m_ComboList.back();
	if (!bRollover && combo.m_fStartSecond == -9999)
		combo.m_fStartSecond = 0;

	combo.m_fSizeSeconds = fSecond - combo.m_fStartSecond;
	combo.m_cnt = cnt;

	if (bRollover)
		combo.m_rollover = cnt;
}

/* This returns the largest combo contained within the song */
PlayerStageStats::Combo_t
PlayerStageStats::GetMaxCombo() const
{
	if (m_ComboList.size() == 0)
		return Combo_t();

	int m = 0;
	for (unsigned i = 1; i < m_ComboList.size(); ++i) {
		if (m_ComboList[i].m_cnt > m_ComboList[m].m_cnt)
			m = i;
	}

	return m_ComboList[m];
}

int
PlayerStageStats::GetComboAtStartOfStage() const
{
	if (m_ComboList.empty())
		return 0;

	return m_ComboList[0].m_rollover;
}

bool
PlayerStageStats::FullComboOfScore(TapNoteScore tnsAllGreaterOrEqual) const
{
	ASSERT(tnsAllGreaterOrEqual >= TNS_W5);
	ASSERT(tnsAllGreaterOrEqual <= TNS_W1);

	// if we've set MissCombo to anything besides 0, it's not a full combo
	if (!m_bPlayerCanAchieveFullCombo)
		return false;

	// If missed any holds, then it's not a full combo
	if (m_iHoldNoteScores[HNS_LetGo] > 0)
		return false;

	// if any checkpoints were missed, it's not a full combo	either
	if (m_iTapNoteScores[TNS_CheckpointMiss] > 0)
		return false;

	// If has any of the judgments below, then not a full combo
	for (int i = TNS_Miss; i < tnsAllGreaterOrEqual; i++) {
		if (m_iTapNoteScores[i] > 0)
			return false;
	}

	// hit any mines when they increment the miss combo? It's not a full combo.
	if (g_MineHitIncrementsMissCombo && m_iTapNoteScores[TNS_HitMine] > 0)
		return false;

	// If has at least one of the judgments equal to or above, then is a full
	// combo.
	for (int i = tnsAllGreaterOrEqual; i < NUM_TapNoteScore; i++) {
		if (m_iTapNoteScores[i] > 0)
			return true;
	}

	return false;
}

TapNoteScore
PlayerStageStats::GetBestFullComboTapNoteScore() const
{
	// Optimization opportunity: ...
	// (seriously? -aj)
	for (TapNoteScore i = TNS_W1; i >= TNS_W5; enum_add(i, -1)) {
		if (FullComboOfScore(i))
			return i;
	}
	return TapNoteScore_Invalid;
}

bool
PlayerStageStats::SingleDigitsOfScore(TapNoteScore tnsAllGreaterOrEqual) const
{
	return FullComboOfScore(tnsAllGreaterOrEqual) &&
		   m_iTapNoteScores[tnsAllGreaterOrEqual] < 10;
}

bool
PlayerStageStats::OneOfScore(TapNoteScore tnsAllGreaterOrEqual) const
{
	return FullComboOfScore(tnsAllGreaterOrEqual) &&
		   m_iTapNoteScores[tnsAllGreaterOrEqual] == 1;
}

int
PlayerStageStats::GetTotalTaps() const
{
	int iTotalTaps = 0;
	for (int i = TNS_Miss; i < NUM_TapNoteScore; i++) {
		iTotalTaps += m_iTapNoteScores[i];
	}
	return iTotalTaps;
}

float
PlayerStageStats::GetPercentageOfTaps(TapNoteScore tns) const
{
	int iTotalTaps = 0;
	for (int i = TNS_Miss; i < NUM_TapNoteScore; i++) {
		iTotalTaps += m_iTapNoteScores[i];
	}
	return m_iTapNoteScores[tns] / static_cast<float>(iTotalTaps);
}

bool
PlayerStageStats::IsDisqualified() const
{
	return m_bDisqualified;
}

void
PlayerStageStats::UnloadReplayData()
{
	m_vNoteRowVector.clear();
	m_vOffsetVector.clear();
	m_vTrackVector.clear();
	m_vTapNoteTypeVector.clear();
	m_vHoldReplayData.clear();

	m_vNoteRowVector.shrink_to_fit();
	m_vOffsetVector.shrink_to_fit();
	m_vTrackVector.shrink_to_fit();
	m_vTapNoteTypeVector.shrink_to_fit();
	m_vHoldReplayData.shrink_to_fit();
}

LuaFunction(GetGradeFromPercent, GetGradeFromPercent(FArg(1)))
  LuaFunction(FormatPercentScore, PlayerStageStats::FormatPercentScore(FArg(1)))

// lua start
#include "Etterna/Models/Lua/LuaBinding.h"

  /** @brief Allow Lua to have access to the PlayerStageStats. */
  class LunaPlayerStageStats : public Luna<PlayerStageStats>
{
public:
	DEFINE_METHOD( GetNumControllerSteps,		m_iNumControllerSteps )
	DEFINE_METHOD( GetLifeRemainingSeconds,		m_fLifeRemainingSeconds )
	DEFINE_METHOD( GetSurvivalSeconds,			GetSurvivalSeconds() )
	DEFINE_METHOD( GetCurrentCombo,				m_iCurCombo )
	DEFINE_METHOD( GetCurrentMissCombo,			m_iCurMissCombo )
	DEFINE_METHOD( GetCurrentScoreMultiplier,	m_iCurScoreMultiplier )
	DEFINE_METHOD( GetScore,					m_iScore )
	DEFINE_METHOD( GetWifeScore,				m_fWifeScore )
	DEFINE_METHOD( GetCurWifeScore,				CurWifeScore)
	DEFINE_METHOD( GetMaxWifeScore,				MaxWifeScore)
	DEFINE_METHOD( GetCurMaxScore,				m_iCurMaxScore )
	DEFINE_METHOD( GetTapNoteScores,			m_iTapNoteScores[Enum::Check<TapNoteScore>(L, 1)] )
	DEFINE_METHOD( GetHoldNoteScores,			m_iHoldNoteScores[Enum::Check<HoldNoteScore>(L, 1)] )
	DEFINE_METHOD( FullCombo,					FullCombo() )
	DEFINE_METHOD( FullComboOfScore,			FullComboOfScore( Enum::Check<TapNoteScore>(L, 1) ) )
	DEFINE_METHOD( MaxCombo,					GetMaxCombo().m_cnt )
	DEFINE_METHOD( GetCurrentLife,				GetCurrentLife() )
	DEFINE_METHOD( GetGrade,					GetGrade() )
	DEFINE_METHOD( GetWifeGrade,				GetWifeGrade())
	DEFINE_METHOD( GetActualDancePoints,		m_iActualDancePoints )
	DEFINE_METHOD( GetPossibleDancePoints,		m_iPossibleDancePoints )
	DEFINE_METHOD( GetCurrentPossibleDancePoints,		m_iCurPossibleDancePoints )
	DEFINE_METHOD( GetPercentDancePoints,		GetPercentDancePoints() )
	DEFINE_METHOD( GetLessonScoreActual,		GetLessonScoreActual() )
	DEFINE_METHOD( GetLessonScoreNeeded,		GetLessonScoreNeeded() )
	DEFINE_METHOD( GetPersonalHighScoreIndex,	m_iPersonalHighScoreIndex )
	DEFINE_METHOD( GetMachineHighScoreIndex,	m_iMachineHighScoreIndex )
	DEFINE_METHOD( IsDisqualified,				IsDisqualified() )
	DEFINE_METHOD( GetAliveSeconds,				m_fAliveSeconds )
	DEFINE_METHOD( GetTotalTaps,				GetTotalTaps() )
	DEFINE_METHOD( GetPercentageOfTaps,			GetPercentageOfTaps( Enum::Check<TapNoteScore>(L, 1) ) )
	DEFINE_METHOD( GetBestFullComboTapNoteScore, GetBestFullComboTapNoteScore() )
	DEFINE_METHOD( GetFailed, 					m_bFailed )
	DEFINE_METHOD( GetSongsPassed, 					m_iSongsPassed )
	DEFINE_METHOD( GetSongsPlayed, 					m_iSongsPlayed )

	static int GetHighScore(T* p, lua_State* L)
	{
		p->m_HighScore.PushSelf(L);
		return 1;
	}

	static int GetPlayedSteps(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (int i = 0; i < min(static_cast<int>(p->m_iStepsPlayed),
								static_cast<int>(p->m_vpPossibleSteps.size()));
			 ++i) {
			p->m_vpPossibleSteps[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetPossibleSteps(T* p, lua_State* L)
	{
		lua_newtable(L);
		for (int i = 0; i < static_cast<int>(p->m_vpPossibleSteps.size());
			 ++i) {
			p->m_vpPossibleSteps[i]->PushSelf(L);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	// Convert to MS so lua doesn't have to
	static int GetOffsetVector(T* p, lua_State* L)
	{
		auto& offs = p->m_vOffsetVector;
		auto& type = p->m_vTapNoteTypeVector;
		vector<float> doot;
		// type would not be empty in Full Replays (> v0.60)
		if (!type.empty()) {
			for (size_t i = 0; i < offs.size(); ++i)
				if (type[i] != TapNoteType_Mine)
					doot.emplace_back(offs[i] * 1000);
		} else {
			// But type is empty if the replay is old :(
			for (size_t i = 0; i < offs.size(); ++i)
				doot.emplace_back(offs[i] * 1000);
		}
		LuaHelpers::CreateTableFromArray(doot, L);
		return 1;
	}

	static int GetNoteRowVector(T* p, lua_State* L)
	{
		LuaHelpers::CreateTableFromArray(p->m_vNoteRowVector, L);
		return 1;
	}

	static int GetTrackVector(T* p, lua_State* L)
	{
		LuaHelpers::CreateTableFromArray(p->m_vTrackVector, L);
		return 1;
	}

	static int GetTapNoteTypeVector(T* p, lua_State* L)
	{
		LuaHelpers::CreateTableFromArray(p->m_vTapNoteTypeVector, L);
		return 1;
	}

	static int WifeScoreOffset(T* p, lua_State* L)
	{
		lua_pushnumber(L, wife2(FArg(1), p->GetTimingScale()));
		return 1;
	}

	// not entirely sure this should be exposed to lua... -mina
	static int UnloadReplayData(T* p, lua_State* L)
	{
		p->UnloadReplayData();
		return 0;
	}

	static int GetComboList(T* p, lua_State* L)
	{
		lua_createtable(L, p->m_ComboList.size(), 0);
		for (size_t i = 0; i < p->m_ComboList.size(); ++i) {
			lua_createtable(L, 0, 6);
			lua_pushstring(L, "StartSecond");
			lua_pushnumber(L, p->m_ComboList[i].m_fStartSecond);
			lua_rawset(L, -3);
			lua_pushstring(L, "SizeSeconds");
			lua_pushnumber(L, p->m_ComboList[i].m_fSizeSeconds);
			lua_rawset(L, -3);
			lua_pushstring(L, "Count");
			lua_pushnumber(L, p->m_ComboList[i].m_cnt);
			lua_rawset(L, -3);
			lua_pushstring(L, "Rollover");
			lua_pushnumber(L, p->m_ComboList[i].m_rollover);
			lua_rawset(L, -3);
			lua_pushstring(L, "StageCount");
			lua_pushnumber(L, p->m_ComboList[i].GetStageCnt());
			lua_rawset(L, -3);
			lua_pushstring(L, "IsZero");
			lua_pushnumber(L, p->m_ComboList[i].IsZero());
			lua_rawset(L, -3);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}
	static int GetLifeRecord(T* p, lua_State* L)
	{
		float last_second = FArg(1);
		int samples = 100;
		if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
			samples = IArg(2);
			if (samples <= 0) {
				LOG->Trace("PlayerStageStats:GetLifeRecord requires an integer "
						   "greater than 0.  Defaulting to 100.");
				samples = 100;
			}
		}
		lua_createtable(L, samples, 0);
		for (int i = 0; i < samples; ++i) {
			// The scale from range is [0, samples-1] because that is i's range.
			float from = SCALE(
			  i, 0, static_cast<float>(samples) - 1.0f, 0.0f, last_second);
			float curr = p->GetLifeRecordLerpAt(from);
			lua_pushnumber(L, curr);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int GetWifeRecord(T* p, lua_State* L)
	{
		float last_second = FArg(1);
		int samples = 100;
		if (lua_gettop(L) >= 2 && !lua_isnil(L, 2)) {
			samples = IArg(2);
			if (samples <= 0) {
				LOG->Trace("PlayerStageStats:GetLifeRecord requires an integer "
						   "greater than 0.  Defaulting to 100.");
				samples = 100;
			}
		}
		lua_createtable(L, samples, 0);
		for (int i = 0; i < samples; ++i) {
			// The scale from range is [0, samples-1] because that is i's range.
			float from = SCALE(
			  i, 0, static_cast<float>(samples) - 1.0f, 0.0f, last_second);
			float curr = p->GetLifeRecordLerpAt(from);
			lua_pushnumber(L, curr);
			lua_rawseti(L, -2, i + 1);
		}
		return 1;
	}

	static int GetRadarPossible(T* p, lua_State* L)
	{
		p->m_radarPossible.PushSelf(L);
		return 1;
	}
	static int GetRadarActual(T* p, lua_State* L)
	{
		p->m_radarActual.PushSelf(L);
		return 1;
	}
	static int SetScore(T* p, lua_State* L)
	{
		if (IArg(1) >= 0) {
			p->m_iScore = IArg(1);
			return 0;
		}
		COMMON_RETURN_SELF;
	}
	static int SetCurMaxScore(T* p, lua_State* L)
	{
		if (IArg(1) >= 0) {
			p->m_iCurMaxScore = IArg(1);
			return 0;
		}
		COMMON_RETURN_SELF;
	}

	static int FailPlayer(T* p, lua_State* L)
	{
		p->m_bFailed = true;
		COMMON_RETURN_SELF;
	}

	LunaPlayerStageStats()
	{
		ADD_METHOD(GetNumControllerSteps);
		ADD_METHOD(GetLifeRemainingSeconds);
		ADD_METHOD(GetSurvivalSeconds);
		ADD_METHOD(GetCurrentCombo);
		ADD_METHOD(GetCurrentMissCombo);
		ADD_METHOD(GetCurrentScoreMultiplier);
		ADD_METHOD(GetScore);
		ADD_METHOD(GetOffsetVector);
		ADD_METHOD(GetTrackVector);
		ADD_METHOD(GetTapNoteTypeVector);
		ADD_METHOD( WifeScoreOffset );
		ADD_METHOD( GetNoteRowVector );
		ADD_METHOD( GetWifeScore );
		ADD_METHOD( GetCurWifeScore );
		ADD_METHOD( GetMaxWifeScore );
		ADD_METHOD( GetCurMaxScore );
		ADD_METHOD( GetTapNoteScores );
		ADD_METHOD( GetHoldNoteScores );
		ADD_METHOD( FullCombo );
		ADD_METHOD( FullComboOfScore );
		ADD_METHOD( MaxCombo );
		ADD_METHOD( GetCurrentLife );
		ADD_METHOD( GetGrade );
		ADD_METHOD( GetWifeGrade );
		ADD_METHOD( GetHighScore );
		ADD_METHOD( GetActualDancePoints );
		ADD_METHOD( GetPossibleDancePoints );
		ADD_METHOD( GetCurrentPossibleDancePoints );
		ADD_METHOD( GetPercentDancePoints );
		ADD_METHOD( GetLessonScoreActual );
		ADD_METHOD( GetLessonScoreNeeded );
		ADD_METHOD( GetPersonalHighScoreIndex );
		ADD_METHOD( GetMachineHighScoreIndex );
		ADD_METHOD( IsDisqualified );
		ADD_METHOD( GetPlayedSteps );
		ADD_METHOD( GetPossibleSteps );
		ADD_METHOD( GetComboList );
		ADD_METHOD( GetLifeRecord );
		ADD_METHOD( GetWifeRecord );
		ADD_METHOD( GetAliveSeconds );
		ADD_METHOD( GetPercentageOfTaps );
		ADD_METHOD( GetTotalTaps );
		ADD_METHOD( GetRadarActual );
		ADD_METHOD( GetRadarPossible );
		ADD_METHOD( GetBestFullComboTapNoteScore );
		ADD_METHOD( GetFailed );
		ADD_METHOD( SetScore );
		ADD_METHOD( GetCurMaxScore );
		ADD_METHOD( SetCurMaxScore );
		ADD_METHOD( FailPlayer );
		ADD_METHOD( GetSongsPassed );
		ADD_METHOD( GetSongsPlayed );
		ADD_METHOD( UnloadReplayData );
	}
};

LUA_REGISTER_CLASS(PlayerStageStats)
// lua end
