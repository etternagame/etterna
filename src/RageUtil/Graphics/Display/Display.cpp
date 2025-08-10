#include "Display.h"
#include "Core/Services/Locator.hpp"
#include <cassert>
#include <source_location>

#ifdef _WIN32
#include "archutils/Win32/GraphicsWindow.h"
#else
#error Display::Display is unfinished for non-Windows platforms
#endif

Display::Display::Display(std::unique_ptr<Renderer> renderer) : m_Renderer(std::move(renderer))
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
    m_Batcher.CleanCommands();
    return m_IsInitDone;
}

void Display::Display::EndFrame()
{
    static bool rendered = false;
    if (!rendered)
    {
        for (auto &cmd : m_Batcher.m_CommandBuffer)
        {
            Locator::getLogger()->debug(cmd);
        }
        Locator::getLogger()->debug("command count: {}", m_Batcher.m_CommandBuffer.size());
        rendered = true;
    }
    m_Batcher.CleanCommands();
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
    m_Batcher.InsertCommand(std::source_location::current().function_name());
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
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetTextureMode(TextureUnit tu, TextureMode tm)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetTextureWrapping(TextureUnit tu, bool b)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

int Display::Display::GetMaxTextureSize() const
{
    return Display::Display::MaxTextureSize;
}

void Display::Display::SetTextureFiltering(TextureUnit tu, bool b)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
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
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetZBias(float f)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetZTestMode(ZTestMode mode)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::ClearZBuffer()
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetCullMode(CullMode mode)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetAlphaTest(bool b)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetMaterial(const RageColor &emissive, const RageColor &ambient, const RageColor &diffuse,
                                   const RageColor &specular, float shininess)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetLighting(bool b)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetLightOff(int index)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetLightDirectional(int index, const RageColor &ambient, const RageColor &diffuse,
                                           const RageColor &specular, const RageVector3 &dir)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

intptr_t Display::Display::CreateRenderTarget(const RenderTargetParam &param, int &iTextureWidthOut,
                                              int &iTextureHeightOut)
{
    return intptr_t();
}

intptr_t Display::Display::GetRenderTarget()
{
    return intptr_t();
}

void Display::Display::SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture)
{
}

void Display::Display::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::SetCelShaded(int stage)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

RageCompiledGeometry *Display::Display::CreateCompiledGeometry()
{
    return nullptr;
}

void Display::Display::DeleteCompiledGeometry(RageCompiledGeometry *p)
{
}

void Display::Display::DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::DrawQuadStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::DrawFanInternal(const RageSpriteVertex v[], int iNumVerts)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::DrawStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::DrawTrianglesInternal(const RageSpriteVertex v[], int iNumVerts)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
}

void Display::Display::DrawCompiledGeometryInternal(const RageCompiledGeometry *p, int iMeshIndex)
{
    m_Batcher.InsertCommand(std::source_location::current().function_name());
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
