#ifndef NOMINMAX // >:3
#define NOMINMAX
#endif
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __unix__
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#define VMA_IMPLEMENTATION
#include "RendererVK.h"

#include <numbers>
#include <RageUtil/File/RageFileManager.h>
#include <RageUtil/Misc/RageMath.h>
#include <vulkan/vulkan_beta.h>
#include <thread>
#include "RenderTargetVK.h"
#include "PlatformUtils.h"
#include <RageUtil/Graphics/Display/Display.h>
#include <Etterna/Singletons/PrefsManager.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

RendererVK::RendererVK()
  : m_Samplers{ nullptr, nullptr, nullptr, nullptr }
{
}

RendererVK::~RendererVK()
{
	if (m_Device != nullptr) {
		m_Device.waitIdle();
	}

	if (m_Cache.has_value()) {
		m_Cache->WriteToDisk();
	}

	for (auto& [handle, texture] : m_Textures) {
		DestroyTexture(texture);
	}

	for (auto& [handle, texture] : m_DepthTextures) {
		DestroyTexture(texture);
	}

	for (auto& texture : m_SwapchainDepthTextures) {
		DestroyTexture(texture);
	}

	for (auto& texture : m_MsaaTextures) {
		DestroyTexture(texture);
	}

	for (int i = 0; i < FramesInFlight; i++) {
		m_VertexBuffer[i].Destroy();
		m_IndexBuffer[i].Destroy();
		m_MatrixStateBuffer[i].Destroy();
		m_StagingBuffer[i].Destroy();
		m_ShaderScratchBuffer[i].Destroy();
	}

	if (m_Allocator != nullptr) {
		vmaDestroyAllocator(m_Allocator);
	}
}

void
RendererVK::InitializeRenderer(const VideoModeParams& p)
{
	InitVulkanState();
	InitSwapchain(p.width,
				  p.height,
				  p.vsync,
				  p.bWindowIsFullscreenBorderless,
				  p.bSmoothLines);
	InitImageViews();
	InitBatchDescriptors();
	InitBatchBuffers(1);
	InitTextureInfo();
	InitGraphicsPipeline();
	InitCommandPool();
	InitCommandBuffers();
	InitSyncStructures();
	InitTextures();
}

#pragma region init utils

static VkBool32
VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
					VkDebugUtilsMessageTypeFlagsEXT messageTypes,
					const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
					void* pUserData)
{
	switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
			Locator::getLogger()->trace("RendererVK debug callback: {}",
										pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
			Locator::getLogger()->warn("RendererVK debug callback: {}",
									   pCallbackData->pMessage);
			break;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			Locator::getLogger()->error("RendererVK debug callback: {}",
										pCallbackData->pMessage);
			break;
		}
		default: {
			Locator::getLogger()->info("RendererVK debug callback: {}",
									   pCallbackData->pMessage);
			break;
		}
	}

	return VK_FALSE;
}

static const char* VulkanFeatureHintNames[] = { "UseIfSupported",
												"Require",
												"Disable" };
StringToX(VulkanFeatureHint);

static const char* VulkanFeatureNameNames[] = {
	"ExtendedDynamicState3BlendingExt",
	"MeshShadersExt",
	"ShaderImageInt64AtomicsExt",
	"MemoryBudgetExt",
	"NvidiaLowLatency2",
	"PresentTimingExt"
};
StringToX(VulkanFeatureName);

#pragma endregion

