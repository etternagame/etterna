/* Utility functions for RageSurfaces. */

#ifndef RAGE_SURFACE_UTILS_H
#define RAGE_SURFACE_UTILS_H

struct RageSurfaceColor;
struct RageSurfacePalette;
struct RageSurfaceFormat;
struct RageSurface;

/** @brief Utility functions for the RageSurfaces. */
namespace RageSurfaceUtils {
uint32_t
decodepixel(const uint8_t* p, int bpp);
void
encodepixel(uint8_t* p, int bpp, uint32_t pixel);

void
GetRawRGBAV(uint32_t pixel, const RageSurfaceFormat& fmt, uint8_t* v);
void
GetRawRGBAV(const uint8_t* p, const RageSurfaceFormat& fmt, uint8_t* v);
void
GetRGBAV(uint32_t pixel, const RageSurface* src, uint8_t* v);
void
GetRGBAV(const uint8_t* p, const RageSurface* src, uint8_t* v);

uint32_t
SetRawRGBAV(const RageSurfaceFormat& fmt, const uint8_t* v);
void
SetRawRGBAV(uint8_t* p, const RageSurface* src, const uint8_t* v);
uint32_t
SetRGBAV(const RageSurfaceFormat& fmt, const uint8_t* v);
void
SetRGBAV(uint8_t* p, const RageSurface* src, const uint8_t* v);

/* Get the number of bits representing each color channel in fmt. */
void
GetBitsPerChannel(const RageSurfaceFormat& fmt, uint32_t bits[4]);

void
CopySurface(const RageSurface* src, RageSurface* dest);
bool
ConvertSurface(const RageSurface* src,
			   RageSurface*& dst,
			   int width,
			   int height,
			   int bpp,
			   uint32_t R,
			   uint32_t G,
			   uint32_t B,
			   uint32_t A);
void
ConvertSurface(RageSurface*& image,
			   int width,
			   int height,
			   int bpp,
			   uint32_t R,
			   uint32_t G,
			   uint32_t B,
			   uint32_t A);

void
FixHiddenAlpha(RageSurface* img);

int
FindSurfaceTraits(const RageSurface* img);

/* The surface contains no transparent pixels and/or never uses its color
 * key, so it doesn't need any alpha bits at all. */
enum
{
	TRAIT_NO_TRANSPARENCY = 0x0001
}; /* 0alpha */

/* The surface contains only transparent values of 0 or 1; no translucency.
 * It only needs one bit of alpha. */
enum
{
	TRAIT_BOOL_TRANSPARENCY = 0x0002
}; /* 1alpha */

void
BlitTransform(const RageSurface* src,
			  RageSurface* dst,
			  const float fCoords[8] /* TL, BR, BL, TR */);

void
Blit(const RageSurface* src, RageSurface* dst, int width = -1, int height = -1);
void
CorrectBorderPixels(RageSurface* img, int width, int height);

bool
SaveSurface(const RageSurface* img, const std::string& file);
RageSurface*
LoadSurface(const std::string& file);

/* Quickly palettize to an gray/alpha texture. */
RageSurface*
PalettizeToGrayscale(const RageSurface* src_surf, int GrayBits, int AlphaBits);

RageSurface*
MakeDummySurface(int height, int width);

void
ApplyHotPinkColorKey(RageSurface*& img);
void
FlipVertically(RageSurface* img);
};

#endif
