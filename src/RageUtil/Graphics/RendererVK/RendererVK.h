#ifndef RENDERER_VULKAN_H
#define RENDERER_VULKAN_H

#include "RageUtil/Graphics/Display/Renderer.h"
#include "Core/Services/Locator.hpp"

#ifdef DEBUG
#define VKDEBUG 1
#endif
#ifdef _DEBUG
#define VKDEBUG 1
#endif

#ifdef VKDEBUG
#define VMA_DEBUG_LOG
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_DEBUG_LOG_FORMAT(format, ...)                                      \
	do {                                                                       \
		char buffer[256];                                                      \
		snprintf(buffer, sizeof(buffer), format, __VA_ARGS__);                 \
		std::string str(buffer);                                               \
		Locator::getLogger()->debug("VulkanMemoryAllocator: " + str);          \
	} while (false)
#endif

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <array>
#include <map>
#include <set>
#include <utility>
#include <optional>
#include "VkUtils.h"
#include "Texture.h"
#include "PersistentBuffer.h"
#include <bitset>
#include "PipelineCache.h"

class RendererVK : public DisplayAdapter::Renderer
{
  public:
	RendererVK();
	std::string GetApiDescription() const override;
	void InitializeRenderer(const VideoModeParams& p) override;
	void OnRender(const ActualVideoModeParams* p,
				  const DisplayAdapter::CommandBatcher& batcher) override;
	bool IsD3DInternal() override;
	intptr_t CreateTexture(RageSurface* img, bool RGBA8) override;
	void UpdateTexture(intptr_t textureHandle,
					   RageSurface* img,
					   int xOffset,
					   int yOffset,
					   int width,
					   int height) override;
	void DeleteTexture(intptr_t handle) override;
	void ClearAllTextures() override;
	RageSurface* CreateScreenshot() override;
	intptr_t CreateRenderTarget(const RenderTargetParam& param,
								int& iTextureWidthOut,
								int& iTextureHeightOut) override;
	intptr_t CreateGraphicsPipeline(
	  const std::string& vertexShaderPath,
	  const std::string& fragmentShaderPath) override;
	void ReloadPipelines() override;
	void TryVideoMode(const VideoModeParams& params) override;
	~RendererVK() override;

  private:
	vk::raii::Context m_Context;
	vk::raii::Instance m_Instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
	vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
	vk::raii::Device m_Device = nullptr;
	vk::raii::SurfaceKHR m_Surface = nullptr;
	vk::raii::Queue m_GraphicsQueue = nullptr;
	uint32_t m_GraphicsQueueFamily = 0;
	vk::raii::Queue m_PresentQueue = nullptr;
	uint32_t m_PresentQueueFamily = 0;
	VmaAllocator m_Allocator = nullptr;
	vk::Format m_DepthFormat = {};
	void InitVulkanState();

	vk::raii::SwapchainKHR m_Swapchain = nullptr;
	vk::Extent2D m_SwapchainExtent;
	std::vector<vk::Image> m_SwapchainImages;
	vk::Format m_ImageFormat = {};
	VkImage m_DepthImage = nullptr;
	VmaAllocation m_DepthAllocation = nullptr;
	vk::raii::ImageView m_DepthView = nullptr;

	bool m_SwapchainIsInvalid = false;
	void InitSwapchain(const VideoModeParams& p);
	void RecreateSwapchain(const VideoModeParams& p);
	void CleanupSwapchain();

	std::vector<vk::raii::ImageView> m_SwapchainImageViews;
	void InitImageViews();

	vk::raii::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
	vk::raii::DescriptorSetLayout m_TextureLayout = nullptr;
	void InitGraphicsPipeline();
	std::vector<vk::DescriptorSetLayoutBinding> GetDescriptorBindings();
	vk::raii::DescriptorPool m_DescriptorPool = nullptr;
	std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

	std::vector<vk::DescriptorSetLayoutBinding> GetTextureBindings();
	vk::raii::DescriptorPool m_TextureDescriptorPool = nullptr;
	vk::raii::DescriptorSet m_TextureDescriptorSet = nullptr;
	void UpdateTextureDescriptor(int index);

	vk::raii::CommandPool m_CommandPool = nullptr;
	void InitCommandPool();

	std::vector<vk::raii::CommandBuffer> m_CommandBuffers;
	void InitCommandBuffers();

	void TransitionImageLayout(vk::Image& image,
							   vk::ImageLayout oldLayout,
							   vk::ImageLayout newLayout,
							   vk::AccessFlags2 srcAccessMask,
							   vk::AccessFlags2 dstAccessMask,
							   vk::PipelineStageFlags2 srcStageMask,
							   vk::PipelineStageFlags2 dstStageMask,
							   vk::raii::CommandBuffer& commandBuffer);

	std::vector<vk::raii::Semaphore> m_PresentCompleteSemaphore;
	std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphore;
	std::vector<vk::raii::Fence> m_InFlightFence;
	uint32_t m_CurrentFrame = 0;
	void InitSyncStructures();
	void RecordCommands(uint32_t imageIndex,
						const DisplayAdapter::CommandBatcher& batcher);
	void SetBlendMode(BlendMode mode, vk::raii::CommandBuffer& buffer);

	constexpr static size_t FramesInFlight = 3;
	constexpr static size_t MaxDrawCount = 400'000;

	std::array<PersistentBuffer, FramesInFlight> m_VertexBuffer;
	std::array<PersistentBuffer, FramesInFlight> m_IndexBuffer;
	std::array<PersistentBuffer, FramesInFlight> m_MatrixStateBuffer;
	std::array<PersistentBuffer, FramesInFlight> m_ShaderScratchBuffer;
	std::array<PersistentBuffer, FramesInFlight> m_StagingBuffer;
	PersistentBuffer m_TextureBuffer;

	void InitBatchBuffers();
	void UpdateBatchBuffers(const DisplayAdapter::CommandBatcher& batcher);

	std::map<intptr_t, Texture> m_Textures;
	std::set<intptr_t> m_EmptyTextureSlots;
	int GetMaxTextureSize();
	int GetMaxTextureCount();
	int m_TextureCount = 0;
	void DestroyTexture(Texture& texture);

	std::array<vk::raii::Sampler, Texture::PossibleSamplerCount> m_Samplers;
	void InitTextures();
	void ResolutionChanged() override;
	intptr_t CreateRenderTargetTexture(int width, int height);

	std::optional<PipelineCache> m_Cache;
};

#endif
