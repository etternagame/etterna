#include "global.h"
#include "ActorUtil.h"
#include "RageFile.h"
#include "RageLog.h"
#include "RageSurface_Load.h"
#include "RageSurface_Load_BMP.h"
#include "RageSurface_Load_GIF.h"
#include "RageSurface_Load_JPEG.h"
#include "RageSurface_Load_PNG.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "PrefsManager.h"
#include "RageUtil.h"
#include "RageSurface.h"
#include <set>

RageSurfaceUtils::OpenResult
RageSurface_stb_Load(const RString& sPath,
					 RageSurface*& ret,
					 bool bHeaderOnly,
					 RString& error)
{
	RageFile f;
	if (!f.Open(sPath)) {
		error = f.GetError();
		return RageSurfaceUtils::OPEN_FATAL_ERROR;
	}

	int x, y, n;
	auto* doot = stbi_load(f.GetPath(), &x, &y, &n, 4);
	if (bHeaderOnly) {
		ret = CreateSurfaceFrom(x, y, 32, 0, 0, 0, 0, nullptr, x * 4);
	} else {
		ret =
		  CreateSurfaceFrom(x,
							y,
							32,
							Swap32BE(0xFF000000),
							Swap32BE(0x00FF0000),
							Swap32BE(0x0000FF00),
							Swap32BE(0x000000FF),
							doot,
							x * 4);
	}

	if (ret == nullptr) {
		stbi_image_free(doot);
		return RageSurfaceUtils::OPEN_UNKNOWN_FILE_FORMAT; // XXX
	}
	ret->stb_loadpoint = true;
	return RageSurfaceUtils::OPEN_OK;
}
static RageSurface*
TryOpenFile(RString sPath,
			bool bHeaderOnly,
			RString& error,
			RString format,
			bool& bKeepTrying)
{
	RageSurface* ret = nullptr;
	RageSurfaceUtils::OpenResult result;

	if (PREFSMAN->UseStbImageLibrary)
		result = RageSurface_stb_Load(sPath, ret, bHeaderOnly, error);
	else if (!format.CompareNoCase("png"))
		result = RageSurface_Load_PNG(sPath, ret, bHeaderOnly, error);
	else if (!format.CompareNoCase("gif"))
		result = RageSurface_Load_GIF(sPath, ret, bHeaderOnly, error);
	else if (!format.CompareNoCase("jpg") || !format.CompareNoCase("jpeg"))
		result = RageSurface_Load_JPEG(sPath, ret, bHeaderOnly, error);
	else if (!format.CompareNoCase("bmp"))
		result = RageSurface_Load_BMP(sPath, ret, bHeaderOnly, error);
	else {
		error = "Unsupported format";
		bKeepTrying = true;
		return nullptr;
	}

	if (result == RageSurfaceUtils::OPEN_OK) {
		ASSERT(ret != nullptr);
		return ret;
	}

	LOG->Trace("Format %s failed: %s", format.c_str(), error.c_str());

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
	bKeepTrying = (result != RageSurfaceUtils::OPEN_FATAL_ERROR);
	switch (result) {
		case RageSurfaceUtils::OPEN_UNKNOWN_FILE_FORMAT:
			bKeepTrying = true;
			error = "Unknown file format";
			break;

		case RageSurfaceUtils::OPEN_FATAL_ERROR:
			/* The file matched, but failed to load.  We know it's this type of
			 * data; don't bother trying the other file types. */
			bKeepTrying = false;
			break;
		default:
			break;
	}

	return nullptr;
}

RageSurface*
RageSurfaceUtils::LoadFile(const RString& sPath,
						   RString& error,
						   bool bHeaderOnly)
{
	{
		RageFile TestOpen;
		if (!TestOpen.Open(sPath)) {
			error = TestOpen.GetError();
			return nullptr;
		}
	}

	set<RString> FileTypes;
	vector<RString> const& exts = ActorUtil::GetTypeExtensionList(FT_Bitmap);
	for (vector<RString>::const_iterator curr = exts.begin();
		 curr != exts.end();
		 ++curr) {
		FileTypes.insert(*curr);
	}

	RString format = GetExtension(sPath);
	format.MakeLower();

	bool bKeepTrying = true;

	/* If the extension matches a format, try that first. */
	if (FileTypes.find(format) != FileTypes.end()) {
		RageSurface* ret =
		  TryOpenFile(sPath, bHeaderOnly, error, format, bKeepTrying);
		if (ret)
			return ret;
		FileTypes.erase(format);
	}

	for (set<RString>::iterator it = FileTypes.begin();
		 bKeepTrying && it != FileTypes.end();
		 ++it) {
		RageSurface* ret =
		  TryOpenFile(sPath, bHeaderOnly, error, *it, bKeepTrying);
		if (ret) {
			LOG->UserLog("Graphic file", sPath, "is really %s", it->c_str());
			return ret;
		}
	}

	return nullptr;
}


/*
 * (c) 2004 Glenn Maynard
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
