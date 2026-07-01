#ifndef DISPLAY_RENDERER_H
#define DISPLAY_RENDERER_H

#include <string>
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageUtil/Graphics/Display/CommandBatcher.h"
#include "RageUtil/Graphics/RageSurface.h"

namespace DisplayAdapter {
class Renderer
{
  public:
	virtual ~Renderer() {}
	virtual std::string GetApiDescription() const = 0;
	virtual void InitializeRenderer(const VideoModeParams& p) = 0;
	virtual void OnRender(const ActualVideoModeParams* p,
						  const CommandBatcher& batcher) = 0;
	virtual bool IsD3DInternal() = 0;
	virtual intptr_t CreateTexture(RageSurface* img, bool RGBA8) = 0;
	virtual void UpdateTexture(intptr_t textureHandle,
							   RageSurface* img,
							   int xOffset,
							   int yOffset,
							   int width,
							   int height) = 0;
	virtual void DeleteTexture(intptr_t handle) = 0;
	virtual void ClearAllTextures() = 0;
	virtual void ResolutionChanged() = 0;
	virtual RageSurface* CreateScreenshot() = 0;
	virtual intptr_t CreateRenderTarget(const RenderTargetParam& param,
										int& iTextureWidthOut,
										int& iTextureHeightOut) = 0;
	virtual intptr_t CreateGraphicsPipeline(
	  const std::string& vertexShaderPath,
	  const std::string& fragmentShaderPath) = 0;
	virtual void ReloadPipelines() = 0;
	virtual void TryVideoMode(const VideoModeParams& params) = 0;
};
}

#endif
