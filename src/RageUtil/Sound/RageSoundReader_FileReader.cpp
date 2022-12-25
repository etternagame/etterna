#include "Etterna/Globals/global.h"
#include "Etterna/Actor/Base/ActorUtil.h"
#include "RageUtil/File/RageFile.h"
#include "Core/Services/Locator.hpp"
#include "RageSoundReader_FileReader.h"
#include "RageUtil/Utils/RageUtil.h"
#include "RageSoundReader_WAV.h"
#include "RageSoundReader_MP3.h"
#include "RageSoundReader_Vorbisfile.h"

#include <set>

RageSoundReader_FileReader::RageSoundReader_FileReader(const RageSoundReader_FileReader& rhs):
  m_pFile(rhs.m_pFile->Copy()),
  m_sError(rhs.m_sError) {
}
RageSoundReader_FileReader::RageSoundReader_FileReader() : m_sError(), m_pFile(nullptr) {}

RageSoundReader_FileReader*
RageSoundReader_FileReader::TryOpenFile(RageFileBasic* pFile,
										std::string& error,
										const std::string& format,
										bool& bKeepTrying)
{
	RageSoundReader_FileReader* Sample = nullptr;

	if (!CompareNoCase(format, "wav"))
		Sample = new RageSoundReader_WAV;

	if (!CompareNoCase(format, "mp3")) {
		if (Sample != nullptr)
			delete Sample;
		Sample = new RageSoundReader_MP3;
	}

	if (!CompareNoCase(format, "oga") || !CompareNoCase(format, "ogg")) {
		if (Sample != nullptr)
			delete Sample;
		Sample = new RageSoundReader_Vorbisfile;
	}

	if (!Sample)
		return nullptr;

	OpenResult ret = Sample->Open(pFile);
	pFile = nullptr; // Sample owns it now
	if (ret == OPEN_OK)
		return Sample;

	std::string err = Sample->GetError();
	delete Sample;

	Locator::getLogger()->warn(
	  "SoundReader Open: Format {} failed: {}", format.c_str(), err.c_str());

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

	return nullptr;
}

#include "RageUtil/File/RageFileDriverMemory.h"

RageSoundReader_FileReader*
RageSoundReader_FileReader::OpenFile(const std::string& filename,
									 std::string& error,
									 bool* pPrebuffer)
{
	std::unique_ptr<RageFile> pFileOpen = std::make_unique<RageFile>();
	if (!pFileOpen->Open(filename)) {
		error = pFileOpen->GetError();
		return nullptr;
	}
	std::unique_ptr<RageFileBasic> pFile = std::move(pFileOpen);

	if (pPrebuffer) {
		if (pFile->GetFileSize() < 1024 * 50) {
			auto pMem = std::make_unique<RageFileObjMem>();
			bool bRet = FileCopy(*pFile, *pMem, error, nullptr);
			if (!bRet) {
				return nullptr;
			}

			pFile = std::move(pMem);
			pFile->Seek(0);
			*pPrebuffer = true;
		} else {
			*pPrebuffer = false;
		}
	}
	std::set<std::string> FileTypes;
	std::vector<std::string> const& sound_exts =
	  ActorUtil::GetTypeExtensionList(FT_Sound);
	for (std::vector<std::string>::const_iterator curr = sound_exts.begin();
		 curr != sound_exts.end();
		 ++curr) {
		FileTypes.insert(*curr);
	}

	std::string format = make_lower(GetExtension(filename));

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

	for (std::set<std::string>::iterator it = FileTypes.begin();
		 bKeepTrying && it != FileTypes.end();
		 ++it) {
		RageSoundReader_FileReader* NewSample =
		  TryOpenFile(pFile->Copy(), error, *it, bKeepTrying);
		if (NewSample) {
            Locator::getLogger()->info("Sound file {} is really {}.",
						 pFile->GetDisplayPath(), it->c_str());
			return NewSample;
		}
	}

	return nullptr;
}
