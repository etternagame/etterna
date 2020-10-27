#ifndef NotesLoaderSM_H
#define NotesLoaderSM_H

#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "Etterna/Models/Misc/GameConstantsAndTypes.h"

#include "Etterna/FileTypes/MsdFile.h" // we require the struct from here.

class Song;
class Steps;
class TimingData;

/**
 * @brief The highest allowable speed before Warps come in.
 *
 * This was brought in from StepMania 4's recent betas. */
const float FAST_BPM_WARP = 9999999.f;

/** @brief The maximum file size for edits. */
const int MAX_EDIT_STEPS_SIZE_BYTES = 60 * 1024; // 60KB

/** @brief Reads a Song from an .SM file. */
struct SMLoader
{
	SMLoader()
	  : fileExt(".sm")
	{
	}

	SMLoader(std::string ext)
	  : fileExt(ext)
	{
	}

	virtual ~SMLoader() = default;

	/**
	 * @brief Attempt to load a song from a specified path.
	 * @param sPath a const reference to the path on the hard drive to check.
	 * @param out a reference to the Song that will retrieve the song
	 * information.
	 * @return its success or failure.
	 */
	virtual bool LoadFromDir(const std::string& sPath, Song& out);
	/**
	 * @brief Perform some cleanup on the loaded song.
	 * @param song a reference to the song that may need cleaning up.
	 * @param bFromCache a flag to determine if this song is loaded from a cache
	 * file.
	 */
	static void TidyUpData(Song& song, bool bFromCache);

	/**
	 * @brief Retrieve the relevant notedata from the simfile.
	 * @param path the path where the simfile lives.
	 * @param out the Steps we are loading the data into. */
	virtual bool LoadNoteDataFromSimfile(const std::string& path, Steps& out);

	/**
	 * @brief Attempt to load the specified sm file.
	 * @param sPath a const reference to the path on the hard drive to check.
	 * @param out a reference to the Song that will retrieve the song
	 * information.
	 * @param bFromCache a check to see if we are getting certain information
	 * from the cache file.
	 * @return its success or failure.
	 */
	virtual bool LoadFromSimfile(const std::string& sPath,
								 Song& out,
								 bool bFromCache = false);
	/**
	 * @brief Retrieve the list of .sm files.
	 * @param sPath a const reference to the path on the hard drive to check.
	 * @param out a vector of files found in the path.
	 */
	virtual void GetApplicableFiles(const std::string& sPath,
									vector<std::string>& out);
	virtual bool LoadFromBGChangesString(
	  BackgroundChange& change,
	  const std::string& sBGChangeExpression);

