#include "Display.h"
#include "CompiledGeometry.h"
#include "Core/Services/Locator.hpp"
#include <cassert>
#include <source_location>
#include <RageUtil/Misc/RageMath.h>

DisplayAdapter::Display::Display(std::unique_ptr<Renderer> renderer)
  : m_Renderer(std::move(renderer))
  , m_RenderState()
{
}

std::string
DisplayAdapter::Display::Init(VideoModeParams&& p,
							  bool bAllowUnacceleratedRenderer)
{
	Locator::getLogger()->info("DisplayAdapter::Display::Init()");
	Locator::getLogger()->info("Current renderer: UnstableDisplay - {}",
							   m_Renderer->GetApiDescription());

	m_Window = LowLevelWindowVK::Create();

	bool ignored = false;
	return SetVideoMode(std::move(p), ignored);
}

void
DisplayAdapter::Display::GetDisplaySpecs(DisplaySpecs& out) const
{
	m_Window->GetDisplaySpecs(out);
}

void
DisplayAdapter::Display::ResolutionChanged()
{
	m_Renderer->ResolutionChanged();
	RageDisplay::ResolutionChanged();
}

bool
DisplayAdapter::Display::BeginFrame()
{
	m_Window->Update();

	m_Batcher.Clear();
	m_RenderState.textureFiltering = true;
	m_RenderState.textureWrapping = false;

	return m_IsInitDone && RageDisplay::BeginFrame();
}

void
DisplayAdapter::Display::EndFrame()
{
	m_Batcher.FixRenderNodeOrder();
	m_Renderer->OnRender(GetActualVideoModeParams(), m_Batcher);
	RageDisplay::EndFrame();
}

const ActualVideoModeParams*
DisplayAdapter::Display::GetActualVideoModeParams() const
{
	return m_Window->GetActualVideoModeParams();
}

std::string
DisplayAdapter::Display::TryVideoMode(const VideoModeParams& p,
									  bool& bNewDeviceOut)
{
	m_Window->TryVideoMode(p, bNewDeviceOut);

	if (!m_IsInitDone) {
		m_Renderer->InitializeRenderer(p);
	} else {
		m_Renderer->TryVideoMode(p);
	}

	ResolutionChanged();

	m_Renderer->OnRender(GetActualVideoModeParams(), m_Batcher);

	m_IsInitDone = true;
	return std::string();
}

#pragma region Texture handling

const RageDisplay::RagePixelFormatDesc*
DisplayAdapter::Display::GetPixelFormatDesc(RagePixelFormat pf) const
{
	assert(pf == RagePixelFormat_RGBA8 || pf == RagePixelFormat_BGRA8);
	static auto rgba8 =
	  RagePixelFormatDesc{ 32,
						   { 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 } };
	static auto bgra8 =
	  RagePixelFormatDesc{ 32,
						   { 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 } };
	return pf == RagePixelFormat_RGBA8 ? &rgba8 : &bgra8;
}

bool
DisplayAdapter::Display::SupportsTextureFormat(RagePixelFormat pixfmt,
											   bool realtime)
{
	return pixfmt == RagePixelFormat_RGBA8 || pixfmt == RagePixelFormat_BGRA8;
}

intptr_t
DisplayAdapter::Display::CreateTexture(RagePixelFormat pixfmt,
									   RageSurface* img,
									   bool bGenerateMipMaps)
{
	assert(SupportsTextureFormat(pixfmt));

	return m_Renderer->CreateTexture(img, pixfmt == RagePixelFormat_RGBA8);
}

void
DisplayAdapter::Display::UpdateTexture(intptr_t uTexHandle,
									   RageSurface* img,
									   int xoffset,
									   int yoffset,
									   int width,
									   int height)
{
	m_Renderer->UpdateTexture(uTexHandle, img, xoffset, yoffset, width, height);
}

void
DisplayAdapter::Display::DeleteTexture(intptr_t iTexHandle)
{
	m_Renderer->DeleteTexture(iTexHandle);
}

void
DisplayAdapter::Display::ClearAllTextures()
{
	m_Renderer->ClearAllTextures();
}

int
DisplayAdapter::Display::GetNumTextureUnits()
{
	return 1;
}

int
DisplayAdapter::Display::GetMaxTextureSize() const
{
	return DisplayAdapter::Display::MaxTextureSize;
}

#pragma endregion

#pragma region RenderState handling

void
DisplayAdapter::Display::SetTexture(TextureUnit tu, intptr_t iTexture)
{
	assert(tu == TextureUnit_1);
	m_RenderState.textureHandle = iTexture;
}

void
DisplayAdapter::Display::SetTextureWrapping(TextureUnit tu, bool b)
{
	assert(tu == TextureUnit_1);
	m_RenderState.textureWrapping = b;
}

void
DisplayAdapter::Display::SetTextureFiltering(TextureUnit tu, bool b)
{
	assert(tu == TextureUnit_1);
	m_RenderState.textureFiltering = b;
}

void
DisplayAdapter::Display::SetBlendMode(BlendMode mode)
{
	m_RenderState.blendingMode = mode;
}

void
DisplayAdapter::Display::SetZWrite(bool b)
{
	m_RenderState.depthWriteEnabled = b;
}

void
DisplayAdapter::Display::SetZTestMode(ZTestMode mode)
{
	m_RenderState.depthTestMode = mode;
}

bool
DisplayAdapter::Display::IsZWriteEnabled() const
{
	return m_RenderState.depthWriteEnabled;
}

bool
DisplayAdapter::Display::IsZTestEnabled() const
{
	return m_RenderState.depthTestMode != ZTEST_OFF;
}

#pragma endregion

#pragma region Draw queueing

