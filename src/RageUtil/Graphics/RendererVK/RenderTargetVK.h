#ifndef RENDERER_VK_RENDER_TARGET_VK_H
#define RENDERER_VK_RENDER_TARGET_VK_H

#include "RageUtil/Graphics/RageDisplay.h"

// TODO: extract to a separate header? OGL/D3D/VK use the same class...
class RenderTarget
{
  public:
	virtual ~RenderTarget() = default;

	virtual void Create(const RenderTargetParam& param,
						int& iTextureWidthOut,
						int& iTextureHeightOut) = 0;

	[[nodiscard]] virtual auto GetTexture() const -> intptr_t = 0;

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

class RenderTargetVK : public RenderTarget
{
  public:
	void Create(const RenderTargetParam& param,
				int& iTextureWidthOut,
				int& iTextureHeightOut) override;
	auto GetTexture() const -> intptr_t override;
	void StartRenderingTo() override;
	void FinishRenderingTo() override;

	intptr_t m_Texture;
};

#endif
