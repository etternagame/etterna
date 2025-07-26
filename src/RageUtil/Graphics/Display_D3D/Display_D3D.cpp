#include "Display_D3D.h"
#include "Core/Services/Locator.hpp"
#include "archutils/Win32/GraphicsWindow.h"
#include <source_location>
#include <exception>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

inline std::string
HrToString(HRESULT hr)
{
	char* errorMsg = nullptr;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
					 FORMAT_MESSAGE_IGNORE_INSERTS,
				   nullptr,
				   hr,
				   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				   reinterpret_cast<LPSTR>(&errorMsg),
				   0,
				   nullptr);

	std::string message = errorMsg ? errorMsg : "Unknown error";
	if (errorMsg)
		LocalFree(errorMsg);
	return message;
}

inline void
ThrowIfFailed(
  HRESULT hr,
  const std::source_location location = std::source_location::current())
{
	if (SUCCEEDED(hr)) {
		return;
	}

	std::string error = HrToString(hr);
	const std::string message =
	  std::format("Failed: HRESULT {} ({}) at {}:{} in function {}",
		  hr, error, location.file_name(), location.line(), location.function_name());
	Locator::getLogger()->error(message);
	throw std::exception(message.c_str());
}

Display_D3D::Display_D3D(): m_DXGIFactoryFlags(0) {}

std::string
Display_D3D::Init(VideoModeParams&& p, bool bAllowUnacceleratedRenderer)
{
	Locator::getLogger()->info("Display_D3D::Init()");
	Locator::getLogger()->info("Current renderer: Direct3D (unstable DirectX 12 version)");
	
#if defined(DEBUG) || defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		m_DXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ThrowIfFailed(
	  CreateDXGIFactory1(IID_PPV_ARGS(&m_DXGIFactory)));

	HRESULT result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));
	if (FAILED(result)) {
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(
		  m_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
		  warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device)));
	}

	m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_DsvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(
	  D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_CbvSrvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(
	  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return std::string();
}

void
Display_D3D::GetDisplaySpecs(DisplaySpecs& out) const
{
}

void
Display_D3D::ResolutionChanged()
{
}

const RageDisplay::RagePixelFormatDesc*
Display_D3D::GetPixelFormatDesc(RagePixelFormat pf) const
{
	return nullptr;
}

bool
Display_D3D::BeginFrame()
{
	return false;
}

void
Display_D3D::EndFrame()
{
}

const ActualVideoModeParams*
Display_D3D::GetActualVideoModeParams() const
{
#ifdef _WIN32
	return GraphicsWindow::GetParams();
#else
#error Display_D3D is meant for Windows... Or something
#endif
}

void
Display_D3D::SetBlendMode(BlendMode mode)
{
}

bool
Display_D3D::SupportsTextureFormat(RagePixelFormat pixfmt, bool realtime)
{
	return false;
}

bool
Display_D3D::SupportsThreadedRendering()
{
	return false;
}

bool
Display_D3D::SupportsPerVertexMatrixScale()
{
	return false;
}

intptr_t
Display_D3D::CreateTexture(RagePixelFormat pixfmt,
						   RageSurface* img,
						   bool bGenerateMipMaps)
{
	return intptr_t();
}

void
Display_D3D::UpdateTexture(intptr_t uTexHandle,
						   RageSurface* img,
						   int xoffset,
						   int yoffset,
						   int width,
						   int height)
{
}

void
Display_D3D::DeleteTexture(intptr_t iTexHandle)
{
}

void
Display_D3D::ClearAllTextures()
{
}

int
Display_D3D::GetNumTextureUnits()
{
	return 0;
}

void
Display_D3D::SetTexture(TextureUnit tu, intptr_t iTexture)
{
}

void
Display_D3D::SetTextureMode(TextureUnit tu, TextureMode tm)
{
}

void
Display_D3D::SetTextureWrapping(TextureUnit tu, bool b)
{
}

int
Display_D3D::GetMaxTextureSize() const
{
	return 0;
}

void
Display_D3D::SetTextureFiltering(TextureUnit tu, bool b)
{
}

bool
Display_D3D::IsZWriteEnabled() const
{
	return false;
}

bool
Display_D3D::IsZTestEnabled() const
{
	return false;
}

void
Display_D3D::SetZWrite(bool b)
{
}

void
Display_D3D::SetZBias(float f)
{
}

void
Display_D3D::SetZTestMode(ZTestMode mode)
{
}

void
Display_D3D::ClearZBuffer()
{
}

void
Display_D3D::SetCullMode(CullMode mode)
{
}

void
Display_D3D::SetAlphaTest(bool b)
{
}

void
Display_D3D::SetMaterial(const RageColor& emissive,
						 const RageColor& ambient,
						 const RageColor& diffuse,
						 const RageColor& specular,
						 float shininess)
{
}

void
Display_D3D::SetLighting(bool b)
{
}

void
Display_D3D::SetLightOff(int index)
{
}

void
Display_D3D::SetLightDirectional(int index,
								 const RageColor& ambient,
								 const RageColor& diffuse,
								 const RageColor& specular,
								 const RageVector3& dir)
{
}

intptr_t
Display_D3D::CreateRenderTarget(const RenderTargetParam& param,
								int& iTextureWidthOut,
								int& iTextureHeightOut)
{
	return intptr_t();
}

intptr_t
Display_D3D::GetRenderTarget()
{
	return intptr_t();
}

void
Display_D3D::SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture)
{
}

void
Display_D3D::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
}

void
Display_D3D::SetCelShaded(int stage)
{
}

RageCompiledGeometry*
Display_D3D::CreateCompiledGeometry()
{
	return nullptr;
}

void
Display_D3D::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
}

void
Display_D3D::DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawQuadStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawFanInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawTrianglesInternal(const RageSpriteVertex v[], int iNumVerts)
{
}

void
Display_D3D::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
											int iNumVerts)
{
}

void
Display_D3D::DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
										  int iMeshIndex)
{
}

std::string
Display_D3D::TryVideoMode(const VideoModeParams& p, bool& bNewDeviceOut)
{
	return std::string();
}

RageSurface*
Display_D3D::CreateScreenshot()
{
	return nullptr;
}
