#include "Etterna/Globals/global.h"
#include "RageSurface.h"
#include "RageUtil/Utils/RageUtil.h"
#include <stb/stb_image.h>
#include <climits>

int32_t
RageSurfacePalette::FindColor(const RageSurfaceColor& color) const
{
	for (auto i = 0; i < ncolors; ++i)
		if (colors[i] == color)
			return i;
	return -1;
}

/* XXX: untested */
int32_t
RageSurfacePalette::FindClosestColor(const RageSurfaceColor& color) const
{
	auto iBest = -1;
	auto iBestDist = INT_MAX;
	for (auto i = 0; i < ncolors; ++i) {
		if (colors[i] == color)
			return i;

		const auto iDist =
		  abs(colors[i].r - color.r) + abs(colors[i].g - color.g) +
		  abs(colors[i].b - color.b) + abs(colors[i].a - color.a);
		if (iDist < iBestDist) {
			iBestDist = iDist;
			iBest = i;
		}
	}

	return iBest;
}

RageSurfaceFormat::RageSurfaceFormat()
  : Rmask(Mask[0])
  , Gmask(Mask[1])
  , Bmask(Mask[2])
  , Amask(Mask[3])
  , Rshift(Shift[0])
  , Gshift(Shift[1])
  , Bshift(Shift[2])
  , Ashift(Shift[3])
{
	palette = nullptr;
}

RageSurfaceFormat::RageSurfaceFormat(const RageSurfaceFormat& cpy)
  : Rmask(Mask[0])
  , Gmask(Mask[1])
  , Bmask(Mask[2])
  , Amask(Mask[3])
  , Rshift(Shift[0])
  , Gshift(Shift[1])
  , Bshift(Shift[2])
  , Ashift(Shift[3])
{
	memcpy(this, &cpy, sizeof(RageSurfaceFormat));
	if (palette != nullptr)
		palette = new RageSurfacePalette(*palette);
}

RageSurfaceFormat::~RageSurfaceFormat()
{
	delete palette;
}

void
RageSurfaceFormat::GetRGB(uint32_t val,
						  uint8_t* r,
						  uint8_t* g,
						  uint8_t* b) const
{
	if (BytesPerPixel == 1) {
		ASSERT(palette != NULL);
		*r = palette->colors[val].r;
		*g = palette->colors[val].g;
		*b = palette->colors[val].b;
	} else {
		*r = int8_t((val & Mask[0]) >> Shift[0] << Loss[0]);
		*g = int8_t((val & Mask[1]) >> Shift[1] << Loss[1]);
		*b = int8_t((val & Mask[2]) >> Shift[2] << Loss[2]);
	}
}

bool
RageSurfaceFormat::MapRGBA(uint8_t r,
						   uint8_t g,
						   uint8_t b,
						   uint8_t a,
						   uint32_t& val) const
{
	if (BytesPerPixel == 1) {
		const RageSurfaceColor c(r, g, b, a);
		const auto n = palette->FindColor(c);
		if (n == -1)
			return false;
		val = static_cast<uint32_t>(n);
	} else {
		val = (r >> Loss[0] << Shift[0]) | (g >> Loss[1] << Shift[1]) |
			  (b >> Loss[2] << Shift[2]) | (a >> Loss[3] << Shift[3]);
	}
	return true;
}

bool
RageSurfaceFormat::operator==(const RageSurfaceFormat& rhs) const
{
	if (!Equivalent(rhs))
		return false;

	if (BytesPerPixel == 1)
		if (memcmp(palette, rhs.palette, sizeof(RageSurfaceFormat)))
			return false;

	return true;
}

bool
RageSurfaceFormat::Equivalent(const RageSurfaceFormat& rhs) const
{
#define COMP(a)                                                                \
	if ((a) != rhs.a)                                                          \
		return false;
	COMP(BytesPerPixel);
	COMP(Rmask);
	COMP(Gmask);
	COMP(Bmask);
	COMP(Amask);

	return true;
}

RageSurface::RageSurface()
{
	pixels = nullptr;
	pixels_owned = true;
	stb_loadpoint = false;
}

