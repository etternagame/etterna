#ifndef RENDERER_VK_PIPELINE_CACHE_H
#define RENDERER_VK_PIPELINE_CACHE_H

#include <cstdint>
#include <string>
#include <map>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

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

	void Init();
	void WriteToDisk();
	void ReloadPipelines();
	intptr_t CreateGraphicsPipeline(const std::string& vertexShaderPath,
									const std::string& fragmentShaderPath, bool reload = false);

	std::vector<PipelineInfo> m_Pipelines;
	std::map<std::pair<std::string, std::string>, intptr_t> m_PipelineLookup;

	// Device is owned by RendererVK which owns PipelineCache so this is... ok?
	vk::raii::Device* m_Device = nullptr;
	vk::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
	vk::DescriptorSetLayout m_TextureLayout = nullptr;
	vk::Format m_DepthFormat = {};
	vk::Format m_ImageFormat = {};
	vk::raii::PipelineCache m_DriverCache = nullptr;
};

#endif
