#include "Etterna/Globals/global.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageUtil/Utils/RageUtil.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

using namespace std;

void
RageSurfaceUtils::Zoom(RageSurface*& src, int dstwidth, int dstheight)
{
	ASSERT_M(dstwidth > 0, ssprintf("%i", dstwidth));
	ASSERT_M(dstheight > 0, ssprintf("%i", dstheight));
	if (src == NULL)
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
	return;
}

/*
 * (c) A. Schiffler, Glenn Maynard
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
 *
 * This is based on code from SDL_rotozoom, under the above license with
 * permission from Andreas Schiffler.
 */
