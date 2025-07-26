#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "RageUtil/Graphics/RageDisplay.h"
#include "RageDisplay_D3D.h"
#include "RageDisplay_D3D_Helpers.h"
#include "RageUtil/Misc/RageException.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageMath.h"
#include "RageUtil/Graphics/RageSurface.h"
#include "RageUtil/Graphics/RageSurfaceUtils.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Win32/GraphicsWindow.h"
#include "RageUtil/File/RageFileManager.h"
#include "RagePixelShader_D3D.h"
#include "RageVertexShader_D3D.h"
#include "D3DRenderTarget_FramebufferObject.h"
#include "RageCompiledGeometrySWD3D.h"

#include <optional>
#include <algorithm>
#include <map>
#include <list>
#include <chrono>
#include <fstream>

// Static libraries
// load Windows D3D9 dynamically
#if defined(_MSC_VER)
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#endif

// Load the palette, if any, for the given texture into a palette slot, and make
// it current.
void
RageDisplay_D3D::SetPalette(unsigned TexResource)
{
	// If the texture isn't paletted, we have nothing to do.
	if (m_TexResourceToTexturePalette.find(TexResource) ==
		m_TexResourceToTexturePalette.end()) {
		return;
	}

	// Is the palette already loaded?
	if (m_TexResourceToPaletteIndex.find(TexResource) ==
		m_TexResourceToPaletteIndex.end()) {
		// It's not. Grab the least recently used slot.
		const auto iPalIndex = m_PaletteIndex.front();

		// If any other texture is currently using this slot, mark that palette
		// unloaded.
		for (auto i = m_TexResourceToPaletteIndex.begin();
			 i != m_TexResourceToPaletteIndex.end();
			 ++i) {
			if (i->second != iPalIndex) {
				continue;
			}
			m_TexResourceToPaletteIndex.erase(i);
			break;
		}

		// Load it.
		auto& pal = m_TexResourceToTexturePalette[TexResource];
		m_Device->SetPaletteEntries(iPalIndex, pal.p);

		m_TexResourceToPaletteIndex[TexResource] = iPalIndex;
	}

	const auto iPalIndex = m_TexResourceToPaletteIndex[TexResource];

	// Find this palette index in the least-recently-used queue and move it to
	// the end.
	for (auto i = m_PaletteIndex.begin(); i != m_PaletteIndex.end(); ++i) {
		if (*i != iPalIndex) {
			continue;
		}
		m_PaletteIndex.erase(i);
		m_PaletteIndex.push_back(iPalIndex);
		break;
	}

	m_Device->SetCurrentTexturePalette(iPalIndex);
}

static const RageDisplay::RagePixelFormatDesc
  PIXEL_FORMAT_DESC[NUM_RagePixelFormat] = {
	  { /* A8B8G8R8 */
		32,
		{ 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 } },
	  { 0, { 0, 0, 0, 0 } },
	  {
		/* A4R4G4B4 */
		16,
		{ 0x0F00, 0x00F0, 0x000F, 0xF000 },
	  },
	  {
		/* A1B5G5R5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x8000 },
	  },
	  {
		/* X1R5G5B5 */
		16,
		{ 0x7C00, 0x03E0, 0x001F, 0x0000 },
	  },
	  { /* B8G8R8 */
		24,
		{ 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 } },
	  {
		/* Paletted */
		8,
		{ 0, 0, 0, 0 } /* N/A */
	  },
	  { /* BGR (N/A; OpenGL only) */
		0,
		{ 0, 0, 0, 0 } },
	  { /* ABGR (N/A; OpenGL only) */
		0,
		{ 0, 0, 0, 0 } },
	  { /* X1R5G5B5 */
		0,
		{ 0, 0, 0, 0 } }
  };

static D3DFORMAT D3DFORMATS[NUM_RagePixelFormat] = {
	D3DFMT_A8R8G8B8, D3DFMT_UNKNOWN, D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5,
	D3DFMT_X1R5G5B5, D3DFMT_R8G8B8,	 D3DFMT_P8,
	D3DFMT_UNKNOWN, // no BGR
	D3DFMT_UNKNOWN, // no ABGR
	D3DFMT_UNKNOWN, // X1R5G5B5
};

auto
RageDisplay_D3D::GetPixelFormatDesc(RagePixelFormat pf) const
  -> const RagePixelFormatDesc*
{
	ASSERT(pf < NUM_RagePixelFormat);
	return &PIXEL_FORMAT_DESC[pf];
}

RageDisplay_D3D::RageDisplay_D3D() = default;

static LocalizedString D3D_NOT_INSTALLED(
  "RageDisplay_D3D",
  "DirectX 9.0c or greater is not installed.  You can download it from:");
const std::string D3D_URL =
  "http://www.microsoft.com/en-us/download/details.aspx?id=8109";
static LocalizedString HARDWARE_ACCELERATION_NOT_AVAILABLE(
  "RageDisplay_D3D",
  "Your system is reporting that Direct3D hardware acceleration is not "
  "available.  Please obtain an updated driver from your video card "
  "manufacturer.");

auto
RageDisplay_D3D::Init(VideoModeParams&& p,
					  bool /* bAllowUnacceleratedRenderer */) -> std::string
{
	GraphicsWindow::Initialize(true);

	Locator::getLogger()->info("RageDisplay_D3D::RageDisplay_D3D()");
	Locator::getLogger()->info("Current renderer: Direct3D");

	m_D3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_D3D == nullptr) {
		Locator::getLogger()->fatal("Direct3DCreate9 failed");
		return D3D_NOT_INSTALLED.GetValue();
	}

	if (FAILED(m_D3D->GetDeviceCaps(
		  D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_DeviceCaps))) {
		return HARDWARE_ACCELERATION_NOT_AVAILABLE.GetValue();
	}

	D3DADAPTER_IDENTIFIER9 identifier;
	m_D3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier);

	Locator::getLogger()->info(
	  "Driver: {}\n"
	  "Description: {}\n"
	  "Max texture size: {}\n"
	  "Alpha in palette: {}\n",
	  identifier.Driver,
	  identifier.Description,
	  m_DeviceCaps.MaxTextureWidth,
	  (m_DeviceCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE) ? "yes" : "no");

	Locator::getLogger()->info("This display adaptor supports the following modes:");
	D3DDISPLAYMODE mode;

	const auto modeCount =
	  m_D3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, m_DefaultAdapterFormat);

	for (UINT u = 0; u < modeCount; u++) {
		if (SUCCEEDED(m_D3D->EnumAdapterModes(D3DADAPTER_DEFAULT, m_DefaultAdapterFormat, u, &mode))) {
			Locator::getLogger()->info("  {}x{} {}Hz, format {}", mode.Width, mode.Height, mode.RefreshRate, mode.Format);
		}
	}

	m_PaletteIndex.clear();
	for (auto i = 0; i < 256; ++i) {
		m_PaletteIndex.push_back(i);
	}

	// Save the original desktop format.
	m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_DesktopMode);

	/* Up until now, all we've done is set up m_D3D and do some queries. Now,
	 * actually initialize the window. Do this after as many error conditions as
	 * possible, because if we have to shut it down again we'll flash a window
	 * briefly. */
	auto bIgnore = false;
	return SetVideoMode(std::move(p), bIgnore);
}

RageDisplay_D3D::~RageDisplay_D3D()
{
	Locator::getLogger()->info("RageDisplay_D3D::~RageDisplay()");

	GraphicsWindow::Shutdown();

	if (m_SpriteVertexDeclaration != nullptr) {
		m_SpriteVertexDeclaration->Release();
		m_SpriteVertexDeclaration = nullptr;
	}

	if (m_ModelVertexDeclaration != nullptr) {
		m_ModelVertexDeclaration->Release();
		m_ModelVertexDeclaration = nullptr;
	}

	if (m_Device != nullptr) {
		m_Device->Release();
		m_Device = nullptr;
	}

	if (m_D3D != nullptr) {
		m_D3D->Release();
		m_D3D = nullptr;
	}

	/* Even after we call Release(), D3D may still affect our window. It seems
	 * to subclass the window, and never release it. Free the DLL after
	 * destroying the window. */
	if (m_D3D9_Module != nullptr) {
		FreeLibrary(m_D3D9_Module);
		m_D3D9_Module = nullptr;
	}
}

