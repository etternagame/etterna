#ifndef NOMINMAX // >:3
#define NOMINMAX
#endif
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __unix__
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "RendererVK.h"

#include <numbers>
#include <RageUtil/File/RageFileManager.h>
#include <RageUtil/Misc/RageMath.h>
#include <vulkan/vulkan_beta.h>
#include <thread>
#include "RenderTargetVK.h"
#include "PlatformUtils.h"
#include <RageUtil/Graphics/Display/Display.h>

std::string
RendererVK::GetApiDescription() const
{
	return "Vulkan";
}

bool
RendererVK::IsD3DInternal()
{
	return false;
}

DisplayAdapter::PipelineHandle
RendererVK::CreateGraphicsPipeline(const std::string& vertexShaderPath,
								   const std::string& fragmentShaderPath)
{
	assert(m_Cache.has_value());
	return m_Cache->CreateGraphicsPipeline(
	  m_Device, vertexShaderPath, fragmentShaderPath, m_MsaaSamples);
}

void
RendererVK::ReloadPipelines()
{
	m_Cache->ReloadPipelines(m_Device, m_MsaaSamples);
}