void
RendererVK::InitVulkanState()
{
	// init changes all UseIfSupported to Require/Disable after checking if
	// they are present with vkbootstrap
	m_ResolvedSettings = GetFeatureSettings();

	VULKAN_HPP_DEFAULT_DISPATCHER.init();

	auto instanceResult = CreateInstance(VulkanDebugCallback);
	if (!instanceResult) {
		Locator::getLogger()->fatal("RendererVK: instance creation failed - {}",
									GetDetailedErrorString(instanceResult));
		Fail();
	}

	m_Instance = vk::raii::Instance(m_Context, instanceResult->instance);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_Instance);

	if (DISPLAY->DisplayDebugModeEnabled()) {
		m_DebugMessenger = vk::raii::DebugUtilsMessengerEXT(
		  m_Instance, instanceResult->debug_messenger);
	}

	m_Surface = CreateSurfaceKHR(m_Instance);

	VkPhysicalDeviceVulkan13Features vk13Features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
	};
	vk13Features.dynamicRendering = vk::True;
	vk13Features.synchronization2 = vk::True;

	VkPhysicalDeviceVulkan12Features vk12Features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
	};
	vk12Features.bufferDeviceAddress = vk::True;
	vk12Features.descriptorIndexing = vk::True;
	vk12Features.runtimeDescriptorArray = vk::True;
	vk12Features.shaderSampledImageArrayNonUniformIndexing = vk::True;
	vk12Features.scalarBlockLayout = vk::True;
	vk12Features.descriptorBindingSampledImageUpdateAfterBind = vk::True;
	vk12Features.descriptorBindingPartiallyBound = vk::True;

	VkPhysicalDeviceFeatures vkFeatures = {};
	vkFeatures.samplerAnisotropy = vk::True;
	vkFeatures.logicOp = vk::True;
	vkFeatures.shaderInt64 = vk::True;

	vkb::PhysicalDeviceSelector selector(*instanceResult);
	selector = selector.set_minimum_version(1, 3)
				 .set_required_features_13(vk13Features)
				 .set_required_features_12(vk12Features)
				 .set_required_features(vkFeatures)
				 .set_surface(static_cast<vk::SurfaceKHR>(m_Surface));
#ifdef __APPLE__
	selector =
	  selector.add_required_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicState3Features{};
	dynamicState3Features.sType =
	  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
	dynamicState3Features.extendedDynamicState3ColorBlendEnable = VK_TRUE;
	dynamicState3Features.extendedDynamicState3ColorBlendEquation = VK_TRUE;
	dynamicState3Features.extendedDynamicState3ColorWriteMask = VK_TRUE;

	if (m_ResolvedSettings.EDS3Blending == VFH_Require) {
		selector = selector
					 .add_required_extension(
					   VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
					 .add_required_extension_features(dynamicState3Features);
	}

	VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
	meshShaderFeatures.sType =
	  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	meshShaderFeatures.meshShader = VK_TRUE;

	if (m_ResolvedSettings.MeshShaders == VFH_Require) {
		selector =
		  selector.add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
			.add_required_extension_features(meshShaderFeatures);
	}

	auto physicalDeviceResult = selector.select();
	if (!physicalDeviceResult) {
		Locator::getLogger()->fatal(
		  "RendererVK: physical device creation failed - {}",
		  GetDetailedErrorString(physicalDeviceResult));
		Fail();
	}

	if (m_ResolvedSettings.EDS3Blending == VFH_UseIfSupported) {
		m_ResolvedSettings.EDS3Blending =
		  physicalDeviceResult->enable_extension_if_present(
			VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME) &&
			  physicalDeviceResult->enable_extension_features_if_present(
				dynamicState3Features)
			? VFH_Require
			: VFH_Disable;
	}

	if (m_ResolvedSettings.MeshShaders == VFH_UseIfSupported) {
		m_ResolvedSettings.MeshShaders =
		  physicalDeviceResult->enable_extension_if_present(
			VK_EXT_MESH_SHADER_EXTENSION_NAME) &&
			  physicalDeviceResult->enable_extension_features_if_present(
				meshShaderFeatures)
			? VFH_Require
			: VFH_Disable;
	}

	vkb::DeviceBuilder deviceBuilder(*physicalDeviceResult);

	auto deviceResult = deviceBuilder.build();
	if (!deviceResult) {
		Locator::getLogger()->fatal(
		  "RendererVK: device creation failed - {}",
		  GetDetailedErrorString(physicalDeviceResult));
		Fail();
	}

	m_PhysicalDevice = vk::raii::PhysicalDevice(
	  m_Instance, physicalDeviceResult->physical_device);
	m_Device = vk::raii::Device(m_PhysicalDevice, deviceResult->device);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_Device);

	Locator::getLogger()->info("RendererVK: selected GPU: {}",
							   physicalDeviceResult->name);

	m_GraphicsQueue = vk::raii::Queue(
	  m_Device, deviceResult->get_queue(vkb::QueueType::graphics).value());
	m_GraphicsQueueFamily =
	  deviceResult->get_queue_index(vkb::QueueType::graphics).value();

	m_PresentQueue = vk::raii::Queue(
	  m_Device, deviceResult->get_queue(vkb::QueueType::present).value());
	m_PresentQueueFamily =
	  deviceResult->get_queue_index(vkb::QueueType::present).value();

	VmaVulkanFunctions vulkanFunctions{};
	vulkanFunctions.vkGetInstanceProcAddr =
	  VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr =
	  VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice =
	  static_cast<vk::PhysicalDevice>(m_PhysicalDevice);
	allocatorInfo.device = static_cast<vk::Device>(m_Device);
	allocatorInfo.instance = static_cast<vk::Instance>(m_Instance);
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.pVulkanFunctions = &vulkanFunctions;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

	ThrowIfFail(vmaCreateAllocator(&allocatorInfo, &m_Allocator));

	const std::vector<vk::Format> depthFormats{ vk::Format::eD32SfloatS8Uint,
												vk::Format::eD24UnormS8Uint };
	for (const auto& format : depthFormats) {
		auto props = m_PhysicalDevice.getFormatProperties2(format);
		if (props.formatProperties.optimalTilingFeatures &
			vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			m_DepthFormat = format;
			break;
		}
	}

	auto limits = m_PhysicalDevice.getProperties().limits;
	auto counts =
	  limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;

	if (counts & vk::SampleCountFlagBits::e4) {
		m_MsaaSamples = vk::SampleCountFlagBits::e4;
	} else if (counts & vk::SampleCountFlagBits::e2) {
		m_MsaaSamples = vk::SampleCountFlagBits::e2;
	} else {
		m_MsaaSamples = vk::SampleCountFlagBits::e1;
	}
}

