/* RageTextureRenderTarget - RageTexture interface for creating render targets.
 */

#ifndef RAGE_TEXTURE_RENDER_TARGET_H
#define RAGE_TEXTURE_RENDER_TARGET_H

#include "RageDisplay.h" // for RenderTargetParam
#include "RageTexture.h"
#include "RageTextureID.h"

class RageTextureRenderTarget : public RageTexture
{
  public:
	RageTextureRenderTarget(const RageTextureID& name,
							const RenderTargetParam& param);
	~RageTextureRenderTarget() override;
	void Invalidate() override { m_iTexHandle = 0; /* don't Destroy() */ }
	void Reload() override;
	intptr_t GetTexHandle() const override { return m_iTexHandle; }

	void BeginRenderingTo(bool bPreserveTexture = true);
	void FinishRenderingTo();

	void PushSelf(lua_State* L) override;

  private:
	const RenderTargetParam m_Param;

	void Create();
	void Destroy();
	intptr_t m_iTexHandle;
	intptr_t m_iPreviousRenderTarget;
};

#endif
