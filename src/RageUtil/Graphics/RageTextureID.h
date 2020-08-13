/* RageTextureID - An identifier for a texture and associated loading
 * parameters. */

#ifndef RAGE_TEXTURE_ID_H
#define RAGE_TEXTURE_ID_H

/* A unique texture is identified by a RageTextureID.  (Loading the
 * same file with two different dither settings is considered two
 * different textures, for example.)  See RageTexture.cpp for explanations
 * of these. */
struct RageTextureID
{
	std::string filename;

	// Maximum size of the texture, per dimension.
	int iMaxSize{ 0 };

	// Generate mipmaps for this texture
	bool bMipMaps{ false };

	/* Maximum number of bits for alpha.  In 16-bit modes, lowering
	 * this gives more bits for color values. (0, 1 or 4) */
	int iAlphaBits{ 0 };

	/* If this is greater than -1, then the image will be loaded as a luma/alpha
	 * map, eg. I4A4.  At most 8 bits per pixel will be used  This only actually
	 * happens when paletted textures are supported.
	 *
	 * If the sum of alpha and grayscale bits is <= 4, and the system supports
	 * 4-bit palettes, then the image will be loaded with 4bpp.
	 *
	 * This may be set to 0, resulting in an alpha map with all pixels white. */
	int iGrayscaleBits{ 0 };

	/* Preferred color depth of the image. (This is overridden for
	 * paletted images and transparencies.)  -1 for default. */
	int iColorDepth{ 0 };

	// If true and color precision is being lost, dither. (slow)
	bool bDither{ false };

	// If true, resize the image to fill the internal texture. (slow)
	bool bStretch{ false };

	/* If true, enable HOT PINK color keying. (deprecated but needed for
	 * banners) */
	bool bHotPinkColorKey{ false }; // #FF00FF

	// These hints will be used in addition to any in the filename.
	std::string AdditionalTextureHints;

	/* Used by RageTextureManager. Order is important; see
	 * RageTextureManager.cpp. Note that this property is not considered for
	 * ordering/equality. Loading a texture with a different loading policy will
	 * reuse the same texture with a different policy. */
	enum TexPolicy
	{
		TEX_VOLATILE,
		TEX_DEFAULT
	} Policy{ TEX_DEFAULT };

	void Init();

	RageTextureID() { Init(); }
	RageTextureID(const std::string& fn)
	{
		Init();
		SetFilename(fn);
	}
	void SetFilename(const std::string& fn);
};

inline auto
operator==(RageTextureID const& lhs, RageTextureID const& rhs) -> bool
{
	return lhs.filename == rhs.filename && lhs.iMaxSize == rhs.iMaxSize && lhs.bMipMaps == rhs.bMipMaps &&
		   lhs.iAlphaBits == rhs.iAlphaBits && lhs.iGrayscaleBits == rhs.iGrayscaleBits && lhs.iColorDepth == rhs.iColorDepth &&
		   lhs.bDither == rhs.bDither && lhs.bStretch == rhs.bStretch && lhs.bHotPinkColorKey == rhs.bHotPinkColorKey &&
		   lhs.AdditionalTextureHints == rhs.AdditionalTextureHints;
	// && lhs.Policy == rhs.Policy; // don't do this
}

inline auto
operator!=(RageTextureID const& lhs, RageTextureID const& rhs) -> bool
{
	return !operator==(lhs, rhs);
}

inline auto
operator<(RageTextureID const& lhs, RageTextureID const& rhs) -> bool
{
#define COMP(a)                                                                \
	if (lhs.a < rhs.a)                                                         \
		return true;                                                           \
	if (lhs.a > rhs.a)                                                         \
		return false;
	COMP(filename);
	COMP(iMaxSize);
	COMP(bMipMaps);
	COMP(iAlphaBits);
	COMP(iGrayscaleBits);
	COMP(iColorDepth);
	COMP(bDither);
	COMP(bStretch);
	COMP(bHotPinkColorKey);
	COMP(AdditionalTextureHints);
	// COMP(Policy); // don't do this
#undef COMP
	return false;
}

inline auto
operator>(RageTextureID const& lhs, RageTextureID const& rhs) -> bool
{
	return operator<(rhs, lhs);
}
inline auto
operator<=(RageTextureID const& lhs, RageTextureID const& rhs) -> bool
{
	return !operator<(rhs, lhs);
}
inline auto
operator>=(RageTextureID const& lhs, RageTextureID const& rhs) -> bool
{
	return !operator<(lhs, rhs);
}

#endif
