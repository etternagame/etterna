#include "Etterna/Globals/global.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_JPEG.h"
#include "RageUtil/File/RageFile.h"
#include "RageUtil/Utils/RageUtil.h"
#include <stb/stb_image_write.h>

bool
RageSurfaceUtils::SaveJPEG(RageSurface* surface, RageFile& f, bool bHighQual)
{
	f.Close();

	RageSurface* res;
	const auto converted = ConvertSurface(surface,
										  res,
										  surface->w,
										  surface->h,
										  24,
										  Swap24BE(0xFF0000),
										  Swap24BE(0x00FF00),
										  Swap24BE(0x0000FF),
										  0);
	if (!converted)
		res = surface;

	const auto quality = bHighQual ? 100 : 70;

	// returns 0 on failure
	const bool success = stbi_write_jpg(
	  f.GetRealPath().c_str(), res->w, res->h, 3, res->pixels, quality);

	if (converted)
		delete res;

	return success;
}
