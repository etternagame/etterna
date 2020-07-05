#ifndef LYRICS_LOADER_H
#define LYRICS_LOADER_H

class Song;
/** @brief Loads lyrics from an LRC file. */
class LyricsLoader
{
  public:
	/**
	 * @brief Load the lyrics into the Song.
	 * @param sPath the path to the Lyrics.
	 * @param out the Song to receive the Lyrics. */
	bool LoadFromLRCFile(const std::string& sPath, Song& out);
};

#endif
