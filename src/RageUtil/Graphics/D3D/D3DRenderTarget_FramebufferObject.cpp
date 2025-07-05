#include "D3DRenderTarget_FramebufferObject.h"
#include "RageUtil/Utils/RageUtil.h"
#include "Core/Services/Locator.hpp"

D3DRenderTarget_FramebufferObject::D3DRenderTarget_FramebufferObject()
{
	m_iFrameBufferHandle = nullptr;
	m_uTexHandle = nullptr;
	m_iDepthBufferHandle = nullptr;
}

D3DRenderTarget_FramebufferObject::~D3DRenderTarget_FramebufferObject()
{
	if (m_iDepthBufferHandle != nullptr) {
		m_iDepthBufferHandle->Release();
	}
	if (m_iFrameBufferHandle != nullptr) {
		m_iFrameBufferHandle->Release();
	}
	if (m_uTexHandle != nullptr) {
		m_uTexHandle->Release();
	}
}

void
D3DRenderTarget_FramebufferObject::Create(const RenderTargetParam& param,
										  int& iTextureWidthOut,
										  int& iTextureHeightOut)
{
	m_Param = param;

	const auto iTextureWidth = power_of_two(param.iWidth);
	const auto iTextureHeight = power_of_two(param.iHeight);

	iTextureWidthOut = iTextureWidth;
	iTextureHeightOut = iTextureHeight;

	D3DFORMAT textureFormat;
	if (param.bWithAlpha) {
		textureFormat = D3DFMT_A8R8G8B8;
	} else {
		textureFormat = D3DFMT_X8R8G8B8;
	}

	if (!SUCCEEDED(g_pd3dDevice->CreateTexture(iTextureWidth,
											   iTextureHeight,
											   1,
											   D3DUSAGE_RENDERTARGET,
											   textureFormat,
											   D3DPOOL_DEFAULT,
											   &m_uTexHandle,
											   nullptr))) {
		Locator::getLogger()->warn("FAILED: CreateTexture failed");
	}

	// Unlike OpenGL, D3D must use a depth stencil when using render targets
	if (!SUCCEEDED(g_pd3dDevice->CreateDepthStencilSurface(
		  iTextureWidth,
		  iTextureHeight,
		  g_d3dpp.AutoDepthStencilFormat,
		  g_d3dpp.MultiSampleType,
		  g_d3dpp.MultiSampleQuality,
		  true,
		  &m_iDepthBufferHandle,
		  nullptr))) {
		Locator::getLogger()->warn("FAILED: Didn't make depth stencil.");
	}
}

void
D3DRenderTarget_FramebufferObject::StartRenderingTo()
{
	// Save default color and depth buffer
	if (!SUCCEEDED(g_pd3dDevice->GetRenderTarget(0, &defaultColorBuffer)))
		Locator::getLogger()->warn("Failed to get default color buffer");

	if (!SUCCEEDED(g_pd3dDevice->GetDepthStencilSurface(&defaultDepthBuffer)))
		Locator::getLogger()->warn("Failed to get default depth buffer");

	// Set the render target to our RenderTarget texture
	m_uTexHandle->GetSurfaceLevel(0, &m_iFrameBufferHandle);
	if (!SUCCEEDED(g_pd3dDevice->SetRenderTarget(0, m_iFrameBufferHandle)))
		Locator::getLogger()->warn("Failed to set target to RenderTarget");

	if (!SUCCEEDED(g_pd3dDevice->SetDepthStencilSurface(m_iDepthBufferHandle)))
		Locator::getLogger()->warn("Failed to set targetDepth to RenderTargetDepth");
}

void
D3DRenderTarget_FramebufferObject::FinishRenderingTo()
{
	// Restore the original color and depth buffers
	if (!SUCCEEDED(g_pd3dDevice->SetRenderTarget(0, defaultColorBuffer)))
		Locator::getLogger()->warn("Failed to set target to BackBuffer");

	if (!SUCCEEDED(g_pd3dDevice->SetDepthStencilSurface(defaultDepthBuffer)))
		Locator::getLogger()->warn("Failed to set targetDepth to BackBufferDepth");
}
