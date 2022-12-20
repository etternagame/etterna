#ifndef STEPS_H
#define STEPS_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "Etterna/Models/Misc/RadarValues.h"
#include "RageUtil/Utils/RageUtil_CachedObject.h"
#include "Etterna/Models/Misc/TimingData.h"
#include "Etterna/Models/NoteData/NoteData.h"

class Profile;
struct lua_State;
class Song;
class Calc;

/**
 * @brief Enforce a limit on the number of chars for the description.
 *
 * In In The Groove, this limit was 12: we do not need such a limit now.
 */
const int MAX_STEPS_DESCRIPTION_LENGTH = 255;

/** @brief The different ways of displaying the BPM. */
enum DisplayBPM
{
	DISPLAY_BPM_ACTUAL,	   /**< Display the song's actual BPM. */
	DISPLAY_BPM_SPECIFIED, /**< Display a specified value or values. */
	DISPLAY_BPM_RANDOM,	   /**< Display a random selection of BPMs. */
	NUM_DisplayBPM,
	DisplayBPM_Invalid
};
auto
DisplayBPMToString(DisplayBPM x) -> const std::string&;
LuaDeclareType(DisplayBPM);

/**
 * @brief Holds note information for a Song.
 *
 * A Song may have one or more Notes. */
class Steps
{
  public:
	/** @brief Set up the Steps with initial values. */
	Steps(Song* song);
	/** @brief Destroy the Steps that are no longer needed. */
	~Steps();

	auto operator=(const Steps &) -> Steps&;

	// initializers
	void CopyFrom(Steps* pSource, StepsType ntTo);
	void CreateBlank(StepsType ntTo);

	void Compress() const;
	void Decompress() const;
	void Decompress();

	/**
	 * @brief Retrieve the description used for this edit.
	 * @return the description used for this edit.
	 */
	auto GetDescription() const -> const std::string& { return m_sDescription; }
	/**
	 * @brief Retrieve the ChartStyle used for this chart.
	 * @return the description used for this chart.
	 */
	auto GetChartStyle() const -> const std::string& { return m_sChartStyle; }
	/**
	 * @brief Retrieve the difficulty used for this edit.
	 * @return the difficulty used for this edit.
	 */
	auto GetDifficulty() const -> const Difficulty { return m_Difficulty; }
	/**
	 * @brief Retrieve the meter used for this edit.
	 * @return the meter used for this edit.
	 */
	auto GetMeter() const -> int { return m_iMeter; }
	auto GetRadarValues() const -> const RadarValues&
	{
		return m_CachedRadarValues;
	}
	/**
	 * @brief Retrieve the author credit used for this edit.
	 * @return the author credit used for this edit.
	 */
	auto GetCredit() const -> const std::string& { return m_sCredit; }

	auto GetChartName() const -> const std::string& { return chartName; }
	void SetChartName(const std::string& name) { this->chartName = name; }
	void SetFilename(const std::string& fn) { m_sFilename = fn; }
	auto GetFilename() const -> const std::string& { return m_sFilename; }
	void SetSavedToDisk(bool b) { m_bSavedToDisk = b; }
	auto GetSavedToDisk() const -> bool { return m_bSavedToDisk; }
	void SetDifficulty(Difficulty dc)
	{
		SetDifficultyAndDescription(dc, GetDescription());
	}
	void SetDescription(const std::string& sDescription)
	{
		SetDifficultyAndDescription(this->GetDifficulty(), sDescription);
	}
	void SetDifficultyAndDescription(Difficulty dc,
									 const std::string& sDescription);
	void SetCredit(const std::string& sCredit);
	void SetChartStyle(const std::string& sChartStyle);
	void SetDupeDiff(bool state) { m_bDuplicateDifficulty = state; }
	auto IsDupeDiff() -> bool { return m_bDuplicateDifficulty; }
	static auto MakeValidEditDescription(std::string& sPreferredDescription)
	  -> bool; // return true if was modified

	void SetLoadedFromProfile(ProfileSlot slot) { m_LoadedFromProfile = slot; }
	void SetMeter(int meter);
	void SetCachedRadarValues(const RadarValues& v);

	// self exaplanatory -mina
	static auto GetNPSVector(const NoteData& nd,
							 const std::vector<float>& etaner,
							 const std::vector<int>& nerv,
							 float rate) -> std::vector<int>;

