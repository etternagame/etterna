/* RageBitmapTexture - Loads a static texture. */

#ifndef RAGEBITMAPTEXTURE_H
#define RAGEBITMAPTEXTURE_H

#include "RageTexture.h"

class RageBitmapTexture : public RageTexture
{
  public:
	RageBitmapTexture(const RageTextureID& name);
	~RageBitmapTexture() override;
	/* only called by RageTextureManager::InvalidateTextures */
	void Invalidate() override { m_uTexHandle = 0; /* don't Destroy() */ }
	void Reload() override;
	intptr_t GetTexHandle() const override
	{
		return m_uTexHandle;
	}; // accessed by RageDisplay

  private:
	void Create(); // called by constructor and Reload
	void Destroy();
	intptr_t m_uTexHandle; // treat as unsigned in OpenGL, ID3D8Texture* for D3D
};

#endif