void
RageDisplay_D3D::GetDisplaySpecs(DisplaySpecs& out) const
{
	out.clear();
	const int iCnt =
	  m_D3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, m_DefaultAdapterFormat);
	std::set<DisplayMode> modes;
	D3DDISPLAYMODE mode;

	for (auto i = 0; i < iCnt; ++i) {
		m_D3D->EnumAdapterModes(
		  D3DADAPTER_DEFAULT, m_DefaultAdapterFormat, i, &mode);
		modes.insert(
		  { mode.Width, mode.Height, static_cast<double>(mode.RefreshRate) });
	}
	// Get the current display mode
	if (m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode) == D3D_OK) {
		D3DADAPTER_IDENTIFIER9 ID;
		m_D3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &ID);
		const DisplayMode active = { mode.Width,
									 mode.Height,
									 static_cast<double>(mode.RefreshRate) };
		const RectI bounds(0, 0, active.width, active.height);
		out.insert(DisplaySpec("", "Fullscreen", modes, active, bounds));
	} else {
		Locator::getLogger()->warn("Could not find active mode for default D3D adapter");
		if (!modes.empty()) {
			const auto& m = *modes.begin();
			const RectI bounds(0, 0, m.width, m.height);
			out.insert(DisplaySpec("", "Fullscreen", modes, m, bounds));
		}
	}
}

auto
RageDisplay_D3D::FindBackBufferType(bool bWindowed, int iBPP) -> D3DFORMAT
{
	HRESULT hr;

	// If windowed, then bpp is ignored.  Use whatever works.
	std::vector<D3DFORMAT> vBackBufferFormats; // throw all possibilities in here

	// When windowed, add all formats; otherwise add only formats that match
	// dwBPP.
	if (iBPP == 32 || bWindowed) {
		vBackBufferFormats.push_back(D3DFMT_R8G8B8);
		vBackBufferFormats.push_back(D3DFMT_X8R8G8B8);
		vBackBufferFormats.push_back(D3DFMT_A8R8G8B8);
	}
	if (iBPP == 16 || bWindowed) {
		vBackBufferFormats.push_back(D3DFMT_R5G6B5);
		vBackBufferFormats.push_back(D3DFMT_X1R5G5B5);
		vBackBufferFormats.push_back(D3DFMT_A1R5G5B5);
	}

	if (!bWindowed && iBPP != 16 && iBPP != 32) {
		GraphicsWindow::Shutdown();
		RageException::Throw("Invalid BPP '%i' specified", iBPP);
	}

	// Test each back buffer format until we find something that works.
	for (auto& vBackBufferFormat : vBackBufferFormats) {
		const auto fmtBackBuffer = vBackBufferFormat;

		D3DFORMAT fmtDisplay;
		if (bWindowed) {
			fmtDisplay = m_DesktopMode.Format;
		} else { // Fullscreen
			fmtDisplay = vBackBufferFormat;
		}

		Locator::getLogger()->debug("Testing format: display {}, back buffer {}, windowed {}...",
		  fmtDisplay,
		  fmtBackBuffer,
		  static_cast<int>(bWindowed));

		hr = m_D3D->CheckDeviceType(D3DADAPTER_DEFAULT,
									 D3DDEVTYPE_HAL,
									 fmtDisplay,
									 fmtBackBuffer,
									 static_cast<BOOL>(bWindowed));

		if (FAILED(hr)) {
			continue; // skip
		}

		// done searching
		Locator::getLogger()->trace("This will work.");
		return fmtBackBuffer;
	}

	Locator::getLogger()->warn("Couldn't find an appropriate back buffer format.");
	return D3DFMT_UNKNOWN;
}

auto
RageDisplay_D3D::SetD3DParams(bool& bNewDeviceOut) -> std::string
{
	// wipe old render targets
	for (auto& rt : m_mapRenderTargets) {
		delete rt.second;
	}
	m_mapRenderTargets.clear();

	if (m_Device == nullptr)
	// device is not yet created. We need to create it
	{
		bNewDeviceOut = true;
		auto hr =
		  m_D3D->CreateDevice(D3DADAPTER_DEFAULT,
							   D3DDEVTYPE_HAL,
							   GraphicsWindow::GetHwnd(),
							   D3DCREATE_HARDWARE_VERTEXPROCESSING,
							   &m_PresentationParameters,
							   &m_Device);
		if (FAILED(hr)) {
			// Likely D3D_ERR_INVALIDCALL.  The driver probably doesn't support
			// this video mode.
			return ssprintf("CreateDevice failed: '%s'",
			  RageDisplay_D3D_Helpers::GetErrorString(hr).c_str());
		}
	} else {
		bNewDeviceOut = false;

		Locator::getLogger()->debug("Resetting D3D device");
		const auto hr = m_Device->Reset(&m_PresentationParameters);
		if (FAILED(hr)) {
			// Likely D3D_ERR_INVALIDCALL.  The driver probably doesn't support
			// this video mode.
			return ssprintf("g_pd3dDevice->Reset failed: '%s'",
							RageDisplay_D3D_Helpers::GetErrorString(hr).c_str());
		}

		ResetShaderSetupForDevice();
	}

	auto result = InitShaderSetupForDevice();
	if (!result.empty()) {
		return result;
	}

	m_Device->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	// Palettes were lost by Reset(), so mark them unloaded.
	m_TexResourceToPaletteIndex.clear();

	return std::string();
}

// If the given parameters have failed, try to lower them.
auto
RageDisplay_D3D::D3DReduceParams(D3DPRESENT_PARAMETERS* pp) -> bool
{
	D3DDISPLAYMODE current;
	current.Format = pp->BackBufferFormat;
	current.Height = pp->BackBufferHeight;
	current.Width = pp->BackBufferWidth;
	current.RefreshRate = pp->FullScreen_RefreshRateInHz;

	const int iCnt =
	  m_D3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, m_DefaultAdapterFormat);
	auto iBest = -1;
	auto iBestScore = 0;
	Locator::getLogger()->debug("cur: {}x{} {}Hz, format {}",
			   current.Width, current.Height,
			   current.RefreshRate, current.Format);
	for (auto i = 0; i < iCnt; ++i) {
		D3DDISPLAYMODE mode;
		m_D3D->EnumAdapterModes(
		  D3DADAPTER_DEFAULT, m_DefaultAdapterFormat, i, &mode);

		// Never change the format.
		if (mode.Format != current.Format) {
			continue;
		}
		// Never increase the parameters.
		if (mode.Height > current.Height || mode.Width > current.Width ||
			mode.RefreshRate > current.RefreshRate) {
			continue;
		}

		// Never go below 640x480 unless we already are.
		if ((current.Width >= 640 && current.Height >= 480) &&
			(mode.Width < 640 || mode.Height < 480)) {
			continue;
		}

		// Never go below 60Hz.
		if ((mode.RefreshRate != 0u) && mode.RefreshRate < 60) {
			continue;
		}

		/* If mode.RefreshRate is 0, it means "default". We don't know what
		 * that means; assume it's 60Hz. */

		// Higher scores are better.
		auto iScore = 0;
		if (current.RefreshRate >= 70 && mode.RefreshRate < 70) {
			/* Top priority: we really want to avoid dropping to a refresh rate
			 * that's below 70Hz. */
			iScore -= 100000;
		} else if (mode.RefreshRate < current.RefreshRate) {
			/* Low priority: We're lowering the refresh rate, but not too far.
			 * current.RefreshRate might be 0, in which case this simply gives
			 * points for higher refresh rates. */
			iScore += (mode.RefreshRate - current.RefreshRate);
		}

		// Medium priority:
		const int iResolutionDiff =
		  (current.Height - mode.Height) + (current.Width - mode.Width);
		iScore -= iResolutionDiff * 100;

		if (iBest == -1 || iScore > iBestScore) {
			iBest = i;
			iBestScore = iScore;
		}

		Locator::getLogger()->trace("try: {}x{} {}Hz, format {}: score {}",
				   mode.Width, mode.Height,
				   mode.RefreshRate, mode.Format, iScore);
	}

	if (iBest == -1) {
		return false;
	}

	D3DDISPLAYMODE BestMode;
	m_D3D->EnumAdapterModes(
	  D3DADAPTER_DEFAULT, m_DefaultAdapterFormat, iBest, &BestMode);
	pp->BackBufferHeight = BestMode.Height;
	pp->BackBufferWidth = BestMode.Width;
	pp->FullScreen_RefreshRateInHz = BestMode.RefreshRate;

	return true;
}