void
DisplayAdapter::Display::DrawQuadsInternal(const RageSpriteVertex v[],
										   int iNumVerts)
{
	m_Batcher.InsertSpriteDrawCommand(
	  DrawMode::Quads, GetCurrentMatrixState(), v, iNumVerts, m_RenderState);
}

void
DisplayAdapter::Display::DrawQuadStripInternal(const RageSpriteVertex v[],
											   int iNumVerts)
{
	m_Batcher.InsertSpriteDrawCommand(DrawMode::QuadStrip,
									  GetCurrentMatrixState(),
									  v,
									  iNumVerts,
									  m_RenderState);
}

void
DisplayAdapter::Display::DrawFanInternal(const RageSpriteVertex v[],
										 int iNumVerts)
{
	m_Batcher.InsertSpriteDrawCommand(
	  DrawMode::Fan, GetCurrentMatrixState(), v, iNumVerts, m_RenderState);
}

void
DisplayAdapter::Display::DrawStripInternal(const RageSpriteVertex v[],
										   int iNumVerts)
{
	m_Batcher.InsertSpriteDrawCommand(
	  DrawMode::Strip, GetCurrentMatrixState(), v, iNumVerts, m_RenderState);
}

void
DisplayAdapter::Display::DrawTrianglesInternal(const RageSpriteVertex v[],
											   int iNumVerts)
{
	m_Batcher.InsertSpriteDrawCommand(DrawMode::Triangles,
									  GetCurrentMatrixState(),
									  v,
									  iNumVerts,
									  m_RenderState);
}

void
DisplayAdapter::Display::DrawSymmetricQuadStripInternal(
  const RageSpriteVertex v[],
  int iNumVerts)
{
	m_Batcher.InsertSpriteDrawCommand(DrawMode::SymmetricQuadStrip,
									  GetCurrentMatrixState(),
									  v,
									  iNumVerts,
									  m_RenderState);
}

void
DisplayAdapter::Display::DrawCompiledGeometryInternal(
  const RageCompiledGeometry* p,
  int iMeshIndex)
{
	m_Batcher.InsertCompiledGeometryDrawCommand(
	  GetCurrentMatrixState(), p, iMeshIndex, m_RenderState);
}

#pragma endregion

intptr_t
DisplayAdapter::Display::CreateRenderTarget(const RenderTargetParam& param,
											int& iTextureWidthOut,
											int& iTextureHeightOut)
{
	return m_Renderer->CreateRenderTarget(
	  param, iTextureWidthOut, iTextureHeightOut);
}

intptr_t
DisplayAdapter::Display::GetRenderTarget()
{
	return m_CurrentRenderTarget;
}

void
DisplayAdapter::Display::SetRenderTarget(intptr_t uTexHandle,
										 bool bPreserveTexture)
{
	m_Batcher.InsertRenderTargetCommand(uTexHandle, bPreserveTexture);
	m_CurrentRenderTarget = uTexHandle;
}

RageCompiledGeometry*
DisplayAdapter::Display::CreateCompiledGeometry()
{
	return new CompiledGeometry;
}

void
DisplayAdapter::Display::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
	assert(p != nullptr);
	delete p;
}

RageSurface*
DisplayAdapter::Display::CreateScreenshot()
{
	return m_Renderer->CreateScreenshot();
}

bool
DisplayAdapter::Display::SupportsThreadedRendering()
{
	return false;
}

bool
DisplayAdapter::Display::SupportsPerVertexMatrixScale()
{
	return false;
}

DisplayAdapter::MatrixState
DisplayAdapter::Display::GetCurrentMatrixState()
{
	MatrixState m = {};
	m.texture = *GetTextureTop();

	RageMatrix temp = {};

	RageMatrixMultiply(&temp, GetViewTop(), GetWorldTop());
	RageMatrixMultiply(&m.wvp, GetProjectionTop(), &temp);

	return m;
}

intptr_t
DisplayAdapter::Display::CreateGraphicsPipeline(
  const std::string& vertexShaderPath,
  const std::string& fragmentShaderPath)
{
	return m_Renderer->CreateGraphicsPipeline(vertexShaderPath,
											  fragmentShaderPath);
}

void
DisplayAdapter::Display::SetGraphicsPipeline(
  intptr_t pipeline,
  const std::vector<uint8_t>& vertexShaderArgs,
  const std::vector<uint8_t>& fragShaderArgs,
  bool persist)
{
	m_Batcher.InsertPipelineChangeCommand(
	  pipeline, vertexShaderArgs, fragShaderArgs, persist);
}

void
DisplayAdapter::Display::ReloadPipelines()
{
	m_Renderer->ReloadPipelines();
}

#pragma region Unsupported / old graphics API functions

void
DisplayAdapter::Display::SetTextureMode(TextureUnit tu, TextureMode tm)
{
}

void
DisplayAdapter::Display::SetZBias(float f)
{
}

void
DisplayAdapter::Display::SetCullMode(CullMode mode)
{
}

void
DisplayAdapter::Display::SetAlphaTest(bool b)
{
}

void
DisplayAdapter::Display::ClearZBuffer()
{
}

void
DisplayAdapter::Display::SetMaterial(const RageColor& emissive,
									 const RageColor& ambient,
									 const RageColor& diffuse,
									 const RageColor& specular,
									 float shininess)
{
}

void
DisplayAdapter::Display::SetLighting(bool b)
{
}

void
DisplayAdapter::Display::SetLightOff(int index)
{
}

void
DisplayAdapter::Display::SetLightDirectional(int index,
											 const RageColor& ambient,
											 const RageColor& diffuse,
											 const RageColor& specular,
											 const RageVector3& dir)
{
}

void
DisplayAdapter::Display::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
}

void
DisplayAdapter::Display::SetCelShaded(int stage)
{
}

#pragma endregion
