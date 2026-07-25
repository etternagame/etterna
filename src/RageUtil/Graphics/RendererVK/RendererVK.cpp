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
#include "RenderTargetVK.h"
#include "PlatformUtils.h"

#ifndef __aarch64__
#include <xmmintrin.h>
#include <emmintrin.h>
#else
// Use sse2neon to transparently provide ARM Neon equivalents of x86_64 SIMD
// intrinsics
#include "sse2neon.h"
#endif
#include <RageUtil/Graphics/Display/Display.h>

constexpr uint64_t Timeout = 2000'000'000;

RendererVK::RendererVK()
  : m_Samplers{ nullptr, nullptr, nullptr, nullptr }
{
}

std::string
RendererVK::GetApiDescription() const
{
	return "Vulkan";
}

void
RendererVK::InitializeRenderer(const VideoModeParams& p)
{
	InitVulkanState();
	InitSwapchain(p);
	InitImageViews();
	InitBatchBuffers();
	InitGraphicsPipeline();
	InitCommandPool();
	InitCommandBuffers();
	InitSyncStructures();
	InitTextures();
}

/// ----------------------------------------
/// here be hazards and unsignaled fences...
/// ----------------------------------------
void
RendererVK::OnRender(const ActualVideoModeParams* p,
					 const DisplayAdapter::CommandBatcher& batcher)
{
	ThrowIfFail(m_Device.waitForFences(
	  *m_InFlightFence[m_CurrentFrame], vk::True, Timeout));

	UpdateBatchBuffers(batcher);

	auto [result, imageIndex] = m_Swapchain.acquireNextImage(
	  Timeout, *m_PresentCompleteSemaphore[m_CurrentFrame], nullptr);
	m_CurrentImage = imageIndex;

	if (result == vk::Result::eErrorOutOfDateKHR ||
		result == vk::Result::eSuboptimalKHR || m_SwapchainIsInvalid) {
		RecreateSwapchain(*p);
		m_SwapchainIsInvalid = false;
		return;
	}
	ThrowIfFail(result);

	m_Device.resetFences(*m_InFlightFence[m_CurrentFrame]);
	m_CommandBuffers[m_CurrentFrame].reset();
	RecordCommands(imageIndex, batcher);

	vk::PipelineStageFlags waitDestinationStageMask(
	  vk::PipelineStageFlagBits::eColorAttachmentOutput);

	vk::SubmitInfo submitInfo{};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &*m_PresentCompleteSemaphore[m_CurrentFrame];
	submitInfo.pWaitDstStageMask = &waitDestinationStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &*m_CommandBuffers[m_CurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &*m_RenderFinishedSemaphore[imageIndex];
	m_GraphicsQueue.submit(submitInfo, *m_InFlightFence[m_CurrentFrame]);

	vk::PresentInfoKHR presentInfoKHR{};
	presentInfoKHR.waitSemaphoreCount = 1;
	presentInfoKHR.pWaitSemaphores = &*m_RenderFinishedSemaphore[imageIndex];
	presentInfoKHR.swapchainCount = 1;
	presentInfoKHR.pSwapchains = &*m_Swapchain;
	presentInfoKHR.pImageIndices = &imageIndex;

	try {
		DISPLAY->FrameLimitBeforeVsync();

		const auto beforePresent = std::chrono::steady_clock::now();
		result = m_PresentQueue.presentKHR(presentInfoKHR);
		const auto afterPresent = std::chrono::steady_clock::now();
		DISPLAY->SetPresentTime(afterPresent - beforePresent);

		DISPLAY->FrameLimitAfterVsync(
		  DISPLAY->GetActualVideoModeParams()->rate);
	} catch (vk::OutOfDateKHRError error) {
		RecreateSwapchain(*p);
		return;
	}

	if (result == vk::Result::eSuboptimalKHR) {
		RecreateSwapchain(*p);
		return;
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % FramesInFlight;
}

bool
RendererVK::IsD3DInternal()
{
	return false;
}

intptr_t
RendererVK::CreateTexture(RageSurface* img, bool RGBA8)
{
	assert(m_EmptyTextureSlots.size());
	intptr_t currentHandle = *m_EmptyTextureSlots.begin();
	m_EmptyTextureSlots.erase(currentHandle);

	Texture texture = {};
	texture.width = img->w;
	texture.height = img->h;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format =
	  RGBA8 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_B8G8R8A8_UNORM;
	imageInfo.extent = { texture.width, texture.height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
					  VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
					  VK_IMAGE_USAGE_SAMPLED_BIT;

	VkImage imagePtr = nullptr;
	VmaAllocationInfo allocInfo = {};
	ThrowIfFail(vmaCreateImage(m_Allocator,
							   &imageInfo,
							   &allocCreateInfo,
							   &imagePtr,
							   &texture.allocation,
							   &allocInfo));
	texture.image = imagePtr;
	texture.allocator = m_Allocator;

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = texture.image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format =
	  RGBA8 ? vk::Format::eR8G8B8A8Unorm : vk::Format::eB8G8R8A8Unorm;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	texture.view = (*m_Device).createImageView(viewInfo);
	texture.currentLayout = vk::ImageLayout::eUndefined;

	texture.InitImageBuffer();
	m_Textures.insert({ currentHandle, texture });

	UpdateTexture(currentHandle, img, 0, 0, img->w, img->h);
	m_DirtyTextureDescriptors.push_back(currentHandle);

	return currentHandle;
}

void
RendererVK::UpdateTexture(intptr_t textureHandle,
						  RageSurface* img,
						  int xOffset,
						  int yOffset,
						  int width,
						  int height)
{
	assert(xOffset == 0);
	assert(yOffset == 0);
	assert(width == img->w);
	assert(height == img->h);
	assert(img->pitch == width * sizeof(uint32_t));
	assert(m_Textures.contains(textureHandle));

	auto& texture = m_Textures[textureHandle];
	std::memcpy(texture.imageBuffer.GetMappedData(),
				img->pixels,
				static_cast<size_t>(img->h) * img->w * sizeof(uint32_t));

	if (!texture.dirty) {
		texture.dirty = true;
		m_DirtyTextures.push_back(&texture);
	}
}

void
RendererVK::DeleteTexture(intptr_t handle)
{
	m_GraphicsQueue.waitIdle();

	DestroyTexture(m_Textures[handle]);
	if (m_DepthTextures.contains(handle)) {
		DestroyTexture(m_DepthTextures[handle]);
		m_DepthTextures.erase(handle);
	}

	m_Textures.erase(handle);
	m_EmptyTextureSlots.insert(handle);
}

void
RendererVK::ClearAllTextures()
{
	m_GraphicsQueue.waitIdle();

	Texture emptyTexture = m_Textures[0];
	for (auto& [handle, texture] : m_Textures) {
		if (handle == 0) {
			continue;
		}

		DestroyTexture(texture);
		if (m_DepthTextures.contains(handle)) {
			DestroyTexture(m_DepthTextures[handle]);
			m_DepthTextures.erase(handle);
		}

		m_EmptyTextureSlots.insert(handle);
	}

	m_Textures.clear();

	m_Textures[0] = emptyTexture;
}

RageSurface*
RendererVK::CreateScreenshot()
{
	m_Device.waitIdle();

	// synchronization2 would require CreateScreenshot to basically return a
	// future / allow OnRender to run to copy the frame without hazards and then
	// go back to CreateScreenshot? so using legacy synchronization...
	auto props =
	  m_PhysicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);

	bool supportsBlitting =
	  (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc) &&
	  (props.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst);

	auto sourceImage =
	  m_SwapchainImages[m_CurrentImage % m_SwapchainImages.size()];

	vk::ImageCreateInfo destImageInfo = {};
	destImageInfo.imageType = vk::ImageType::e2D;
	destImageInfo.format = vk::Format::eR8G8B8A8Unorm;
	destImageInfo.extent.width = m_SwapchainExtent.width;
	destImageInfo.extent.height = m_SwapchainExtent.height;
	destImageInfo.extent.depth = 1;
	destImageInfo.arrayLayers = 1;
	destImageInfo.mipLevels = 1;
	destImageInfo.initialLayout = vk::ImageLayout::eUndefined;
	destImageInfo.samples = vk::SampleCountFlagBits::e1;
	destImageInfo.tiling = vk::ImageTiling::eLinear;
	destImageInfo.usage = vk::ImageUsageFlagBits::eTransferDst;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags =
	  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
	  VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkImage destImageRaw = VK_NULL_HANDLE;
	VmaAllocation destAlloc = VK_NULL_HANDLE;
	VmaAllocationInfo destAllocInfo = {};
	ThrowIfFail(vmaCreateImage(m_Allocator,
							   &*destImageInfo,
							   &allocCreateInfo,
							   &destImageRaw,
							   &destAlloc,
							   &destAllocInfo));

	vk::CommandBufferAllocateInfo copyBufferInfo = {};
	copyBufferInfo.level = vk::CommandBufferLevel::ePrimary;
	copyBufferInfo.commandPool = m_CommandPool;
	copyBufferInfo.commandBufferCount = 1;
	vk::raii::CommandBuffer copyBuffer =
	  std::move(m_Device.allocateCommandBuffers(copyBufferInfo)[0]);

	copyBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	vk::ImageMemoryBarrier barrier = {};
	barrier.srcAccessMask = vk::AccessFlagBits::eNone;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.image = destImageRaw;
	barrier.subresourceRange =
	  vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

	copyBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
							   vk::PipelineStageFlagBits::eTransfer,
							   {},
							   {},
							   {},
							   { barrier });

	barrier.srcAccessMask = vk::AccessFlagBits::eNone;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
	barrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.image = sourceImage;

	copyBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
							   vk::PipelineStageFlagBits::eTransfer,
							   {},
							   {},
							   {},
							   { barrier });

	if (supportsBlitting) {
		vk::Offset3D blitSize = {};
		blitSize.x = m_SwapchainExtent.width;
		blitSize.y = m_SwapchainExtent.height;
		blitSize.z = 1;

		vk::ImageBlit blitRegion = {};
		blitRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blitRegion.srcSubresource.layerCount = 1;
		blitRegion.srcOffsets[1] = blitSize;
		blitRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		blitRegion.dstSubresource.layerCount = 1;
		blitRegion.dstOffsets[1] = blitSize;

		copyBuffer.blitImage(sourceImage,
							 vk::ImageLayout::eTransferSrcOptimal,
							 destImageRaw,
							 vk::ImageLayout::eTransferDstOptimal,
							 { blitRegion },
							 vk::Filter::eNearest);
	} else {
		vk::ImageCopy copyRegion = {};
		copyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.extent.width = m_SwapchainExtent.width;
		copyRegion.extent.height = m_SwapchainExtent.height;
		copyRegion.extent.depth = 1;

		copyBuffer.copyImage(sourceImage,
							 vk::ImageLayout::eTransferSrcOptimal,
							 destImageRaw,
							 vk::ImageLayout::eTransferDstOptimal,
							 { copyRegion });
	}

	barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
	barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.newLayout = vk::ImageLayout::eGeneral;
	barrier.image = destImageRaw;

	copyBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
							   vk::PipelineStageFlagBits::eTransfer,
							   {},
							   {},
							   {},
							   { barrier });

	barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
	barrier.dstAccessMask = vk::AccessFlagBits::eNone;
	barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
	barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier.image = sourceImage;

	copyBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
							   vk::PipelineStageFlagBits::eTransfer,
							   {},
							   {},
							   {},
							   { barrier });
	copyBuffer.end();

	vk::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &(*copyBuffer);

	vk::FenceCreateInfo fenceInfo = {};
	vk::raii::Fence fence(m_Device, fenceInfo);
	m_GraphicsQueue.submit({ submitInfo }, fence);
	ThrowIfFail(m_Device.waitForFences({ fence }, VK_TRUE, Timeout));

	VkImageSubresource subresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subresourceLayout = {};

	vkGetImageSubresourceLayout(
	  (vk::Device)m_Device, destImageRaw, &subresource, &subresourceLayout);

	uint8_t* data = static_cast<uint8_t*>(destAllocInfo.pMappedData) +
					subresourceLayout.offset;
	const uint32_t stride = subresourceLayout.rowPitch / sizeof(uint32_t);

	RageSurface* surface = CreateSurface(m_SwapchainExtent.width,
										 m_SwapchainExtent.height,
										 32,
										 0x000000ff,
										 0x0000ff00,
										 0x00ff0000,
										 0xff000000);

	auto* dest = reinterpret_cast<uint32_t*>(surface->pixels);

	bool needsSwizzle =
	  !supportsBlitting && (m_ImageFormat == vk::Format::eB8G8R8A8Unorm);

	const __m128i alphaMask = _mm_set1_epi32(0xff000000u);
	const __m128i blueMask = _mm_set1_epi32(0x000000FF);
	const __m128i greenMask = _mm_set1_epi32(0x0000FF00);
	const __m128i redMask = _mm_set1_epi32(0x00FF0000);

	if (needsSwizzle) {
		// if the screen image was in a different format (say, BGRA vs RGBA), we
		// need to swap R/B this is done in chunks of 4 pixels in an attempt to
		// speed thingies up also, set alpha to 255 to fix the image

		for (uint32_t y = 0; y < surface->h; y++) {
			const uint32_t* srcRow =
			  reinterpret_cast<const uint32_t*>(data) + y * stride;
			uint32_t* destRow =
			  reinterpret_cast<uint32_t*>(dest + y * surface->w);

			int i = 0;
			for (; i + 3 < surface->w; i += 4) {
				__m128i p = _mm_loadu_si128((const __m128i*)(srcRow + i));
				__m128i B = _mm_and_si128(p, blueMask);
				__m128i G = _mm_and_si128(p, greenMask);
				__m128i R = _mm_and_si128(p, redMask);

				B = _mm_slli_epi32(B, 16);
				R = _mm_srli_epi32(R, 16);

				__m128i res = _mm_or_si128(B, G);
				res = _mm_or_si128(res, R);
				res = _mm_or_si128(res, alphaMask);

				_mm_storeu_si128((__m128i*)(destRow + i), res);
			}
			for (; i < surface->w; ++i) {
				uint32_t p = srcRow[i];
				destRow[i] = ((p & 0x00FF0000u) >> 16) | ((p & 0x0000FF00u)) |
							 ((p & 0x000000FFu) << 16) | 0xFF000000u;
			}
		}
	} else {
		// if there's no need to swizzle, just set alpha to 255 to un-screwup
		// the screen image

		for (int y = 0; y < surface->h; y++) {
			const uint32_t* srcRow =
			  reinterpret_cast<const uint32_t*>(data) + y * stride;
			uint32_t* destRow =
			  reinterpret_cast<uint32_t*>(dest + y * surface->w);

			int i = 0;
			for (; i + 3 < surface->w; i += 4) {
				__m128i pixels = _mm_loadu_si128((const __m128i*)(srcRow + i));
				__m128i result = _mm_or_si128(pixels, alphaMask);
				_mm_storeu_si128((__m128i*)(destRow + i), result);
			}
			for (; i < surface->w; i++) {
				destRow[i] = srcRow[i] | 0xFF000000u;
			}
		}
	}

	vmaDestroyImage(m_Allocator, destImageRaw, destAlloc);

	return surface;
}

