#include "Etterna/Globals/global.h"
#include "RageUtil/File/RageFile.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_PNG.h"
#include "RageUtil/Utils/RageUtil.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

// Note: This function is only called on RageDisplay.cpp:978 and the sError
// result is not used in the response.
bool
RageSurfaceUtils::SavePNG(RageSurface* pImg, RageFile& f, std::string& sError)
{
	// Functions from "stb_image_write.h" return 0 on failure
	f.Close(); // The RageFile reference is already opened. Should be closed for
			   // following function to succeed.

	RageSurface* res;
	const auto converted = ConvertSurface(pImg,
										  res,
										  pImg->w,
										  pImg->h,
										  32,
										  Swap32BE(0xFF000000),
										  Swap32BE(0x00FF0000),
										  Swap32BE(0x0000FF00),
										  Swap32BE(0x000000FF));

	if (!converted)
		res = pImg;

	// stride_in_bytes is image width in bytes
	const auto success =
	  0 !=
	  stbi_write_png(
		f.GetRealPath().c_str(), res->w, res->h, 4, res->pixels, res->w * 4);
	if (converted)
		delete res; // If we converted then we created a new surface which we
					// need to delete

	return success;
}
