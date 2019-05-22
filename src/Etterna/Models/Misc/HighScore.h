#ifndef HIGH_SCORE_H
#define HIGH_SCORE_H

#include "DateTime.h"
#include "GameConstantsAndTypes.h"
#include "Grade.h"
#include "NoteTypes.h"
#include "RageUtil/Utils/RageUtil_AutoPtr.h"

class XNode;
struct RadarValues;
struct lua_State;

using std::string;

struct HighScoreImpl;
/** @brief The high score that is earned by a player.
 *
 * This is scoring data that is persisted between sessions. */
struct HighScore
{
	HighScore();

	/**
	 * @brief Retrieve the name of the player that set the high score.
	 * @return the name of the player. */
	string GetName() const;
	string GetChartKey() const;
	int GetSSRCalcVersion() const;
	/**
	 * @brief Retrieve the grade earned from this score.
	 * @return the grade.
	 */
	Grade GetGrade() const;
	/**
	 * @brief Retrieve the score earned.
	 * @return the score. */
	unsigned int GetScore() const;
	/**
	 * @brief Determine if any judgments were tallied during this run.
	 * @return true if no judgments were recorded, false otherwise. */
	bool IsEmpty() const;
	Grade GetWifeGrade() const;
	float ConvertDpToWife();
	float GetPercentDP() const;
	float GetWifeScore() const;
	float GetWifePoints() const;
	float GetSSRNormPercent() const;
	float GetMusicRate() const;
	float GetJudgeScale() const;
	bool GetChordCohesion() const;
	bool GetEtternaValid() const;
	bool IsUploadedToServer(string s) const;
	std::vector<float> timeStamps;
	const std::vector<float>& GetOffsetVector() const;
	const std::vector<int>& GetNoteRowVector() const;
	const std::vector<int>& GetTrackVector() const;
	const std::vector<TapNoteType>& GetTapNoteTypeVector() const;
	const std::vector<HoldReplayResult>& GetHoldReplayDataVector() const;
	std::vector<float> GetCopyOfOffsetVector() const;
	std::vector<int> GetCopyOfNoteRowVector() const;
	std::vector<int> GetCopyOfTrackVector() const;
	std::vector<TapNoteType> GetCopyOfTapNoteTypeVector() const;
	std::vector<HoldReplayResult> GetCopyOfHoldReplayDataVector() const;
	std::vector<float> GetCopyOfSetOnlineReplayTimestampVector() const;
	string GetScoreKey() const;
	int GetTopScore() const;
	int GetReplayType() const;
	/**
	 * @brief Determine how many seconds the player had left in Survival mode.
	 * @return the number of seconds left. */
	float GetSurviveSeconds() const;
	float GetSurvivalSeconds() const;
	unsigned int GetMaxCombo() const;
	/**
	 * @brief Get the modifiers used for this run.
	 * @return the modifiers. */
	string GetModifiers() const;
	DateTime GetDateTime() const;
	string GetPlayerGuid() const;
	string GetMachineGuid() const;
	string GetCountryCode() const;
	int GetProductID() const;
	int GetTapNoteScore(TapNoteScore tns) const;
	int GetHoldNoteScore(HoldNoteScore tns) const;
	const RadarValues& GetRadarValues() const;
	float GetLifeRemainingSeconds() const;
	/**
	 * @brief Determine if this score was from a situation that would cause
	 * disqualification.
	 * @return true if the score would be disqualified, false otherwise. */
	bool GetDisqualified() const;

	/**
	 * @brief Set the name of the Player that earned the score.
	 * @param sName the name of the Player. */
	void SetName(const string& sName);
	void SetChartKey(const string& ck);
	void SetSSRCalcVersion(int cv);
	void SetGrade(Grade g);
	void SetScore(unsigned int iScore);
	void SetPercentDP(float f);
	void SetWifeScore(float f);
	void SetWifePoints(float f);
	void SetSSRNormPercent(float f);
	void SetMusicRate(float f);
	void SetSurviveSeconds(float f);
	void SetJudgeScale(float f);
	void SetChordCohesion(bool b);
	void SetEtternaValid(bool b);
	void AddUploadedServer(string s);
	void SetOffsetVector(const std::vector<float>& v);
	void SetNoteRowVector(const std::vector<int>& v);
	void SetTrackVector(const std::vector<int>& v);
	void SetTapNoteTypeVector(const std::vector<TapNoteType>& v);
	void SetHoldReplayDataVector(const std::vector<HoldReplayResult>& v);
	void SetOnlineReplayTimestampVector(const std::vector<float>& v);
	void SetScoreKey(const string& ck);
	void SetRescoreJudgeVector(const std::vector<int>& v);
	void SetAliveSeconds( float f );
	void SetMaxCombo( unsigned int i );
	void SetModifiers( const string &s );
	void SetDateTime( DateTime d );
	void SetPlayerGuid( const string &s );
	void SetMachineGuid( const string &s );
	void SetProductID( int i );
	void SetTapNoteScore( TapNoteScore tns, int i );
	void SetHoldNoteScore( HoldNoteScore tns, int i );
	void SetRadarValues( const RadarValues &rv );
	void SetLifeRemainingSeconds( float f );
	void SetDisqualified( bool b );
	void SetReplayType( int i );

