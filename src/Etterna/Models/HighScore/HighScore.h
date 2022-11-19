#ifndef HIGH_SCORE_H
#define HIGH_SCORE_H

#include "Etterna/Models/Misc/DateTime.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "Etterna/Models/HighScore/Replay.h"

class XNode;
struct RadarValues;
struct lua_State;
class Replay;

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
	[[nodiscard]] auto GetName() const -> const std::string&;
	[[nodiscard]] auto GetChartKey() const -> const std::string&;
	[[nodiscard]] auto GetSSRCalcVersion() const -> int;
	/**
	 * @brief Retrieve the grade earned from this score.
	 * @return the grade.
	 */
	[[nodiscard]] auto GetGrade() const -> Grade;
	/**
	 * @brief Retrieve the score earned.
	 * @return the score. */
	[[nodiscard]] auto GetScore() const -> unsigned int;
	/**
	 * @brief Determine if any judgments were tallied during this run.
	 * @return true if no judgments were recorded, false otherwise. */
	[[nodiscard]] auto IsEmpty() const -> bool;
	[[nodiscard]] auto IsEmptyNormalized() const -> bool;
	[[nodiscard]] auto GetWifeGrade() const -> Grade;
	auto ConvertDpToWife() -> float;
	[[nodiscard]] auto GetPercentDP() const -> float;
	[[nodiscard]] auto GetWifeScore() const -> float;
	[[nodiscard]] auto GetWifePoints() const -> float;
	[[nodiscard]] auto GetSSRNormPercent() const -> float;
	[[nodiscard]] auto GetMusicRate() const -> float;
	[[nodiscard]] auto GetSongOffset() const -> float;
	[[nodiscard]] auto GetJudgeScale() const -> float;
	[[nodiscard]] auto GetChordCohesion() const -> bool;
	[[nodiscard]] auto GetEtternaValid() const -> bool;
	[[nodiscard]] auto GetDSFlag() const -> bool;
	[[nodiscard]] auto GetStageSeed() const -> int;
	[[nodiscard]] auto IsUploadedToServer(const std::string& s) const -> bool;
	std::vector<float> timeStamps;
	[[nodiscard]] auto GetOffsetVector() -> const std::vector<float>&;
	[[nodiscard]] auto GetNoteRowVector() -> const std::vector<int>&;
	[[nodiscard]] auto GetTrackVector() -> const std::vector<int>&;
	[[nodiscard]] auto GetTapNoteTypeVector()
	  -> const std::vector<TapNoteType>&;
	[[nodiscard]] auto GetHoldReplayDataVector()
	  -> const std::vector<HoldReplayResult>&;
	[[nodiscard]] auto GetMineReplayDataVector()
	  -> const std::vector<MineReplayResult>&;
	[[nodiscard]] auto GetCopyOfOffsetVector() -> std::vector<float>;
	[[nodiscard]] auto GetCopyOfNoteRowVector() -> std::vector<int>;
	[[nodiscard]] auto GetCopyOfTrackVector() -> std::vector<int>;
	[[nodiscard]] auto GetCopyOfTapNoteTypeVector() -> std::vector<TapNoteType>;
	[[nodiscard]] auto GetCopyOfHoldReplayDataVector()
	  -> std::vector<HoldReplayResult>;
	[[nodiscard]] auto GetCopyOfMineReplayDataVector()
	  -> std::vector<MineReplayResult>;
	[[nodiscard]] auto GetCopyOfSetOnlineReplayTimestampVector()
	  -> std::vector<float>;
	[[nodiscard]] auto GetInputDataVector() -> const std::vector<InputDataEvent>&;
	[[nodiscard]] auto GetMissDataVector()
	  -> const std::vector<MissReplayResult>&;
	[[nodiscard]] auto GetScoreKey() const -> const std::string&;
	[[nodiscard]] auto GetTopScore() const -> int;
	[[nodiscard]] auto GetReplayType() -> ReplayType;
	[[nodiscard]] auto HasColumnData() -> bool;
	[[nodiscard]] auto GetPlayedSeconds() const -> float;
	[[nodiscard]] auto GetMaxCombo() const -> unsigned int;
	/**
	 * @brief Get the modifiers used for this run.
	 * @return the modifiers. */
	[[nodiscard]] auto GetModifiers() const -> const std::string&;
	[[nodiscard]] auto GetDateTime() const -> DateTime;
	[[nodiscard]] auto GetPlayerGuid() const -> const std::string&;
	[[nodiscard]] auto GetMachineGuid() const -> const std::string&;
	[[nodiscard]] auto GetCountryCode() const -> const std::string&;
	[[nodiscard]] auto GetProductID() const -> int;
	[[nodiscard]] auto GetTapNoteScore(TapNoteScore tns) const -> int;
	[[nodiscard]] auto GetTNSNormalized(TapNoteScore tns) const -> int;
	[[nodiscard]] auto GetHoldNoteScore(HoldNoteScore hns) const -> int;
	[[nodiscard]] auto GetRadarValues() const -> const RadarValues&;
	[[nodiscard]] auto GetLifeRemainingSeconds() const -> float;
	/**
	 * @brief Determine if this score was from a situation that would cause
	 * disqualification.
	 * @return true if the score would be disqualified, false otherwise. */
	[[nodiscard]] auto GetDisqualified() const -> bool;

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
	void SetSongOffset(float f);
	void SetPlayedSeconds(float f);
	void SetJudgeScale(float f);
	void SetChordCohesion(bool b);
	void SetEtternaValid(bool b);
	void SetDSFlag(bool b);
	void SetStageSeed(int i);
	void AddUploadedServer(const std::string& s);
	void SetInputDataVector(const std::vector<InputDataEvent>& v);
	void SetMissDataVector(const std::vector<MissReplayResult>& v);
	void SetOffsetVector(const std::vector<float>& v);
	void SetNoteRowVector(const std::vector<int>& v);
	void SetTrackVector(const std::vector<int>& v);
	void SetTapNoteTypeVector(const std::vector<TapNoteType>& v);
	void SetHoldReplayDataVector(const std::vector<HoldReplayResult>& v);
	void SetMineReplayDataVector(const std::vector<MineReplayResult>& v);
	void SetOnlineReplayTimestampVector(const std::vector<float>& v);
	void SetScoreKey(const std::string& sk);
	void SetRescoreJudgeVector(const std::vector<int>& v);
	void SetMaxCombo(unsigned int i);
	void SetModifiers(const std::string& s);
	void SetDateTime(DateTime d);
	void SetPlayerGuid(const std::string& s);
	void SetMachineGuid(const std::string& s);
	void SetProductID(int i);
	void SetTapNoteScore(TapNoteScore tns, int i);
	void SetHoldNoteScore(HoldNoteScore hns, int i);
	void SetRadarValues(const RadarValues& rv);
	void SetLifeRemainingSeconds(float f);
	void SetDisqualified(bool b);

	auto GetNameMutable() -> std::string*;

	[[nodiscard]] auto GetNameMutable() const -> const std::string*
	{
		return const_cast<std::string*>(
		  const_cast<HighScore*>(this)->GetNameMutable());
	}

	auto operator<(HighScore const& other) const -> bool;
	auto operator>(HighScore const& other) const -> bool;
	auto operator<=(HighScore const& other) const -> bool;
	auto operator>=(HighScore const& other) const -> bool;
	auto operator==(HighScore const& other) const -> bool;
	auto operator!=(HighScore const& other) const -> bool;
	auto operator=(const HighScore &) -> HighScore&;

	[[nodiscard]] auto CreateEttNode() const -> XNode*;
	void LoadFromEttNode(const XNode* pNode);

	auto WriteReplayData() -> bool;
	auto WriteInputData() -> bool;
	auto LoadReplayData() -> bool;
	auto LoadReplayDataBasic(const std::string& dir) -> bool;
	auto LoadReplayDataFull(const std::string& dir) -> bool;
	virtual auto HasReplayData() -> bool;
	void InitReplay();
	void UnloadReplayData();
	void ResetSkillsets();

	[[nodiscard]] auto GetDisplayName() const -> const std::string&;

	// Mina stuff - Mina
	auto RescoreToWife2Judge(int x) -> float;
	// update wifescore (judge the score was achieved on) and ssrnorm
	auto RescoreToWife3(float pmax) -> bool;
	auto RescoreToDPJudge(int x) -> float;
	[[nodiscard]] auto GetSkillsetSSR(Skillset ss) const -> float;
	[[nodiscard]] auto GetWifeVersion() const -> int;
	void SetSkillsetSSR(Skillset ss, float ssr);
	void SetValidationKey(ValidationKey vk, std::string k);
	void SetTopScore(int i);
	auto GenerateValidationKeys() -> std::string;
	[[nodiscard]] auto GetValidationKey(ValidationKey vk) const
	  -> const std::string&;
	void SetWifeVersion(int i);
	auto GetRescoreJudgeVector(int x) -> std::vector<int>;
	auto NormalizeJudgments() -> bool;
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
	void PushReplay(lua_State* L)
	{
		CheckReplayIsInit();
		replay->PushSelf(L);
	}
	Replay* replay = nullptr;

  private:
	struct HSImplUniquePtr {
		std::unique_ptr<HighScoreImpl> p;

		HSImplUniquePtr();
		~HSImplUniquePtr();
		HSImplUniquePtr(const HSImplUniquePtr& rhs);
		HSImplUniquePtr(std::unique_ptr<HighScoreImpl> ptr);
		auto operator=(const HSImplUniquePtr& rhs) -> HSImplUniquePtr&;

		HighScoreImpl *operator->() { return &*p; }
		HighScoreImpl &operator*() { return *p; }
		HighScoreImpl const *operator->() const { return &*p; }
		HighScoreImpl const &operator*() const { return *p; }
	};
	HSImplUniquePtr m_Impl;
	void CheckReplayIsInit();
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

	[[nodiscard]] auto CreateNode() const -> XNode*;
	void LoadFromNode(const XNode* pNode);
	auto operator<(Screenshot const& rhs) const -> bool
	{
		return highScore.GetDateTime() < rhs.highScore.GetDateTime();
	}

	auto operator==(Screenshot const& rhs) const -> bool
	{
		return sFileName == rhs.sFileName;
	}
};

#endif
