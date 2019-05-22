#ifndef STEPS_H
#define STEPS_H

#include "Etterna/Models/Misc/Difficulty.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"
#include "Etterna/Models/Misc/Grade.h"
#include "Etterna/Models/Misc/PlayerNumber.h"
#include "Etterna/Models/Misc/RadarValues.h"
#include "RageUtil/Utils/RageUtil_AutoPtr.h"
#include "RageUtil/Utils/RageUtil_CachedObject.h"
#include "Etterna/Models/Misc/TimingData.h"

class Profile;
class NoteData;
struct lua_State;
class Song;

typedef std::vector<float> SDiffs;
typedef std::vector<SDiffs> MinaSD;
using std::string;

/**
 * @brief Enforce a limit on the number of chars for the description.
 *
 * In In The Groove, this limit was 12: we do not need such a limit now.
 */
const int MAX_STEPS_DESCRIPTION_LENGTH = 255;

/** @brief The different ways of displaying the BPM. */
enum DisplayBPM
{
	DISPLAY_BPM_ACTUAL,	/**< Display the song's actual BPM. */
	DISPLAY_BPM_SPECIFIED, /**< Display a specified value or values. */
	DISPLAY_BPM_RANDOM,	/**< Display a random selection of BPMs. */
	NUM_DisplayBPM,
	DisplayBPM_Invalid
};
const RString&
DisplayBPMToString(DisplayBPM dbpm);
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

	// initializers
	void CopyFrom(Steps* pSource, StepsType ntTo, float fMusicLengthSeconds);
	void CreateBlank(StepsType ntTo);

	void Compress() const;
	void Decompress() const;
	void Decompress();

	/**
	 * @brief Determine if these steps were created by the autogenerator.
	 * @return true if they were, false otherwise.
	 */

	/**
	 * @brief Determine if this set of Steps is an edit.
	 *
	 * Edits have a special value of difficulty to make it easy to determine.
	 * @return true if this is an edit, false otherwise.
	 */
	bool IsAnEdit() const { return m_Difficulty == Difficulty_Edit; }
	/**
	 * @brief Determine if this set of Steps is a player edit.
	 *
	 * Player edits also have to be loaded from a player's profile slot, not the
	 * machine.
	 * @return true if this is a player edit, false otherwise. */
	bool IsAPlayerEdit() const { return IsAnEdit(); }
	/**
	 * @brief Determine if these steps were loaded from a player's profile.
	 * @return true if they were from a player profile, false otherwise.
	 */
	bool WasLoadedFromProfile() const
	{
		return m_LoadedFromProfile != ProfileSlot_Invalid;
	}
	ProfileSlot GetLoadedFromProfileSlot() const { return m_LoadedFromProfile; }
	/**
	 * @brief Retrieve the description used for this edit.
	 * @return the description used for this edit.
	 */
	RString GetDescription() const { return m_sDescription; }
	/**
	 * @brief Retrieve the ChartStyle used for this chart.
	 * @return the description used for this chart.
	 */
	RString GetChartStyle() const { return m_sChartStyle; }
	/**
	 * @brief Retrieve the difficulty used for this edit.
	 * @return the difficulty used for this edit.
	 */
	Difficulty GetDifficulty() const { return m_Difficulty; }
	/**
	 * @brief Retrieve the meter used for this edit.
	 * @return the meter used for this edit.
	 */
	int GetMeter() const { return m_iMeter; }
	const RadarValues& GetRadarValues() const { return m_CachedRadarValues; }
	/**
	 * @brief Retrieve the author credit used for this edit.
	 * @return the author credit used for this edit.
	 */
	RString GetCredit() const { return m_sCredit; }

	RString GetChartName() const { return chartName; }
	void SetChartName(const RString& name) { this->chartName = name; }
	void SetFilename(const RString& fn) { m_sFilename = fn; }
	RString GetFilename() const { return m_sFilename; }
	void SetSavedToDisk(bool b) { m_bSavedToDisk = b; }
	bool GetSavedToDisk() const { return m_bSavedToDisk; }
	void SetDifficulty(Difficulty dc)
	{
		SetDifficultyAndDescription(dc, GetDescription());
	}
	void SetDescription(const RString& sDescription)
	{
		SetDifficultyAndDescription(this->GetDifficulty(), sDescription);
	}
	void SetDifficultyAndDescription(Difficulty dc,
									 const RString& sDescription);
	void SetCredit(const RString& sCredit);
	void SetChartStyle(const RString& sChartStyle);
	static bool MakeValidEditDescription(
	  RString& sPreferredDescription); // return true if was modified

	void SetLoadedFromProfile(ProfileSlot slot) { m_LoadedFromProfile = slot; }
	void SetMeter(int meter);
	void SetCachedRadarValues(const RadarValues& v);

	// self exaplanatory -mina
	std::vector<int> GetNPSVector(NoteData& nd,
							 std::vector<int> nerv,
							 std::vector<float> etaner,
							 float rate);
	// takes size of chord and counts how many -NOTES- are in
	// chords of that exact size (this functionally means
	// multiplying chord counter by chord size) in a row -mina
	// (jumps won't count as hands, etc)
	std::vector<int> GetCNPSVector(NoteData& nd,
							  std::vector<int> nerv,
							  std::vector<float> etaner,
							  int chordsize,
							  float rate);
	float PredictMeter() const { return 1.f; }

	unsigned GetHash() const;
	void GetNoteData(NoteData& noteDataOut) const;
	NoteData GetNoteData() const;
	void SetNoteData(const NoteData& noteDataNew);
	void SetSMNoteData(const RString& notes_comp);
	void GetSMNoteData(RString& notes_comp_out) const;

	/**
	 * @brief Retrieve the NoteData from the original source.
	 * @return true if successful, false for failure. */
	bool GetNoteDataFromSimfile();

	/**
	 * @brief Determine if we are missing any note data.
	 *
	 * This takes advantage of the fact that we usually compress our data.
	 * @return true if our notedata is empty, false otherwise. */
	bool IsNoteDataEmpty() const;

	void GetETTNoteData(RString& notes_comp_out) const;
	void TidyUpData();
	void CalculateRadarValues(float fMusicLengthSeconds);

	/**
	 * @brief The TimingData used by the Steps.
	 *
	 * This is required to allow Split Timing. */
	TimingData m_Timing;

	/**
	 * @brief Retrieves the appropriate timing data for the Steps.  Falls
	 * back on the Song if needed. */
	const TimingData* GetTimingData() const;
	TimingData* GetTimingData()
	{
		return const_cast<TimingData*>(
		  static_cast<const Steps*>(this)->GetTimingData());
	};

	/* Now for half the reason I'm bothering to do this... generate a chart key
	using note data and timingdata in conjuction. Do it during load and save it
	in the steps data so that we have to do it as few times as possible.*/
	const string& GetChartKey() const { return ChartKey; }
	std::vector<float> thestuffs = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
	MinaSD stuffnthings = { thestuffs, thestuffs, thestuffs, thestuffs,
							thestuffs, thestuffs, thestuffs, thestuffs,
							thestuffs, thestuffs, thestuffs, thestuffs,
							thestuffs, thestuffs, thestuffs, thestuffs,
							thestuffs, thestuffs, thestuffs, thestuffs,
							thestuffs };
	void SetChartKey(const RString& k) { ChartKey = k; }
	void SetAllMSD(const MinaSD& msd) { stuffnthings = msd; }
	MinaSD GetAllMSD() const { return stuffnthings; }
	std::map<float, Skillset> SortSkillsetsAtRate(float x, bool includeoverall);

	void CalcEtternaMetadata();

	string GenerateBustedChartKey(NoteData& nd, TimingData* td, int cores);
	std::vector<string> bustedkeys;
	void MakeBustedKeys();

	// you are all idiots for not just doing this in the first place -mina
	float firstsecond = 0.f;
	float lastsecond = 0.f;

	// this is bugged and returns true for files with negative bpms when it
	// shouldn't - mina
	bool IsRecalcValid();

	// prolly needs an enum or something idk - mina
	float GetMSD(float x, int i) const;

	/* This is a reimplementation of the lua version of the script to generate
	chart keys, except this time using the notedata stored in game memory
	immediately after reading it than parsing it using lua. - Mina */
	RString GenerateChartKey(NoteData& nd, TimingData* td);

	/* Append all of the bpms in the given range to the input string */
	void FillStringWithBPMs(size_t startRow,
							size_t endRow,
							std::vector<int>& nerv,
							NoteData& nd,
							TimingData* td,
							RString& inOut);

	/**
	 * @brief Determine if the Steps have any major timing changes during
	 * gameplay.
	 * @return true if it does, or false otherwise. */
	bool HasSignificantTimingChanges() const;

	/**
	 * @brief Determine if the Steps have any attacks.
	 * @return true if it does, or false otherwise. */
	bool HasAttacks() const;

	const RString GetMusicPath() const; // Returns the path for loading.
	const RString& GetMusicFile()
	  const; // Returns the filename for the simfile.
	void SetMusicFile(const RString& file);

	// Lua
	void PushSelf(lua_State* L);

	StepsType m_StepsType;
	/** @brief The string form of the StepsType, for dealing with unrecognized
	 * styles. */
	RString m_StepsTypeStr;
	/** @brief The Song these Steps are associated with */
	Song* m_pSong;

	CachedObject<Steps> m_CachedObject;

	void SetDisplayBPM(const DisplayBPM type) { this->displayBPMType = type; }
	DisplayBPM GetDisplayBPM() const { return this->displayBPMType; }
	void SetMinBPM(const float f) { this->specifiedBPMMin = f; }
	float GetMinBPM() const { return this->specifiedBPMMin; }
	void SetMaxBPM(const float f) { this->specifiedBPMMax = f; }
	float GetMaxBPM() const { return this->specifiedBPMMax; }
	void GetDisplayBpms(DisplayBpms& addTo) const;

  private:
	string ChartKey = "";
	/* We can have one or both of these; if we have both, they're always
	 * identical. Call Compress() to force us to only have
	 * m_sNoteDataCompressed; otherwise, creation of these is transparent. */
	mutable HiddenPtr<NoteData> m_pNoteData;
	mutable bool m_bNoteDataIsFilled;
	mutable RString m_sNoteDataCompressed;

	/** @brief The name of the file where these steps are stored. */
	RString m_sFilename;
	/** @brief true if these Steps were loaded from or saved to disk. */
	bool m_bSavedToDisk;
	/** @brief allows the steps to specify their own music file. */
	RString m_MusicFile;
	/** @brief What profile was used? This is ProfileSlot_Invalid if not from a
	 * profile. */
	ProfileSlot m_LoadedFromProfile;

	/* These values are pulled from the autogen source first, if there is one.
	 */
	/** @brief The hash of the steps. This is used only for Edit Steps. */
	mutable unsigned m_iHash;
	/** @brief The name of the edit, or some other useful description.
	 This used to also contain the step author's name. */
	RString m_sDescription;
	/** @brief The style of the chart. (e.g. "Pad", "Keyboard") */
	RString m_sChartStyle;
	/** @brief The difficulty that these steps are assigned to. */
	Difficulty m_Difficulty;
	/** @brief The numeric difficulty of the Steps, ranging from MIN_METER to
	 * MAX_METER. */
	int m_iMeter;
	/** @brief The radar values used for each player. */
	RadarValues m_CachedRadarValues;
	bool m_bAreCachedRadarValuesJustLoaded;
	/** @brief The name of the person who created the Steps. */
	RString m_sCredit;
	/** @brief The name of the chart. */
	RString chartName;
	/** @brief How is the BPM displayed for this chart? */
	DisplayBPM displayBPMType;
	/** @brief What is the minimum specified BPM? */
	float specifiedBPMMin;
	/**
	 * @brief What is the maximum specified BPM?
	 * If this is a range, then min should not be equal to max. */
	float specifiedBPMMax;
};

#endif

/**
 * @file
 * @author Chris Danford, Glenn Maynard (c) 2001-2004
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
