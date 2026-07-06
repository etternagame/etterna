#ifndef RENDERER_VK_PIPELINE_CACHE_H
#define RENDERER_VK_PIPELINE_CACHE_H

#include <cstdint>
#include <string>
#include <map>
#include <utility>
#include <vulkan/vulkan_raii.hpp>
#include "RageUtil/Graphics/Display/PipelineHandle.h"

struct PipelineInfo
{
	vk::raii::PipelineLayout PipelineLayout = nullptr;
	vk::raii::Pipeline GraphicsPipeline = nullptr;
	std::string VertexShaderPath;
	std::string FragmentShaderPath;
};


struct PipelineCache
{
	constexpr static std::string_view CacheName = "Cache/pipelineCache.bin";

	void Init(vk::raii::Device& device);
	void WriteToDisk();
	void ReloadPipelines(vk::raii::Device& device);
	DisplayAdapter::PipelineHandle CreateGraphicsPipeline(
	  vk::raii::Device& device,
									const std::string& vertexShaderPath,
									const std::string& fragmentShaderPath,
									bool reload = false);

	std::vector<PipelineInfo> m_Pipelines;
	std::map<std::pair<std::string, std::string>, intptr_t> m_PipelineLookup;

	vk::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
	vk::DescriptorSetLayout m_TextureLayout = nullptr;
	vk::Format m_DepthFormat = {};
	vk::Format m_ImageFormat = {};
	vk::raii::PipelineCache m_DriverCache = nullptr;
};

#endif