RendererVKFeatureSettings
RendererVK::GetFeatureSettings()
{
	std::string hints = PREFSMAN->m_VulkanExtHints;

	RendererVKFeatureSettings defaults = {
		VFH_UseIfSupported, VFH_UseIfSupported, VFH_UseIfSupported,
		VFH_UseIfSupported, VFH_Disable,		VFH_Disable,
	};

	if (hints.empty()) {
		return defaults;
	}

	std::vector<std::string> hintStrings;
	split(hints, ",", hintStrings);

	std::vector<std::string> parts;
	for (const auto& hintString : hintStrings) {
		parts.clear();
		split(hintString, ":", parts, false);

		if (parts.size() != 2) {
			Locator::getLogger()->fatal(
			  "Invalid Vulkan renderer feature hint \'{}\' (expected in the "
			  "form \'FeatureName:(UseIfSupported|Require|Disable)\')",
			  hintString);
			Fail();
		}

		VulkanFeatureName name = StringToVulkanFeatureName(parts[0]);
		VulkanFeatureHint hint = StringToVulkanFeatureHint(parts[1]);

		if (name == VulkanFeatureName_Invalid) {
			Locator::getLogger()->fatal("Invalid feature name \'{}\' in Vulkan "
										"renderer feature hint \'{}\'",
										parts[0],
										hintString);
			Fail();
		}

		if (hint == VulkanFeatureName_Invalid) {
			Locator::getLogger()->fatal(
			  "Invalid feature hint type \'{}\' in Vulkan "
			  "renderer feature hint \'{}\'",
			  parts[1],
			  hintString);
			Fail();
		}

		switch (name) {
			case VFN_ExtendedDynamicState3BlendingExt: {
				defaults.EDS3Blending = hint;
				break;
			}
			case VFN_MeshShadersExt: {
				defaults.MeshShaders = hint;
				break;
			}
			case VFN_ShaderImageInt64AtomicsExt: {
				defaults.ImageInt64Atomics = hint;
				break;
			}
			case VFN_MemoryBudgetExt: {
				defaults.MemoryBudget = hint;
				break;
			}
			case VFN_NvidiaLowLatency2: {
				defaults.NvidiaLowLatency2 = hint;
				break;
			}
			case VFN_PresentTimingExt: {
				defaults.PresentTiming = hint;
				break;
			}
			default: {
				Locator::getLogger()->fatal(
				  "Invalid feature name \'{}\' in Vulkan "
				  "renderer feature hint \'{}\'",
				  parts[0],
				  hintString);
				Fail();
			}
		}
	}

	return defaults;
}

