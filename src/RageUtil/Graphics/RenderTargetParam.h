#ifndef RENDER_TARGET_PARAM_H
#define RENDER_TARGET_PARAM_H

struct RenderTargetParam
{
	RenderTargetParam() = default;

	// The dimensions of the actual render target, analogous to a window size:
	int iWidth{ 0 }, iHeight{ 0 };

	bool bWithDepthBuffer{ false };
	bool bWithAlpha{ false };
	bool bFloat{ false };
};

#endif