#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __unix__
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "RenderTargetVK.h"
#include <cassert>

void
RenderTargetVK::Create(const RenderTargetParam& param,
					   int& iTextureWidthOut,
					   int& iTextureHeightOut)
{
	m_Param = param;
	const auto width = param.iWidth;
	const auto height = param.iHeight;

	iTextureWidthOut = width;
	iTextureHeightOut = height;

	m_Texture = 0;
}

auto
RenderTargetVK::GetTexture() const -> intptr_t
{
	assert(false && "Should not be called");
	return m_Texture;
}

void
RenderTargetVK::StartRenderingTo()
{
	assert(false && "Should not be called");
}

void
RenderTargetVK::FinishRenderingTo()
{
	assert(false && "Should not be called");
}