void
RageDisplay_D3D::SetPresentParametersFromVideoModeParams(const VideoModeParams& p,
										D3DPRESENT_PARAMETERS* pD3Dpp)
{
	ZERO(*pD3Dpp);
	const auto displayFormat = FindBackBufferType(p.windowed, p.bpp);
	auto enableMultiSampling = false;

	if (p.bSmoothLines &&
		SUCCEEDED(m_D3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
													 D3DDEVTYPE_HAL,
													 displayFormat,
													 p.windowed,
													 D3DMULTISAMPLE_8_SAMPLES,
													 nullptr))) {
		enableMultiSampling = true;
	}

	pD3Dpp->BackBufferWidth = p.width;
	pD3Dpp->BackBufferHeight = p.height;
	pD3Dpp->BackBufferFormat = displayFormat;
	pD3Dpp->BackBufferCount = 1;
	pD3Dpp->MultiSampleType =
	  enableMultiSampling ? D3DMULTISAMPLE_8_SAMPLES : D3DMULTISAMPLE_NONE;
	pD3Dpp->SwapEffect = D3DSWAPEFFECT_DISCARD;
	pD3Dpp->hDeviceWindow = GraphicsWindow::GetHwnd();
	pD3Dpp->Windowed = static_cast<BOOL>(p.windowed);
	pD3Dpp->EnableAutoDepthStencil = TRUE;
	pD3Dpp->AutoDepthStencilFormat = D3DFMT_D16;
	pD3Dpp->PresentationInterval =
	  p.vsync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

	pD3Dpp->FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	if (!p.windowed && p.rate != REFRESH_DEFAULT) {
		pD3Dpp->FullScreen_RefreshRateInHz = p.rate;
	}

	pD3Dpp->Flags = 0;

	Locator::getLogger()->info(
	  "Present Parameters: {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}",
	  pD3Dpp->BackBufferWidth,
	  pD3Dpp->BackBufferHeight,
	  pD3Dpp->BackBufferFormat,
	  pD3Dpp->BackBufferCount,
	  pD3Dpp->MultiSampleType,
	  pD3Dpp->SwapEffect,
	  (void*)pD3Dpp->hDeviceWindow,
	  pD3Dpp->Windowed,
	  pD3Dpp->EnableAutoDepthStencil,
	  pD3Dpp->AutoDepthStencilFormat,
	  pD3Dpp->Flags,
	  pD3Dpp->FullScreen_RefreshRateInHz,
	  pD3Dpp->PresentationInterval);
}

void
RageDisplay_D3D::SetShaderInputs(TextureUnit textureUnitIndex)
{
	auto matrix = GetWorldViewProjectionMatrix();
	auto hr = m_Device->SetVertexShaderConstantF(0, *matrix, 4);
	RageDisplay_D3D_Helpers::LogHResultFailure(hr);

	// D3D9 doesn't support bitwise operations so this will do for now
	auto time =
	  static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(
						   std::chrono::steady_clock::now().time_since_epoch())
						   .count());
	hr = m_Device->SetVertexShaderConstantF(4, &time, 1);
	RageDisplay_D3D_Helpers::LogHResultFailure(hr);

	int index = static_cast<int>(textureUnitIndex);
	int textureIndex[4] = { index, 0, 0, 0 };
	hr = m_Device->SetPixelShaderConstantI(0, textureIndex, 1);
	RageDisplay_D3D_Helpers::LogHResultFailure(hr);

	SetPixelShaderUniform();
	SetVertexShaderUniform();
}

void
RageDisplay_D3D::SetPixelShaderUniform()
{

	auto pixelUniform = m_PixelShaderHandler->TryPopUniform();
	if (!pixelUniform.has_value()) {
		return;
	}

	for (auto& uniform : pixelUniform->boolData) {
		auto hr = m_Device->SetPixelShaderConstantB(
		  uniform.m_StartRegister, &uniform.m_Data[0], uniform.m_Data.size());
		RageDisplay_D3D_Helpers::LogHResultFailure(hr);
	}

	for (auto& uniform : pixelUniform->intData) {
		const size_t uniformSize =
		  uniform.m_Data.size() % 4 + uniform.m_Data.size() / 4;

		auto hr = m_Device->SetPixelShaderConstantI(
		  uniform.m_StartRegister, &uniform.m_Data[0], uniformSize);
		RageDisplay_D3D_Helpers::LogHResultFailure(hr);
	}

	for (auto& uniform : pixelUniform->floatData) {
		const size_t uniformSize =
		  uniform.m_Data.size() % 4 + uniform.m_Data.size() / 4;

		auto hr = m_Device->SetPixelShaderConstantF(
		  uniform.m_StartRegister, &uniform.m_Data[0], uniformSize);
		RageDisplay_D3D_Helpers::LogHResultFailure(hr);
	}
}

void
RageDisplay_D3D::SetVertexShaderUniform()
{

	auto vertexUniform = m_VertexShaderHandler->TryPopUniform();
	if (!vertexUniform.has_value()) {
		return;
	}

	for (auto& uniform : vertexUniform->boolData) {
		auto hr = m_Device->SetVertexShaderConstantB(
		  uniform.m_StartRegister, &uniform.m_Data[0], uniform.m_Data.size());
		RageDisplay_D3D_Helpers::LogHResultFailure(hr);
	}

	for (auto& uniform : vertexUniform->intData) {
		const size_t uniformSize =
		  uniform.m_Data.size() % 4 + uniform.m_Data.size() / 4;

		auto hr = m_Device->SetVertexShaderConstantI(
		  uniform.m_StartRegister, &uniform.m_Data[0], uniformSize);
		RageDisplay_D3D_Helpers::LogHResultFailure(hr);
	}

	for (auto& uniform : vertexUniform->floatData) {
		const size_t uniformSize =
		  uniform.m_Data.size() % 4 + uniform.m_Data.size() / 4;

		auto hr = m_Device->SetVertexShaderConstantF(
		  uniform.m_StartRegister, &uniform.m_Data[0], uniformSize);
		RageDisplay_D3D_Helpers::LogHResultFailure(hr);
	}
}

void
RageDisplay_D3D::PrepareForDrawingPrimitives(bool useVertexDeclaration,
											 TextureUnit textureUnitIndex)
{
	SetShadersForDeclaration(useVertexDeclaration);
	SendCurrentMatrices();
	SetShaderInputs(textureUnitIndex);
}

std::string
RageDisplay_D3D::InitShaderSetupForDevice()
{
	auto shaderProfiles = GetSupportedShaderProfiles();

	// (for now?) only the latest vertex shader version for D3D9
	ASSERT(shaderProfiles[0] == "vs_3_0");
	ASSERT(m_DeviceCaps.VertexShaderVersion >= D3DVS_VERSION(3, 0));
	// (for now?) only the latest pixel shader version for D3D9
	ASSERT(shaderProfiles[1] == "ps_3_0");
	ASSERT(m_DeviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0));

	m_PixelShaderHandler.emplace(
	  RageDisplayType::D3D,
	  RageShaderType::Fragment,
	  [&](const std::string& path) {
		  return RageDisplay_D3D_Helpers::CompilePixelShaderFromPath(path,
																	 m_Device);
	  });

	m_VertexShaderHandler.emplace(
	  RageDisplayType::D3D,
	  RageShaderType::Vertex,
	  [&](const std::string& path) {
		  return RageDisplay_D3D_Helpers::CompileVertexShaderFromPath(path,
																	  m_Device);
	  });

	auto hr = m_Device->CreateVertexDeclaration(
	  RageDisplay_D3D_Helpers::SpriteDeclaration, &m_SpriteVertexDeclaration);
	if (FAILED(hr)) {
		return ssprintf("CreateVertexDeclaration failed: '%s'",
						RageDisplay_D3D_Helpers::GetErrorString(hr).c_str());
	}

	hr = m_Device->CreateVertexDeclaration(
	  RageDisplay_D3D_Helpers::ModelDeclaration, &m_ModelVertexDeclaration);
	if (FAILED(hr)) {
		return ssprintf("CreateVertexDeclaration failed: '%s'",
						RageDisplay_D3D_Helpers::GetErrorString(hr).c_str());
	}

	return std::string();
}

void
RageDisplay_D3D::ResetShaderSetupForDevice()
{
	if (m_ModelVertexDeclaration != nullptr) {
		m_ModelVertexDeclaration->Release();
		m_ModelVertexDeclaration = nullptr;
	}

	if (m_SpriteVertexDeclaration != nullptr) {
		m_SpriteVertexDeclaration->Release();
		m_SpriteVertexDeclaration = nullptr;
	}

	m_PixelShaderHandler = std::nullopt;
	m_VertexShaderHandler = std::nullopt;

	m_PreviousVertexDecl = nullptr;
	m_PreviousVertexShader = nullptr;
	m_PreviousPixelShader = nullptr;
}

