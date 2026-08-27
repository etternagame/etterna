#include "Etterna/Globals/global.h"
#include "RageUtil/File/RageFile.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurface_Save_BMP.h"
#include "RageUtil/Utils/RageUtil.h"
#include <stb/stb_image_write.h>


bool
RageSurfaceUtils::SaveBMP(RageSurface* surface, RageFile& f)
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

	// returns 0 on failure
	const bool success = stbi_write_bmp(
	  f.GetRealPath().c_str(), res->w, res->h, 3, res->pixels);

	if (converted)
		delete res;

	return success;
}
