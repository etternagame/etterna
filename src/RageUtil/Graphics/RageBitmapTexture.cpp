#include "Etterna/Globals/global.h"
#include "RageBitmapTexture.h"
#include "RageDisplay.h"
#include "RageUtil/Misc/RageLog.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurfaceUtils_Dither.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageSurface_Load.h"
#include "RageTextureManager.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Etterna/Globals/StepMania.h"
#include "arch/Dialog/Dialog.h"
#include "Etterna/Singletons/PrefsManager.h"

#include <algorithm>
#include <string>

using std::max;
using std::min;
using std::string;

static void
GetResolutionFromFileName(std::string& sPath, int& iWidth, int& iHeight)
{
	/* Match:
	 *  Foo (res 512x128).png
	 * Also allow, eg:
	 *  Foo (dither, res 512x128).png
	 * Be careful that this doesn't get mixed up with frame dimensions. */
	static Regex re("\\([^\\)]*res ([0-9]+)x([0-9]+).*\\)");

	vector<std::string> asMatches;
	if (!re.Compare(sPath, asMatches))
		return;

	// Check for nonsense values.  Some people might not intend the hint. -Kyz
	const auto maybe_width = StringToInt(asMatches[0]);
	const auto maybe_height = StringToInt(asMatches[1]);
	if (maybe_width <= 0 || maybe_height <= 0) {
		return;
	}
	iWidth = maybe_width;
	iHeight = maybe_height;
}

RageBitmapTexture::RageBitmapTexture(const RageTextureID& name)
  : RageTexture(name)
  , m_uTexHandle(0)
{
	Create();
}

RageBitmapTexture::~RageBitmapTexture()
{
	Destroy();
}

void
RageBitmapTexture::Reload()
{
	Destroy();
	Create();
}

/*
 * Each dwMaxSize, dwTextureColorDepth and iAlphaBits are maximums; we may
 * use less.  iAlphaBits must be 0, 1 or 4.
 *
 * XXX: change iAlphaBits == 4 to iAlphaBits == 8 to indicate "as much alpha
 * as needed", since that's what it really is; still only use 4 in 16-bit
 * textures.
 *
 * Dither forces dithering when loading 16-bit textures.
 * Stretch forces the loaded image to fill the texture completely.
 */
