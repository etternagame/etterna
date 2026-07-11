/* RageAnimatedTexture - loads an animation's (GIF) textures. */
#ifndef RAGEANIMATEDTEXTURE_H
#define RAGEANIMATEDTEXTURE_H

#include "RageTexture.h"

class RageAnimatedTexture : public RageTexture
{
  public:
	RageAnimatedTexture(const RageTextureID& id);
	~RageAnimatedTexture() override;
	void Invalidate() override { m_TextureHandle = 0; /* don't Destroy() */ }
	void Reload() override;
	void Update(float time) override;
	intptr_t GetTexHandle() const override { return m_TextureHandle; };

  private:
	void Create();
	void Destroy();
	intptr_t m_TextureHandle;
	size_t m_CurrentFrame;
	std::vector<RageSurface*> m_Surfaces;
	unsigned char* m_Pixels;
	int* m_Delays;
	float m_Time;
};

#endif
