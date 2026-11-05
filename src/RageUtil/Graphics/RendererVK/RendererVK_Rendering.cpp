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

bool
RendererVK::IsReadyForRender()
{
	if (!m_SwapchainIsInvalid) {
		return true;
	}

	try {
		RecreateSwapchain();
	} catch (std::exception e) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		return false;
	}

	m_SwapchainIsInvalid = false;
	return true;
}

/// ----------------------------------------
/// here be hazards and unsignaled fences...
/// ----------------------------------------
void
RendererVK::OnRender(const ActualVideoModeParams* p,
					 const DisplayAdapter::CommandBatcher& batcher)
{
	if (m_SwapchainIsInvalid) {
		// pause the current thread in IsReadyForRender() so we don't do a heavy
		// spin thing
		// we'll attempt to recreate the swapchain in IsReadyForRender() too
		return;
	}

	if (m_ResolvedSettings.NvidiaLowLatency2 == VFH_Require) {
		vk::SetLatencyMarkerInfoNV marker{};
		marker.presentID = m_PresentId;
		marker.marker = vk::LatencyMarkerNV::eSimulationEnd;

		m_Swapchain.setLatencyMarkerNV(marker);
	}

	ThrowIfFail(m_Device.waitForFences(
	  *m_InFlightFence[m_CurrentFrame], vk::True, Timeout));

	auto [result, imageIndex] = m_Swapchain.acquireNextImage(
	  Timeout, *m_PresentCompleteSemaphore[m_CurrentFrame], nullptr);
	m_CurrentImage = imageIndex;

	if (result == vk::Result::eErrorOutOfDateKHR ||
		result == vk::Result::eSuboptimalKHR) {
		m_SwapchainIsInvalid = true;
		return;
	}
	ThrowIfFail(result);

	if (m_ResolvedSettings.NvidiaLowLatency2 == VFH_Require) {
		vk::SetLatencyMarkerInfoNV marker{};
		marker.presentID = m_PresentId;
		marker.marker = vk::LatencyMarkerNV::eRendersubmitStart;

		m_Swapchain.setLatencyMarkerNV(marker);
	}

	UpdateBatchBuffers(batcher);

	if (m_ResolvedSettings.MemoryBudget == VFH_Require) {
		// no this is not the same thing as the imageIndex in acquireNextImage
		// should overflow in like 5 days @ 10K FPS
		vmaSetCurrentFrameIndex(m_Allocator, m_FrameCounterForAllocator++);
	}

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

	vk::LatencySubmissionPresentIdNV latencyId{};
	latencyId.presentID = m_PresentId;
	submitInfo.pNext = &latencyId;

	m_GraphicsQueue.submit(submitInfo, *m_InFlightFence[m_CurrentFrame]);

	if (m_ResolvedSettings.NvidiaLowLatency2 == VFH_Require) {
		vk::SetLatencyMarkerInfoNV marker{};
		marker.presentID = m_PresentId;
		marker.marker = vk::LatencyMarkerNV::eRendersubmitEnd;

		m_Swapchain.setLatencyMarkerNV(marker);
	}

	vk::PresentInfoKHR presentInfoKHR{};
	presentInfoKHR.waitSemaphoreCount = 1;
	presentInfoKHR.pWaitSemaphores = &*m_RenderFinishedSemaphore[imageIndex];
	presentInfoKHR.swapchainCount = 1;
	presentInfoKHR.pSwapchains = &*m_Swapchain;
	presentInfoKHR.pImageIndices = &imageIndex;

	try {
		// for frame pacing/limiting there's VK_EXT_present_timing but it's
		// kinda fresh at the time of writing so eh

		const auto beforePresent = std::chrono::steady_clock::now();

		if (m_ResolvedSettings.NvidiaLowLatency2 == VFH_Require) {
			vk::SetLatencyMarkerInfoNV marker{};
			marker.presentID = m_PresentId;
			marker.marker = vk::LatencyMarkerNV::ePresentStart;

			m_Swapchain.setLatencyMarkerNV(marker);
		}

		result = m_PresentQueue.presentKHR(presentInfoKHR);

		if (m_ResolvedSettings.NvidiaLowLatency2 == VFH_Require) {
			vk::SetLatencyMarkerInfoNV marker{};
			marker.presentID = m_PresentId;
			marker.marker = vk::LatencyMarkerNV::ePresentEnd;

			m_Swapchain.setLatencyMarkerNV(marker);
		}

		m_PresentId++;

		const auto afterPresent = std::chrono::steady_clock::now();
		DISPLAY->SetPresentTime(afterPresent - beforePresent);
	} catch (vk::OutOfDateKHRError error) {
		m_SwapchainIsInvalid = true;
		return;
	}

	if (result == vk::Result::eSuboptimalKHR) {
		m_SwapchainIsInvalid = true;
		return;
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % FramesInFlight;
}

void
RendererVK::MarkFrameStart()
{
	if (m_ResolvedSettings.NvidiaLowLatency2 != VFH_Require) {
		return;
	}

	vk::LatencySleepInfoNV sleepInfo{};
	sleepInfo.signalSemaphore = *m_NvidiaLatencySemaphore;
	sleepInfo.value = ++m_NvidiaLatencySleepValue;
	m_Swapchain.latencySleepNV(sleepInfo);

	vk::SemaphoreWaitInfo waitInfo{};
	waitInfo.setSemaphores(*m_NvidiaLatencySemaphore);
	waitInfo.setValues(m_NvidiaLatencySleepValue);

	ThrowIfFail(m_Device.waitSemaphores(waitInfo, Timeout));

	vk::SetLatencyMarkerInfoNV marker{};
	marker.presentID = m_PresentId;
	marker.marker = vk::LatencyMarkerNV::eSimulationStart;

	m_Swapchain.setLatencyMarkerNV(marker);

	// hacky?
	marker.marker = vk::LatencyMarkerNV::eInputSample;

	m_Swapchain.setLatencyMarkerNV(marker);
}

void
RendererVK::RescaleBatchBuffers(size_t sizeScale)
{
	assert(sizeScale > 0);
	m_Device.waitIdle();

	for (int i = 0; i < FramesInFlight; i++) {
		m_VertexBuffer[i].Destroy();
		m_IndexBuffer[i].Destroy();
		m_MatrixStateBuffer[i].Destroy();
		m_StagingBuffer[i].Destroy();
		m_ShaderScratchBuffer[i].Destroy();
	}

	InitBatchBuffers(sizeScale);
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

	for (auto& textureHandle : m_DirtyTextures) {
		if (!m_Textures.contains(textureHandle)) {
			continue;
		}

		auto& texture = m_Textures.at(textureHandle);
		vk::ImageMemoryBarrier2 barrier{};
		barrier.srcStageMask =
		  (texture.currentLayout == vk::ImageLayout::eUndefined)
			? vk::PipelineStageFlagBits2::eNone
			: vk::PipelineStageFlagBits2::eFragmentShader |
				vk::PipelineStageFlagBits2::eVertexShader;
		barrier.srcAccessMask =
		  (texture.currentLayout == vk::ImageLayout::eUndefined)
			? vk::AccessFlags2()
			: vk::AccessFlagBits2::eShaderRead;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
		barrier.oldLayout = texture.currentLayout;
		barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture.image;
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

	for (auto& textureHandle : m_DirtyTextures) {
		if (!m_Textures.contains(textureHandle)) {
			continue;
		}

		auto& texture = m_Textures.at(textureHandle);
		vk::BufferImageCopy2 copyRegion{};
		copyRegion.imageExtent =
		  vk::Extent3D{ texture.width, texture.height, 1 };
		copyRegion.imageSubresource.aspectMask =
		  vk::ImageAspectFlagBits::eColor;
		copyRegion.imageSubresource.layerCount = 1;

		vk::CopyBufferToImageInfo2 copyInfo{};
		copyInfo.srcBuffer = texture.imageBuffer.buffer;
		copyInfo.dstImage = texture.image;
		copyInfo.dstImageLayout = vk::ImageLayout::eTransferDstOptimal;
		copyInfo.setRegions({ copyRegion });
		buffer.copyBufferToImage2(copyInfo);
	}

	for (auto& textureHandle : m_DirtyTextures) {
		if (!m_Textures.contains(textureHandle)) {
			continue;
		}

		auto& texture = m_Textures.at(textureHandle);
		vk::ImageMemoryBarrier2 barrier{};
		barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
		barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
		barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader |
							   vk::PipelineStageFlagBits2::eVertexShader;
		barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = texture.image;
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

	for (auto& textureHandle : m_DirtyTextures) {
		if (!m_Textures.contains(textureHandle)) {
			continue;
		}

		auto& texture = m_Textures.at(textureHandle);
		texture.currentLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		texture.dirty = false;
		texture.initialized = true;
	}

	for (auto& textureHandle : m_DirtyDepthTextures) {
		if (textureHandle > 0 && !m_Textures.contains(textureHandle)) {
			continue;
		}

		auto& texture = textureHandle <= 0
						  ? m_SwapchainDepthTextures[-1 * textureHandle]
						  : m_DepthTextures.at(textureHandle);
		TransitionImageLayout(texture.image,
							  texture.currentLayout,
							  vk::ImageLayout::eDepthStencilAttachmentOptimal,
							  vk::AccessFlagBits2::eNone,
							  vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
							  vk::PipelineStageFlagBits2::eNone,
							  vk::PipelineStageFlagBits2::eAllGraphics,
							  vk::ImageAspectFlagBits::eDepth |
								vk::ImageAspectFlagBits::eStencil,
							  buffer);
		texture.currentLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	}

	m_DirtyTextures.clear();
	m_DirtyPreBarriers.clear();
	m_DirtyPostBarriers.clear();
	m_DirtyDepthTextures.clear();

	m_DirtyImageInfos.reserve(m_DirtyTextureDescriptors.size());
	m_DirtyImageDescWrites.reserve(m_DirtyTextureDescriptors.size());

	for (int handle : m_DirtyTextureDescriptors) {
		vk::DescriptorImageInfo& imageInfo = m_DirtyImageInfos.emplace_back();
		imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		imageInfo.imageView = m_EmptyTextureSlots.contains(handle)
								? m_Textures[0].view
								: m_Textures.at(handle).view;

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

	if (m_MsaaTexturesAreDirty) {
		for (auto& texture : m_MsaaTextures) {
			TransitionImageLayout(
			  texture.image,
			  texture.currentLayout,
			  vk::ImageLayout::eColorAttachmentOptimal,
			  vk::AccessFlags2(),
			  vk::AccessFlagBits2::eColorAttachmentWrite,
			  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			  vk::ImageAspectFlagBits::eColor,
			  buffer);

			texture.currentLayout = vk::ImageLayout::eColorAttachmentOptimal;
		}

		m_MsaaTexturesAreDirty = false;
	}

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
	BlendMode currentBlendMode = BLEND_NORMAL;

	for (auto& node : batcher.m_RenderNodes) {
		bool swapchain = node.RenderTarget == 0;
		bool useMsaa = !(m_MsaaSamples & vk::SampleCountFlagBits::e1);

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
		  vk::ImageAspectFlagBits::eColor,
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
			: m_SwapchainDepthTextures[m_CurrentFrame].view;
		depthInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
		if (node.PreserveRenderTarget && !swapchain &&
			m_DepthTextures.contains(node.RenderTarget)) {
			depthInfo.loadOp = vk::AttachmentLoadOp::eLoad;
		} else {
			depthInfo.loadOp = vk::AttachmentLoadOp::eClear;
			depthInfo.clearValue = vk::ClearDepthStencilValue(1.0f, 0);
		}

		if (useMsaa) {
			colorInfo.imageView = m_MsaaTextures[m_CurrentFrame].view;
			colorInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;
			colorInfo.resolveImageView = view;
			colorInfo.resolveImageLayout =
			  vk::ImageLayout::eColorAttachmentOptimal;
			colorInfo.loadOp = vk::AttachmentLoadOp::eClear;
			colorInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
			colorInfo.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
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
			if (call.IndexCount == 0) {
				continue;
			}

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

			vk::CullModeFlags cullMode = {};
			switch (call.cullMode) {
				case CULL_BACK: {
					cullMode = vk::CullModeFlagBits::eBack;
					break;
				}
				case CULL_FRONT: {
					cullMode = vk::CullModeFlagBits::eFront;
					break;
				}
				case CULL_NONE: {
					cullMode = vk::CullModeFlagBits::eNone;
					break;
				}
			}
			buffer.setCullMode(cullMode);

			if (m_ResolvedSettings.EDS3Blending == VFH_Require) {
				SetBlendMode(call.BlendingMode, buffer);
			}

			if (call.Settings.GraphicsPipeline != currentPipeline ||
				(m_ResolvedSettings.EDS3Blending != VFH_Require &&
				 call.BlendingMode != currentBlendMode)) {
				currentPipeline = call.Settings.GraphicsPipeline;
				currentBlendMode = call.BlendingMode;

				buffer.bindPipeline(
				  vk::PipelineBindPoint::eGraphics,
				  m_Cache->m_Pipelines[currentPipeline]
					.Pipelines[m_ResolvedSettings.EDS3Blending == VFH_Require
								 ? 0
								 : currentBlendMode]);
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
		  vk::ImageAspectFlagBits::eColor,
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
