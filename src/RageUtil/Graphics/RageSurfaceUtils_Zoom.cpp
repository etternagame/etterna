#include "Etterna/Globals/global.h"
#include "RageSurface.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageUtil/Utils/RageUtil.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb/stb_image_resize.h>

void
RageSurfaceUtils::Zoom(RageSurface*& src, int dstwidth, int dstheight)
{
	ASSERT_M(dstwidth > 0, ssprintf("%i", dstwidth));
	ASSERT_M(dstheight > 0, ssprintf("%i", dstheight));
	if (src == nullptr)
		return;

	if (src->w == dstwidth && src->h == dstheight)
		return;

	RageSurface* dst = nullptr;
	dst = CreateSurface(dstwidth,
						dstheight,
						32,
						src->fmt.Rmask,
						src->fmt.Gmask,
						src->fmt.Bmask,
						src->fmt.Amask);
	stbir_resize_uint8(
	  src->pixels, src->w, src->h, 0, dst->pixels, dstwidth, dstheight, 0, 4);
	delete src;
	src = dst;
}
