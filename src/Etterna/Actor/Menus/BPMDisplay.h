#ifndef BPM_DISPLAY_H
#define BPM_DISPLAY_H

#include "Etterna/Actor/Base/BitmapText.h"
#include "Etterna/Models/Misc/ThemeMetric.h"
class Song;
class Steps;
struct DisplayBpms;

/** @brief Displays a BPM or a range of BPMs. */
class BPMDisplay : public BitmapText
{
  public:
	/** @brief Set up the BPM Display with default values. */
	BPMDisplay();
	/** @brief Copy the BPMDisplay to another. */
	BPMDisplay* Copy() const override;
	/** @brief Load the various metrics needed. */
	void Load();
	/**
	 * @brief Update the display as required.
	 * @param fDeltaTime the changed time.
	 */
	void Update(float fDeltaTime) override;
	void LoadFromNode(const XNode* pNode) override;
	/**
	 * @brief Use the BPM[s] from a song.
	 * @param pSong the song in question.
	 */
	void SetBpmFromSong(const Song* pSong, bool bIgnoreCurrentRate = false);
	/**
	 * @brief Use the BPM[s] from a steps.
	 * @param pSteps the steps in question.
	 */
	void SetBpmFromSteps(const Steps* pSteps, bool bIgnoreCurrentRate = false);
	/**
	 * @brief Use a specified, constant BPM.
	 * @param fBPM the constant BPM.
	 */
	void SetConstantBpm(float fBPM);
	/**
	 * @brief Have the BPMDisplay cycle between various BPMs.
	 */
	void CycleRandomly();
	/** @brief Don't use a BPM at all. */
	void NoBPM();
	/** @brief Have the BPMDisplay use various BPMs. */
	void SetVarious();
	/** @brief Have the GameState determine which BPMs to display. */
	void SetFromGameState();

	// Lua
	void PushSelf(lua_State* L) override;

  protected:
	/**
	 * @brief Retrieve the active BPM on display.
	 * @return the active BPM on display.
	 */
	float GetActiveBPM() const;
	/**
	 * @brief Set the range to be used for the display.
	 * @param bpms the set of BPMs to be used.
	 */
	void SetBPMRange(const DisplayBpms& bpms);

	/** @brief The commands to use when there is no BPM. */
	ThemeMetric<apActorCommands> SET_NO_BPM_COMMAND;
	/** @brief The commands to use when there is a normal BPM. */
	ThemeMetric<apActorCommands> SET_NORMAL_COMMAND;
	/** @brief The commands to use when the BPM can change between 2 or more
	 * values. */
	ThemeMetric<apActorCommands> SET_CHANGING_COMMAND;
	/** @brief The commands to use when the BPM is random. */
	ThemeMetric<apActorCommands> SET_RANDOM_COMMAND;
	/** @brief The commands to use if it is an extra stage. */
	ThemeMetric<apActorCommands> SET_EXTRA_COMMAND;
	/** @brief A flag to determine if the BPMs cycle from low to high or just
	 * display both. */
	ThemeMetric<bool> CYCLE;
	/** @brief A flag to determine if QUESTIONMARKS_TEXT is shown. */
	ThemeMetric<bool> SHOW_QMARKS;
	/** @brief How often the random BPMs cycle themselves. */
	ThemeMetric<float> RANDOM_CYCLE_SPEED;
	/** @brief The text used to separate the low and high BPMs. */
	ThemeMetric<std::string> SEPARATOR;
	/** @brief The text used when there is no BPM. */
	ThemeMetric<std::string> NO_BPM_TEXT;
	/** @brief The text used when there are various BPMs for the song. */
	ThemeMetric<std::string> VARIOUS_TEXT;
	/** @brief The text used when it is a random BPM. */
	ThemeMetric<std::string> RANDOM_TEXT;
	/** @brief The text used as one possible option for random BPM. */
	ThemeMetric<std::string> QUESTIONMARKS_TEXT;
	/** @brief The format string used for the numbers. */
	ThemeMetric<std::string> BPM_FORMAT_STRING;

	/** @brief The lowest valued BPM. */
	float m_fBPMFrom;
	/** @brief The highest valued BPM. */
	float m_fBPMTo;
	/** @brief The current BPM index used. */
	int m_iCurrentBPM;
	/** @brief The list of BPMs. */
	std::vector<float> m_BPMS;
	float m_fPercentInState;
	/** @brief How long it takes to cycle the various BPMs. */
	float m_fCycleTime;
};

#endif