intptr_t
RendererVK::CreateRenderTarget(const RenderTargetParam& param,
							   int& iTextureWidthOut,
							   int& iTextureHeightOut)
{
	RenderTargetVK target = {};
	target.Create(param, iTextureWidthOut, iTextureHeightOut);
	target.m_Texture = CreateRenderTargetTexture(target.GetParam().iWidth,
												 target.GetParam().iHeight,
												 param.bWithAlpha,
												 param.bWithDepthBuffer);
	return target.m_Texture;
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

	if (m_DepthImage != nullptr) {
		m_DepthView = nullptr;
		vmaDestroyImage(m_Allocator, m_DepthImage, m_DepthAllocation);
		m_DepthImage = nullptr;
		m_DepthAllocation = nullptr;
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

void
RendererVK::InitVulkanState()
{
	auto instanceResult = CreateInstance(VulkanDebugCallback);
	if (!instanceResult) {
		Locator::getLogger()->fatal("RendererVK: instance creation failed - {}",
									GetDetailedErrorString(instanceResult));
		Fail();
	}

	m_Instance = vk::raii::Instance(m_Context, instanceResult->instance);

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
	auto physicalDeviceResult =
	  selector.set_minimum_version(1, 3)
		.set_required_features_13(vk13Features)
		.set_required_features_12(vk12Features)
		.set_required_features(vkFeatures)
		.set_surface(static_cast<vk::SurfaceKHR>(m_Surface))
		.add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
#ifdef __APPLE__
		.add_required_extension(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
#endif
		.select();
	if (!physicalDeviceResult) {
		Locator::getLogger()->fatal(
		  "RendererVK: physical device creation failed - {}",
		  GetDetailedErrorString(physicalDeviceResult));
		Fail();
	}

	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT dynamicState3Features{};
	dynamicState3Features.sType =
	  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
	dynamicState3Features.extendedDynamicState3ColorBlendEnable = VK_TRUE;
	dynamicState3Features.extendedDynamicState3ColorBlendEquation = VK_TRUE;
	dynamicState3Features.extendedDynamicState3ColorWriteMask = VK_TRUE;

	vkb::DeviceBuilder deviceBuilder(*physicalDeviceResult);
	auto deviceResult = deviceBuilder.add_pNext(&dynamicState3Features).build();
	if (!deviceResult) {
		Locator::getLogger()->fatal(
		  "RendererVK: device creation failed - {}",
		  GetDetailedErrorString(physicalDeviceResult));
		Fail();
	}

	m_PhysicalDevice = vk::raii::PhysicalDevice(
	  m_Instance, physicalDeviceResult->physical_device);
	m_Device = vk::raii::Device(m_PhysicalDevice, deviceResult->device);

	Locator::getLogger()->debug("RendererVK: selected GPU: {}",
								physicalDeviceResult->name);

	m_GraphicsQueue = vk::raii::Queue(
	  m_Device, deviceResult->get_queue(vkb::QueueType::graphics).value());
	m_GraphicsQueueFamily =
	  deviceResult->get_queue_index(vkb::QueueType::graphics).value();

	m_PresentQueue = vk::raii::Queue(
	  m_Device, deviceResult->get_queue(vkb::QueueType::present).value());
	m_PresentQueueFamily =
	  deviceResult->get_queue_index(vkb::QueueType::present).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice =
	  static_cast<vk::PhysicalDevice>(m_PhysicalDevice);
	allocatorInfo.device = static_cast<vk::Device>(m_Device);
	allocatorInfo.instance = static_cast<vk::Instance>(m_Instance);
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

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
}

void
RendererVK::InitSwapchain(const VideoModeParams& p)
{
	vkb::SwapchainBuilder swapchainBuilder(
	  *m_PhysicalDevice, *m_Device, *m_Surface);

	swapchainBuilder.set_desired_min_image_count(FramesInFlight)
	  .set_desired_format(
		{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	  .set_desired_format(
		{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	  .set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
	  .set_desired_extent(p.width, p.height)
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
	  p.bWindowIsFullscreenBorderless ? VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT
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

	vk::ImageCreateInfo depthImageInfo = {};
	depthImageInfo.imageType = vk::ImageType::e2D;
	depthImageInfo.format = m_DepthFormat;
	depthImageInfo.extent =
	  vk::Extent3D(vkbSwapchain.extent.width, vkbSwapchain.extent.height, 1);
	depthImageInfo.mipLevels = 1;
	depthImageInfo.arrayLayers = 1;
	depthImageInfo.samples = vk::SampleCountFlagBits::e1;
	depthImageInfo.tiling = vk::ImageTiling::eOptimal;
	depthImageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	depthImageInfo.initialLayout = vk::ImageLayout::eUndefined;

	VmaAllocationCreateInfo depthAllocInfo = {};
	depthAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	depthAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	ThrowIfFail(vmaCreateImage(m_Allocator,
							   &*depthImageInfo,
							   &depthAllocInfo,
							   &m_DepthImage,
							   &m_DepthAllocation,
							   nullptr));

	vk::ImageViewCreateInfo depthViewInfo = {};
	depthViewInfo.image = m_DepthImage;
	depthViewInfo.viewType = vk::ImageViewType::e2D;
	depthViewInfo.format = m_DepthFormat;
	vk::ImageSubresourceRange subRange = {};
	subRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
	subRange.levelCount = 1;
	subRange.layerCount = 1;
	depthViewInfo.subresourceRange = subRange;
	m_DepthView = vk::raii::ImageView(m_Device, depthViewInfo);
}

void
RendererVK::RecreateSwapchain(const VideoModeParams& p)
{
	m_Device.waitIdle();

	CleanupSwapchain();
	InitSwapchain(p);
	InitImageViews();
	InitSyncStructures();
}

void
RendererVK::CleanupSwapchain()
{
	m_SwapchainImageViews.clear();
	m_Swapchain = nullptr;

	if (m_DepthImage != nullptr) {
		m_DepthView = nullptr;
		vmaDestroyImage(m_Allocator, m_DepthImage, m_DepthAllocation);
		m_DepthImage = nullptr;
		m_DepthAllocation = nullptr;
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
	m_Cache->Init(m_Device);

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
RendererVK::TransitionImageLayout(vk::Image& image,
								  vk::ImageLayout oldLayout,
								  vk::ImageLayout newLayout,
								  vk::AccessFlags2 srcAccessMask,
								  vk::AccessFlags2 dstAccessMask,
								  vk::PipelineStageFlags2 srcStageMask,
								  vk::PipelineStageFlags2 dstStageMask,
								  vk::raii::CommandBuffer& commandBuffer)
{
	vk::ImageMemoryBarrier2 barrier{};
	barrier.srcStageMask = srcStageMask;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstStageMask = dstStageMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	vk::ImageSubresourceRange range{};
	range.aspectMask = vk::ImageAspectFlagBits::eColor;
	range.baseMipLevel = 0;
	range.levelCount = 1;
	range.baseArrayLayer = 0;
	range.layerCount = 1;

	barrier.subresourceRange = range;

	vk::DependencyInfo dependencyInfo{};
	dependencyInfo.dependencyFlags = {};
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;

	commandBuffer.pipelineBarrier2(dependencyInfo);
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
RendererVK::RecordCommands(uint32_t imageIndex,
						   const DisplayAdapter::CommandBatcher& batcher)
{
	auto& buffer = m_CommandBuffers[m_CurrentFrame];
	buffer.begin(vk::CommandBufferBeginInfo(
	  vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	m_DirtyPreBarriers.reserve(m_DirtyTextures.size());
	m_DirtyPostBarriers.reserve(m_DirtyTextures.size());

	for (auto& texture : m_DirtyTextures) {
		vk::ImageMemoryBarrier2 barrier{};
		barrier.srcStageMask =
		  (texture->currentLayout == vk::ImageLayout::eUndefined)
			? vk::PipelineStageFlagBits2::eNone
			: vk::PipelineStageFlagBits2::eAllGraphics;
		barrier.srcAccessMask =
		  (texture->currentLayout == vk::ImageLayout::eUndefined)
			? vk::AccessFlags2()
			: vk::AccessFlagBits2::eShaderRead;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
		barrier.oldLayout = texture->currentLayout;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture->image;
		barrier.subresourceRange = vk::ImageSubresourceRange(
		  vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		m_DirtyPreBarriers.push_back(barrier);
	}

	if (!m_DirtyPreBarriers.empty()) {
		vk::DependencyInfo depInfo{};
		depInfo.imageMemoryBarrierCount =
		  static_cast<uint32_t>(m_DirtyPreBarriers.size());
		depInfo.pImageMemoryBarriers = m_DirtyPreBarriers.data();
		buffer.pipelineBarrier2(depInfo);
	}

	for (auto& texture : m_DirtyTextures) {
		vk::BufferImageCopy2 copyRegion{};
		copyRegion.imageExtent =
		  vk::Extent3D{ texture->width, texture->height, 1 };
		copyRegion.imageSubresource.aspectMask =
		  vk::ImageAspectFlagBits::eColor;
		copyRegion.imageSubresource.layerCount = 1;

		vk::CopyBufferToImageInfo2 copyInfo{};
		copyInfo.srcBuffer = texture->imageBuffer.buffer;
		copyInfo.dstImage = texture->image;
		copyInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
		copyInfo.setRegions({ copyRegion });
		buffer.copyBufferToImage2(copyInfo);
	}

	for (auto& texture : m_DirtyTextures) {
		vk::ImageMemoryBarrier2 barrier{};
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllGraphics;
		barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture->image;
		barrier.subresourceRange = vk::ImageSubresourceRange(
		  vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
		m_DirtyPostBarriers.push_back(barrier);
	}

	if (!m_DirtyPostBarriers.empty()) {
		vk::DependencyInfo depInfo{};
		depInfo.imageMemoryBarrierCount =
		  static_cast<uint32_t>(m_DirtyPostBarriers.size());
		depInfo.pImageMemoryBarriers = m_DirtyPostBarriers.data();
		buffer.pipelineBarrier2(depInfo);
	}

	for (auto& texture : m_DirtyTextures) {
		texture->currentLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		texture->dirty = false;
		texture->initialized = true;
	}
	m_DirtyTextures.clear();
	m_DirtyPreBarriers.clear();
	m_DirtyPostBarriers.clear();

	m_DirtyImageInfos.reserve(m_DirtyTextureDescriptors.size());
	m_DirtyImageDescWrites.reserve(m_DirtyTextureDescriptors.size());

	for (int handle : m_DirtyTextureDescriptors) {
		vk::DescriptorImageInfo& imageInfo = m_DirtyImageInfos.emplace_back();
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = m_EmptyTextureSlots.contains(handle)
								? m_Textures[0].view
								: m_Textures[handle].view;

		vk::WriteDescriptorSet write{};
		write.dstSet = m_TextureDescriptorSet;
		write.dstBinding = 2;
		write.dstArrayElement = handle;
		write.descriptorCount = 1;
		write.descriptorType = vk::DescriptorType::eSampledImage;
		write.pImageInfo = &imageInfo;

		m_DirtyImageDescWrites.push_back(write);
	}

	if (!m_DirtyImageDescWrites.empty()) {
		m_Device.updateDescriptorSets(m_DirtyImageDescWrites, {});
	}

	m_DirtyTextureDescriptors.clear();
	m_DirtyImageInfos.clear();
	m_DirtyImageDescWrites.clear();

	vk::BufferCopy stagingCopy{};
	stagingCopy.srcOffset = 0;
	stagingCopy.dstOffset = 0;
	stagingCopy.size =
	  sizeof(DisplayAdapter::Vertex) * batcher.m_VertexBuffer.size();
	buffer.copyBuffer(m_StagingBuffer[m_CurrentFrame].buffer,
					  m_VertexBuffer[m_CurrentFrame].buffer,
					  { stagingCopy });

	stagingCopy.srcOffset += stagingCopy.size;
	stagingCopy.size = sizeof(uint32_t) * batcher.m_IndexBuffer.size();
	buffer.copyBuffer(m_StagingBuffer[m_CurrentFrame].buffer,
					  m_IndexBuffer[m_CurrentFrame].buffer,
					  { stagingCopy });

	stagingCopy.srcOffset += stagingCopy.size;
	stagingCopy.size =
	  sizeof(DisplayAdapter::MatrixState) * batcher.m_MatrixStateBuffer.size();
	buffer.copyBuffer(m_StagingBuffer[m_CurrentFrame].buffer,
					  m_MatrixStateBuffer[m_CurrentFrame].buffer,
					  { stagingCopy });

	std::vector<vk::BufferMemoryBarrier2> bufferBarriers;
	bufferBarriers.reserve(4);

	vk::BufferMemoryBarrier2 vertexBarrier{};
	vertexBarrier.srcStageMask = vk::PipelineStageFlagBits2::eCopy;
	vertexBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	vertexBarrier.dstStageMask = vk::PipelineStageFlagBits2::eVertexShader;
	vertexBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
	vertexBarrier.buffer = m_VertexBuffer[m_CurrentFrame].buffer;
	vertexBarrier.offset = 0;
	vertexBarrier.size = VK_WHOLE_SIZE;
	bufferBarriers.push_back(vertexBarrier);

	vk::BufferMemoryBarrier2 indexBarrier{};
	indexBarrier.srcStageMask = vk::PipelineStageFlagBits2::eCopy;
	indexBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	indexBarrier.dstStageMask = vk::PipelineStageFlagBits2::eIndexInput;
	indexBarrier.dstAccessMask = vk::AccessFlagBits2::eIndexRead;
	indexBarrier.buffer = m_IndexBuffer[m_CurrentFrame].buffer;
	indexBarrier.offset = 0;
	indexBarrier.size = VK_WHOLE_SIZE;
	bufferBarriers.push_back(indexBarrier);

	vk::BufferMemoryBarrier2 matrixBarrier{};
	matrixBarrier.srcStageMask = vk::PipelineStageFlagBits2::eCopy;
	matrixBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	matrixBarrier.dstStageMask = vk::PipelineStageFlagBits2::eVertexShader;
	matrixBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
	matrixBarrier.buffer = m_MatrixStateBuffer[m_CurrentFrame].buffer;
	matrixBarrier.offset = 0;
	matrixBarrier.size = VK_WHOLE_SIZE;
	bufferBarriers.push_back(matrixBarrier);

	vk::BufferMemoryBarrier2 scratchBarrier{};
	scratchBarrier.srcStageMask = vk::PipelineStageFlagBits2::eHost;
	scratchBarrier.srcAccessMask = vk::AccessFlagBits2::eHostWrite;
	scratchBarrier.dstStageMask = vk::PipelineStageFlagBits2::eVertexShader |
								  vk::PipelineStageFlagBits2::eFragmentShader;
	scratchBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
	scratchBarrier.buffer = m_ShaderScratchBuffer[m_CurrentFrame].buffer;
	scratchBarrier.offset = 0;
	scratchBarrier.size = VK_WHOLE_SIZE;
	bufferBarriers.push_back(scratchBarrier);

	vk::DependencyInfo dependencyInfo{};
	dependencyInfo.dependencyFlags = {};
	dependencyInfo.setBufferMemoryBarriers(bufferBarriers);
	buffer.pipelineBarrier2(dependencyInfo);

	buffer.bindDescriptorSets(
	  vk::PipelineBindPoint::eGraphics,
	  *m_Cache->m_Pipelines[0].PipelineLayout,
	  0,
	  { *m_DescriptorSets[m_CurrentFrame], *m_TextureDescriptorSet },
	  nullptr);
	buffer.bindIndexBuffer(
	  m_IndexBuffer[m_CurrentFrame].Get(), 0, vk::IndexType::eUint32);

	intptr_t currentPipeline = -1;

	for (auto& node : batcher.m_RenderNodes) {
		bool swapchain = node.RenderTarget == 0;

		auto image = swapchain ? m_SwapchainImages[imageIndex]
							   : m_Textures[node.RenderTarget].image;
		auto view = swapchain ? m_SwapchainImageViews[imageIndex]
							  : m_Textures[node.RenderTarget].view;
		auto extent = swapchain
						? m_SwapchainExtent
						: vk::Extent2D(m_Textures[node.RenderTarget].width,
									   m_Textures[node.RenderTarget].height);

		TransitionImageLayout(
		  image,
		  swapchain ? vk::ImageLayout::eUndefined
					: m_Textures[node.RenderTarget].currentLayout,
		  vk::ImageLayout::eColorAttachmentOptimal,
		  swapchain ? vk::AccessFlags2() : vk::AccessFlagBits2::eShaderRead,
		  vk::AccessFlagBits2::eColorAttachmentWrite |
			vk::AccessFlagBits2::eColorAttachmentRead,
		  swapchain ? vk::PipelineStageFlagBits2::eColorAttachmentOutput
					: vk::PipelineStageFlagBits2::eAllGraphics,
		  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		  buffer);

		if (!swapchain) {
			m_Textures[node.RenderTarget].currentLayout =
			  vk::ImageLayout::eColorAttachmentOptimal;
		}

		vk::RenderingAttachmentInfo colorInfo = {};
		colorInfo.imageView = view;
		colorInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorInfo.storeOp = vk::AttachmentStoreOp::eStore;
		if (node.PreserveRenderTarget && !swapchain) {
			colorInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		} else {
			colorInfo.loadOp = vk::AttachmentLoadOp::eClear;
			colorInfo.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
		}

		vk::RenderingAttachmentInfo depthInfo = {};
		depthInfo.imageView =
		  (!swapchain && m_DepthTextures.contains(node.RenderTarget))
			? m_DepthTextures[node.RenderTarget].view
			: m_DepthView;
		depthInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
		if (node.PreserveRenderTarget && !swapchain &&
			m_DepthTextures.contains(node.RenderTarget)) {
			depthInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		} else {
			depthInfo.loadOp = vk::AttachmentLoadOp::eClear;
			depthInfo.clearValue = vk::ClearDepthStencilValue(1.0f, 0);
		}

		vk::RenderingInfo renderInfo = {};
		renderInfo.renderArea =
		  vk::Rect2D{ { 0, 0 }, { extent.width, extent.height } };
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = 1;
		renderInfo.pColorAttachments = &colorInfo;
		renderInfo.pDepthAttachment = &depthInfo;

		buffer.setScissor(
		  0,
		  vk::Rect2D(vk::Offset2D(0, 0),
					 vk::Extent2D(extent.width, extent.height)));
		buffer.setViewport(0,
						   vk::Viewport(0.0f,
										static_cast<float>(extent.height),
										static_cast<float>(extent.width),
										-static_cast<float>(extent.height),
										0.0f,
										1.0f));

		buffer.beginRendering(renderInfo);

		for (auto& call : node.DrawCalls) {
			buffer.setDepthTestEnable(
			  call.DepthTestMode != ZTEST_OFF ? VK_TRUE : VK_FALSE);
			buffer.setDepthWriteEnable(call.DepthWriteEnabled ? VK_TRUE
															  : VK_FALSE);

			vk::CompareOp depthCompareOp = vk::CompareOp::eAlways;
			switch (call.DepthTestMode) {
				case ZTEST_OFF: {
					break;
				}
				case ZTEST_WRITE_ON_PASS: {
					depthCompareOp = vk::CompareOp::eLessOrEqual;
					break;
				}
				case ZTEST_WRITE_ON_FAIL: {
					depthCompareOp = vk::CompareOp::eGreater;
					break;
				}
				default: {
					Locator::getLogger()->error(
					  "Invalid ZTestMode encountered: {}",
					  static_cast<int>(call.DepthTestMode));
					Fail();
				}
			}
			buffer.setDepthCompareOp(depthCompareOp);

			SetBlendMode(call.BlendingMode, buffer);

			if (call.Settings.GraphicsPipeline != currentPipeline) {
				currentPipeline = call.Settings.GraphicsPipeline;
				buffer.bindPipeline(
				  vk::PipelineBindPoint::eGraphics,
				  m_Cache->m_Pipelines[currentPipeline].GraphicsPipeline);
			}

			uint64_t vertexArg =
			  call.Settings.VertexShaderArg == UINT64_MAX
				? 0
				: m_ShaderScratchBuffer[m_CurrentFrame].gpuAddress +
					call.Settings.VertexShaderArg;

			buffer.pushConstants<uint64_t>(
			  *m_Cache->m_Pipelines[0].PipelineLayout,
			  vk::ShaderStageFlagBits::eVertex |
				vk::ShaderStageFlagBits::eFragment,
			  0,
			  { vertexArg });

			uint64_t fragArg =
			  call.Settings.FragShaderArg == UINT64_MAX
				? 0
				: m_ShaderScratchBuffer[m_CurrentFrame].gpuAddress +
					call.Settings.FragShaderArg;

			buffer.pushConstants<uint64_t>(
			  *m_Cache->m_Pipelines[0].PipelineLayout,
			  vk::ShaderStageFlagBits::eVertex |
				vk::ShaderStageFlagBits::eFragment,
			  sizeof(uint64_t),
			  { fragArg });

			buffer.drawIndexed(call.IndexCount, 1, call.IndexOffset, 0, 0);
		}

		buffer.endRendering();

		TransitionImageLayout(
		  image,
		  swapchain ? vk::ImageLayout::eColorAttachmentOptimal
					: m_Textures[node.RenderTarget].currentLayout,
		  swapchain ? vk::ImageLayout::ePresentSrcKHR
					: vk::ImageLayout::eShaderReadOnlyOptimal,
		  vk::AccessFlagBits2::eColorAttachmentWrite,
		  swapchain ? vk::AccessFlags2() : vk::AccessFlagBits2::eShaderRead,
		  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		  swapchain ? vk::PipelineStageFlagBits2::eNone
					: vk::PipelineStageFlagBits2::eAllGraphics,
		  buffer);

		if (!swapchain) {
			m_Textures[node.RenderTarget].currentLayout =
			  vk::ImageLayout::eShaderReadOnlyOptimal;
		}
	}

	buffer.end();
}

void
RendererVK::SetBlendMode(BlendMode mode, vk::raii::CommandBuffer& buffer)
{
	vk::Bool32 enableBlending = VK_TRUE;
	vk::ColorComponentFlags colorWriteMask =
	  vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
	  vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::ColorBlendEquationEXT blendEquation{};
	blendEquation.colorBlendOp = vk::BlendOp::eAdd;
	blendEquation.alphaBlendOp = vk::BlendOp::eAdd;

	switch (mode) {
		case BLEND_NORMAL: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstColorBlendFactor =
			  vk::BlendFactor::eOneMinusSrcAlpha;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstAlphaBlendFactor =
			  vk::BlendFactor::eOneMinusSrcAlpha;
			break;
		}

		case BLEND_ADD: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eOne;
			break;
		}

		case BLEND_SUBTRACT: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			break;
		}

		case BLEND_MODULATE: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eSrcColor;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eSrcColor;
			break;
		}

		case BLEND_COPY_SRC: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			break;
		}

		case BLEND_ALPHA_MASK: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			break;
		}

		case BLEND_ALPHA_KNOCK_OUT: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstAlphaBlendFactor =
			  vk::BlendFactor::eOneMinusSrcAlpha;
			break;
		}

		case BLEND_ALPHA_MULTIPLY: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			break;
		}

		case BLEND_WEIGHTED_MULTIPLY: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eDstColor;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eSrcColor;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eDstColor;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eSrcColor;
			break;
		}

		case BLEND_INVERT_DEST: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.colorBlendOp = vk::BlendOp::eSubtract;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eOne;
			blendEquation.alphaBlendOp = vk::BlendOp::eSubtract;
			break;
		}

		case BLEND_NO_EFFECT: {
			blendEquation.srcColorBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstColorBlendFactor = vk::BlendFactor::eOne;
			blendEquation.srcAlphaBlendFactor = vk::BlendFactor::eZero;
			blendEquation.dstAlphaBlendFactor = vk::BlendFactor::eOne;
			colorWriteMask = vk::ColorComponentFlags(0);
			break;
		}

		default: {
			Locator::getLogger()->error("Invalid BlendMode: {}",
										static_cast<int>(mode));
			Fail();
		}
	}

	buffer.setColorBlendEnableEXT(0, { enableBlending });
	buffer.setColorWriteMaskEXT(0, { colorWriteMask });
	buffer.setColorBlendEquationEXT(0, { blendEquation });
}

void
RendererVK::InitBatchBuffers()
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

	for (int i = 0; i < FramesInFlight; i++) {
		vk::BufferCreateInfo vertexBufferInfo{};
		vertexBufferInfo.size =
		  sizeof(DisplayAdapter::Vertex) * DisplayAdapter::MaxVertexCount;
		vertexBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst |
								 vk::BufferUsageFlagBits::eStorageBuffer;
		VmaAllocationCreateInfo vertexAllocInfo = {};
		vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		m_VertexBuffer[i].Init(m_Allocator, vertexBufferInfo, vertexAllocInfo);

		vk::BufferCreateInfo indexBufferInfo{};
		indexBufferInfo.size =
		  sizeof(uint32_t) * 4 * DisplayAdapter::MaxVertexCount;
		indexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer |
								vk::BufferUsageFlagBits::eTransferDst;
		VmaAllocationCreateInfo indexAllocInfo = {};
		indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		m_IndexBuffer[i].Init(m_Allocator, indexBufferInfo, indexAllocInfo);

		vk::BufferCreateInfo matrixBufferInfo{};
		matrixBufferInfo.size =
		  sizeof(DisplayAdapter::MatrixState) * DisplayAdapter::MaxVertexCount;
		matrixBufferInfo.usage = vk::BufferUsageFlagBits::eStorageBuffer |
								 vk::BufferUsageFlagBits::eTransferDst;
		VmaAllocationCreateInfo matrixAllocInfo = {};
		matrixAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		m_MatrixStateBuffer[i].Init(
		  m_Allocator, matrixBufferInfo, matrixAllocInfo);

		vk::BufferCreateInfo stagingBufferInfo{};
		stagingBufferInfo.size =
		  sizeof(DisplayAdapter::Vertex) * DisplayAdapter::MaxVertexCount +
		  sizeof(uint32_t) * 4 * DisplayAdapter::MaxVertexCount +
		  sizeof(DisplayAdapter::MatrixState) * DisplayAdapter::MaxVertexCount;
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
		scratchBufferInfo.size = sizeof(uint8_t) * 1'000'000;
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
RendererVK::UpdateBatchBuffers(const DisplayAdapter::CommandBatcher& batcher)
{
	if (batcher.m_VertexBuffer.empty()) {
		return;
	}

	uint8_t* stagingBuffer =
	  static_cast<uint8_t*>(m_StagingBuffer[m_CurrentFrame].GetMappedData());
	std::memcpy(stagingBuffer,
				batcher.m_VertexBuffer.data(),
				sizeof(DisplayAdapter::Vertex) * batcher.m_VertexBuffer.size());

	stagingBuffer +=
	  sizeof(DisplayAdapter::Vertex) * batcher.m_VertexBuffer.size();

	std::memcpy(stagingBuffer,
				batcher.m_IndexBuffer.data(),
				sizeof(uint32_t) * batcher.m_IndexBuffer.size());

	stagingBuffer += sizeof(uint32_t) * batcher.m_IndexBuffer.size();

	std::memcpy(stagingBuffer,
				batcher.m_MatrixStateBuffer.data(),
				sizeof(DisplayAdapter::MatrixState) *
				  batcher.m_MatrixStateBuffer.size());

	std::memcpy(m_ShaderScratchBuffer[m_CurrentFrame].GetMappedData(),
				batcher.m_ShaderScratchBuffer.data(),
				sizeof(uint8_t) * batcher.m_ShaderScratchBuffer.size());
}

int
RendererVK::GetMaxTextureSize()
{
	if (m_TextureSize == -1) {
		m_TextureSize = std::min(
		  DisplayAdapter::Display::MaxTextureSize,
		  static_cast<size_t>(
			m_PhysicalDevice.getProperties().limits.maxImageDimension2D));
	}
	return m_TextureSize;
}

int
RendererVK::GetMaxTextureCount()
{
	if (!m_TextureCount) {
		m_TextureCount = std::min(
		  static_cast<size_t>(Texture::MaxTextures),
		  static_cast<size_t>(m_PhysicalDevice.getProperties()
								.limits.maxDescriptorSetSampledImages));
	}

	return m_TextureCount;
}

void
RendererVK::DestroyTexture(Texture& texture)
{
	texture.DestroyImageBuffer();
	if (texture.image) {
		vmaDestroyImage(m_Allocator, texture.image, texture.allocation);
	}
	if (texture.view) {
		vkDestroyImageView(*m_Device, texture.view, nullptr);
	}

	texture = {};
}

void
RendererVK::InitTextures()
{
	vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();
	vk::SamplerCreateInfo samplerInfo = {};
	samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
	samplerInfo.anisotropyEnable = vk::True;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.compareEnable = vk::False;
	samplerInfo.compareOp = vk::CompareOp::eAlways;
	std::array<vk::DescriptorImageInfo, Texture::PossibleSamplerCount>
	  samplerImageInfo;

	for (size_t i = 0; i < m_Samplers.size(); i++) {
		samplerInfo.magFilter =
		  (i & Texture::Filtering) ? vk::Filter::eLinear : vk::Filter::eNearest;
		samplerInfo.minFilter =
		  (i & Texture::Filtering) ? vk::Filter::eLinear : vk::Filter::eNearest;
		samplerInfo.addressModeU = (i & Texture::Wrapping)
									 ? vk::SamplerAddressMode::eRepeat
									 : vk::SamplerAddressMode::eClampToBorder;
		samplerInfo.addressModeV = (i & Texture::Wrapping)
									 ? vk::SamplerAddressMode::eRepeat
									 : vk::SamplerAddressMode::eClampToBorder;
		m_Samplers[i] = vk::raii::Sampler(m_Device, samplerInfo);
		samplerImageInfo[i].sampler = m_Samplers[i];
	}

	vk::WriteDescriptorSet writeDescriptor = {};
	writeDescriptor.dstSet = m_TextureDescriptorSet;
	writeDescriptor.dstBinding = 3;
	writeDescriptor.descriptorCount = Texture::PossibleSamplerCount;
	writeDescriptor.descriptorType = vk::DescriptorType::eSampler;
	writeDescriptor.pImageInfo = samplerImageInfo.data();

	m_Device.updateDescriptorSets({ writeDescriptor }, nullptr);

	for (int i = 0; i < GetMaxTextureCount(); i++) {
		m_EmptyTextureSlots.insert(i);
	}

	RageSurface* img =
	  CreateSurface(1, 1, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	CreateTexture(img, true);

	delete img;
}

void
RendererVK::ResolutionChanged()
{
	m_SwapchainIsInvalid = true;
}

intptr_t
RendererVK::CreateRenderTargetTexture(int width,
									  int height,
									  bool withAlpha,
									  bool withDepth)
{
	assert(m_EmptyTextureSlots.size());
	intptr_t currentHandle = *m_EmptyTextureSlots.begin();
	m_EmptyTextureSlots.erase(currentHandle);

	Texture texture = {};
	texture.width = width;
	texture.height = height;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent = { texture.width, texture.height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.usage =
	  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	VkImage imagePtr = nullptr;
	VmaAllocationInfo allocInfo = {};
	ThrowIfFail(vmaCreateImage(m_Allocator,
							   &imageInfo,
							   &allocCreateInfo,
							   &imagePtr,
							   &texture.allocation,
							   &allocInfo));
	texture.image = imagePtr;

	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = texture.image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = vk::Format::eR8G8B8A8Unorm;
	viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	texture.view = (*m_Device).createImageView(viewInfo);
	texture.currentLayout = vk::ImageLayout::eUndefined;

	viewInfo.components.r = vk::ComponentSwizzle::eR;
	viewInfo.components.g = vk::ComponentSwizzle::eG;
	viewInfo.components.b = vk::ComponentSwizzle::eB;

	if (withAlpha) {
		viewInfo.components.a = vk::ComponentSwizzle::eA;
	} else {
		viewInfo.components.a = vk::ComponentSwizzle::eOne;
	}

	m_Textures.insert({ currentHandle, texture });
	m_DirtyTextureDescriptors.push_back(currentHandle);

	if (withDepth) {
		Texture depthTexture = {};
		depthTexture.width = width;
		depthTexture.height = height;
		depthTexture.initialized = true;

		vk::ImageCreateInfo depthImageInfo = {};
		depthImageInfo.imageType = vk::ImageType::e2D;
		depthImageInfo.format = m_DepthFormat;
		depthImageInfo.extent = vk::Extent3D(width, height, 1);
		depthImageInfo.mipLevels = 1;
		depthImageInfo.arrayLayers = 1;
		depthImageInfo.samples = vk::SampleCountFlagBits::e1;
		depthImageInfo.tiling = vk::ImageTiling::eOptimal;
		depthImageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		depthImageInfo.initialLayout = vk::ImageLayout::eUndefined;

		VmaAllocationCreateInfo depthAllocInfo = {};
		depthAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		VkImage depthImagePtr = nullptr;
		ThrowIfFail(vmaCreateImage(m_Allocator,
								   &*depthImageInfo,
								   &depthAllocInfo,
								   &depthImagePtr,
								   &depthTexture.allocation,
								   nullptr));
		depthTexture.image = depthImagePtr;

		vk::ImageViewCreateInfo depthViewInfo = {};
		depthViewInfo.image = depthTexture.image;
		depthViewInfo.viewType = vk::ImageViewType::e2D;
		depthViewInfo.format = m_DepthFormat;
		vk::ImageSubresourceRange subRange = {};
		subRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		subRange.levelCount = 1;
		subRange.layerCount = 1;
		depthViewInfo.subresourceRange = subRange;
		depthTexture.view = (*m_Device).createImageView(depthViewInfo);

		m_DepthTextures.insert({ currentHandle, depthTexture });
	}

	return currentHandle;
}

DisplayAdapter::PipelineHandle
RendererVK::CreateGraphicsPipeline(const std::string& vertexShaderPath,
								   const std::string& fragmentShaderPath)
{
	assert(m_Cache.has_value());
	return m_Cache->CreateGraphicsPipeline(
	  m_Device, vertexShaderPath, fragmentShaderPath);
}

void
RendererVK::ReloadPipelines()
{
	m_Cache->ReloadPipelines(m_Device);
}

void
RendererVK::TryVideoMode(const VideoModeParams& params)
{
	m_Device.waitIdle();

	m_Surface = CreateSurfaceKHR(m_Instance);
	RecreateSwapchain(params);
}
