#include "Etterna/Globals/global.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_PNG.h"
#include "RageUtil/Utils/RageUtil.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

// Note: This function is only called on RageDisplay.cpp:978 and the sError result is not used in the response.
bool RageSurfaceUtils::SavePNG(RageSurface* pImg, RageFile& f, RString& sError) {
	// Functions from "stb_image_write.h" return 0 on failure
	f.Close(); // The RageFile reference is already opened. Should be closed for following function to succeed.

	RageSurface* res;
	bool converted = RageSurfaceUtils::ConvertSurface(pImg, res, pImg->w, pImg->h, 32,
													  Swap32BE(0xFF000000),
													  Swap32BE(0x00FF0000),
													  Swap32BE(0x0000FF00),
													  Swap32BE(0x000000FF));

	if (!converted) res = pImg;

	// stride_in_bytes is image width in bytes
	bool success = 0 != stbi_write_png(f.GetRealPath(), res->w, res->h, 4, res->pixels, res->w * 4);
	if (converted) delete res; // If we converted then we created a new surface which we need to delete

	return success;
}

/*
 * (c) 2004-2006 Glenn Maynard
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