// Set the video mode.
auto
RageDisplay_D3D::TryVideoMode(const VideoModeParams& _p, bool& bNewDeviceOut)
  -> std::string
{
	auto p = _p;
	Locator::getLogger()->warn("RageDisplay_D3D::TryVideoMode( {}, {}, {}, {}, {}, {} )",
	  static_cast<int>(p.windowed),
	  p.width,
	  p.height,
	  p.bpp,
	  p.rate,
	  static_cast<int>(p.vsync));

	if (FindBackBufferType(p.windowed, p.bpp) ==
		D3DFMT_UNKNOWN) { // no possible back buffer formats
		return ssprintf("FindBackBufferType(%i,%i) failed",
						p.windowed,
						p.bpp); // failed to set mode
	}

	/* Set up and display the window before setting up D3D. If we don't do this,
	 * then setting up a fullscreen window (when we're not coming from windowed)
	 * causes all other windows on the system to be resized to the new
	 * resolution. */
	GraphicsWindow::CreateGraphicsWindow(p);

	SetPresentParametersFromVideoModeParams(p, &m_PresentationParameters);

	// Display the window immediately, so we don't display the desktop ...
	while (true) {
		// Try the video mode.
		auto sErr = SetD3DParams(bNewDeviceOut);
		if (sErr.empty()) {
			break;
		}

		/* It failed. We're probably selecting a video mode that isn't
		 * supported. If we're fullscreen, search the mode list and find the
		 * nearest lower mode. */
		if (p.windowed || !D3DReduceParams(&m_PresentationParameters)) {
			return sErr;
		}

		// Store the new settings we're about to try.
		p.height = m_PresentationParameters.BackBufferHeight;
		p.width = m_PresentationParameters.BackBufferWidth;
		if (m_PresentationParameters.FullScreen_RefreshRateInHz == D3DPRESENT_RATE_DEFAULT) {
			p.rate = REFRESH_DEFAULT;
		} else {
			p.rate = m_PresentationParameters.FullScreen_RefreshRateInHz;
		}
	}

	/* Call this again after changing the display mode. If we're going to a
	 * window from fullscreen, the first call can't set a larger window than the
	 * old fullscreen resolution or set the window position. */
	GraphicsWindow::CreateGraphicsWindow(p);

	ResolutionChanged();

	// Present once the window is created so we don't display a white frame
	// while initializing
	m_Device->Present(nullptr, nullptr, nullptr, nullptr);

	// Ensure device is in a clean state when resolution changes occur
	RecoverFromDeviceLoss();

	return std::string(); // mode change successful
}

void
RageDisplay_D3D::ResolutionChanged()
{
	// LOG->Warn( "RageDisplay_D3D::ResolutionChanged" );

	RageDisplay::ResolutionChanged();
}

// Reset anything which doesn't survive device loss
void
RageDisplay_D3D::RecoverFromDeviceLoss()
{
	m_LastFVF = 0;
}

auto
RageDisplay_D3D::GetMaxTextureSize() const -> int
{
	return m_DeviceCaps.MaxTextureWidth;
}

auto
RageDisplay_D3D::BeginFrame() -> bool
{
	GraphicsWindow::Update();

	switch (m_Device->TestCooperativeLevel()) {
		case D3DERR_DEVICELOST:
			RecoverFromDeviceLoss();
			return false;
		case D3DERR_DEVICENOTRESET: {
			auto bIgnore = false;
			const auto sError = SetD3DParams(bIgnore);
			if (!sError.empty()) {
				RageException::Throw(sError.c_str());
			}

			break;
		}
	}

	m_Device->Clear(0,
						nullptr,
						D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
						D3DCOLOR_XRGB(0, 0, 0),
						1.0F,
						0x00000000);

	m_Device->BeginScene();

	return RageDisplay::BeginFrame();
}

void
RageDisplay_D3D::EndFrame()
{
	m_Device->EndScene();

	FrameLimitBeforeVsync();

	const auto beforePresent = std::chrono::steady_clock::now();
	m_Device->Present(nullptr, nullptr, nullptr, nullptr);

	const auto afterPresent = std::chrono::steady_clock::now();
	SetPresentTime(afterPresent - beforePresent);

	FrameLimitAfterVsync((*GetActualVideoModeParams()).rate);

	RageDisplay::EndFrame();
}

auto
RageDisplay_D3D::SupportsTextureFormat(RagePixelFormat pixfmt,
									   bool /*realtime*/) -> bool
{
	// Some cards (Savage) don't support alpha in palettes.
	// Don't allow paletted textures if this is the case.
	if (pixfmt == RagePixelFormat_PAL &&
		((m_DeviceCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE) == 0u)) {
		return false;
	}

	if (D3DFORMATS[pixfmt] == D3DFMT_UNKNOWN) {
		return false;
	}

	const auto d3dfmt = D3DFORMATS[pixfmt];
	const auto hr = m_D3D->CheckDeviceFormat(D3DADAPTER_DEFAULT,
											  D3DDEVTYPE_HAL,
											  m_PresentationParameters.BackBufferFormat,
											  0,
											  D3DRTYPE_TEXTURE,
											  d3dfmt);

	return SUCCEEDED(hr);
}

auto
RageDisplay_D3D::SupportsThreadedRendering() -> bool
{
	return true;
}

auto
RageDisplay_D3D::CreateScreenshot() -> RageSurface*
{
	RageSurface* result = nullptr;

	// Get the back buffer.
	IDirect3DSurface9* pSurface;
	if (SUCCEEDED(m_Device->GetBackBuffer(
		  0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface))) {
		// Get the back buffer description.
		D3DSURFACE_DESC desc;
		pSurface->GetDesc(&desc);

		// Copy the back buffer into a surface of a type we support.
		IDirect3DSurface9* pCopy;
		if (SUCCEEDED(m_Device->CreateOffscreenPlainSurface(desc.Width,
																desc.Height,
																D3DFMT_A8R8G8B8,
																D3DPOOL_SCRATCH,
																&pCopy,
																nullptr))) {
			if (SUCCEEDED(D3DXLoadSurfaceFromSurface(pCopy,
													 nullptr,
													 nullptr,
													 pSurface,
													 nullptr,
													 nullptr,
													 D3DX_FILTER_NONE,
													 0))) {
				// Update desc from the copy.
				pCopy->GetDesc(&desc);

				D3DLOCKED_RECT lr;

				{
					RECT rect;
					rect.left = 0;
					rect.top = 0;
					rect.right = desc.Width;
					rect.bottom = desc.Height;

					pCopy->LockRect(&lr, &rect, D3DLOCK_READONLY);
				}

				auto* surface = CreateSurfaceFromPixfmt(RagePixelFormat_RGBA8,
														lr.pBits,
														desc.Width,
														desc.Height,
														lr.Pitch);
				ASSERT(surface != nullptr);

				// We need to make a copy, since lr.pBits will go away when we
				// call UnlockRect().
				result = CreateSurface(surface->w,
									   surface->h,
									   surface->fmt.BitsPerPixel,
									   surface->fmt.Rmask,
									   surface->fmt.Gmask,
									   surface->fmt.Bmask,
									   surface->fmt.Amask);
				RageSurfaceUtils::CopySurface(surface, result);
				delete surface;

				pCopy->UnlockRect();
			}

			pCopy->Release();
		}

		pSurface->Release();
	}

	return result;
}

auto
RageDisplay_D3D::GetActualVideoModeParams() const
  -> const ActualVideoModeParams*
{
	return static_cast<ActualVideoModeParams*>(GraphicsWindow::GetParams());
}