RageSurface::RageSurface(const RageSurface& cpy)
{
	w = cpy.w;
	h = cpy.h;
	pitch = cpy.pitch;
	flags = cpy.flags;
	pixels_owned = true;
	stb_loadpoint = cpy.stb_loadpoint;
	if (cpy.pixels) {
		pixels = new uint8_t[pitch * h];
		memcpy(pixels, cpy.pixels, pitch * h);
	} else
		pixels = nullptr;
}

RageSurface::~RageSurface()
{

	if (pixels_owned)
		delete[] pixels;
	else if (stb_loadpoint)
		stbi_image_free(pixels);
}

static int
GetShiftFromMask(uint32_t mask)
{
	if (!mask)
		return 0;

	auto iShift = 0;
	while ((mask & 1) == 0) {
		mask >>= 1;
		++iShift;
	}
	return iShift;
}

static int
GetBitsFromMask(uint32_t mask)
{
	if (!mask)
		return 0;

	mask >>= GetShiftFromMask(mask);

	auto iBits = 0;
	while ((mask & 1) == 1) {
		mask >>= 1;
		++iBits;
	}
	return iBits;
}

void
SetupFormat(RageSurfaceFormat& fmt,
			int width,
			int height,
			int BitsPerPixel,
			uint32_t Rmask,
			uint32_t Gmask,
			uint32_t Bmask,
			uint32_t Amask)
{
	fmt.BitsPerPixel = BitsPerPixel;
	fmt.BytesPerPixel = BitsPerPixel / 8;
	if (fmt.BytesPerPixel == 1) {
		ZERO(fmt.Mask);
		ZERO(fmt.Shift);

		// Loss for paletted textures is zero; the actual palette entries are
		// 8-bit.
		ZERO(fmt.Loss);

		fmt.palette = new RageSurfacePalette;
		fmt.palette->ncolors = 256;
	} else {
		fmt.Mask[0] = Rmask;
		fmt.Mask[1] = Gmask;
		fmt.Mask[2] = Bmask;
		fmt.Mask[3] = Amask;

		fmt.Shift[0] = GetShiftFromMask(Rmask);
		fmt.Shift[1] = GetShiftFromMask(Gmask);
		fmt.Shift[2] = GetShiftFromMask(Bmask);
		fmt.Shift[3] = GetShiftFromMask(Amask);

		fmt.Loss[0] = static_cast<uint8_t>(8 - GetBitsFromMask(Rmask));
		fmt.Loss[1] = static_cast<uint8_t>(8 - GetBitsFromMask(Gmask));
		fmt.Loss[2] = static_cast<uint8_t>(8 - GetBitsFromMask(Bmask));
		fmt.Loss[3] = static_cast<uint8_t>(8 - GetBitsFromMask(Amask));
	}
}

RageSurface*
CreateSurface(int width,
			  int height,
			  int BitsPerPixel,
			  uint32_t Rmask,
			  uint32_t Gmask,
			  uint32_t Bmask,
			  uint32_t Amask)
{
	auto* pImg = new RageSurface;

	SetupFormat(
	  pImg->fmt, width, height, BitsPerPixel, Rmask, Gmask, Bmask, Amask);

	pImg->w = width;
	pImg->h = height;
	pImg->flags = 0;
	pImg->pitch = width * BitsPerPixel / 8;
	pImg->pixels = new uint8_t[pImg->pitch * height];

	/*
	if( BitsPerPixel == 8 )
	{
		pImg->fmt.palette = new RageSurfacePalette;
	}
	*/

	return pImg;
}

RageSurface*
CreateSurfaceFrom(int width,
				  int height,
				  int BitsPerPixel,
				  uint32_t Rmask,
				  uint32_t Gmask,
				  uint32_t Bmask,
				  uint32_t Amask,
				  uint8_t* pPixels,
				  uint32_t pitch)
{
	auto* pImg = new RageSurface;

	SetupFormat(
	  pImg->fmt, width, height, BitsPerPixel, Rmask, Gmask, Bmask, Amask);

	pImg->w = width;
	pImg->h = height;
	pImg->flags = 0;
	pImg->pitch = pitch;
	pImg->pixels = pPixels;
	pImg->pixels_owned = false;

	return pImg;
}
