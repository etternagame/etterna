#ifndef DISPLAY_RENDER_STATE_H
#define DISPLAY_RENDER_STATE_H

#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Graphics/RageDisplay.h"

namespace DisplayAdapter {

struct RenderState
{
	bool textureWrapping = false;
	bool textureFiltering = false;
	intptr_t textureHandle = 0;
	BlendMode blendingMode = BLEND_NORMAL;
	ZTestMode depthTestMode = ZTEST_OFF;
	bool depthWriteEnabled = false;
};

} // namespace DisplayAdapter

#endif