void
RageDisplay_D3D::SendCurrentMatrices()
{
	static RageMatrix Centering;
	static RageMatrix Projection;

	if (Centering != *GetCentering() || Projection != *GetProjectionTop()) {
		Centering = *GetCentering();
		Projection = *GetProjectionTop();

		RageMatrix m;
		RageMatrixMultiply(&m, GetCentering(), GetProjectionTop());

		if (m_bInvertY) {
			RageMatrix flip;
			RageMatrixScale(&flip, +1, -1, +1);
			RageMatrixMultiply(&m, &flip, &m);
		}

		// Convert to OpenGL-style "pixel-centered" coords
		auto m2 = GetCenteringMatrix(-0.5F, -0.5F, 0, 0);
		RageMatrix projection;
		RageMatrixMultiply(&projection, &m2, &m);
		m_Device->SetTransform(D3DTS_PROJECTION,
								   reinterpret_cast<D3DMATRIX*>(&projection));

		m_Device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)GetViewTop());
		m_Device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)GetWorldTop());

		for (size_t tu = 0; tu < TextureUnitCount; tu++)
		{
			// If no texture is set for this texture unit, don't bother setting
			// it up.
			IDirect3DBaseTexture9* pTexture = nullptr;
			m_Device->GetTexture(tu, &pTexture);
			if (pTexture == nullptr) {
				continue;
			}
			pTexture->Release();

			// Optimization opportunity: Turn off texture transform if not using
			// texture coords.
			m_Device->SetTextureStageState(
			  tu, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

			if (m_bSphereMapping[tu]) {
				static const auto tex = RageMatrix(0.5F,
												   0.0F,
												   0.0F,
												   0.0F,
												   0.0F,
												   -0.5F,
												   0.0F,
												   0.0F,
												   0.0F,
												   0.0F,
												   0.0F,
												   0.0F,
												   0.5F,
												   -0.5F,
												   0.0F,
												   1.0F);
				m_Device->SetTransform(
				  static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + tu),
				  (D3DMATRIX*)&tex);

				// Tell D3D to use transformed reflection vectors as texture
				// co-ordinate 0 and then transform this coordinate by the
				// specified texture matrix.
				m_Device->SetTextureStageState(
				  tu,
				  D3DTSS_TEXCOORDINDEX,
				  D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
			} else {
				/* Direct3D is expecting a 3x3 matrix loaded into the 4x4 in
				 * order to transform the 2-component texture coordinates. We
				 * currently only use translate and scale, and ignore the z
				 * component entirely, so convert the texture matrix from 4x4 to
				 * 3x3 by dropping z. */

				const auto& tex1 = *GetTextureTop();
				const auto tex2 = RageMatrix(tex1.m[0][0],
											 tex1.m[0][1],
											 tex1.m[0][3],
											 0,
											 tex1.m[1][0],
											 tex1.m[1][1],
											 tex1.m[1][3],
											 0,
											 tex1.m[3][0],
											 tex1.m[3][1],
											 tex1.m[3][3],
											 0,
											 0,
											 0,
											 0,
											 0);
				m_Device->SetTransform(
				  D3DTRANSFORMSTATETYPE(D3DTS_TEXTURE0 + tu),
				  (D3DMATRIX*)&tex2);

				m_Device->SetTextureStageState(
				  tu, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
			}
		 }
	}
}

auto
RageDisplay_D3D::CreateCompiledGeometry() -> RageCompiledGeometry*
{
	return new RageCompiledGeometrySWD3D;
}

void
RageDisplay_D3D::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
	delete p;
}

void
RageDisplay_D3D::SetShader(const RageShaderWeakRef& shader)
{
	if (shader.GetShader() == nullptr ||
		shader.GetShaderType() == RageShaderType::Invalid ||
		shader.GetDisplayType() != RageDisplayType::D3D) {
		return;
	}

	if (shader.GetShaderType() == RageShaderType::Vertex) {
		m_VertexShaderHandler->TrySetActiveShader(shader.GetShader());
		return;
	}

	m_PixelShaderHandler->TrySetActiveShader(shader.GetShader());
}

RageShaderWeakRef RageDisplay_D3D::CreateShaderFromPath(const std::string& path,
					 RageShaderType shaderType, bool useAsDefault)
{
	auto resolvedPath = FILEMAN->ResolvePath(path);
	resolvedPath = resolvedPath.substr(1); // for some reason the / at the start screws with D3D9
	switch (shaderType) {
		case RageShaderType::Vertex:
			return m_VertexShaderHandler->GetOrCreateShaderFromPath(resolvedPath,
															  useAsDefault);
		case RageShaderType::Fragment:
			return m_PixelShaderHandler->GetOrCreateShaderFromPath(resolvedPath,
															  useAsDefault);
		default:
			return RageShaderWeakRef();
	}
}

void
RageDisplay_D3D::SetShadersForDeclaration(bool useSpriteDeclaration)
{
	IDirect3DVertexDeclaration9* vertexDecl = useSpriteDeclaration
												? m_SpriteVertexDeclaration
												 : m_ModelVertexDeclaration;

	RageVertexShader_D3D* vertexShader =
	  reinterpret_cast<RageVertexShader_D3D*>(m_VertexShaderHandler->GetCurrentShader());

	RagePixelShader_D3D* pixelShader =
	  reinterpret_cast<RagePixelShader_D3D*>(
		m_PixelShaderHandler->GetCurrentShader());

	HRESULT hr = S_OK;

	if (vertexDecl != m_PreviousVertexDecl) {
		m_PreviousVertexDecl = vertexDecl;

		hr = m_Device->SetVertexDeclaration(vertexDecl);
		if (FAILED(hr)) {
			Locator::getLogger()->warn(
			  "SetVertexDeclaration failed ({})",
			  RageDisplay_D3D_Helpers::GetErrorString(hr));
		}
	}

	if (vertexShader != m_PreviousVertexShader) {
		m_PreviousVertexShader = vertexShader;

		hr = m_Device->SetVertexShader(
		  vertexShader->GetShaderForDevice(m_Device, false));
		if (FAILED(hr)) {
			Locator::getLogger()->warn(
			  "SetVertexShader failed ({})",
			  RageDisplay_D3D_Helpers::GetErrorString(hr));
		}
	}

	if (pixelShader != m_PreviousPixelShader) {
		m_PreviousPixelShader = pixelShader;

		hr = m_Device->SetPixelShader(
		  pixelShader->GetShaderForDevice(m_Device, false));
		if (FAILED(hr)) {
			Locator::getLogger()->warn(
			  "SetPixelShader failed ({})",
			  RageDisplay_D3D_Helpers::GetErrorString(hr));
		}
	}
}

std::vector<std::string>
RageDisplay_D3D::GetSupportedShaderProfiles()
{
	return { D3DXGetVertexShaderProfile(m_Device),
			 D3DXGetPixelShaderProfile(m_Device) };
}

bool
RageDisplay_D3D::IsShaderInCache(const RageShaderWeakRef& shader)
{
	if (shader.GetDisplayType() == RageDisplayType::Invalid ||
		shader.GetShaderType() == RageShaderType::Invalid ||
		shader.GetShader() == nullptr) {
		return false;
	}

	if (shader.GetShaderType() == RageShaderType::Fragment) {
		if (!m_PixelShaderHandler.has_value()) {
			return false;
		}

		return shader.IsDefault()
				 ? m_PixelShaderHandler->IsDefaultShaderInCache(
					 shader.GetLookupKey())
				 : m_PixelShaderHandler->IsShaderInCache(shader.GetLookupKey());
	}

	if (!m_VertexShaderHandler.has_value()) {
		return false;
	}

	return shader.IsDefault()
			 ? m_VertexShaderHandler->IsDefaultShaderInCache(
				 shader.GetLookupKey())
			 : m_VertexShaderHandler->IsShaderInCache(shader.GetLookupKey());
}

void
RageDisplay_D3D::DrawQuadsInternal(const RageSpriteDrawing& drawing)
{
	auto [v, iNumVerts] = GetDrawRange(drawing);

	// there isn't a quad primitive in D3D, so we have to fake it with indexed
	// triangles
	const auto iNumQuads = iNumVerts / 4;
	const auto iNumTriangles = iNumQuads * 2;
	const auto iNumIndices = iNumTriangles * 3;

	// make a temporary index buffer
	static std::vector<int> vIndices;
	const int iOldSize = vIndices.size();
	const auto uNewSize = std::max(iOldSize, iNumIndices);
	vIndices.resize(uNewSize);
	for (auto i = iOldSize / 6; i < iNumQuads; i++) {
		vIndices[i * 6 + 0] = i * 4 + 0;
		vIndices[i * 6 + 1] = i * 4 + 1;
		vIndices[i * 6 + 2] = i * 4 + 2;
		vIndices[i * 6 + 3] = i * 4 + 2;
		vIndices[i * 6 + 4] = i * 4 + 3;
		vIndices[i * 6 + 5] = i * 4 + 0;
	}

	PrepareForDrawingPrimitives(true, drawing.textureUnitIndex);

	auto result = m_Device->DrawIndexedPrimitiveUP(
	  D3DPT_TRIANGLELIST,
	  // PrimitiveType
	  0,
	  // MinIndex
	  iNumVerts,
	  // NumVertices
	  iNumTriangles,
	  // PrimitiveCount,
	  &vIndices[0],
	  // pIndexData,
	  D3DFMT_INDEX32,
	  // IndexDataFormat,
	  v,
	  // pVertexStreamZeroData,
	  sizeof(RageSpriteVertex) // VertexStreamZeroStride
	);
}

