/** @brief SSCLoader - Reads a Song and its Steps from a .SSC file. */
#ifndef NotesLoaderSSC_H
#define NotesLoaderSSC_H

#include "GameConstantsAndTypes.h"
#include "NotesLoaderSM.h"

class MsdFile;
class Song;
class Steps;
class TimingData;
class SSCLoader;
/**
 * @brief The various states while parsing a .ssc file.
 */
enum SSCLoadingStates
{
	GETTING_SONG_INFO, /**< Retrieving song information. */
	GETTING_STEP_INFO, /**< Retrieving step information. */
	NUM_SSCLoadingStates /**< The number of states used. */
};

// LoadNoteDataFromSimfile uses LoadNoteDataTagIDs because its parts operate
// on state variables internal to the function.
namespace SSC {
	enum LoadNoteDataTagIDs
	{
		LNDID_version,
		LNDID_stepstype,
		LNDID_chartname,
		LNDID_description,
		LNDID_difficulty,
		LNDID_meter,
		LNDID_credit,
		LNDID_notes,
		LNDID_notes2,
		LNDID_notedata
	};

	// LoadNoteDataFromSimfile uses LoadNoteDataTagIDs because its parts operate
	// on state variables internal to the function.struct StepsTagInfo
	struct StepsTagInfo
	{
		SSCLoader* loader;
		Song* song;
		Steps* steps;
		TimingData* timing;
		const MsdFile::value_t* params;
		const RString& path;
		bool has_own_timing;
		bool ssc_format;
		bool from_cache;
		bool for_load_edit;
		StepsTagInfo(SSCLoader* l, Song* s, const RString& p, bool fc)
			:loader(l), song(s), path(p), has_own_timing(false), ssc_format(false),
			from_cache(fc), for_load_edit(false)
		{}
	};
	struct SongTagInfo
	{
		SSCLoader* loader;
		Song* song;
		const MsdFile::value_t* params;
		const RString& path;
		bool from_cache;
		SongTagInfo(SSCLoader* l, Song* s, const RString& p, bool fc)
			:loader(l), song(s), path(p), from_cache(fc)
		{}
	};
	vector<float> msdsplit(const RString& s);
}
/** @brief The version where fakes started to be used as a radar category. */
const float VERSION_RADAR_FAKE = 0.53f;
/** @brief The version where WarpSegments started to be utilized. */
const float VERSION_WARP_SEGMENT = 0.56f;
/** @brief The version that formally introduced Split Timing. */
const float VERSION_SPLIT_TIMING = 0.7f;
/** @brief The version that moved the step's Offset higher up. */
const float VERSION_OFFSET_BEFORE_ATTACK = 0.72f;
/** @brief The version that introduced the Chart Name tag. */
const float VERSION_CHART_NAME_TAG = 0.74f;
/** @brief The version that introduced the cache switch tag. */
const float VERSION_CACHE_SWITCH_TAG = 0.77f;
/** @brief The version where note count was added as a radar category. */
const float VERSION_RADAR_NOTECOUNT = 0.83f;

/**
 * @brief The SSCLoader handles all of the parsing needed for .ssc files.
 */
struct SSCLoader : public SMLoader
{
	SSCLoader() : SMLoader(".ssc") {}
	
	/**
	 * @brief Attempt to load the specified ssc file.
	 * @param sPath a const reference to the path on the hard drive to check.
	 * @param out a reference to the Song that will retrieve the song information.
	 * @param bFromCache a check to see if we are getting certain information from the cache file.
	 * @return its success or failure.
	 */
	bool LoadFromSimfile( const RString &sPath, Song &out, bool bFromCache = false ) override;
	
	/**
	 * @brief Attempt to load an edit from the hard drive.
	 * @param sEditFilePath a path on the hard drive to check.
	 * @param slot the Profile of the user with the edit.
	 * @param bAddStepsToSong a flag to determine if we add the edit steps to the song file.
	 * @return its success or failure.
	 */
	bool LoadEditFromFile( const RString &sEditFilePath, ProfileSlot slot, bool bAddStepsToSong, Song *givenSong=NULL ) override;
	/**
	 * @brief Attempt to parse the edit file in question.
	 * @param msd the edit file itself.
	 * @param sEditFilePath a const reference to a path on the hard drive to check.
	 * @param slot the Profile of the user with the edit.
	 * @param bAddStepsToSong a flag to determine if we add the edit steps to the song file.
	 * @return its success or failure.
	 */
	bool LoadEditFromMsd( const MsdFile &msd, const RString &sEditFilePath, ProfileSlot slot, bool bAddStepsToSong, Song *givenSong=NULL ) override;
	
	/**
	 * @brief Retrieve the specific NoteData from the file.
	 * @param cachePath the path to the cache file.
	 * @param out the Steps to receive just the particular notedata.
	 * @return true if successful, false otherwise. */
	bool LoadNoteDataFromSimfile( const RString &cachePath, Steps &out ) override;
	
	static void ProcessBPMs( TimingData &, const RString &sParam, string &songName);
	static void ProcessStops( TimingData &, const RString &sParam, string &songName);
	static void ProcessWarps( TimingData &, const RString &sParam, const float, string &songName);
	static void ProcessLabels(TimingData &out, const RString &sParam, string &songName);
	static void ProcessCombos( TimingData &, const RString &line, string &songName, const int = -1);
	void ProcessCombos(TimingData &, const RString &line, const int = -1) override;
	static void ProcessScrolls( TimingData &, const RString sParam, string &songName);

};

#endif
/**
 * @file
 * @author Jason Felds (c) 2011
 *
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
