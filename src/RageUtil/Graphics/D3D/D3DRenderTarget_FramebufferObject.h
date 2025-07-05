#ifndef D3D_RENDER_TARGET_FRAMEBUFFER_OBJECT_H
#define D3D_RENDER_TARGET_FRAMEBUFFER_OBJECT_H

#include "RageUtil/Graphics/RenderTarget.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnew-returns-null"
#pragma clang diagnostic ignored "-Wcomment"
#endif
#include <d3dx9tex.h>
#include <d3d9.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

// Ported from OpenGL - xwidghet
class D3DRenderTarget_FramebufferObject : public RenderTarget
{
  public:
	D3DRenderTarget_FramebufferObject();
	~D3DRenderTarget_FramebufferObject() override;
	void Create(const RenderTargetParam& param,
				int& iTextureWidthOut,
				int& iTextureHeightOut) override;
	[[nodiscard]] auto GetTexture() const -> uintptr_t override
	{
		return reinterpret_cast<uintptr_t>(m_uTexHandle);
	}
	void StartRenderingTo() override;
	void FinishRenderingTo() override;

	[[nodiscard]] auto InvertY() const -> bool override { return true; }

  private:
	IDirect3DSurface9* m_iFrameBufferHandle;
	IDirect3DTexture9* m_uTexHandle;
	IDirect3DSurface9* m_iDepthBufferHandle;
};

#endif