	string* GetNameMutable();
	const string* GetNameMutable() const
	{
		return const_cast<string*>(
		  const_cast<HighScore*>(this)->GetNameMutable());
	}

	void Unset();

	bool operator<(HighScore const& other) const;
	bool operator>(HighScore const& other) const;
	bool operator<=(HighScore const& other) const;
	bool operator>=(HighScore const& other) const;
	bool operator==(HighScore const& other) const;
	bool operator!=(HighScore const& other) const;

	XNode* CreateNode() const;
	XNode* CreateEttNode() const;
	void LoadFromNode(const XNode* pNode);
	void LoadFromEttNode(const XNode* pNode);

	bool WriteReplayData();
	bool WriteInputData(const std::vector<float>& oop);
	bool LoadReplayData();
	bool LoadReplayDataBasic();
	bool LoadReplayDataFull();
	virtual bool HasReplayData();
	void UnloadReplayData();
	void ResetSkillsets();

	string GetDisplayName() const;

	// Mina stuff - Mina
	float RescoreToWifeJudge(int x);
	float RescoreToWifeJudgeDuringLoad(int x); // uuugh -mina
	float RescoreToDPJudge(int x);
	float GetSkillsetSSR(Skillset ss) const;
	void SetSkillsetSSR(Skillset ss, float ssr);
	void SetValidationKey(ValidationKey vk, string k);
	void SetTopScore(int i);
	string GenerateValidationKeys();
	string GetValidationKey(ValidationKey vk) const;
	std::vector<int> GetRescoreJudgeVector(int x);
	// laazy
	string scoreid;
	int userid = -1;
	string avatar;
	string countryCode;

	int norms = 0;
	int musics = 0;
	int judges = 0;
	// Lua
	void PushSelf(lua_State* L);

  private:
	HiddenPtr<HighScoreImpl> m_Impl;
};

/** @brief The list of high scores */
struct HighScoreList
{
  public:
	/**
	 * @brief Set up the HighScore List with default values.
	 *
	 * This used to call Init(), but it's better to be explicit here. */
	HighScoreList()
	  : vHighScores()
	  , dtLastPlayed()
	{
	}

	void Init();

	int GetNumTimesPlayed() const { return iNumTimesPlayed; }
	DateTime GetLastPlayed() const
	{
		ASSERT(iNumTimesPlayed >
			   0); // don't call this unless the song has been played
		return dtLastPlayed;
	}
	const HighScore& GetTopScore() const;

	void AddHighScore(HighScore hs, int& iIndexOut, bool bIsMachine);
	void IncrementPlayCount(DateTime dtLastPlayed);
	void RemoveAllButOneOfEachName();

	void MergeFromOtherHSL(HighScoreList& other, bool is_machine);

	std::vector<HighScore> vHighScores;
	Grade HighGrade{ Grade_NoData };

	// Lua
	void PushSelf(lua_State* L);

  private:
	int iNumTimesPlayed{ 0 };
	DateTime dtLastPlayed; // meaningless if iNumTimesPlayed == 0
};

/** @brief the picture taken of the high score. */
struct Screenshot
{
	/** @brief the filename of the screen shot. There is no directory part. */
	RString sFileName;
	/** @brief The MD5 hash of the screen shot file above. */
	RString sMD5;
	/** @brief The actual high score in question. */
	HighScore highScore;

	XNode* CreateNode() const;
	void LoadFromNode(const XNode* pNode);
	bool operator<(Screenshot const& rhs) const
	{
		return highScore.GetDateTime() < rhs.highScore.GetDateTime();
	}

	bool operator==(Screenshot const& rhs) const
	{
		return sFileName == rhs.sFileName;
	}
};

#endif

/**
 * @file
 * @author Chris Danford (c) 2004
 * @section LICENSE
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
