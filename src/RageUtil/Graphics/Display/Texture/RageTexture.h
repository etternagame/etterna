/* RageTexture - Abstract class for a texture and metadata.  */

#ifndef RAGE_TEXTURE_H
#define RAGE_TEXTURE_H

#include "RageTextureID.h"
#include "RageUtil/Misc/RageTimer.h"
#include "RageUtil/Misc/RageTypes.h"

struct lua_State;
struct RageSurface;

class RageTexture
{
  public:
	RageTexture(const RageTextureID& file);
	virtual ~RageTexture() = 0;
	virtual void Update(float /* fDeltaTime */) {}
	virtual void Reload() {}
	virtual void Invalidate() {
	} /* only called by RageTextureManager::InvalidateTextures */
	[[nodiscard]] virtual auto GetTexHandle() const
	  -> intptr_t = 0; // accessed by RageDisplay

	// movie texture/animated texture stuff
	virtual void SetPosition(float /* fSeconds */) {}	// seek
	virtual void DecodeSeconds(float /* fSeconds */) {} // decode
	virtual void SetPlaybackRate(float /*unused*/) {}
	[[nodiscard]] virtual auto IsAMovie() const -> bool { return false; }
	virtual void SetLooping(bool /*unused*/) {}

	[[nodiscard]] auto GetSourceWidth() const -> int { return m_iSourceWidth; }
	[[nodiscard]] auto GetSourceHeight() const -> int
	{
		return m_iSourceHeight;
	}
	[[nodiscard]] auto GetTextureWidth() const -> int
	{
		return m_iTextureWidth;
	}
	[[nodiscard]] auto GetTextureHeight() const -> int
	{
		return m_iTextureHeight;
	}
	[[nodiscard]] auto GetImageWidth() const -> int { return m_iImageWidth; }
	[[nodiscard]] auto GetImageHeight() const -> int { return m_iImageHeight; }

	[[nodiscard]] auto GetFramesWide() const -> int { return m_iFramesWide; }
	[[nodiscard]] auto GetFramesHigh() const -> int { return m_iFramesHigh; }

	[[nodiscard]] auto GetSourceFrameWidth() const -> int
	{
		return GetSourceWidth() / GetFramesWide();
	}
	[[nodiscard]] auto GetSourceFrameHeight() const -> int
	{
		return GetSourceHeight() / GetFramesHigh();
	}
	[[nodiscard]] auto GetTextureFrameWidth() const -> int
	{
		return GetTextureWidth() / GetFramesWide();
	}
	[[nodiscard]] auto GetTextureFrameHeight() const -> int
	{
		return GetTextureHeight() / GetFramesHigh();
	}
	[[nodiscard]] auto GetImageFrameWidth() const -> int
	{
		return GetImageWidth() / GetFramesWide();
	}
	[[nodiscard]] auto GetImageFrameHeight() const -> int
	{
		return GetImageHeight() / GetFramesHigh();
	}

	// Use these to convert between the different coordinate systems:
	[[nodiscard]] auto GetSourceToImageCoordsRatioX() const -> float
	{
		return float(GetImageWidth()) / GetSourceWidth();
	}
	[[nodiscard]] auto GetImageToTexCoordsRatioX() const -> float
	{
		return 1.0F / GetTextureWidth();
	}
	[[nodiscard]] auto GetSourceToTexCoordsRatioX() const -> float
	{
		return GetSourceToImageCoordsRatioX() * GetImageToTexCoordsRatioX();
	}
	[[nodiscard]] auto GetSourceToImageCoordsRatioY() const -> float
	{
		return float(GetImageHeight()) / GetSourceHeight();
	}
	[[nodiscard]] auto GetImageToTexCoordsRatioY() const -> float
	{
		return 1.0F / GetTextureHeight();
	}
	[[nodiscard]] auto GetSourceToTexCoordsRatioY() const -> float
	{
		return GetSourceToImageCoordsRatioY() * GetImageToTexCoordsRatioY();
	}

	[[nodiscard]] auto GetTextureCoordRect(int frameNo) const -> const RectF*;
	[[nodiscard]] auto GetNumFrames() const -> int
	{
		return m_iFramesWide * m_iFramesHigh;
	}

	// Used by RageTextureManager. Order is important; see
	// RageTextureManager.cpp.
	[[nodiscard]] auto GetPolicy() const -> const RageTextureID::TexPolicy&
	{
		return m_ID.Policy;
	}
	auto GetPolicy() -> RageTextureID::TexPolicy& { return m_ID.Policy; }
	int m_iRefCount;
	bool m_bWasUsed;
	RageTimer m_lastRefTime;

	// The ID that we were asked to load:
	[[nodiscard]] auto GetID() const -> const RageTextureID& { return m_ID; }

	static void GetFrameDimensionsFromFileName(const std::string& sPath,
											   int* puFramesWide,
											   int* puFramesHigh,
											   int source_width = 0,
											   int source_height = 0);

	virtual auto GetAverageColor(unsigned increment = 1) const -> const RageColor;

	// Lua
	virtual void PushSelf(lua_State* L);

  private:
	/* We might change settings when loading (due to hints, hardware
	 * limitations, etc). The data actually loaded is here: */
	RageTextureID m_ID;

  protected:
	int m_iSourceWidth,
	  m_iSourceHeight; // dimensions of the original image loaded from disk
	int m_iTextureWidth,
	  m_iTextureHeight;				   // dimensions of the texture in memory
	int m_iImageWidth, m_iImageHeight; // dimensions of the image in the texture
	int m_iFramesWide, m_iFramesHigh;  // The number of frames of animation in
									   // each row and column of this texture
	vector<RectF> m_TextureCoordRects; // size = m_iFramesWide * m_iFramesHigh
	RageSurface* m_pSurface{ nullptr };

	virtual void CreateFrameRects();
};

#endif
