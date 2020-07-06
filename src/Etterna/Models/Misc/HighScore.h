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
	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::string& GetChartKey() const;
	[[nodiscard]] int GetSSRCalcVersion() const;
	/**
	 * @brief Retrieve the grade earned from this score.
	 * @return the grade.
	 */
	[[nodiscard]] Grade GetGrade() const;
	/**
	 * @brief Retrieve the score earned.
	 * @return the score. */
	[[nodiscard]] unsigned int GetScore() const;
	/**
	 * @brief Determine if any judgments were tallied during this run.
	 * @return true if no judgments were recorded, false otherwise. */
	[[nodiscard]] bool IsEmpty() const;
	[[nodiscard]] Grade GetWifeGrade() const;
	float ConvertDpToWife();
	[[nodiscard]] float GetPercentDP() const;
	[[nodiscard]] float GetWifeScore() const;
	[[nodiscard]] float GetWifePoints() const;
	[[nodiscard]] float GetSSRNormPercent() const;
	[[nodiscard]] float GetMusicRate() const;
	[[nodiscard]] float GetJudgeScale() const;
	[[nodiscard]] bool GetChordCohesion() const;
	[[nodiscard]] bool GetEtternaValid() const;
	[[nodiscard]] bool IsUploadedToServer(const std::string& s) const;
	std::vector<float> timeStamps;
	[[nodiscard]] const std::vector<float>& GetOffsetVector() const;
	[[nodiscard]] const std::vector<int>& GetNoteRowVector() const;
	[[nodiscard]] const std::vector<int>& GetTrackVector() const;
	[[nodiscard]] const std::vector<TapNoteType>& GetTapNoteTypeVector() const;
	[[nodiscard]] const std::vector<HoldReplayResult>& GetHoldReplayDataVector()
	  const;
	[[nodiscard]] std::vector<float> GetCopyOfOffsetVector() const;
	[[nodiscard]] std::vector<int> GetCopyOfNoteRowVector() const;
	[[nodiscard]] std::vector<int> GetCopyOfTrackVector() const;
	[[nodiscard]] std::vector<TapNoteType> GetCopyOfTapNoteTypeVector() const;
	[[nodiscard]] std::vector<HoldReplayResult> GetCopyOfHoldReplayDataVector()
	  const;
	[[nodiscard]] std::vector<float> GetCopyOfSetOnlineReplayTimestampVector()
	  const;
	[[nodiscard]] const std::string& GetScoreKey() const;
	[[nodiscard]] int GetTopScore() const;
	[[nodiscard]] int GetReplayType() const;
	/**
	 * @brief Determine how many seconds the player had left in Survival mode.
	 * @return the number of seconds left. */
	[[nodiscard]] float GetSurviveSeconds() const;
	[[nodiscard]] float GetSurvivalSeconds() const;
	[[nodiscard]] unsigned int GetMaxCombo() const;
	/**
	 * @brief Get the modifiers used for this run.
	 * @return the modifiers. */
	[[nodiscard]] const std::string& GetModifiers() const;
	[[nodiscard]] DateTime GetDateTime() const;
	[[nodiscard]] const std::string& GetPlayerGuid() const;
	[[nodiscard]] const std::string& GetMachineGuid() const;
	[[nodiscard]] const std::string& GetCountryCode() const;
	[[nodiscard]] int GetProductID() const;
	[[nodiscard]] int GetTapNoteScore(TapNoteScore tns) const;
	[[nodiscard]] int GetHoldNoteScore(HoldNoteScore tns) const;
	[[nodiscard]] const RadarValues& GetRadarValues() const;
	[[nodiscard]] float GetLifeRemainingSeconds() const;
	/**
	 * @brief Determine if this score was from a situation that would cause
	 * disqualification.
	 * @return true if the score would be disqualified, false otherwise. */
	[[nodiscard]] bool GetDisqualified() const;

	/**
	 * @brief Set the name of the Player that earned the score.
	 * @param sName the name of the Player. */
	void SetName(const std::string& sName);
	void SetChartKey(const std::string& ck);
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
	void SetScoreKey(const std::string& ck);
	void SetRescoreJudgeVector(const std::vector<int>& v);
	void SetAliveSeconds(float f);
	void SetMaxCombo(unsigned int i);
	void SetModifiers(const std::string& s);
	void SetDateTime(DateTime d);
	void SetPlayerGuid(const std::string& s);
	void SetMachineGuid(const std::string& s);
	void SetProductID(int i);
	void SetTapNoteScore(TapNoteScore tns, int i);
	void SetHoldNoteScore(HoldNoteScore tns, int i);
	void SetRadarValues(const RadarValues& rv);
	void SetLifeRemainingSeconds(float f);
	void SetDisqualified(bool b);
	void SetReplayType(int i);

	std::string* GetNameMutable();

	[[nodiscard]] const std::string* GetNameMutable() const
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

	[[nodiscard]] XNode* CreateNode() const;
	[[nodiscard]] XNode* CreateEttNode() const;
	void LoadFromNode(const XNode* pNode);
	void LoadFromEttNode(const XNode* pNode);

	bool WriteReplayData();
	bool WriteInputData(const std::vector<float>& oop);
	bool LoadReplayData();
	bool LoadReplayDataBasic(string dir);
	bool LoadReplayDataFull(string dir);
	virtual bool HasReplayData();
	void UnloadReplayData();
	void ResetSkillsets();

	[[nodiscard]] const std::string& GetDisplayName() const;

	// Mina stuff - Mina
	float RescoreToWife2Judge(int x);
	// update wifescore (judge the score was achieved on) and ssrnorm
	bool RescoreToWife3(float pmax);
	float RescoreToDPJudge(int x);
	[[nodiscard]] float GetSkillsetSSR(Skillset ss) const;
	[[nodiscard]] int GetWifeVersion() const;
	void SetSkillsetSSR(Skillset ss, float ssr);
	void SetValidationKey(ValidationKey vk, std::string k);
	void SetTopScore(int i);
	std::string GenerateValidationKeys();
	[[nodiscard]] const std::string& GetValidationKey(ValidationKey vk) const;
	std::vector<int> GetRescoreJudgeVector(int x);
	// laazy
	std::string scoreid;
	int userid = -1;
	std::string avatar;
	std::string countryCode;
	bool forceuploadedthissession = false;
	int norms = 0;
	int musics = 0;
	int judges = 0;
	// Lua
	void PushSelf(lua_State* L);

  private:
	HiddenPtr<HighScoreImpl> m_Impl;
};

/** @brief the picture taken of the high score. */
struct Screenshot
{
	/** @brief the filename of the screen shot. There is no directory part. */
	std::string sFileName;
	/** @brief The MD5 hash of the screen shot file above. */
	std::string sMD5;
	/** @brief The actual high score in question. */
	HighScore highScore;

	[[nodiscard]] XNode* CreateNode() const;
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
