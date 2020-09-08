#ifndef RAGE_MOVIE_TEXTURE_GENERIC_H
#define RAGE_MOVIE_TEXTURE_GENERIC_H

#include "MovieTexture.h"

class FFMpeg_Helper;
struct RageSurface;
struct RageTextureLock;
class RageTextureRenderTarget;
class Sprite;

enum MovieDecoderPixelFormatYCbCr
{
	PixelFormatYCbCr_YUYV422,
	NUM_PixelFormatYCbCr,
	PixelFormatYCbCr_Invalid
};

class MovieDecoder
{
  public:
	virtual ~MovieDecoder() {}

	virtual std::string Open(const std::string& sFile) = 0;
	virtual void Close() = 0;
	virtual void Rewind() = 0;

	/*
	 * Decode a frame.  Return 1 on success, 0 on EOF, -1 on fatal error.
	 *
	 * If we're lagging behind the video, fTargetTime will be the target
	 * timestamp.  The decoder may skip frames to catch up.  On return,
	 * the current timestamp must be <= fTargetTime.
	 *
	 * Otherwise, fTargetTime will be -1, and the next frame should be
	 * decoded; skip frames only if necessary to recover from errors.
	 */
	virtual int DecodeFrame(float fTargetTime) = 0;

	/*
	 * Get the currently-decoded frame.
	 */
	virtual void GetFrame(RageSurface* pOut) = 0;

	/* Return the dimensions of the image, in pixels (before aspect ratio
	 * adjustments). */
	virtual int GetWidth() const = 0;
	virtual int GetHeight() const = 0;

	/* Return the aspect ratio of a pixel in the image.  Usually 1. */
	virtual float GetSourceAspectRatio() const { return 1.0f; }

	/*
	 * Create a surface acceptable to pass to GetFrame.  This should be
	 * a surface which is realtime-compatible with DISPLAY, and should
	 * attempt to obey bPreferHighColor.  The given size will usually be
	 * the next power of two higher than GetWidth/GetHeight, but on systems
	 * with limited texture resolution, may be smaller.
	 *
	 * If DISPLAY supports the EffectMode_YUYV422 blend mode, this may be
	 * a packed-pixel YUV surface.  UYVY maps to RGBA, respectively.  If
	 * used, set fmtout.
	 */
	virtual RageSurface* CreateCompatibleSurface(
	  int iTextureWidth,
	  int iTextureHeight,
	  bool bPreferHighColor,
	  MovieDecoderPixelFormatYCbCr& fmtout) = 0;

	/* The following functions return information about the current frame,
	 * decoded by the last successful call to GetFrame, and will never be
	 * called before that. */

	/* Get the timestamp, in seconds, when the current frame should be
	 * displayed.  The first frame will always be 0. */
	virtual float GetTimestamp() const = 0;

	/* Get the duration, in seconds, to display the current frame. */
	virtual float GetFrameDuration() const = 0;
};

class MovieTexture_Generic : public RageMovieTexture
{
  public:
	MovieTexture_Generic(const RageTextureID& ID, MovieDecoder* pDecoder);
	virtual ~MovieTexture_Generic();
	std::string Init();

	/* only called by RageTextureManager::InvalidateTextures */
	void Invalidate();

	virtual void Reload();

	virtual void SetPosition(float fSeconds);
	virtual void DecodeSeconds(float fSeconds);
	virtual void SetPlaybackRate(float fRate) { m_fRate = fRate; }
	void SetLooping(bool bLooping = true) { m_bLoop = bLooping; }
	intptr_t GetTexHandle() const;

	static EffectMode GetEffectMode(MovieDecoderPixelFormatYCbCr fmt);

  private:
	MovieDecoder* m_pDecoder;

	float m_fRate;
	enum
	{
		FRAME_NONE,	  /* no frame available; call GetFrame to get one */
		FRAME_DECODED /* frame decoded; waiting until it's time to display it */
	} m_ImageWaiting;
	bool m_bLoop;
	bool m_bWantRewind;

	intptr_t m_uTexHandle;
	std::shared_ptr<RageTextureRenderTarget> m_pRenderTarget;
	std::shared_ptr<RageTexture> m_pTextureIntermediate;
	Sprite* m_pSprite;

	RageTextureLock* m_pTextureLock;

	/* The time the movie is actually at: */
	float m_fClock;
	bool m_bFrameSkipMode;

	void UpdateFrame();

	void CreateTexture();
	void DestroyTexture();

	bool DecodeFrame();
	float CheckFrameTime();
};

#endif
