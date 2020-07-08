#ifndef NOTES_LOADER_SMA_H
#define NOTES_LOADER_SMA_H

#include "Etterna/Models/Misc/BackgroundUtil.h"
#include "NotesLoaderSM.h"

class MsdFile;
class Song;
class Steps;
class TimingData;

/** @brief Reads a Song from a .SMA file. */
struct SMALoader : public SMLoader
{
	SMALoader()
	  : SMLoader(".sma")
	{
	}

	bool LoadFromSimfile(const std::string& sPath,
						 Song& out,
						 bool bFromCache = false) override;

	void ProcessBeatsPerMeasure(TimingData& out, const std::string& sParam);
	void ProcessMultipliers(TimingData& out,
							int iRowsPerBeat,
							const std::string& sParam);
	/**
	 * @brief Process the Speed Segments from the string.
	 * @param out the TimingData being modified.
	 * @param line the string in question.
	 * @param rowsPerBeat the number of rows per beat for this purpose. */
	void ProcessSpeeds(TimingData& out,
					   const std::string& line,
					   int rowsPerBeat) override;
};

#endif