	/**
	 * @brief Parse BPM Changes data from a string.
	 * @param out the vector to put the data in.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	void ParseBPMs(vector<std::pair<float, float>>& out,
				   const std::string& line,
				   const int rowsPerBeat = -1);
	/**
	 * @brief Process the BPM Segments from the string.
	 * @param out the TimingData being modified.
	 * @param vBPMChanges the vector of BPM Changes data. */
	void ProcessBPMs(TimingData& out,
					 const vector<std::pair<float, float>>& vBPMChanges);
	/**
	 * @brief Parse Stops data from a string.
	 * @param out the vector to put the data in.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	void ParseStops(vector<std::pair<float, float>>& out,
					const std::string& line,
					const int rowsPerBeat = -1);
	/**
	 * @brief Process the Stop Segments from the data.
	 * @param out the TimingData being modified.
	 * @param vStops the vector of Stops data. */
	void ProcessStops(TimingData& out,
					  const vector<std::pair<float, float>>& vStops);
	/**
	 * @brief Process BPM and stop segments from the data.
	 * @param out the TimingData being modified.
	 * @param vBPMs the vector of BPM changes.
	 * @param vStops the vector of stops. */
	void ProcessBPMsAndStops(TimingData& out,
							 vector<std::pair<float, float>>& vBPMs,
							 vector<std::pair<float, float>>& vStops);
	/**
	 * @brief Process the Delay Segments from the string.
	 * @param out the TimingData being modified.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	void ProcessDelays(TimingData& out,
					   const std::string& line,
					   const int rowsPerBeat = -1);
	static void ProcessDelays(TimingData& out,
							  const std::string& line,
							  const std::string& songname,
							  const int rowsPerBeat = -1);
	/**
	 * @brief Process the Time Signature Segments from the string.
	 * @param out the TimingData being modified.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	void ProcessTimeSignatures(TimingData& out,
							   const std::string& line,
							   const int rowsPerBeat = -1);
	static void ProcessTimeSignatures(TimingData& out,
									  const std::string& line,
									  const std::string& songname,
									  const int rowsPerBeat = -1);
	/**
	 * @brief Process the Tickcount Segments from the string.
	 * @param out the TimingData being modified.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	void ProcessTickcounts(TimingData& out,
						   const std::string& line,
						   const int rowsPerBeat = -1);
	static void ProcessTickcounts(TimingData& out,
								  const std::string& line,
								  const std::string& songname,
								  const int rowsPerBeat = -1);

	/**
	 * @brief Process the Speed Segments from the string.
	 * @param out the TimingData being modified.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	virtual void ProcessSpeeds(TimingData& out,
							   const std::string& line,
							   const int rowsPerBeat = -1);
	static void ProcessSpeeds(TimingData& out,
							  const std::string& line,
							  const std::string& songname,
							  const int rowsPerBeat = -1);

	virtual void ProcessCombos(TimingData& /* out */,
							   const std::string& line,
							   const int /* rowsPerBeat */ = -1)
	{
	}

	/**
	 * @brief Process the Fake Segments from the string.
	 * @param out the TimingData being modified.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	virtual void ProcessFakes(TimingData& out,
							  const std::string& line,
							  const int rowsPerBeat = -1);
	static void ProcessFakes(TimingData& out,
							 const std::string& line,
							 const std::string& songname,
							 const int rowsPerBeat = -1);

	virtual void ProcessBGChanges(Song& out,
								  const std::string& sValueName,
								  const std::string& sPath,
								  const std::string& sParam);

	void ProcessInstrumentTracks(Song& out, const std::string& sParam);

	/**
	 * @brief Convert a row value to the proper beat value.
	 *
	 * This is primarily used for assistance with converting SMA files.
	 * @param line The line that contains the value.
	 * @param rowsPerBeat the number of rows per beat according to the original
	 * file.
	 * @return the converted beat value. */
	static float RowToBeat(const std::string& line, const int rowsPerBeat);

  protected:
	/**
	 * @brief Process the different tokens we have available to get NoteData.
	 * @param stepsType The current StepsType.
	 * @param description The description of the chart.
	 * @param difficulty The difficulty (in words) of the chart.
	 * @param meter the difficulty (in numbers) of the chart.
	 * @param radarValues the calculated radar values.
	 * @param noteData the note data itself.
	 * @param out the Steps getting the data. */
	virtual void LoadFromTokens(std::string sStepsType,
								std::string sDescription,
								std::string sDifficulty,
								std::string sMeter,
								std::string sNoteData,
								Steps& out);

	/**
	 * @brief Retrieve the file extension associated with this loader.
	 * @return the file extension. */
	std::string GetFileExtension() const { return fileExt; }

  public:
	// SetSongTitle and GetSongTitle changed to public to allow the functions
	// used by the parser helper to access them. -Kyz
	/**
	 * @brief Set the song title.
	 * @param t the song title. */
	virtual void SetSongTitle(const std::string& title);

	/**
	 * @brief Get the song title.
	 * @return the song title. */
	virtual std::string GetSongTitle() const;

  private:
	/** @brief The file extension in use. */
	const std::string fileExt;
	/** @brief The song title that is being processed. */
	std::string songTitle;
};

#endif
