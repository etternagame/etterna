#include "Display.h"
#include "Core/Services/Locator.hpp"
#include <cassert>
#include <source_location>

#ifdef _WIN32
#include "archutils/Win32/GraphicsWindow.h"
#else
#error Display::Display is unfinished for non-Windows platforms
#endif

Display::Display::Display(std::unique_ptr<Renderer> renderer) : m_Renderer(std::move(renderer)), m_RenderState()
{
}

std::string Display::Display::Init(VideoModeParams &&p, bool bAllowUnacceleratedRenderer)
{
    Locator::getLogger()->info("Display::Display::Init()");
    Locator::getLogger()->info("Current renderer: UnstableDisplay - {}", m_Renderer->GetApiDescription());

    m_Renderer->StartLoadingPipeline();

    bool ignored = false;
    return SetVideoMode(std::move(p), ignored);
}

void Display::Display::GetDisplaySpecs(DisplaySpecs &out) const
{
}

void Display::Display::ResolutionChanged()
{
    RageDisplay::ResolutionChanged();
}

const RageDisplay::RagePixelFormatDesc *Display::Display::GetPixelFormatDesc(RagePixelFormat pf) const
{
    assert(pf == RagePixelFormat_RGBA8);
    static auto desc = RagePixelFormatDesc{32, {0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF}};
    return &desc;
}

bool Display::Display::BeginFrame()
{
    m_Batcher.Clear();
    m_RenderState.cullMode = CULL_NONE;
    m_RenderState.zTestMode = ZTEST_OFF;
    m_RenderState.blendMode = BLEND_NORMAL;
    m_RenderState.zBias = 0.0f;
    m_RenderState.zWrite = false;
    m_RenderState.alphaTest = true;
    m_RenderState.textureFiltering[0] = true;
    m_RenderState.textureMode[0] = TextureMode_Invalid;
    m_RenderState.textureWrapping[0] = false;

    return m_IsInitDone;
}

void Display::Display::EndFrame()
{
    static bool rendered = false;
    if (!rendered)
    {
        Locator::getLogger()->debug("command count: {}, command buffer size: {}", m_Batcher.m_CommandCount,
                                    m_Batcher.m_CommandBuffer.size());
        rendered = true;
    }
    m_Batcher.Clear();
    m_Renderer->OnUpdate();
    m_Renderer->OnRender(GetActualVideoModeParams());
}

const ActualVideoModeParams *Display::Display::GetActualVideoModeParams() const
{
#ifdef _WIN32
    return GraphicsWindow::GetParams();
#else
#error Display::Display is unfinished for non-Windows platforms
#endif
}

void Display::Display::SetBlendMode(BlendMode mode)
{
    if (m_RenderState.blendMode == mode)
    {
        return;
    }

    m_RenderState.blendMode = mode;
    Command command = {};
    command.type = CommandType::SetBlendMode;
    command.blendMode = mode;
    m_Batcher.InsertCommand(command);
}

bool Display::Display::SupportsTextureFormat(RagePixelFormat pixfmt, bool realtime)
{
    return pixfmt == RagePixelFormat_RGBA8;
}

bool Display::Display::SupportsThreadedRendering()
{
    return false;
}

bool Display::Display::SupportsPerVertexMatrixScale()
{
    return false;
}

intptr_t Display::Display::CreateTexture(RagePixelFormat pixfmt, RageSurface *img, bool bGenerateMipMaps)
{
    assert(pixfmt == RagePixelFormat_RGBA8);

    return 0;
}

void Display::Display::UpdateTexture(intptr_t uTexHandle, RageSurface *img, int xoffset, int yoffset, int width,
                                     int height)
{
}

void Display::Display::DeleteTexture(intptr_t iTexHandle)
{
}

void Display::Display::ClearAllTextures()
{
}

int Display::Display::GetNumTextureUnits()
{
    return TextureUnit::NUM_TextureUnit;
}