	auto GetNPSPerMeasure(const NoteData& nd,
						  const std::vector<float>& etaner,
						  const std::vector<int>& nerv,
						  float rate) -> std::vector<float>;

	// takes size of chord and counts how many -NOTES- are in
	// chords of that exact size (this functionally means
	// multiplying chord counter by chord size) in a row -mina
	// (jumps won't count as hands, etc)
	static auto GetCNPSVector(const NoteData& nd,
							  const std::vector<int>& nerv,
							  const std::vector<float>& etaner,
							  int chordsize,
							  float rate) -> std::vector<int>;

	auto GetHash() const -> unsigned;
	void GetNoteData(NoteData& noteDataOut) const;
	auto GetNoteData() const -> NoteData;
	void SetNoteData(const NoteData& noteDataNew) const;
	void SetSMNoteData(const std::string& notes_comp);
	void GetSMNoteData(std::string& notes_comp_out) const;

	/**
	 * @brief Retrieve the NoteData from the original source.
	 * @return true if successful, false for failure. */
	auto GetNoteDataFromSimfile() -> bool;

	/**
	 * @brief Determine if we are missing any note data.
	 *
	 * This takes advantage of the fact that we usually compress our data.
	 * @return true if our notedata is empty, false otherwise. */
	auto IsNoteDataEmpty() const -> bool;

	void GetETTNoteData(std::string& notes_comp_out) const;
	void TidyUpData();
	void CalculateRadarValues();

	/**
	 * @brief The TimingData used by the Steps.
	 *
	 * This is required to allow Split Timing. */
	TimingData m_Timing;

	/**
	 * @brief Retrieves the appropriate timing data for the Steps.  Falls
	 * back on the Song if needed. */
	auto GetTimingData() const -> const TimingData*;
	auto GetTimingData() -> TimingData*
	{
		return const_cast<TimingData*>(
		  static_cast<const Steps*>(this)->GetTimingData());
	};

	/* Now for half the reason I'm bothering to do this... generate a chart key
	using note data and timingdata in conjuction. Do it during load and save it
	in the steps data so that we have to do it as few times as possible.*/
	auto GetChartKey() const -> const std::string& { return ChartKey; }
	std::vector<float> dummy = { 0.F, 0.F, 0.F, 0.F, 0.F, 0.F, 0.F, 0.F };
	std::vector<std::vector<float>> diffByRate = {
		dummy, dummy, dummy, dummy, dummy, dummy, dummy,
		dummy, dummy, dummy, dummy, dummy, dummy, dummy,
		dummy, dummy, dummy, dummy, dummy, dummy, dummy
	};
	void SetChartKey(const std::string& k) { ChartKey = k; }
	void SetAllMSD(const std::vector<std::vector<float>>& msd)
	{
		diffByRate = msd;
	}
	auto GetAllMSD() const -> std::vector<std::vector<float>>
	{
		return diffByRate;
	}
	auto SortSkillsetsAtRate(float x, bool includeoverall)
	  -> std::vector<std::pair<Skillset, float>>;

	void CalcEtternaMetadata(Calc* calc = nullptr);
	auto DoATestThing(float ev, Skillset ss, float rate, Calc* calc) -> float;
	void GetCalcDebugOutput(); // now spits out everything with 1 calc call
	std::vector<std::vector<std::vector<std::vector<float>>>>
	  calcdebugoutput; // probably should clear this periodically
	void UnloadCalcDebugOutput();

	float firstsecond = 0.F;
	float lastsecond = 0.F;

	// this is bugged and returns true for files with negative bpms when it
	// shouldn't - mina
	auto IsRecalcValid() -> bool;

	auto GetMSD(float rate, int ss) const -> float
	{
		return GetMSD(rate, static_cast<Skillset>(ss));
	}
	auto GetMSD(float rate, Skillset ss) const -> float;

	/* This is a reimplementation of the lua version of the script to generate
	chart keys, except this time using the notedata stored in game memory
	immediately after reading it than parsing it using lua. - Mina */
	static auto GenerateChartKey(NoteData& nd, TimingData* td) -> std::string;

	/**
	 * @brief Determine if the Steps have any major timing changes during
	 * gameplay.
	 * @return true if it does, or false otherwise. */
	auto HasSignificantTimingChanges() const -> bool;

	auto IsPlayableForCurrentGame() const -> bool;

	auto GetMusicPath() const
	  -> const std::string; // Returns the path for loading.
	auto GetMusicFile() const
	  -> const std::string&; // Returns the filename for the simfile.
	void SetMusicFile(const std::string& file);