D3DXMATRIX*
RageDisplay_D3D::GetWorldViewProjectionMatrix()
{
	// TODO: this is probably very slow
	static D3DXMATRIX World, View, Proj, WVP;
	static D3DXMATRIX currentWorld, currentView, currentProj;

	m_Device->GetTransform(D3DTS_PROJECTION, &currentProj);
	m_Device->GetTransform(D3DTS_VIEW, &currentView);
	m_Device->GetTransform(D3DTS_WORLD, &currentWorld);

	if (currentWorld != World || currentView != View || currentProj != Proj) {
		if (currentWorld != World)
			World = currentWorld;
		if (currentView != View)
			View = currentView;
		if (currentProj != Proj)
			Proj = currentProj;
		WVP = currentWorld * currentView * currentProj;
	}

	return &WVP;
}

void
RageDisplay_D3D::DrawQuadStripInternal(const RageSpriteDrawing& drawing)
{
	auto [v, iNumVerts] = GetDrawRange(drawing);

	// there isn't a quad strip primitive in D3D, so we have to fake it with
	// indexed triangles
	const auto iNumQuads = (iNumVerts - 2) / 2;
	const auto iNumTriangles = iNumQuads * 2;
	const auto iNumIndices = iNumTriangles * 3;

	// make a temporary index buffer
	static std::vector<int> vIndices;
	const int iOldSize = vIndices.size();
	const auto iNewSize = std::max(iOldSize, iNumIndices);
	vIndices.resize(iNewSize);
	for (auto i = iOldSize / 6; i < iNumQuads; i++) {
		vIndices[i * 6 + 0] = i * 2 + 0;
		vIndices[i * 6 + 1] = i * 2 + 1;
		vIndices[i * 6 + 2] = i * 2 + 2;
		vIndices[i * 6 + 3] = i * 2 + 1;
		vIndices[i * 6 + 4] = i * 2 + 2;
		vIndices[i * 6 + 5] = i * 2 + 3;
	}

	PrepareForDrawingPrimitives(true, drawing.textureUnitIndex);

	m_Device->DrawIndexedPrimitiveUP(
	  D3DPT_TRIANGLELIST,
	  // PrimitiveType
	  0,
	  // MinIndex
	  iNumVerts,
	  // NumVertices
	  iNumTriangles,
	  // PrimitiveCount,
	  &vIndices[0],
	  // pIndexData,
	  D3DFMT_INDEX32,
	  // IndexDataFormat,
	  v,
	  // pVertexStreamZeroData,
	  sizeof(RageSpriteVertex) // VertexStreamZeroStride
	);
}

void
RageDisplay_D3D::DrawSymmetricQuadStripInternal(
  const RageSpriteDrawing& drawing)
{
	auto [v, iNumVerts] = GetDrawRange(drawing);

	const auto iNumPieces = (iNumVerts - 3) / 3;
	const auto iNumTriangles = iNumPieces * 4;
	const auto iNumIndices = iNumTriangles * 3;

	// make a temporary index buffer
	static std::vector<int> vIndices;
	const int iOldSize = vIndices.size();
	const auto iNewSize = std::max(iOldSize, iNumIndices);
	vIndices.resize(iNewSize);
	for (auto i = iOldSize / 12; i < iNumPieces; i++) {
		// { 1, 3, 0 } { 1, 4, 3 } { 1, 5, 4 } { 1, 2, 5 }
		vIndices[i * 12 + 0] = i * 3 + 1;
		vIndices[i * 12 + 1] = i * 3 + 3;
		vIndices[i * 12 + 2] = i * 3 + 0;
		vIndices[i * 12 + 3] = i * 3 + 1;
		vIndices[i * 12 + 4] = i * 3 + 4;
		vIndices[i * 12 + 5] = i * 3 + 3;
		vIndices[i * 12 + 6] = i * 3 + 1;
		vIndices[i * 12 + 7] = i * 3 + 5;
		vIndices[i * 12 + 8] = i * 3 + 4;
		vIndices[i * 12 + 9] = i * 3 + 1;
		vIndices[i * 12 + 10] = i * 3 + 2;
		vIndices[i * 12 + 11] = i * 3 + 5;
	}

	PrepareForDrawingPrimitives(true, drawing.textureUnitIndex);

	m_Device->DrawIndexedPrimitiveUP(
	  D3DPT_TRIANGLELIST,
	  // PrimitiveType
	  0,
	  // MinIndex
	  iNumVerts,
	  // NumVertices
	  iNumTriangles,
	  // PrimitiveCount,
	  &vIndices[0],
	  // pIndexData,
	  D3DFMT_INDEX32,
	  // IndexDataFormat,
	  v,
	  // pVertexStreamZeroData,
	  sizeof(RageSpriteVertex) // VertexStreamZeroStride
	);
}

void
RageDisplay_D3D::DrawFanInternal(const RageSpriteDrawing& drawing)
{
	PrepareForDrawingPrimitives(true, drawing.textureUnitIndex);

	auto [v, iNumVerts] = GetDrawRange(drawing);

	m_Device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,
								  // PrimitiveType
								  iNumVerts - 2,
								  // PrimitiveCount,
								  v,
								  // pVertexStreamZeroData,
								  sizeof(RageSpriteVertex));
}

void
RageDisplay_D3D::DrawStripInternal(const RageSpriteDrawing& drawing)
{
	PrepareForDrawingPrimitives(true, drawing.textureUnitIndex);

	auto [v, iNumVerts] = GetDrawRange(drawing);

	m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,
								  // PrimitiveType
								  iNumVerts - 2,
								  // PrimitiveCount,
								  v,
								  // pVertexStreamZeroData,
								  sizeof(RageSpriteVertex));
}

void
RageDisplay_D3D::DrawTrianglesInternal(const RageSpriteDrawing& drawing)
{
	PrepareForDrawingPrimitives(true, drawing.textureUnitIndex);

	auto [ v, iNumVerts ] = GetDrawRange(drawing);

	m_Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
								  // PrimitiveType
								  iNumVerts / 3,
								  // PrimitiveCount,
								  v,
								  // pVertexStreamZeroData,
								  sizeof(RageSpriteVertex));
}

