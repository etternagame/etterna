#ifndef DISPLAY_RENDERER_H
#define DISPLAY_RENDERER_H

#include <string>
#include "RageUtil/Graphics/RageDisplay.h"
#include "TextureCommand.h"

namespace Display {
class Renderer
{
  public:
	virtual ~Renderer() {}
    virtual [[nodiscard]] std::string GetApiDescription() const = 0;
    virtual void StartLoadingPipeline() = 0;
	virtual void FinishLoadingPipeline(const VideoModeParams& p) = 0;
	virtual void LoadAssets(const VideoModeParams& p) = 0;
	virtual void OnRender(const ActualVideoModeParams* p) = 0;
	virtual bool IsD3DInternal() = 0;
	virtual intptr_t PushTextureCommand(const TextureCommand& command) = 0;
};
}

#endif
