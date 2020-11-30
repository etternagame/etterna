#ifndef CORE_GRAPHICS_DISPLAY_OPENGL_FRAMEBUFFERRENDERTARGETGL_HPP
#define CORE_GRAPHICS_DISPLAY_OPENGL_FRAMEBUFFERRENDERTARGETGL_HPP

#include "RageDisplay_OGL_Helpers.h"

class RenderTarget_FramebufferObject : public RenderTarget {
  public:
	RenderTarget_FramebufferObject();
	~RenderTarget_FramebufferObject() override;
	void Create(const RenderTargetParam& param, int& iTextureWidthOut, int& iTextureHeightOut) override;
	unsigned GetTexture() const override { return m_iTexHandle; }
	void StartRenderingTo() override;
	void FinishRenderingTo() override;
	bool InvertY() const override { return true; }

  private:
    std::vector<RageVector3> m_vPosition;
	std::vector<RageVector2> m_vTexture;
	std::vector<RageVector3> m_vNormal;
	std::vector<msTriangle> m_vTriangles;
	std::vector<RageVector2> m_vTexMatrixScale;

	unsigned int m_iFrameBufferHandle;
	unsigned int m_iTexHandle;
	unsigned int m_iDepthBufferHandle;
};

#endif //CORE_GRAPHICS_DISPLAY_OPENGL_FRAMEBUFFERRENDERTARGETGL_HPP
