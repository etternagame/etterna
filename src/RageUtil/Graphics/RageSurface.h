/* RageSurface - holds a simple 2d graphic surface */

#ifndef RAGE_SURFACE_H
#define RAGE_SURFACE_H

/* XXX remove? */
struct RageSurfaceColor
{
	uint8_t r, g, b, a;
	RageSurfaceColor()
	  : r(0)
	  , g(0)
	  , b(0)
	  , a(0)
	{
	}
	RageSurfaceColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_)
	  : r(r_)
	  , g(g_)
	  , b(b_)
	  , a(a_)
	{
	}
};

inline bool
operator==(RageSurfaceColor const& lhs, RageSurfaceColor const& rhs)
{
	return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}

inline bool
operator!=(RageSurfaceColor const& lhs, RageSurfaceColor const& rhs)
{
	return !operator==(lhs, rhs);
}

struct RageSurfacePalette
{
	RageSurfaceColor colors[256];
	int32_t ncolors;

	/* Find the exact color; returns -1 if not found. */
	int32_t FindColor(const RageSurfaceColor& color) const;
	int32_t FindClosestColor(const RageSurfaceColor& color) const;
};

struct RageSurfaceFormat
{
	RageSurfaceFormat();
	RageSurfaceFormat(const RageSurfaceFormat& cpy);
	~RageSurfaceFormat();

	int32_t BytesPerPixel = 1;
	int32_t BitsPerPixel = 8;
	uint32_t Mask[4];
	uint32_t Shift[4];
	uint8_t Loss[4];
	uint32_t &Rmask, &Gmask, &Bmask, &Amask;	 /* deprecated */
	uint32_t &Rshift, &Gshift, &Bshift, &Ashift; /* deprecated */
	RageSurfacePalette* palette;

	void GetRGB(uint32_t val, uint8_t* r, uint8_t* g, uint8_t* b) const;

	/* Return the decoded value for the given color; the result can be compared
	 * to decodepixel() results.  If the image is paletted and the color isn't
	 * found, val is undefined and false is returned. */
	bool MapRGBA(uint8_t r,
				 uint8_t g,
				 uint8_t b,
				 uint8_t a,
				 uint32_t& val) const;

	/* MapRGBA, but also do a nearest-match on palette colors. */
	uint32_t MapNearestRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const;

	bool operator==(const RageSurfaceFormat& rhs) const;

	/* Like operator==, but ignores the palette (which is really a part of the
	 * surface, not the format). */
	bool Equivalent(const RageSurfaceFormat& rhs) const;
};

struct RageSurface
{
	RageSurfaceFormat fmt;
	uint8_t* pixels;
	bool pixels_owned;
	bool stb_loadpoint;
	int32_t w = 0, h = 0, pitch = 0;
	int32_t flags = 0;

	RageSurface();
	RageSurface(const RageSurface& cpy);
	~RageSurface();
};

RageSurface*
CreateSurface(int width,
			  int height,
			  int bpp,
			  uint32_t Rmask,
			  uint32_t Gmask,
			  uint32_t Bmask,
			  uint32_t Amask);
RageSurface*
CreateSurfaceFrom(int width,
				  int height,
				  int bpp,
				  uint32_t Rmask,
				  uint32_t Gmask,
				  uint32_t Bmask,
				  uint32_t Amask,
				  uint8_t* pPixels,
				  uint32_t pitch);

#endif
