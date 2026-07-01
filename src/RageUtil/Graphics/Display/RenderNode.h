#ifndef DISPLAY_RENDER_NODE_H
#define DISPLAY_RENDER_NODE_H

#include <cstdint>
#include <vector>
#include <set>

namespace DisplayAdapter {

struct PipelineSettings
{
	intptr_t GraphicsPipeline = 0;
	uint64_t VertexShaderArg = UINT64_MAX;
	uint64_t FragShaderArg = UINT64_MAX;
};

struct DrawCall
{
	PipelineSettings Settings = {};
	size_t IndexOffset = 0;
	size_t IndexCount = 0;
	BlendMode BlendingMode = BLEND_NORMAL;
	ZTestMode DepthTestMode = ZTEST_OFF;
	bool DepthWriteEnabled = false;
};

struct RenderNode
{
	intptr_t RenderTarget = 0;
	bool PreserveRenderTarget = false;
	std::vector<DrawCall> DrawCalls;
};

}

#endif