void
RendererVK::InitSwapchain(uint32_t width,
						  uint32_t height,
						  bool vSync,
						  bool borderlessWindow,
						  bool smoothLines)
{
	m_SwapchainVSync = vSync;
	m_SwapchainBorderless = borderlessWindow;
	m_SmoothLines = smoothLines;

	if (!m_SmoothLines) {
		m_MsaaSamples = vk::SampleCountFlagBits::e1;
	}

	vkb::SwapchainBuilder swapchainBuilder(
	  *m_PhysicalDevice, *m_Device, *m_Surface);

	swapchainBuilder.set_desired_min_image_count(FramesInFlight)
	  .set_desired_format(
		{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	  .set_desired_format(
		{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	  .set_desired_present_mode(vSync ? VK_PRESENT_MODE_FIFO_KHR
									  : VK_PRESENT_MODE_IMMEDIATE_KHR)
	  .set_desired_extent(width, height)
	  .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
							 VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	  .set_clipped(true);

	auto caps = *m_PhysicalDevice.getSurfaceCapabilitiesKHR(*m_Surface);
	swapchainBuilder.set_pre_transform_flags(caps.currentTransform);

	VkCompositeAlphaFlagBitsKHR compositeAlpha =
	  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if (!(caps.supportedCompositeAlpha & compositeAlpha)) {
		if (caps.supportedCompositeAlpha &
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
			compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
		else
			compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	swapchainBuilder.set_composite_alpha_flags(compositeAlpha);

#ifdef _WIN32
	VkSurfaceFullScreenExclusiveInfoEXT fullScreenInfo = {
		VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT
	};
	fullScreenInfo.fullScreenExclusive =
	  borderlessWindow ? VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT
					   : VK_FULL_SCREEN_EXCLUSIVE_ALLOWED_EXT;
	swapchainBuilder.add_pNext(&fullScreenInfo);
#endif

	auto swapchain_ret = swapchainBuilder.build();
	if (!swapchain_ret) {
		Locator::getLogger()->fatal(
		  "RendererVK: swapchain creation failed - {}",
		  GetDetailedErrorString(swapchain_ret));
		Fail();
	}

	vkb::Swapchain vkbSwapchain = swapchain_ret.value();

	m_Swapchain = vk::raii::SwapchainKHR(m_Device, vkbSwapchain.swapchain);
	m_SwapchainImages = m_Swapchain.getImages();
	m_ImageFormat = static_cast<vk::Format>(vkbSwapchain.image_format);
	m_SwapchainExtent =
	  vk::Extent2D(vkbSwapchain.extent.width, vkbSwapchain.extent.height);

	for (int i = 0; auto& texture : m_SwapchainDepthTextures) {
		DestroyTexture(texture);

		texture.width = vkbSwapchain.extent.width;
		texture.height = vkbSwapchain.extent.height;

		vk::ImageCreateInfo depthImageInfo = {};
		depthImageInfo.imageType = vk::ImageType::e2D;
		depthImageInfo.format = m_DepthFormat;
		depthImageInfo.extent = vk::Extent3D(
		  vkbSwapchain.extent.width, vkbSwapchain.extent.height, 1);
		depthImageInfo.mipLevels = 1;
		depthImageInfo.arrayLayers = 1;
		depthImageInfo.samples =
		  smoothLines ? m_MsaaSamples : vk::SampleCountFlagBits::e1;
		depthImageInfo.tiling = vk::ImageTiling::eOptimal;
		depthImageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		depthImageInfo.initialLayout = vk::ImageLayout::eUndefined;

		VmaAllocationCreateInfo depthAllocInfo = {};
		depthAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		depthAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		VkImage depthImagePtr = VK_NULL_HANDLE;
		ThrowIfFail(vmaCreateImage(m_Allocator,
								   &*depthImageInfo,
								   &depthAllocInfo,
								   &depthImagePtr,
								   &texture.allocation,
								   nullptr));
		texture.image = depthImagePtr;

		vk::ImageViewCreateInfo depthViewInfo = {};
		depthViewInfo.image = texture.image;
		depthViewInfo.viewType = vk::ImageViewType::e2D;
		depthViewInfo.format = m_DepthFormat;
		vk::ImageSubresourceRange subRange = {};
		subRange.aspectMask =
		  vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		subRange.levelCount = 1;
		subRange.layerCount = 1;
		depthViewInfo.subresourceRange = subRange;
		texture.view = (*m_Device).createImageView(depthViewInfo);

		m_DirtyDepthTextures.push_back(-1 * static_cast<intptr_t>(i));

		i++;
	}

	if (!smoothLines || m_MsaaSamples & vk::SampleCountFlagBits::e1) {
		return;
	}

	for (auto& texture : m_MsaaTextures) {
		DestroyTexture(texture);

		texture.width = vkbSwapchain.extent.width;
		texture.height = vkbSwapchain.extent.height;

		vk::ImageCreateInfo imageInfo = {};
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.format = m_ImageFormat;
		imageInfo.extent = vk::Extent3D(
		  vkbSwapchain.extent.width, vkbSwapchain.extent.height, 1);
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = m_MsaaSamples;
		imageInfo.tiling = vk::ImageTiling::eOptimal;
		imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment |
						  vk::ImageUsageFlagBits::eTransientAttachment;
		imageInfo.initialLayout = vk::ImageLayout::eUndefined;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

		VkImage depthImagePtr = VK_NULL_HANDLE;
		ThrowIfFail(vmaCreateImage(m_Allocator,
								   &*imageInfo,
								   &allocInfo,
								   &depthImagePtr,
								   &texture.allocation,
								   nullptr));
		texture.image = depthImagePtr;

		vk::ImageViewCreateInfo viewInfo = {};
		viewInfo.image = texture.image;
		viewInfo.viewType = vk::ImageViewType::e2D;
		viewInfo.format = m_ImageFormat;
		vk::ImageSubresourceRange subRange = {};
		subRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		subRange.levelCount = 1;
		subRange.layerCount = 1;
		viewInfo.subresourceRange = subRange;
		texture.view = (*m_Device).createImageView(viewInfo);
	}

	m_MsaaTexturesAreDirty = true;
}

void
RendererVK::RecreateSwapchain()
{
	m_Device.waitIdle();

	CleanupSwapchain();
	InitSwapchain(m_SwapchainExtent.width,
				  m_SwapchainExtent.height,
				  m_SwapchainVSync,
				  m_SwapchainBorderless,
				  m_SmoothLines);
	InitImageViews();
	InitSyncStructures();
}

void
RendererVK::CleanupSwapchain()
{
	m_SwapchainImageViews.clear();
	m_Swapchain = nullptr;

	for (auto& texture : m_SwapchainDepthTextures) {
		DestroyTexture(texture);
	}

	for (auto& texture : m_MsaaTextures) {
		DestroyTexture(texture);
	}
}

void
RendererVK::InitImageViews()
{
	m_SwapchainImageViews.clear();
	vk::ImageViewCreateInfo createInfo{};
	createInfo.viewType = vk::ImageViewType::e2D;
	createInfo.format = m_ImageFormat;
	createInfo.subresourceRange = {
		vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
	};

	for (auto& image : m_SwapchainImages) {
		createInfo.image = image;
		m_SwapchainImageViews.emplace_back(m_Device, createInfo);
	}
}

void
RendererVK::InitGraphicsPipeline()
{
	m_Cache.emplace();
	m_Cache->m_DescriptorSetLayout = m_DescriptorSetLayout;
	m_Cache->m_TextureLayout = m_TextureLayout;
	m_Cache->m_DepthFormat = m_DepthFormat;
	m_Cache->m_ImageFormat = m_ImageFormat;
	m_Cache->Init(m_Device, m_ResolvedSettings.EDS3Blending == VFH_Require);

	CreateGraphicsPipeline("Data/Shaders/Vulkan/vertex.glsl",
						   "Data/Shaders/Vulkan/fragment.glsl");
}

std::vector<vk::DescriptorSetLayoutBinding>
RendererVK::GetDescriptorBindings()
{
	return { vk::DescriptorSetLayoutBinding(0,
											vk::DescriptorType::eStorageBuffer,
											1,
											vk::ShaderStageFlagBits::eVertex),
			 vk::DescriptorSetLayoutBinding(1,
											vk::DescriptorType::eStorageBuffer,
											1,
											vk::ShaderStageFlagBits::eVertex) };
}

std::vector<vk::DescriptorSetLayoutBinding>
RendererVK::GetTextureBindings()
{
	return {
		vk::DescriptorSetLayoutBinding(2,
									   vk::DescriptorType::eSampledImage,
									   GetMaxTextureCount(),
									   vk::ShaderStageFlagBits::eAllGraphics),
		vk::DescriptorSetLayoutBinding(3,
									   vk::DescriptorType::eSampler,
									   Texture::PossibleSamplerCount,
									   vk::ShaderStageFlagBits::eAllGraphics)
	};
}

void
RendererVK::InitCommandPool()
{
	vk::CommandPoolCreateInfo poolInfo{};
	poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;

	m_CommandPool = vk::raii::CommandPool(m_Device, poolInfo);
}

void
RendererVK::InitCommandBuffers()
{
	m_CommandBuffers.clear();
	vk::CommandBufferAllocateInfo allocInfo{};
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = FramesInFlight;

	m_CommandBuffers = vk::raii::CommandBuffers(m_Device, allocInfo);
}

void
RendererVK::InitSyncStructures()
{
	m_PresentCompleteSemaphore.clear();
	m_RenderFinishedSemaphore.clear();
	m_InFlightFence.clear();

	for (size_t i = 0; i < FramesInFlight; i++) {
		m_PresentCompleteSemaphore.emplace_back(m_Device,
												vk::SemaphoreCreateInfo());
		m_InFlightFence.emplace_back(
		  m_Device, vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
	}
	for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
		m_RenderFinishedSemaphore.emplace_back(m_Device,
											   vk::SemaphoreCreateInfo());
	}
}

void
RendererVK::InitBatchDescriptors()
{
	vk::DescriptorPoolSize poolSizes[1] = {};
	poolSizes[0].type = vk::DescriptorType::eStorageBuffer;
	poolSizes[0].descriptorCount = 2 * FramesInFlight;

	vk::DescriptorPoolCreateInfo poolInfo = {};
	poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = FramesInFlight;
	m_DescriptorPool = vk::raii::DescriptorPool(m_Device, poolInfo);

	auto bindings = GetDescriptorBindings();
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, bindings);

	m_DescriptorSetLayout = vk::raii::DescriptorSetLayout(m_Device, layoutInfo);

	std::vector<vk::DescriptorSetLayout> layouts(FramesInFlight,
												 *m_DescriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo(
	  *m_DescriptorPool, FramesInFlight, layouts.data());
	m_DescriptorSets = m_Device.allocateDescriptorSets(allocInfo);
}

void
RendererVK::InitBatchBuffers(size_t sizeScale)
{
	for (int i = 0; i < FramesInFlight; i++) {
		vk::BufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.size = sizeof(DisplayAdapter::Vertex) *
								DisplayAdapter::MaxVertexCount * sizeScale;
		vertexBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst |
								 vk::BufferUsageFlagBits::eStorageBuffer;
		VmaAllocationCreateInfo vertexAllocInfo = {};
		vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		m_VertexBuffer[i].Init(m_Allocator, vertexBufferInfo, vertexAllocInfo);

		vk::BufferCreateInfo indexBufferInfo{};
		indexBufferInfo.size =
		  sizeof(uint32_t) * 4 * DisplayAdapter::MaxVertexCount * sizeScale;
		indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer |
								vk::BufferUsageFlagBits::eTransferDst;
		VmaAllocationCreateInfo indexAllocInfo = {};
		indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		m_IndexBuffer[i].Init(m_Allocator, indexBufferInfo, indexAllocInfo);

		vk::BufferCreateInfo matrixBufferInfo{};
		matrixBufferInfo.size = sizeof(DisplayAdapter::MatrixState) *
								DisplayAdapter::MaxVertexCount * sizeScale;
		matrixBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer |
								 vk::BufferUsageFlagBits::eTransferDst;
		VmaAllocationCreateInfo matrixAllocInfo = {};
		matrixAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		m_MatrixStateBuffer[i].Init(
		  m_Allocator, matrixBufferInfo, matrixAllocInfo);

		vk::BufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.size =
		  vertexBufferInfo.size + indexBufferInfo.size + matrixBufferInfo.size;
		stagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
		VmaAllocationCreateInfo stagingAllocInfo{};
		stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		stagingAllocInfo.flags =
		  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
		  VMA_ALLOCATION_CREATE_MAPPED_BIT;
		stagingAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_StagingBuffer[i].Init(
		  m_Allocator, stagingBufferInfo, stagingAllocInfo);

		vk::BufferCreateInfo scratchBufferInfo{};
		scratchBufferInfo.size =
		  sizeof(uint32_t) * DisplayAdapter::MaxVertexCount * sizeScale;
		scratchBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer |
								  vk::BufferUsageFlagBits::eShaderDeviceAddress;
		VmaAllocationCreateInfo scratchAllocInfo = {};
		scratchAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		scratchAllocInfo.flags =
		  VMA_ALLOCATION_CREATE_MAPPED_BIT |
		  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		scratchAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		m_ShaderScratchBuffer[i].Init(
		  m_Allocator, scratchBufferInfo, scratchAllocInfo);
		vk::BufferDeviceAddressInfo scratchAddressInfo = {};
		scratchAddressInfo.buffer = m_ShaderScratchBuffer[i].buffer;
		m_ShaderScratchBuffer[i].gpuAddress =
		  m_Device.getBufferAddress(scratchAddressInfo);

		vk::DescriptorBufferInfo triangleInfo(
		  m_VertexBuffer[i].Get(), 0, VK_WHOLE_SIZE);
		vk::DescriptorBufferInfo matrixInfo(
		  m_MatrixStateBuffer[i].Get(), 0, VK_WHOLE_SIZE);

		std::vector<vk::WriteDescriptorSet> writes = {
			vk::WriteDescriptorSet(m_DescriptorSets[i],
								   0,
								   0,
								   1,
								   vk::DescriptorType::eStorageBuffer,
								   nullptr,
								   &triangleInfo,
								   nullptr),
			vk::WriteDescriptorSet(m_DescriptorSets[i],
								   1,
								   0,
								   1,
								   vk::DescriptorType::eStorageBuffer,
								   nullptr,
								   &matrixInfo,
								   nullptr)
		};

		m_Device.updateDescriptorSets(writes, nullptr);
	}
}

void
RendererVK::InitTextureInfo()
{
	auto textureBindings = GetTextureBindings();
	std::vector<vk::DescriptorBindingFlags> bindingFlags(
	  textureBindings.size(),
	  vk::DescriptorBindingFlagBits::eUpdateAfterBind |
		vk::DescriptorBindingFlagBits::ePartiallyBound);

	vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo(
	  bindingFlags);

	vk::DescriptorSetLayoutCreateInfo textureLayoutInfo({}, textureBindings);
	textureLayoutInfo.pNext = &bindingFlagsInfo;
	textureLayoutInfo.flags =
	  vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

	m_TextureLayout =
	  vk::raii::DescriptorSetLayout(m_Device, textureLayoutInfo);

	vk::DescriptorPoolSize texturePoolSizes[2] = {};
	texturePoolSizes[0].type = vk::DescriptorType::eSampledImage;
	texturePoolSizes[0].descriptorCount = GetMaxTextureCount();
	texturePoolSizes[1].type = vk::DescriptorType::eSampler;
	texturePoolSizes[1].descriptorCount = Texture::PossibleSamplerCount;

	vk::DescriptorPoolCreateInfo texturePoolInfo = {};
	texturePoolInfo.flags =
	  vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
	  vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	texturePoolInfo.poolSizeCount = 2;
	texturePoolInfo.pPoolSizes = texturePoolSizes;
	texturePoolInfo.maxSets = 1;

	m_TextureDescriptorPool =
	  vk::raii::DescriptorPool(m_Device, texturePoolInfo);

	vk::DescriptorSetAllocateInfo textureSetAllocInfo(
	  *m_TextureDescriptorPool, 1, &*m_TextureLayout);

	m_TextureDescriptorSet =
	  std::move(m_Device.allocateDescriptorSets(textureSetAllocInfo)[0]);
}

void
RendererVK::ResolutionChanged()
{
	m_SwapchainIsInvalid = true;
}

void
RendererVK::TryVideoMode(const VideoModeParams& params)
{
	m_Device.waitIdle();

	if (m_Surface != nullptr) {
		CleanupSwapchain();
		m_Surface = nullptr;
	}

	m_Surface = CreateSurfaceKHR(m_Instance);
	InitSwapchain(params.width,
				  params.height,
				  params.vsync,
				  params.bWindowIsFullscreenBorderless,
				  params.bSmoothLines);
	InitImageViews();
	InitSyncStructures();
}