void
RageDisplay_D3D::DrawCompiledGeometryInternal(const RageCompiledGeometry* p,
											  int iMeshIndex)
{
	/* If lighting is off, then the current material will have no effect. We
	 * want to still be able to color models with lighting off, so shove the
	 * material color in texture factor and modify the texture stage to use it
	 * instead of the vertex color (our models don't have vertex coloring
	 * anyway). */
	DWORD bLighting;
	m_Device->GetRenderState(D3DRS_LIGHTING, &bLighting);

	if (bLighting == 0u) {
		m_Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		m_Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	}

	PrepareForDrawingPrimitives(false, TextureUnit_1);
	p->Draw(iMeshIndex);

	if (bLighting == 0u) {
		m_Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		m_Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
}

void
RageDisplay_D3D::ClearAllTextures()
{
	for (size_t tu = 0; tu < TextureUnitCount; tu++) {
		SetTexture(static_cast<TextureUnit>(tu), 0);
	}
}

auto
RageDisplay_D3D::GetNumTextureUnits() -> int
{
	return std::min(TextureUnitCount,
					static_cast<size_t>(m_DeviceCaps.MaxSimultaneousTextures));
}

void
RageDisplay_D3D::SetTexture(TextureUnit tu, intptr_t iTexture)
{
	//	m_DeviceCaps.MaxSimultaneousTextures = 1;
	if (tu >= static_cast<int>(m_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	if (iTexture == 0) {
		m_Device->SetTexture(tu, nullptr);

		/* Intentionally commented out. Don't mess with texture stage state
		 * when just setting the texture. Model sets its texture modes before
		 * setting the final texture. */
		// m_Device->SetTextureStageState( tu, D3DTSS_COLOROP,
		// D3DTOP_DISABLE );
	} else {
		auto* pTex = reinterpret_cast<IDirect3DTexture9*>(iTexture);
		m_Device->SetTexture(tu, pTex);

		/* Intentionally commented out. Don't mess with texture stage state
		 * when just setting the texture. Model sets its texture modes before
		 * setting the final texture. */
		// m_Device->SetTextureStageState( tu, D3DTSS_COLOROP,
		// D3DTOP_MODULATE );

		// Set palette (if any)
		SetPalette(iTexture);
	}
}

void
RageDisplay_D3D::SetTextureMode(TextureUnit tu, TextureMode tm)
{
	if (tu >= static_cast<int>(m_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	switch (tm) {
		case TextureMode_Modulate:
			// Use D3DTA_CURRENT instead of diffuse so that multitexturing works
			// properly.  For stage 0, D3DTA_CURRENT is the diffuse color.

			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLORARG2, D3DTA_CURRENT);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLOROP, D3DTOP_MODULATE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		case TextureMode_Add:
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLORARG2, D3DTA_CURRENT);
			m_Device->SetTextureStageState(tu, D3DTSS_COLOROP, D3DTOP_ADD);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		case TextureMode_Glow:
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLORARG2, D3DTA_CURRENT);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			m_Device->SetTextureStageState(
			  tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		default:
			Locator::getLogger()->warn("RageDisplay_D3D::SetTextureMode called with invalid TextureMode");
	}
}

void
RageDisplay_D3D::SetTextureFiltering(TextureUnit tu, bool b)
{
	if (tu >= static_cast<int>(m_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	m_Device->SetSamplerState(
	  tu, D3DSAMP_MINFILTER, b ? D3DTEXF_LINEAR : D3DTEXF_POINT);
	m_Device->SetSamplerState(
	  tu, D3DSAMP_MAGFILTER, b ? D3DTEXF_LINEAR : D3DTEXF_POINT);
}

void
RageDisplay_D3D::SetBlendMode(BlendMode mode)
{
	m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

	if (mode == BLEND_INVERT_DEST) {
		m_Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
	} else {
		m_Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	}

	switch (mode) {
		case BLEND_NORMAL:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		case BLEND_ADD:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
			// This is not the right way to do BLEND_SUBTRACT.  This code is
			// only here to prevent crashing when someone tries to use it. -Kyz
		case BLEND_SUBTRACT:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			break;
		case BLEND_MODULATE:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		case BLEND_COPY_SRC:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			break;
			/* Effects currently missing in D3D: BLEND_ALPHA_MASK,
			 * BLEND_ALPHA_KNOCK_OUT These two may require DirectX9 since
			 * D3DRS_SRCALPHA and D3DRS_DESTALPHA don't seem to exist in DX8.
			 * -aj */
		case BLEND_ALPHA_MASK:
			// RGB: iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			// Alpha: iSourceAlpha = GL_ZERO; iDestAlpha = GL_SRC_ALPHA;

			m_Device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
			m_Device->SetRenderState(D3DRS_DESTBLENDALPHA,
										 D3DBLEND_SRCALPHA);

			break;
		case BLEND_ALPHA_KNOCK_OUT:
			// RGB: iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			// Alpha: iSourceAlpha = GL_ZERO; iDestAlpha =
			// GL_ONE_MINUS_SRC_ALPHA;

			m_Device->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
			m_Device->SetRenderState(D3DRS_DESTBLENDALPHA,
										 D3DBLEND_INVSRCALPHA);

			break;
		case BLEND_ALPHA_MULTIPLY:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			break;
		case BLEND_WEIGHTED_MULTIPLY:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		case BLEND_INVERT_DEST:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		case BLEND_NO_EFFECT:
			m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		default:
			FAIL_M(ssprintf("Invalid BlendMode: %i", mode));
	}
}

auto
RageDisplay_D3D::IsZWriteEnabled() const -> bool
{
	DWORD b;
	m_Device->GetRenderState(D3DRS_ZWRITEENABLE, &b);
	return b != 0;
}

void
RageDisplay_D3D::SetZBias(float f)
{
	D3DVIEWPORT9 viewData;
	m_Device->GetViewport(&viewData);
	viewData.MinZ = SCALE(f, 0.0F, 1.0F, 0.05F, 0.0F);
	viewData.MaxZ = SCALE(f, 0.0F, 1.0F, 1.0F, 0.95F);
	m_Device->SetViewport(&viewData);
}

auto
RageDisplay_D3D::IsZTestEnabled() const -> bool
{
	DWORD b;
	m_Device->GetRenderState(D3DRS_ZFUNC, &b);
	return b != D3DCMP_ALWAYS;
}

void
RageDisplay_D3D::SetZWrite(bool b)
{
	m_Device->SetRenderState(D3DRS_ZWRITEENABLE, static_cast<DWORD>(b));
}

void
RageDisplay_D3D::SetZTestMode(ZTestMode mode)
{
	m_Device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	DWORD dw;
	switch (mode) {
		case ZTEST_OFF:
			dw = D3DCMP_ALWAYS;
			break;
		case ZTEST_WRITE_ON_PASS:
			dw = D3DCMP_LESSEQUAL;
			break;
		case ZTEST_WRITE_ON_FAIL:
			dw = D3DCMP_GREATER;
			break;
		default:
			dw = D3DCMP_NEVER;
			FAIL_M(ssprintf("Invalid ZTestMode: %i", mode));
	}
	m_Device->SetRenderState(D3DRS_ZFUNC, dw);
}

void
RageDisplay_D3D::ClearZBuffer()
{
	m_Device->Clear(
	  0, nullptr, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0F, 0x00000000);
}

void
RageDisplay_D3D::SetTextureWrapping(TextureUnit tu, bool b)
{
	if (tu >= static_cast<int>(m_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	const int mode = b ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
	m_Device->SetSamplerState(tu, D3DSAMP_ADDRESSU, mode);
	m_Device->SetSamplerState(tu, D3DSAMP_ADDRESSV, mode);
}

void
RageDisplay_D3D::SetMaterial(const RageColor& emissive,
							 const RageColor& ambient,
							 const RageColor& diffuse,
							 const RageColor& specular,
							 float shininess)
{
	/* If lighting is off, then the current material will have no effect.
	 * We want to still be able to color models with lighting off, so shove the
	 * material color in texture factor and modify the texture stage to use it
	 * instead of the vertex color (our models don't have vertex coloring
	 * anyway). */
	DWORD bLighting;
	m_Device->GetRenderState(D3DRS_LIGHTING, &bLighting);

	if (bLighting != 0u) {
		D3DMATERIAL9 mat;
		memcpy(&mat.Diffuse, diffuse, sizeof(float) * 4);
		memcpy(&mat.Ambient, ambient, sizeof(float) * 4);
		memcpy(&mat.Specular, specular, sizeof(float) * 4);
		memcpy(&mat.Emissive, emissive, sizeof(float) * 4);
		mat.Power = shininess;
		m_Device->SetMaterial(&mat);
	} else {
		auto c = diffuse;
		c.r += emissive.r + ambient.r;
		c.g += emissive.g + ambient.g;
		c.b += emissive.b + ambient.b;
		RageVColor c2 = c;
		const auto c3 = *reinterpret_cast<DWORD*>(&c2);
		m_Device->SetRenderState(D3DRS_TEXTUREFACTOR, c3);
	}
}

void
RageDisplay_D3D::SetLighting(bool b)
{
	m_Device->SetRenderState(D3DRS_LIGHTING, static_cast<DWORD>(b));
}

void
RageDisplay_D3D::SetLightOff(int index)
{
	m_Device->LightEnable(index, 0);
}

void
RageDisplay_D3D::SetLightDirectional(int index,
									 const RageColor& ambient,
									 const RageColor& diffuse,
									 const RageColor& specular,
									 const RageVector3& dir)
{
	m_Device->LightEnable(index, 1);

	D3DLIGHT9 light;
	ZERO(light);
	light.Type = D3DLIGHT_DIRECTIONAL;

	/* Z for lighting is flipped for D3D compared to OpenGL.
	 * XXX: figure out exactly why this is needed. Our transforms are probably
	 * goofed up, but the Z test is the same for both API's, so I'm not sure
	 * why we don't see other weirdness. -Chris */
	float position[] = { dir.x, dir.y, -dir.z };
	memcpy(&light.Direction, position, sizeof(position));
	memcpy(&light.Diffuse, diffuse, sizeof(diffuse));
	memcpy(&light.Ambient, ambient, sizeof(ambient));
	memcpy(&light.Specular, specular, sizeof(specular));

	// Same as OpenGL defaults.  Not used in directional lights.
	//	light.Attenuation0 = 1;
	//	light.Attenuation1 = 0;
	//	light.Attenuation2 = 0;

	m_Device->SetLight(index, &light);
}

void
RageDisplay_D3D::SetCullMode(CullMode mode)
{
	switch (mode) {
		case CULL_BACK:
			m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
			break;
		case CULL_FRONT:
			m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			break;
		case CULL_NONE:
			m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			break;
		default:
			FAIL_M(ssprintf("Invalid CullMode: %i", mode));
	}
}

void
RageDisplay_D3D::DeleteTexture(intptr_t iTexHandle)
{
	if (iTexHandle == 0) {
		return;
	}

	auto* pTex = reinterpret_cast<IDirect3DTexture9*>(iTexHandle);
	pTex->Release();

	// Delete render target (if any)
	if (m_mapRenderTargets.find(iTexHandle) != m_mapRenderTargets.end()) {
		delete m_mapRenderTargets[iTexHandle];
		m_mapRenderTargets.erase(iTexHandle);
		return;
	}

	// Delete palette (if any)
	if (m_TexResourceToPaletteIndex.find(iTexHandle) !=
		m_TexResourceToPaletteIndex.end()) {
		m_TexResourceToPaletteIndex.erase(
		  m_TexResourceToPaletteIndex.find(iTexHandle));
	}
	if (m_TexResourceToTexturePalette.find(iTexHandle) !=
		m_TexResourceToTexturePalette.end()) {
		m_TexResourceToTexturePalette.erase(
		  m_TexResourceToTexturePalette.find(iTexHandle));
	}
}

auto
RageDisplay_D3D::CreateTexture(RagePixelFormat pixfmt,
							   RageSurface* img,
							   bool /*bGenerateMipMaps*/) -> intptr_t
{
	HRESULT hr;
	IDirect3DTexture9* pTex;
	hr = m_Device->CreateTexture(power_of_two(img->w),
									 power_of_two(img->h),
									 1,
									 0,
									 D3DFORMATS[pixfmt],
									 D3DPOOL_MANAGED,
									 &pTex,
									 nullptr);

	if (FAILED(hr)) {
		RageException::Throw("CreateTexture(%i,%i,%s) failed: %s",
							 img->w,
							 img->h,
							 RagePixelFormatToString(pixfmt).c_str(),
		  RageDisplay_D3D_Helpers::GetErrorString(hr).c_str());
	}

	const auto uTexHandle = reinterpret_cast<intptr_t>(pTex);

	if (pixfmt == RagePixelFormat_PAL) {
		// Save palette
		TexturePalette pal{};
		memset(pal.p, 0, sizeof(pal.p));
		for (auto i = 0; i < img->fmt.palette->ncolors; i++) {
			auto& c = img->fmt.palette->colors[i];
			pal.p[i].peRed = c.r;
			pal.p[i].peGreen = c.g;
			pal.p[i].peBlue = c.b;
			pal.p[i].peFlags = c.a;
		}

		ASSERT(m_TexResourceToTexturePalette.find(uTexHandle) ==
			   m_TexResourceToTexturePalette.end());
		m_TexResourceToTexturePalette[uTexHandle] = pal;
	}

	UpdateTexture(uTexHandle, img, 0, 0, img->w, img->h);

	return uTexHandle;
}

void
RageDisplay_D3D::UpdateTexture(intptr_t uTexHandle,
							   RageSurface* img,
							   int xoffset,
							   int yoffset,
							   int width,
							   int height)
{
	auto* pTex = reinterpret_cast<IDirect3DTexture9*>(uTexHandle);
	ASSERT(pTex != nullptr);

	RECT rect;
	rect.left = xoffset;
	rect.top = yoffset;
	rect.right = width - xoffset;
	rect.bottom = height - yoffset;

	D3DLOCKED_RECT lr;
	pTex->LockRect(0, &lr, &rect, 0);

	D3DSURFACE_DESC desc;
	pTex->GetLevelDesc(0, &desc);
	ASSERT(xoffset + width <= static_cast<int>(desc.Width));
	ASSERT(yoffset + height <= static_cast<int>(desc.Height));

	// Copy bits
	int texpixfmt;
	for (texpixfmt = 0; texpixfmt < NUM_RagePixelFormat; ++texpixfmt) {
		if (D3DFORMATS[texpixfmt] == desc.Format) {
			break;
		}
	}
	ASSERT(texpixfmt != NUM_RagePixelFormat);

	auto* Texture = CreateSurfaceFromPixfmt(
	  RagePixelFormat(texpixfmt), lr.pBits, width, height, lr.Pitch);
	ASSERT(Texture != nullptr);
	RageSurfaceUtils::Blit(img, Texture, width, height);

	delete Texture;

	pTex->UnlockRect(0);
}

void
RageDisplay_D3D::SetAlphaTest(bool b)
{
	m_Device->SetRenderState(D3DRS_ALPHATESTENABLE, static_cast<DWORD>(b));
	m_Device->SetRenderState(D3DRS_ALPHAREF, 0);
	m_Device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
}

auto
RageDisplay_D3D::GetOrthoMatrix(float l,
								float r,
								float b,
								float t,
								float zn,
								float zf) -> RageMatrix
{
	auto m = RageDisplay::GetOrthoMatrix(l, r, b, t, zn, zf);

	// Convert from OpenGL's [-1,+1] Z values to D3D's [0,+1].
	RageMatrix tmp;
	RageMatrixScaling(&tmp, 1, 1, 0.5F);
	RageMatrixMultiply(&m, &tmp, &m);

	RageMatrixTranslation(&tmp, 0, 0, 0.5F);
	RageMatrixMultiply(&m, &tmp, &m);

	return m;
}

auto
RageDisplay_D3D::CreateRenderTarget(const RenderTargetParam& param,
									int& iTextureWidthOut,
									int& iTextureHeightOut) -> intptr_t
{
	auto* pTarget = new D3DRenderTarget_FramebufferObject(m_Device, m_PresentationParameters);

	pTarget->Create(param, iTextureWidthOut, iTextureHeightOut);

	const auto uTexture = pTarget->GetTexture();

	ASSERT(m_mapRenderTargets.find(uTexture) == m_mapRenderTargets.end());
	m_mapRenderTargets[uTexture] = pTarget;

	return uTexture;
}

auto
RageDisplay_D3D::GetRenderTarget() -> intptr_t
{
	for (const auto& g_mapRenderTarget : m_mapRenderTargets) {
		if (g_mapRenderTarget.second == m_pCurrentRenderTarget) {
			return g_mapRenderTarget.first;
		}
	}
	return 0;
}

void
RageDisplay_D3D::SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture)
{
	if (uTexHandle == 0) {
		m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		/* Pop matrixes affected by SetDefaultRenderStates. */
		DISPLAY->CameraPopMatrix();

		/* Reset the viewport. */
		D3DVIEWPORT9 viewData;
		m_Device->GetViewport(&viewData);
		viewData.Width = GetActualVideoModeParams()->width;
		viewData.Height = GetActualVideoModeParams()->height;
		m_Device->SetViewport(&viewData);

		if (m_pCurrentRenderTarget != nullptr) {
			m_pCurrentRenderTarget->FinishRenderingTo();
		}
		m_pCurrentRenderTarget = nullptr;
		m_Device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, 0u);
		return;
	}

	/* If we already had a render target, disable it. */
	if (m_pCurrentRenderTarget != nullptr) {
		SetRenderTarget(0, true);
	}

	/* Enable the new render target. */
	ASSERT(m_mapRenderTargets.find(uTexHandle) != m_mapRenderTargets.end());
	auto* pTarget = m_mapRenderTargets[uTexHandle];
	pTarget->StartRenderingTo();
	m_pCurrentRenderTarget = pTarget;

	/* Set the viewport to the size of the render target. */
	D3DVIEWPORT9 viewData;
	m_Device->GetViewport(&viewData);
	viewData.Width = pTarget->GetParam().iWidth;
	viewData.Height = pTarget->GetParam().iHeight;
	m_Device->SetViewport(&viewData);

	/* If this render target implementation flips Y, compensate.   Inverting
	 * will switch the winding order. */
	if (m_bInvertY) {
		m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	}

	/* The render target may be in a different D3D context, so re-send
	 * state.  Push matrixes affected by SetDefaultRenderStates. */
	DISPLAY->CameraPushMatrix();
	SetDefaultRenderStates();
	SetZWrite(true);

	// Need to blend the render targets together, not sure why OpenGL doesn't
	// need this -xwidghet
	m_Device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, 1u);

	/* If bPreserveTexture is false, clear the render target.  Only clear the
	 * depth buffer if the target has one; otherwise we're clearing the real
	 * depth buffer. */
	if (!bPreserveTexture) {
		const int iBit = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;

		/* Since we need the depth buffer to use render targets we can't give
		this option if (pTarget->GetParam().bWithDepthBuffer)
		{
			iBit |= D3DCLEAR_ZBUFFER;
		}*/

		if (FAILED(m_Device->Clear(0, nullptr, iBit, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0F, 0x00000000))) {
			Locator::getLogger()->warn("Failed to clear render target");
		}
	}
}

void
RageDisplay_D3D::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
	m_bSphereMapping[tu] = b;
}

void
RageDisplay_D3D::SetCelShaded(int stage)
{
	// todo: implement me!
}

auto
RageDisplay_D3D::IsD3DInternal() -> bool
{
	return true;
}

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