void Display::Display::SetTexture(TextureUnit tu, intptr_t iTexture)
{
    if (m_RenderState.textures[tu] == iTexture)
    {
        return;
    }

    m_RenderState.textures[tu] = iTexture;
    Command command = {};
    command.type = CommandType::SetTexture;
    command.texture = {tu, iTexture};

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetTextureMode(TextureUnit tu, TextureMode tm)
{
    if (m_RenderState.textureMode[tu] == tm)
    {
        return;
    }

    m_RenderState.textureMode[tu] = tm;
    Command command = {};
    command.type = CommandType::SetTextureMode;
    command.texture = {tu, tm};

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetTextureWrapping(TextureUnit tu, bool b)
{
    if (m_RenderState.textureWrapping[tu] == b)
    {
        return;
    }

    m_RenderState.textureWrapping[tu] = b;
    Command command = {};
    command.type = CommandType::SetTextureWrapping;
    command.texture = {tu, b};

    m_Batcher.InsertCommand(command);
}

int Display::Display::GetMaxTextureSize() const
{
    return Display::Display::MaxTextureSize;
}

void Display::Display::SetTextureFiltering(TextureUnit tu, bool b)
{
    if (m_RenderState.textureFiltering[tu] == b)
    {
        return;
    }

    m_RenderState.textureFiltering[tu] = b;
    Command command = {};
    command.type = CommandType::SetTextureFiltering;
    command.texture = {tu, b};

    m_Batcher.InsertCommand(command);
}

bool Display::Display::IsZWriteEnabled() const
{
    return false;
}

bool Display::Display::IsZTestEnabled() const
{
    return false;
}

void Display::Display::SetZWrite(bool b)
{
    if (m_RenderState.zWrite == b)
    {
        return;
    }

    m_RenderState.zWrite = b;
    Command command = {};
    command.type = CommandType::SetZWrite;
    command.zWrite = b;

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetZBias(float f)
{
    if (m_RenderState.zBias == f)
    {
        return;
    }

    m_RenderState.zBias = f;
    Command command = {};
    command.type = CommandType::SetZBias;
    command.zBias = f;

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetZTestMode(ZTestMode mode)
{
    if (m_RenderState.zTestMode == mode)
    {
        return;
    }

    m_RenderState.zTestMode = mode;
    Command command = {};
    command.type = CommandType::SetZTestMode;
    command.zTestMode = mode;

    m_Batcher.InsertCommand(command);
}

void Display::Display::ClearZBuffer()
{
    Command command = {};
    command.type = CommandType::ClearZBuffer;

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetCullMode(CullMode mode)
{
    if (m_RenderState.cullMode == mode)
    {
        return;
    }

    m_RenderState.cullMode = mode;
    Command command = {};
    command.type = CommandType::SetCullMode;
    command.cullMode = mode;

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetAlphaTest(bool b)
{
    if (m_RenderState.alphaTest == b)
    {
        return;
    }
    m_RenderState.alphaTest = b;
    Command command = {};
    command.type = CommandType::SetAlphaTest;
    command.alphaTest = b;

    m_Batcher.InsertCommand(command);
}

void Display::Display::SetMaterial(const RageColor &emissive, const RageColor &ambient, const RageColor &diffuse,
                                   const RageColor &specular, float shininess)
{
    assert(false && "Not implemented");
}

void Display::Display::SetLighting(bool b)
{
    assert(false && "Not implemented");
}

void Display::Display::SetLightOff(int index)
{
    assert(false && "Not implemented");
}

void Display::Display::SetLightDirectional(int index, const RageColor &ambient, const RageColor &diffuse,
                                           const RageColor &specular, const RageVector3 &dir)
{
    assert(false && "Not implemented");
}

intptr_t Display::Display::CreateRenderTarget(const RenderTargetParam &param, int &iTextureWidthOut,
                                              int &iTextureHeightOut)
{
    assert(false && "Not implemented");
    return intptr_t();
}

intptr_t Display::Display::GetRenderTarget()
{
    assert(false && "Not implemented");
    return intptr_t();
}

void Display::Display::SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture)
{
    assert(false && "Not implemented");
}

void Display::Display::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
    assert(false && "Not implemented");
}

void Display::Display::SetCelShaded(int stage)
{
    assert(false && "Not implemented");
}

RageCompiledGeometry *Display::Display::CreateCompiledGeometry()
{
    assert(false && "Not implemented");
    return nullptr;
}

void Display::Display::DeleteCompiledGeometry(RageCompiledGeometry *p)
{
    assert(false && "Not implemented");
}

void Display::Display::DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts)
{
	MatrixState m;
	SetMatricesForState(m);
	m_Batcher.InsertDrawCommand(
	  DrawMode::Quads, m, (uint8_t*)v, iNumVerts * sizeof(RageSpriteVertex));
}

void Display::Display::DrawQuadStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
	MatrixState m;
	SetMatricesForState(m);
	m_Batcher.InsertDrawCommand(
	  DrawMode::QuadStrip, m, (uint8_t*)v, iNumVerts * sizeof(RageSpriteVertex));
}

void Display::Display::DrawFanInternal(const RageSpriteVertex v[], int iNumVerts)
{
	MatrixState m;
	SetMatricesForState(m);
	m_Batcher.InsertDrawCommand(
	  DrawMode::Fan, m, (uint8_t*)v, iNumVerts * sizeof(RageSpriteVertex));
}

void Display::Display::DrawStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
	MatrixState m;
	SetMatricesForState(m);
	m_Batcher.InsertDrawCommand(
	  DrawMode::Strip, m, (uint8_t*)v, iNumVerts * sizeof(RageSpriteVertex));
}

void Display::Display::DrawTrianglesInternal(const RageSpriteVertex v[], int iNumVerts)
{
	MatrixState m;
	SetMatricesForState(m);
	m_Batcher.InsertDrawCommand(
	  DrawMode::Triangles, m, (uint8_t*)v, iNumVerts * sizeof(RageSpriteVertex));
}

void Display::Display::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
	MatrixState m;
	SetMatricesForState(m);
	m_Batcher.InsertDrawCommand(
	  DrawMode::SymmetricQuadStrip, m, (uint8_t*)v, iNumVerts * sizeof(RageSpriteVertex));
}

void Display::Display::DrawCompiledGeometryInternal(const RageCompiledGeometry *p, int iMeshIndex)
{
	assert(false && "Not implemented");
}

std::string Display::Display::TryVideoMode(const VideoModeParams &p, bool &bNewDeviceOut)
{
#ifdef _WIN32
    GraphicsWindow::CreateGraphicsWindow(p);
#else
#error Display::Display is unfinished for non-Windows platforms
#endif

    m_Renderer->FinishLoadingPipeline(p);
    m_Renderer->LoadAssets(p);

    ResolutionChanged();
    // OnRender() with a black clearing to not whiteblast people?

    m_IsInitDone = true;
    return std::string();
}

RageSurface *Display::Display::CreateScreenshot()
{
    return nullptr;
}

void Display::Display::SetMatricesForState(MatrixState &matrixState)
{
    matrixState.projection = *GetProjectionTop();
    matrixState.view = *GetViewTop();
    matrixState.world = *GetWorldTop();
    matrixState.texture = *GetTextureTop();
}