	// Lua
	void PushSelf(lua_State* L);

	StepsType m_StepsType;
	/** @brief The std::string form of the StepsType, for dealing with
	 * unrecognized styles. */
	std::string m_StepsTypeStr;
	/** @brief The Song these Steps are associated with */
	Song* m_pSong;

	std::vector<NoteInfo> serializenotedatacache;

	CachedObject<Steps> m_CachedObject;

	void SetDisplayBPM(const DisplayBPM type) { this->displayBPMType = type; }
	auto GetDisplayBPM() const -> DisplayBPM { return this->displayBPMType; }
	void SetMinBPM(const float f) { this->specifiedBPMMin = f; }
	auto GetMinBPM() const -> float { return this->specifiedBPMMin; }
	void SetMaxBPM(const float f) { this->specifiedBPMMax = f; }
	void SetFirstSecond(const float f) { this->firstsecond = f; }
	void SetLastSecond(const float f) { this->lastsecond = f; }
	auto GetMaxBPM() const -> float { return this->specifiedBPMMax; }
	void GetDisplayBpms(DisplayBpms& addTo,
						bool bIgnoreCurrentRate = false) const;
	/** @brief Returns length of step in seconds. If a rate is supplied, the
	 * returned length is scaled by it.*/
	auto GetLengthSeconds(float rate = 1) const -> float
	{
		return (lastsecond - firstsecond) / rate;
	}

	auto Getdebugstrings() -> const std::vector<std::string>&
	{
		return debugstrings;
	}
	auto IsSkillsetHighestOfChart(Skillset skill, float rate) -> bool;

  private:
	std::string ChartKey = "";
	struct UniquePtrNoteData {
		std::unique_ptr<NoteData> p;
		UniquePtrNoteData(): p(std::make_unique<NoteData>()) { }
		UniquePtrNoteData(UniquePtrNoteData& rhs) {
			p = rhs.p ? std::make_unique<NoteData>(*rhs.p) : nullptr;
		}
		UniquePtrNoteData &operator=(const UniquePtrNoteData& rhs) {
			p = rhs.p ? std::make_unique<NoteData>(*rhs.p) : nullptr;
			return *this;
		}
		NoteData *operator->() { return &*p; }
		NoteData &operator*() { return *p; }
	};
	/* We can have one or both of these; if we have both, they're always
	 * identical. Call Compress() to force us to only have
	 * m_sNoteDataCompressed; otherwise, creation of these is transparent. */
	mutable UniquePtrNoteData m_pNoteData;
	mutable bool m_bNoteDataIsFilled;
	mutable std::string m_sNoteDataCompressed;

	/** @brief The name of the file where these steps are stored. */
	std::string m_sFilename;
	/** @brief true if these Steps were loaded from or saved to disk. */
	bool m_bSavedToDisk;
	/** @brief allows the steps to specify their own music file. */
	std::string m_MusicFile;
	/** @brief What profile was used? This is ProfileSlot_Invalid if not from a
	 * profile. */
	ProfileSlot m_LoadedFromProfile;

	/* These values are pulled from the autogen source first, if there is one.
	 */
	/** @brief The hash of the steps. This is used only for Edit Steps. */
	mutable unsigned m_iHash;
	/** @brief The name of the edit, or some other useful description.
	 This used to also contain the step author's name. */
	std::string m_sDescription;
	/** @brief The style of the chart. (e.g. "Pad", "Keyboard") */
	std::string m_sChartStyle;
	/** @brief The difficulty that these steps are assigned to. */
	Difficulty m_Difficulty;
	/** @brief The numeric difficulty of the Steps, ranging from MIN_METER to
	 * MAX_METER. */
	int m_iMeter;
	/** @brief The radar values used for each player. */
	RadarValues m_CachedRadarValues;
	bool m_bAreCachedRadarValuesJustLoaded;
	/** @brief The name of the person who created the Steps. */
	std::string m_sCredit;
	/** @brief The name of the chart. */
	std::string chartName;
	/** @brief How is the BPM displayed for this chart? */
	DisplayBPM displayBPMType;
	/** @brief What is the minimum specified BPM? */
	float specifiedBPMMin;
	/**
	 * @brief What is the maximum specified BPM?
	 * If this is a range, then min should not be equal to max. */
	float specifiedBPMMax;

	bool m_bDuplicateDifficulty = false;
	std::vector<std::string> debugstrings;
};

#endif
