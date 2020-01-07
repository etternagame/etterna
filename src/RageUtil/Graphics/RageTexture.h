/* RageTexture - Abstract class for a texture and metadata.  */

#ifndef RAGE_TEXTURE_H
#define RAGE_TEXTURE_H

#include "RageTextureID.h"
#include "RageUtil/Misc/RageTimer.h"
#include "RageUtil/Misc/RageTypes.h"

struct lua_State;
class RageTexture
{
  public:
	RageTexture(const RageTextureID& file);
	virtual ~RageTexture() = 0;
	virtual void Update(float /* fDeltaTime */) {}
	virtual void Reload() {}
	virtual void Invalidate() {
	} /* only called by RageTextureManager::InvalidateTextures */
	virtual intptr_t GetTexHandle() const = 0; // accessed by RageDisplay

	// movie texture/animated texture stuff
	virtual void SetPosition(float /* fSeconds */) {}   // seek
	virtual void DecodeSeconds(float /* fSeconds */) {} // decode
	virtual void SetPlaybackRate(float) {}
	virtual bool IsAMovie() const { return false; }
	virtual void SetLooping(bool) {}

	int GetSourceWidth() const { return m_iSourceWidth; }
	int GetSourceHeight() const { return m_iSourceHeight; }
	int GetTextureWidth() const { return m_iTextureWidth; }
	int GetTextureHeight() const { return m_iTextureHeight; }
	int GetImageWidth() const { return m_iImageWidth; }
	int GetImageHeight() const { return m_iImageHeight; }

	int GetFramesWide() const { return m_iFramesWide; }
	int GetFramesHigh() const { return m_iFramesHigh; }

	int GetSourceFrameWidth() const
	{
		return GetSourceWidth() / GetFramesWide();
	}
	int GetSourceFrameHeight() const
	{
		return GetSourceHeight() / GetFramesHigh();
	}
	int GetTextureFrameWidth() const
	{
		return GetTextureWidth() / GetFramesWide();
	}
	int GetTextureFrameHeight() const
	{
		return GetTextureHeight() / GetFramesHigh();
	}
	int GetImageFrameWidth() const { return GetImageWidth() / GetFramesWide(); }
	int GetImageFrameHeight() const
	{
		return GetImageHeight() / GetFramesHigh();
	}

	// Use these to convert between the different coordinate systems:
	float GetSourceToImageCoordsRatioX() const
	{
		return float(GetImageWidth()) / GetSourceWidth();
	}
	float GetImageToTexCoordsRatioX() const { return 1.0f / GetTextureWidth(); }
	float GetSourceToTexCoordsRatioX() const
	{
		return GetSourceToImageCoordsRatioX() * GetImageToTexCoordsRatioX();
	}
	float GetSourceToImageCoordsRatioY() const
	{
		return float(GetImageHeight()) / GetSourceHeight();
	}
	float GetImageToTexCoordsRatioY() const
	{
		return 1.0f / GetTextureHeight();
	}
	float GetSourceToTexCoordsRatioY() const
	{
		return GetSourceToImageCoordsRatioY() * GetImageToTexCoordsRatioY();
	}

	const RectF* GetTextureCoordRect(int frameNo) const;
	int GetNumFrames() const { return m_iFramesWide * m_iFramesHigh; }

	// Used by RageTextureManager. Order is important; see
	// RageTextureManager.cpp.
	const RageTextureID::TexPolicy& GetPolicy() const { return m_ID.Policy; }
	RageTextureID::TexPolicy& GetPolicy() { return m_ID.Policy; }
	int m_iRefCount;
	bool m_bWasUsed;
	RageTimer m_lastRefTime;

	// The ID that we were asked to load:
	const RageTextureID& GetID() const { return m_ID; }

	static void GetFrameDimensionsFromFileName(const RString& sPath,
											   int* puFramesWide,
											   int* puFramesHigh,
											   int source_width = 0,
											   int source_height = 0);

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
	std::vector<RectF> m_TextureCoordRects; // size = m_iFramesWide * m_iFramesHigh

	virtual void CreateFrameRects();
};

#endif