void
RageBitmapTexture::Create()
{
	auto actualID = GetID();

	ASSERT(!actualID.filename.empty());

	/* Load the image into a RageSurface. */
	std::string error;
	RageSurface* pImg = nullptr;
	if (actualID.filename == TEXTUREMAN->GetScreenTextureID().filename) {
		pImg = TEXTUREMAN->GetScreenSurface();
	} else {
		pImg = RageSurfaceUtils::LoadFile(actualID.filename, error);
	}

	/* Tolerate corrupt/unknown images. */
	if (pImg == nullptr) {
		auto warning = ssprintf("RageBitmapTexture: Couldn't load %s: %s",
								actualID.filename.c_str(),
								error.c_str());
		LOG->Warn("%s", warning.c_str());
		Dialog::OK(warning, "missing_texture");
		pImg = RageSurfaceUtils::MakeDummySurface(64, 64);
		ASSERT(pImg != nullptr);
	}

	if (actualID.bHotPinkColorKey)
		RageSurfaceUtils::ApplyHotPinkColorKey(pImg);

	{
		/* Do this after setting the color key for paletted images; it'll also
		 * return TRAIT_NO_TRANSPARENCY if the color key is never used. */
		auto iTraits = RageSurfaceUtils::FindSurfaceTraits(pImg);
		if (iTraits & RageSurfaceUtils::TRAIT_NO_TRANSPARENCY)
			actualID.iAlphaBits = 0;
		else if (iTraits & RageSurfaceUtils::TRAIT_BOOL_TRANSPARENCY)
			actualID.iAlphaBits = 1;
	}

	// look in the file name for a format hints
	auto sHintString =
	  make_lower(GetID().filename + actualID.AdditionalTextureHints);

	if (sHintString.find("32bpp") != string::npos)
		actualID.iColorDepth = 32;
	else if (sHintString.find("16bpp") != string::npos)
		actualID.iColorDepth = 16;
	if (sHintString.find("dither") != string::npos)
		actualID.bDither = true;
	if (sHintString.find("stretch") != string::npos)
		actualID.bStretch = true;
	if (sHintString.find("mipmaps") != string::npos)
		actualID.bMipMaps = true;
	if (sHintString.find("nomipmaps") != string::npos)
		actualID.bMipMaps = false; // check for "nomipmaps" after "mipmaps"

	/* If the image is marked grayscale, then use all bits not used for alpha
	 * for the intensity.  This way, if an image has no alpha, you get an 8-bit
	 * grayscale; if it only has boolean transparency, you get a 7-bit
	 * grayscale. */
	if (sHintString.find("grayscale") != string::npos)
		actualID.iGrayscaleBits = 8 - actualID.iAlphaBits;

	/* This indicates that the only component in the texture is alpha; assume
	 * all color is white. */
	if (sHintString.find("alphamap") != string::npos)
		actualID.iGrayscaleBits = 0;

	/* No iGrayscaleBits for images that are already paletted.  We don't support
	 * that; and that hint is intended for use on images that are already
	 * grayscale, it's not intended to change a color image into a grayscale
	 * image. */
	if (actualID.iGrayscaleBits != -1 && pImg->fmt.BitsPerPixel == 8)
		actualID.iGrayscaleBits = -1;

	/* Cap the max texture size to the hardware max. */
	actualID.iMaxSize = min(actualID.iMaxSize, DISPLAY->GetMaxTextureSize());

	/* Save information about the source. */
	m_iSourceWidth = pImg->w;
	m_iSourceHeight = pImg->h;

	/* in-game image dimensions are the same as the source graphic */
	m_iImageWidth = m_iSourceWidth;
	m_iImageHeight = m_iSourceHeight;

	/* if "doubleres" (high resolution) and we're not allowing high res
	 * textures, then image dimensions are half of the source */
	if (sHintString.find("doubleres") != string::npos) {
		if (!StepMania::GetHighResolutionTextures()) {
			m_iImageWidth = m_iImageWidth / 2;
			m_iImageHeight = m_iImageHeight / 2;
		}
	}

	/* image size cannot exceed max size */
	m_iImageWidth = min(m_iImageWidth, actualID.iMaxSize);
	m_iImageHeight = min(m_iImageHeight, actualID.iMaxSize);

	/* Texture dimensions need to be a power of two; jump to the next. */
	m_iTextureWidth = power_of_two(m_iImageWidth);
	m_iTextureHeight = power_of_two(m_iImageHeight);

	/* If we're under 8x8, increase it, to avoid filtering problems on odd
	 * hardware. */
	if (m_iTextureWidth < 8 || m_iTextureHeight < 8) {
		actualID.bStretch = true;
		m_iTextureWidth = max(8, m_iTextureWidth);
		m_iTextureHeight = max(8, m_iTextureHeight);
	}

	ASSERT_M(m_iTextureWidth <= actualID.iMaxSize,
			 ssprintf("w %i, %i", m_iTextureWidth, actualID.iMaxSize));
	ASSERT_M(m_iTextureHeight <= actualID.iMaxSize,
			 ssprintf("h %i, %i", m_iTextureHeight, actualID.iMaxSize));

	if (actualID.bStretch) {
		/* The hints asked for the image to be stretched to the texture size,
		 * probably for tiling. */
		m_iImageWidth = m_iTextureWidth;
		m_iImageHeight = m_iTextureHeight;
	}

	if (pImg->w != m_iImageWidth || pImg->h != m_iImageHeight)
		RageSurfaceUtils::Zoom(pImg, m_iImageWidth, m_iImageHeight);

	if (actualID.iGrayscaleBits != -1 &&
		DISPLAY->SupportsTextureFormat(RagePixelFormat_PAL)) {
		auto pGrayscale = RageSurfaceUtils::PalettizeToGrayscale(
		  pImg, actualID.iGrayscaleBits, actualID.iAlphaBits);

		delete pImg;
		pImg = pGrayscale;
	}

	// Figure out which texture format we want the renderer to use.
	RagePixelFormat pixfmt;

	// If the source is palleted, always load as paletted if supported.
	if (pImg->fmt.BitsPerPixel == 8 &&
		DISPLAY->SupportsTextureFormat(RagePixelFormat_PAL)) {
		pixfmt = RagePixelFormat_PAL;
	} else {
		// not paletted
		switch (actualID.iColorDepth) {
			case 16: {
				// Bits of alpha in the source:
				auto iSourceAlphaBits = 8 - pImg->fmt.Loss[3];

				// Don't use more than we were hinted to.
				iSourceAlphaBits = min(actualID.iAlphaBits, iSourceAlphaBits);

				switch (iSourceAlphaBits) {
					case 0:
					case 1:
						pixfmt = RagePixelFormat_RGB5A1;
						break;
					default:
						pixfmt = RagePixelFormat_RGBA4;
						break;
				}
			} break;
			case 32:
				pixfmt = RagePixelFormat_RGBA8;
				break;
			default:
				FAIL_M(ssprintf("%i", actualID.iColorDepth));
		}
	}

	// Make we're using a supported format. Every card supports either RGBA8 or
	// RGBA4.
	if (!DISPLAY->SupportsTextureFormat(pixfmt)) {
		pixfmt = RagePixelFormat_RGBA8;
		if (!DISPLAY->SupportsTextureFormat(pixfmt))
			pixfmt = RagePixelFormat_RGBA4;
	}

	/* Dither if appropriate.
	 * XXX: This is a special case: don't bother dithering to RGBA8888.
	 * We actually want to dither only if the destination has greater color
	 * depth on at least one color channel than the source. For example, it
	 * doesn't make sense to do this when pixfmt is RGBA5551 if the image is
	 * only RGBA555. */
	if (actualID.bDither &&
		(pixfmt == RagePixelFormat_RGBA4 || pixfmt == RagePixelFormat_RGB5A1)) {
		// Dither down to the destination format.
		auto pfd = DISPLAY->GetPixelFormatDesc(pixfmt);
		auto dst = CreateSurface(pImg->w,
								 pImg->h,
								 pfd->bpp,
								 pfd->masks[0],
								 pfd->masks[1],
								 pfd->masks[2],
								 pfd->masks[3]);

		RageSurfaceUtils::ErrorDiffusionDither(pImg, dst);
		delete pImg;
		pImg = dst;
	}

	/* This needs to be done *after* the final resize, since that resize
	 * may introduce new alpha bits that need to be set.  It needs to be
	 * done *before* we set up the palette, since it might change it. */
	RageSurfaceUtils::FixHiddenAlpha(pImg);

	/* Scale up to the texture size, if needed. */
	RageSurfaceUtils::ConvertSurface(pImg,
									 m_iTextureWidth,
									 m_iTextureHeight,
									 pImg->fmt.BitsPerPixel,
									 pImg->fmt.Mask[0],
									 pImg->fmt.Mask[1],
									 pImg->fmt.Mask[2],
									 pImg->fmt.Mask[3]);

	m_uTexHandle = DISPLAY->CreateTexture(pixfmt, pImg, actualID.bMipMaps);

	CreateFrameRects();

	{
		// Enforce frames in the image have even dimensions.
		// Otherwise, pixel/texel alignment will be off.
		auto iDimensionMultiple = 2;

		if (sHintString.find("doubleres") != string::npos) {
			iDimensionMultiple = 4;
		}

		auto bRunCheck = true;

		// Don't check if the artist intentionally blanked the image by making
		// it very tiny.
		if (this->GetSourceWidth() <= iDimensionMultiple ||
			this->GetSourceHeight() <= iDimensionMultiple)
			bRunCheck = false;

		// HACK: Don't check song graphics. Many of them are weird dimensions.
		if (!TEXTUREMAN->GetOddDimensionWarning())
			bRunCheck = false;

		// Don't check if this is the screen texture, the theme can't do
		// anything about it. -Kyz
		if (actualID == TEXTUREMAN->GetScreenTextureID()) {
			bRunCheck = false;
		}

		if (bRunCheck && PREFSMAN->m_verbose_log > 1) {
			auto fFrameWidth = this->GetSourceWidth() /
							   static_cast<float>(this->GetFramesWide());
			auto fFrameHeight = this->GetSourceHeight() /
								static_cast<float>(this->GetFramesHigh());
			auto fBetterFrameWidth =
			  ceilf(fFrameWidth / iDimensionMultiple) * iDimensionMultiple;
			auto fBetterFrameHeight =
			  ceilf(fFrameHeight / iDimensionMultiple) * iDimensionMultiple;
			auto fBetterSourceWidth = this->GetFramesWide() * fBetterFrameWidth;
			auto fBetterSourceHeight =
			  this->GetFramesHigh() * fBetterFrameHeight;
			if (fFrameWidth != fBetterFrameWidth ||
				fFrameHeight != fBetterFrameHeight) {
				auto sWarning = ssprintf(
				  "The graphic '%s' has frame dimensions that aren't a "
				  "multiple of %d.\n"
				  "The entire image is %dx%d and frame size is %.1fx%.1f.\n"
				  "Image quality will be much improved if you resize the "
				  "graphic to %.0fx%.0f, which is a frame size of %.0fx%.0f.",
				  actualID.filename.c_str(),
				  iDimensionMultiple,
				  this->GetSourceWidth(),
				  this->GetSourceHeight(),
				  fFrameWidth,
				  fFrameHeight,
				  fBetterSourceWidth,
				  fBetterSourceHeight,
				  fBetterFrameWidth,
				  fBetterFrameHeight);
				LOG->Warn("%s", sWarning.c_str());
				Dialog::OK(sWarning, "FRAME_DIMENSIONS_WARNING");
			}
		}
	}

	delete pImg;

	// Check for hints that override the apparent "size".
	GetResolutionFromFileName(
	  actualID.filename, m_iSourceWidth, m_iSourceHeight);

	/* if "doubleres" (high resolution) then we want the image to appear in-game
	 * with dimensions 1/2 of the source. So, cut down the source dimension here
	 * after everything above is finished operating with the real image
	 * source dimensions. */
	if (sHintString.find("doubleres") != string::npos) {
		m_iSourceWidth = m_iSourceWidth / 2;
		m_iSourceHeight = m_iSourceHeight / 2;
	}

	std::string sProperties;
	sProperties += RagePixelFormatToString(pixfmt) + " ";
	if (actualID.iAlphaBits == 0)
		sProperties += "opaque ";
	if (actualID.iAlphaBits == 1)
		sProperties += "matte ";
	if (actualID.bStretch)
		sProperties += "stretch ";
	if (actualID.bDither)
		sProperties += "dither ";
	sProperties.erase(sProperties.size() - 1);
	// LOG->Trace( "RageBitmapTexture: Loaded '%s' (%ux%u); %s, source %d,%d;
	// image %d,%d.", 	actualID.filename.c_str(), GetTextureWidth(),
	// GetTextureHeight(), 	sProperties.c_str(), m_iSourceWidth,
	// m_iSourceHeight, 	m_iImageWidth, m_iImageHeight );
}

void
RageBitmapTexture::Destroy()
{
	// if DISPLAY is dead at this point, the program is probably dead
	// memory will be free.... hopefully....
	if (DISPLAY != nullptr)
		DISPLAY->DeleteTexture(m_uTexHandle);
}
