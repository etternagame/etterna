#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageSoundReader_FileReader.h"
#include "RageUtil/Utils/RageUtil.h"

#include <set>
#include "RageSoundReader_WAV.h"

#include "RageSoundReader_MP3.h"

#include "RageSoundReader_Vorbisfile.h"

RageSoundReader_FileReader*
RageSoundReader_FileReader::TryOpenFile(RageFileBasic* pFile,
										RString& error,
										const RString& format,
										bool& bKeepTrying)
{
	RageSoundReader_FileReader* Sample = NULL;

	if (!format.CompareNoCase("wav"))
		Sample = new RageSoundReader_WAV;

	if (!format.CompareNoCase("mp3")) {
		if (Sample != nullptr)
			delete Sample;
		Sample = new RageSoundReader_MP3;
	}

	if (!format.CompareNoCase("oga") || !format.CompareNoCase("ogg")) {
		if (Sample != nullptr)
			delete Sample;
		Sample = new RageSoundReader_Vorbisfile;
	}

	if (!Sample)
		return NULL;

	OpenResult ret = Sample->Open(pFile);
	pFile = NULL; // Sample owns it now
	if (ret == OPEN_OK)
		return Sample;

	RString err = Sample->GetError();
	delete Sample;

	LOG->Trace("Format %s failed: %s", format.c_str(), err.c_str());

	/*
	 * The file failed to open, or failed to read.  This indicates a problem
	 * that will affect all readers, so don't waste time trying more readers.
	 * (OPEN_IO_ERROR)
	 *
	 * Errors fall in two categories:
	 * OPEN_UNKNOWN_FILE_FORMAT: Data was successfully read from the file, but
	 * it's the wrong file format.  The error message always looks like "unknown
	 * file format" or "Not Vorbis data"; ignore it so we always give a
	 * consistent error message, and continue trying other file formats.
	 *
	 * OPEN_FATAL_ERROR: Either the file was opened successfully and appears to
	 * be the correct format, but a fatal format-specific error was encountered
	 * that will probably not be fixed by using a different reader (for example,
	 * an Ogg file that doesn't actually contain any audio streams); or the file
	 * failed to open or read ("I/O error", "permission denied"), in which case
	 * all other readers will probably fail, too.  The returned error is used,
	 * and no other formats will be tried.
	 */
	bKeepTrying = (ret != OPEN_FATAL_ERROR);
	switch (ret) {
		case OPEN_UNKNOWN_FILE_FORMAT:
			bKeepTrying = true;
			error = "Unknown file format";
			break;

		case OPEN_FATAL_ERROR:
			/* The file matched, but failed to load.  We know it's this type of
			 * data; don't bother trying the other file types. */
			bKeepTrying = false;
			error = err;
			break;
		default:
			break;
	}

	return NULL;
}

#include "RageUtil/File/RageFileDriverMemory.h"

RageSoundReader_FileReader*
RageSoundReader_FileReader::OpenFile(const RString& filename,
									 RString& error,
									 bool* pPrebuffer)
{
	HiddenPtr<RageFileBasic> pFile;
	{
		auto* pFileOpen = new RageFile;
		if (!pFileOpen->Open(filename)) {
			error = pFileOpen->GetError();
			delete pFileOpen;
			return NULL;
		}
		pFile = pFileOpen;
	}

	if (pPrebuffer) {
		if (pFile->GetFileSize() < 1024 * 50) {
			auto* pMem = new RageFileObjMem;
			bool bRet = FileCopy(*pFile, *pMem, error, NULL);
			if (!bRet) {
				delete pMem;
				return NULL;
			}

			pFile = pMem;
			pFile->Seek(0);
			*pPrebuffer = true;
		} else {
			*pPrebuffer = false;
		}
	}
	set<RString> FileTypes;
	vector<std::string> const& sound_exts =
	  ActorUtil::GetTypeExtensionList(FT_Sound);
	for (vector<std::string>::const_iterator curr = sound_exts.begin();
		 curr != sound_exts.end();
		 ++curr) {
		FileTypes.insert(*curr);
	}

	RString format = GetExtension(filename);
	format.MakeLower();

	error = "";

	bool bKeepTrying = true;

	/* If the extension matches a format, try that first. */
	if (FileTypes.find(format) != FileTypes.end()) {
		RageSoundReader_FileReader* NewSample =
		  TryOpenFile(pFile->Copy(), error, format, bKeepTrying);
		if (NewSample)
			return NewSample;
		FileTypes.erase(format);
	}

	for (set<RString>::iterator it = FileTypes.begin();
		 bKeepTrying && it != FileTypes.end();
		 ++it) {
		RageSoundReader_FileReader* NewSample =
		  TryOpenFile(pFile->Copy(), error, *it, bKeepTrying);
		if (NewSample) {
			LOG->UserLog("Sound file",
						 pFile->GetDisplayPath(),
						 "is really %s.",
						 it->c_str());
			return NewSample;
		}
	}

	return NULL;
}
