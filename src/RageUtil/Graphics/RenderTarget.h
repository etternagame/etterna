#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "RenderTargetParam.h"
#include <cstdint>

class RenderTarget
{
  public:
	virtual ~RenderTarget() = default;

	virtual void Create(const RenderTargetParam& param,
						int& iTextureWidthOut,
						int& iTextureHeightOut) = 0;

	[[nodiscard]] virtual auto GetTexture() const -> uintptr_t = 0;

	/* Render to this RenderTarget. */
	virtual void StartRenderingTo() = 0;

	/* Stop rendering to this RenderTarget.  Update the texture, if necessary,
	 * and make it available. */
	virtual void FinishRenderingTo() = 0;

	[[nodiscard]] virtual auto InvertY() const -> bool { return false; }

	[[nodiscard]] auto GetParam() const -> const RenderTargetParam&
	{
		return m_Param;
	}

  protected:
	RenderTargetParam m_Param;
};

#endif
