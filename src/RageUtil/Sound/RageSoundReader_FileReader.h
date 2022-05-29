/* SoundReader_FileReader - base class for SoundReaders that read from files. */

#ifndef RAGE_SOUND_READER_FILE_READER_H
#define RAGE_SOUND_READER_FILE_READER_H

#include <memory>
#include "RageSoundReader.h"

class RageFileBasic;

#define SoundReader_FileReader RageSoundReader_FileReader
class RageSoundReader_FileReader : public RageSoundReader
{
  public:
	/*
	 * Return OPEN_OK if the file is open and ready to go.  Return
	 * OPEN_UNKNOWN_FILE_FORMAT if the file appears to be of a different type.
	 * Return OPEN_FATAL_ERROR if the file appears to be the correct type, but
	 * there was an error initializing the file.
	 *
	 * If the file can not be opened at all, or contains no data, return
	 * OPEN_MATCH_BUT_FAIL.
	 */
	enum OpenResult
	{
		OPEN_OK,
		OPEN_UNKNOWN_FILE_FORMAT = 1,
		OPEN_FATAL_ERROR = 2,
	};
	RageSoundReader_FileReader(const RageSoundReader_FileReader&);
	RageSoundReader_FileReader();

	/* Takes ownership of pFile (even on failure). */
	virtual OpenResult Open(RageFileBasic* pFile) = 0;
	float GetStreamToSourceRatio() const override { return 1.0f; }
	std::string GetError() const override { return m_sError; }

	/* Open a file.  If pPrebuffer is non-NULL, and the file is sufficiently
	 * small, the (possibly compressed) data will be loaded entirely into
	 * memory, and pPrebuffer will be set to true. */
	static RageSoundReader_FileReader* OpenFile(const std::string& filename,
												std::string& error,
												bool* pPrebuffer = NULL);

  protected:
	void SetError(const std::string& sError) const { m_sError = sError; }
	std::unique_ptr<RageFileBasic> m_pFile;

  private:
	static RageSoundReader_FileReader* TryOpenFile(RageFileBasic* pFile,
												   std::string& error,
												   const std::string& format,
												   bool& bKeepTrying);
	mutable std::string m_sError;
};

#endif
