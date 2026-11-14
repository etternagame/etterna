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

#ifndef __aarch64__
#include <xmmintrin.h>
#include <emmintrin.h>
#else
// Use sse2neon to transparently provide ARM Neon equivalents of x86_64 SIMD
// intrinsics
#include "sse2neon.h"
#endif
#include <RageUtil/Graphics/Display/Display.h>


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

	// old ahh renderers scale the textures to powers-of-two for Reasons(TM)
	imageInfo.extent = { (uint32_t)power_of_two(texture.width),
						 (uint32_t)power_of_two(texture.height),
						 1 };
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

	UpdateTexture(currentHandle, img, 0, 0, texture.width, texture.height);
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
		m_DirtyTextures.push_back(textureHandle);
	}
}

void
RendererVK::DeleteTexture(intptr_t handle)
{
	m_GraphicsQueue.waitIdle();

	assert(m_Textures.contains(handle));
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
	VkSubresourceLayout subresourceLayout = (*m_Device).getImageSubresourceLayout(destImageRaw, subresource);

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

void
RendererVK::TransitionImageLayout(vk::Image& image,
								  vk::ImageLayout oldLayout,
								  vk::ImageLayout newLayout,
								  vk::AccessFlags2 srcAccessMask,
								  vk::AccessFlags2 dstAccessMask,
								  vk::PipelineStageFlags2 srcStageMask,
								  vk::PipelineStageFlags2 dstStageMask,
								  vk::ImageAspectFlags aspectMask,
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
	range.aspectMask = aspectMask;
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
		(*m_Device).destroyImageView(texture.view);
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
	viewInfo.components.r = vk::ComponentSwizzle::eR;
	viewInfo.components.g = vk::ComponentSwizzle::eG;
	viewInfo.components.b = vk::ComponentSwizzle::eB;

	if (withAlpha) {
		viewInfo.components.a = vk::ComponentSwizzle::eA;
	} else {
		viewInfo.components.a = vk::ComponentSwizzle::eOne;
	}

	texture.view = (*m_Device).createImageView(viewInfo);
	texture.currentLayout = vk::ImageLayout::eUndefined;

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
		subRange.aspectMask =
		  vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		subRange.levelCount = 1;
		subRange.layerCount = 1;
		depthViewInfo.subresourceRange = subRange;
		depthTexture.view = (*m_Device).createImageView(depthViewInfo);

		m_DepthTextures.insert({ currentHandle, depthTexture });
		m_DirtyDepthTextures.push_back(currentHandle);
	}

	return currentHandle;
}
