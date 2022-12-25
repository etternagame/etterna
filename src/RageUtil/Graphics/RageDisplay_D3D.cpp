#include "Etterna/Globals/global.h"
#include "Etterna/Models/Misc/DisplaySpec.h"
#include "Etterna/Models/Misc/EnumHelper.h"
#include "Etterna/Models/Misc/LocalizedString.h"
#include "RageDisplay.h"
#include "RageDisplay_D3D.h"
#include "RageUtil/Misc/RageException.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Misc/RageMath.h"
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageUtil/Misc/RageTypes.h"
#include "RageUtil/Utils/RageUtil.h"
#include "archutils/Win32/GraphicsWindow.h"

#include <algorithm>
#include <map>
#include <list>
#include <chrono>


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnew-returns-null"
#pragma clang diagnostic ignored "-Wcomment"
#endif
#include <d3dx9tex.h>
#include <d3d9.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

// Static libraries
// load Windows D3D9 dynamically
#if defined(_MSC_VER)
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#endif


// The DXGetErrorStringW function comes from the DirectX Error Library  (See https://walbourn.github.io/wheres-dxerr-lib/ )
//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
const WCHAR* WINAPI DXGetErrorStringW( _In_ HRESULT hr )
{
#define  CHK_ERRA(hrchk) \
        case hrchk: \
             return L## #hrchk;

#define HRESULT_FROM_WIN32b(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)))

#define  CHK_ERR_WIN32A(hrchk) \
        case HRESULT_FROM_WIN32b(hrchk): \
        case hrchk: \
             return L## #hrchk;

   switch(hr)
   {
// Common Win32 error codes
        CHK_ERRA(S_OK)
        CHK_ERRA(S_FALSE)

// d3d9.h error codes
//      CHK_ERRA(D3D_OK)
        CHK_ERRA(D3DERR_WRONGTEXTUREFORMAT)
        CHK_ERRA(D3DERR_UNSUPPORTEDCOLOROPERATION)
        CHK_ERRA(D3DERR_UNSUPPORTEDCOLORARG)
        CHK_ERRA(D3DERR_UNSUPPORTEDALPHAOPERATION)
        CHK_ERRA(D3DERR_UNSUPPORTEDALPHAARG)
        CHK_ERRA(D3DERR_TOOMANYOPERATIONS)
        CHK_ERRA(D3DERR_CONFLICTINGTEXTUREFILTER)
        CHK_ERRA(D3DERR_UNSUPPORTEDFACTORVALUE)
        CHK_ERRA(D3DERR_CONFLICTINGRENDERSTATE)
        CHK_ERRA(D3DERR_UNSUPPORTEDTEXTUREFILTER)
        CHK_ERRA(D3DERR_CONFLICTINGTEXTUREPALETTE)
        CHK_ERRA(D3DERR_DRIVERINTERNALERROR)
        CHK_ERRA(D3DERR_NOTFOUND)
        CHK_ERRA(D3DERR_MOREDATA)
        CHK_ERRA(D3DERR_DEVICELOST)
        CHK_ERRA(D3DERR_DEVICENOTRESET)
        CHK_ERRA(D3DERR_NOTAVAILABLE)
        CHK_ERRA(D3DERR_OUTOFVIDEOMEMORY)
        CHK_ERRA(D3DERR_INVALIDDEVICE)
        CHK_ERRA(D3DERR_INVALIDCALL)
        CHK_ERRA(D3DERR_DRIVERINVALIDCALL)
        //CHK_ERRA(D3DERR_WASSTILLDRAWING)
        CHK_ERRA(D3DOK_NOAUTOGEN)

	    // Extended for Windows Vista
	    CHK_ERRA(D3DERR_DEVICEREMOVED)
	    CHK_ERRA(S_NOT_RESIDENT)
	    CHK_ERRA(S_RESIDENT_IN_SHARED_MEMORY)
	    CHK_ERRA(S_PRESENT_MODE_CHANGED)
	    CHK_ERRA(S_PRESENT_OCCLUDED)
	    CHK_ERRA(D3DERR_DEVICEHUNG)

        // Extended for Windows 7
        CHK_ERRA(D3DERR_UNSUPPORTEDOVERLAY)
        CHK_ERRA(D3DERR_UNSUPPORTEDOVERLAYFORMAT)
        CHK_ERRA(D3DERR_CANNOTPROTECTCONTENT)
        CHK_ERRA(D3DERR_UNSUPPORTEDCRYPTO)
        CHK_ERRA(D3DERR_PRESENT_STATISTICS_DISJOINT)

// dxgi.h error codes
        CHK_ERRA(DXGI_STATUS_OCCLUDED)
        CHK_ERRA(DXGI_STATUS_CLIPPED)
        CHK_ERRA(DXGI_STATUS_NO_REDIRECTION)
        CHK_ERRA(DXGI_STATUS_NO_DESKTOP_ACCESS)
        CHK_ERRA(DXGI_STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE)
        CHK_ERRA(DXGI_STATUS_MODE_CHANGED)
        CHK_ERRA(DXGI_STATUS_MODE_CHANGE_IN_PROGRESS)
        CHK_ERRA(DXGI_ERROR_INVALID_CALL)
        CHK_ERRA(DXGI_ERROR_NOT_FOUND)
        CHK_ERRA(DXGI_ERROR_MORE_DATA)
        CHK_ERRA(DXGI_ERROR_UNSUPPORTED)
        CHK_ERRA(DXGI_ERROR_DEVICE_REMOVED)
        CHK_ERRA(DXGI_ERROR_DEVICE_HUNG)
        CHK_ERRA(DXGI_ERROR_DEVICE_RESET)
        CHK_ERRA(DXGI_ERROR_WAS_STILL_DRAWING)
        CHK_ERRA(DXGI_ERROR_FRAME_STATISTICS_DISJOINT)
        CHK_ERRA(DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE)
        CHK_ERRA(DXGI_ERROR_DRIVER_INTERNAL_ERROR)
        CHK_ERRA(DXGI_ERROR_NONEXCLUSIVE)
        CHK_ERRA(DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
        CHK_ERRA(DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED)
        CHK_ERRA(DXGI_ERROR_REMOTE_OUTOFMEMORY)

        default: return L"Unknown error.";
    }
}

auto GetErrorString(HRESULT hr) -> std::string
{
    const wchar_t* msg = DXGetErrorStringW(hr);
    if (msg)
        return WStringToString(std::wstring(msg));
    return "";
}

// Globals
HMODULE g_D3D9_Module = nullptr;
LPDIRECT3D9 g_pd3d = nullptr;
LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;
D3DCAPS9 g_DeviceCaps;
D3DDISPLAYMODE g_DesktopMode;
D3DPRESENT_PARAMETERS g_d3dpp;
int g_ModelMatrixCnt = 0;
DWORD g_lastFVF = 0;
static bool g_bSphereMapping[NUM_TextureUnit] = { false, false };

// Need default color and depth buffer to restore them after using render
// targets
IDirect3DSurface9* defaultColorBuffer = nullptr;
IDirect3DSurface9* defaultDepthBuffer = nullptr;

// TODO(Sam): Instead of defining this here, enumerate the possible formats and
// select whatever one we want to use. This format should be fine for the uses
// of this application though.
const D3DFORMAT g_DefaultAdapterFormat = D3DFMT_X8R8G8B8;

static std::map<intptr_t, RenderTarget*> g_mapRenderTargets;
static RenderTarget* g_pCurrentRenderTarget = nullptr;

static bool g_bInvertY = false;

/* Direct3D doesn't associate a palette with textures. Instead, we load a
 * palette into a slot. We need to keep track of which texture's palette is
 * stored in what slot. */
std::map<intptr_t, int> g_TexResourceToPaletteIndex;
std::list<int> g_PaletteIndex;

struct TexturePalette
{
	PALETTEENTRY p[256];
};

std::map<intptr_t, TexturePalette> g_TexResourceToTexturePalette;

// Load the palette, if any, for the given texture into a palette slot, and make
// it current.
static void
SetPalette(unsigned TexResource)
{
	// If the texture isn't paletted, we have nothing to do.
	if (g_TexResourceToTexturePalette.find(TexResource) ==
		g_TexResourceToTexturePalette.end()) {
		return;
	}

	// Is the palette already loaded?
	if (g_TexResourceToPaletteIndex.find(TexResource) ==
		g_TexResourceToPaletteIndex.end()) {
		// It's not. Grab the least recently used slot.
		const auto iPalIndex = g_PaletteIndex.front();

		// If any other texture is currently using this slot, mark that palette
		// unloaded.
		for (auto i = g_TexResourceToPaletteIndex.begin();
			 i != g_TexResourceToPaletteIndex.end();
			 ++i) {
			if (i->second != iPalIndex) {
				continue;
			}
			g_TexResourceToPaletteIndex.erase(i);
			break;
		}

		// Load it.
		auto& pal = g_TexResourceToTexturePalette[TexResource];
		g_pd3dDevice->SetPaletteEntries(iPalIndex, pal.p);

		g_TexResourceToPaletteIndex[TexResource] = iPalIndex;
	}

	const auto iPalIndex = g_TexResourceToPaletteIndex[TexResource];

	// Find this palette index in the least-recently-used queue and move it to
	// the end.
	for (auto i = g_PaletteIndex.begin(); i != g_PaletteIndex.end(); ++i) {
		if (*i != iPalIndex) {
			continue;
		}
		g_PaletteIndex.erase(i);
		g_PaletteIndex.push_back(iPalIndex);
		break;
	}

	g_pd3dDevice->SetCurrentTexturePalette(iPalIndex);
}

#define D3DFVF_RageSpriteVertex                                                \
	(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define D3DFVF_RageModelVertex (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

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

	g_pd3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (g_pd3d == nullptr) {
		Locator::getLogger()->fatal("Direct3DCreate9 failed");
		return D3D_NOT_INSTALLED.GetValue();
	}

	if (FAILED(g_pd3d->GetDeviceCaps(
		  D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &g_DeviceCaps))) {
		return HARDWARE_ACCELERATION_NOT_AVAILABLE.GetValue();
	}

	D3DADAPTER_IDENTIFIER9 identifier;
	g_pd3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier);

	Locator::getLogger()->info(
	  "Driver: {}\n"
	  "Description: {}\n"
	  "Max texture size: {}\n"
	  "Alpha in palette: {}\n",
	  identifier.Driver,
	  identifier.Description,
	  g_DeviceCaps.MaxTextureWidth,
	  (g_DeviceCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE) ? "yes" : "no");

	Locator::getLogger()->info("This display adaptor supports the following modes:");
	D3DDISPLAYMODE mode;

	const auto modeCount =
	  g_pd3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, g_DefaultAdapterFormat);

	for (UINT u = 0; u < modeCount; u++) {
		if (SUCCEEDED(g_pd3d->EnumAdapterModes(D3DADAPTER_DEFAULT, g_DefaultAdapterFormat, u, &mode))) {
			Locator::getLogger()->info("  {}x{} {}Hz, format {}", mode.Width, mode.Height, mode.RefreshRate, mode.Format);
		}
	}

	g_PaletteIndex.clear();
	for (auto i = 0; i < 256; ++i) {
		g_PaletteIndex.push_back(i);
	}

	// Save the original desktop format.
	g_pd3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &g_DesktopMode);

	/* Up until now, all we've done is set up g_pd3d and do some queries. Now,
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

	if (g_pd3dDevice != nullptr) {
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}

	if (g_pd3d != nullptr) {
		g_pd3d->Release();
		g_pd3d = nullptr;
	}

	/* Even after we call Release(), D3D may still affect our window. It seems
	 * to subclass the window, and never release it. Free the DLL after
	 * destroying the window. */
	if (g_D3D9_Module != nullptr) {
		FreeLibrary(g_D3D9_Module);
		g_D3D9_Module = nullptr;
	}
}

void
RageDisplay_D3D::GetDisplaySpecs(DisplaySpecs& out) const
{
	out.clear();
	const int iCnt =
	  g_pd3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, g_DefaultAdapterFormat);
	std::set<DisplayMode> modes;
	D3DDISPLAYMODE mode;

	for (auto i = 0; i < iCnt; ++i) {
		g_pd3d->EnumAdapterModes(
		  D3DADAPTER_DEFAULT, g_DefaultAdapterFormat, i, &mode);
		modes.insert(
		  { mode.Width, mode.Height, static_cast<double>(mode.RefreshRate) });
	}
	// Get the current display mode
	if (g_pd3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode) == D3D_OK) {
		D3DADAPTER_IDENTIFIER9 ID;
		g_pd3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &ID);
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
FindBackBufferType(bool bWindowed, int iBPP) -> D3DFORMAT
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
			fmtDisplay = g_DesktopMode.Format;
		} else { // Fullscreen
			fmtDisplay = vBackBufferFormat;
		}

		Locator::getLogger()->debug("Testing format: display {}, back buffer {}, windowed {}...",
				   fmtDisplay,
				   fmtBackBuffer,
				   static_cast<int>(bWindowed));

		hr = g_pd3d->CheckDeviceType(D3DADAPTER_DEFAULT,
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
SetD3DParams(bool& bNewDeviceOut) -> std::string
{
	// wipe old render targets
	for (auto& rt : g_mapRenderTargets) {
		delete rt.second;
	}
	g_mapRenderTargets.clear();

	if (g_pd3dDevice == nullptr)
	// device is not yet created. We need to create it
	{
		bNewDeviceOut = true;
		const auto hr =
		  g_pd3d->CreateDevice(D3DADAPTER_DEFAULT,
							   D3DDEVTYPE_HAL,
							   GraphicsWindow::GetHwnd(),
							   D3DCREATE_HARDWARE_VERTEXPROCESSING,
							   &g_d3dpp,
							   &g_pd3dDevice);
		if (FAILED(hr)) {
			// Likely D3D_ERR_INVALIDCALL.  The driver probably doesn't support
			// this video mode.
			return ssprintf("CreateDevice failed: '%s'",
							GetErrorString(hr).c_str());
		}
	} else {
		bNewDeviceOut = false;
		// LOG->Warn( "Resetting D3D device" );
		const auto hr = g_pd3dDevice->Reset(&g_d3dpp);
		if (FAILED(hr)) {
			// Likely D3D_ERR_INVALIDCALL.  The driver probably doesn't support
			// this video mode.
			return ssprintf("g_pd3dDevice->Reset failed: '%s'",
							GetErrorString(hr).c_str());
		}
	}

	g_pd3dDevice->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	// Palettes were lost by Reset(), so mark them unloaded.
	g_TexResourceToPaletteIndex.clear();

	return std::string();
}

// If the given parameters have failed, try to lower them.
static auto
D3DReduceParams(D3DPRESENT_PARAMETERS* pp) -> bool
{
	D3DDISPLAYMODE current;
	current.Format = pp->BackBufferFormat;
	current.Height = pp->BackBufferHeight;
	current.Width = pp->BackBufferWidth;
	current.RefreshRate = pp->FullScreen_RefreshRateInHz;

	const int iCnt =
	  g_pd3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, g_DefaultAdapterFormat);
	auto iBest = -1;
	auto iBestScore = 0;
	Locator::getLogger()->debug("cur: {}x{} {}Hz, format {}",
			   current.Width, current.Height,
			   current.RefreshRate, current.Format);
	for (auto i = 0; i < iCnt; ++i) {
		D3DDISPLAYMODE mode;
		g_pd3d->EnumAdapterModes(
		  D3DADAPTER_DEFAULT, g_DefaultAdapterFormat, i, &mode);

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
	g_pd3d->EnumAdapterModes(
	  D3DADAPTER_DEFAULT, g_DefaultAdapterFormat, iBest, &BestMode);
	pp->BackBufferHeight = BestMode.Height;
	pp->BackBufferWidth = BestMode.Width;
	pp->FullScreen_RefreshRateInHz = BestMode.RefreshRate;

	return true;
}

static void
SetPresentParametersFromVideoModeParams(const VideoModeParams& p,
										D3DPRESENT_PARAMETERS* pD3Dpp)
{
	ZERO(*pD3Dpp);
	const auto displayFormat = FindBackBufferType(p.windowed, p.bpp);
	auto enableMultiSampling = false;

	if (p.bSmoothLines &&
		SUCCEEDED(g_pd3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
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

	SetPresentParametersFromVideoModeParams(p, &g_d3dpp);

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
		if (p.windowed || !D3DReduceParams(&g_d3dpp)) {
			return sErr;
		}

		// Store the new settings we're about to try.
		p.height = g_d3dpp.BackBufferHeight;
		p.width = g_d3dpp.BackBufferWidth;
		if (g_d3dpp.FullScreen_RefreshRateInHz == D3DPRESENT_RATE_DEFAULT) {
			p.rate = REFRESH_DEFAULT;
		} else {
			p.rate = g_d3dpp.FullScreen_RefreshRateInHz;
		}
	}

	/* Call this again after changing the display mode. If we're going to a
	 * window from fullscreen, the first call can't set a larger window than the
	 * old fullscreen resolution or set the window position. */
	GraphicsWindow::CreateGraphicsWindow(p);

	ResolutionChanged();

	// Present once the window is created so we don't display a white frame
	// while initializing
	g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

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
	g_lastFVF = 0;
}

auto
RageDisplay_D3D::GetMaxTextureSize() const -> int
{
	return g_DeviceCaps.MaxTextureWidth;
}

auto
RageDisplay_D3D::BeginFrame() -> bool
{
	GraphicsWindow::Update();

	switch (g_pd3dDevice->TestCooperativeLevel()) {
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

	g_pd3dDevice->Clear(0,
						nullptr,
						D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
						D3DCOLOR_XRGB(0, 0, 0),
						1.0F,
						0x00000000);

	g_pd3dDevice->BeginScene();

	return RageDisplay::BeginFrame();
}

void
RageDisplay_D3D::EndFrame()
{
	g_pd3dDevice->EndScene();

	FrameLimitBeforeVsync();

	const auto beforePresent = std::chrono::steady_clock::now();
	g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

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
		((g_DeviceCaps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE) == 0u)) {
		return false;
	}

	if (D3DFORMATS[pixfmt] == D3DFMT_UNKNOWN) {
		return false;
	}

	const auto d3dfmt = D3DFORMATS[pixfmt];
	const auto hr = g_pd3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
											  D3DDEVTYPE_HAL,
											  g_d3dpp.BackBufferFormat,
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
	if (SUCCEEDED(g_pd3dDevice->GetBackBuffer(
		  0, 0, D3DBACKBUFFER_TYPE_MONO, &pSurface))) {
		// Get the back buffer description.
		D3DSURFACE_DESC desc;
		pSurface->GetDesc(&desc);

		// Copy the back buffer into a surface of a type we support.
		IDirect3DSurface9* pCopy;
		if (SUCCEEDED(g_pd3dDevice->CreateOffscreenPlainSurface(desc.Width,
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

		if (g_bInvertY) {
			RageMatrix flip;
			RageMatrixScale(&flip, +1, -1, +1);
			RageMatrixMultiply(&m, &flip, &m);
		}

		// Convert to OpenGL-style "pixel-centered" coords
		auto m2 = GetCenteringMatrix(-0.5F, -0.5F, 0, 0);
		RageMatrix projection;
		RageMatrixMultiply(&projection, &m2, &m);
		g_pd3dDevice->SetTransform(D3DTS_PROJECTION,
								   reinterpret_cast<D3DMATRIX*>(&projection));

		g_pd3dDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)GetViewTop());
		g_pd3dDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)GetWorldTop());

		FOREACH_ENUM(TextureUnit, tu)
		{
			// If no texture is set for this texture unit, don't bother setting
			// it up.
			IDirect3DBaseTexture9* pTexture = nullptr;
			g_pd3dDevice->GetTexture(tu, &pTexture);
			if (pTexture == nullptr) {
				continue;
			}
			pTexture->Release();

			// Optimization opportunity: Turn off texture transform if not using
			// texture coords.
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

			if (g_bSphereMapping[tu]) {
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
				g_pd3dDevice->SetTransform(
				  static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0 + tu),
				  (D3DMATRIX*)&tex);

				// Tell D3D to use transformed reflection vectors as texture
				// co-ordinate 0 and then transform this coordinate by the
				// specified texture matrix.
				g_pd3dDevice->SetTextureStageState(
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
				g_pd3dDevice->SetTransform(
				  D3DTRANSFORMSTATETYPE(D3DTS_TEXTURE0 + tu),
				  (D3DMATRIX*)&tex2);

				g_pd3dDevice->SetTextureStageState(
				  tu, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
			}
		}
	}
}

class RageCompiledGeometrySWD3D : public RageCompiledGeometry
{
  public:
	void Allocate(const std::vector<msMesh>& /*vMeshes*/) override
	{
		m_vVertex.resize(
		  std::max(1U, static_cast<unsigned>(GetTotalVertices())));
		m_vTriangles.resize(
		  std::max(1U, static_cast<unsigned>(GetTotalTriangles())));
	}

	void Change(const std::vector<msMesh>& vMeshes) override
	{
		for (unsigned i = 0; i < vMeshes.size(); i++) {
			const auto& meshInfo = m_vMeshInfo[i];
			const auto& mesh = vMeshes[i];
			const auto& Vertices = mesh.Vertices;
			const auto& Triangles = mesh.Triangles;

			for (unsigned j = 0; j < Vertices.size(); j++) {
				m_vVertex[meshInfo.iVertexStart + j] = Vertices[j];
			}

			for (unsigned j = 0; j < Triangles.size(); j++) {
				for (unsigned k = 0; k < 3; k++) {
					m_vTriangles[meshInfo.iTriangleStart + j]
					  .nVertexIndices[k] =
					  static_cast<uint16_t>(meshInfo.iVertexStart) +
					  Triangles[j].nVertexIndices[k];
				}
			}
		}
	}

	void Draw(int iMeshIndex) const override
	{
		const auto& meshInfo = m_vMeshInfo[iMeshIndex];

		if (meshInfo.m_bNeedsTextureMatrixScale) {
			// Kill the texture translation.
			// XXX: Change me to scale the translation by the
			// TextureTranslationScale of the first vertex.
			RageMatrix m;
			g_pd3dDevice->GetTransform(D3DTS_TEXTURE0,
									   reinterpret_cast<D3DMATRIX*>(&m));

			m.m[2][0] = 0;
			m.m[2][1] = 0;

			g_pd3dDevice->SetTransform(D3DTS_TEXTURE0,
									   reinterpret_cast<D3DMATRIX*>(&m));
		}

		if (g_lastFVF != D3DFVF_RageModelVertex) {
			g_lastFVF = D3DFVF_RageModelVertex;
			g_pd3dDevice->SetFVF(D3DFVF_RageModelVertex);
		}

		g_pd3dDevice->DrawIndexedPrimitiveUP(
		  D3DPT_TRIANGLELIST,
		  // PrimitiveType
		  meshInfo.iVertexStart,
		  // MinIndex
		  meshInfo.iVertexCount,
		  // NumVertices
		  meshInfo.iTriangleCount,
		  // PrimitiveCount,
		  &m_vTriangles[0] + meshInfo.iTriangleStart,
		  // pIndexData,
		  D3DFMT_INDEX16,
		  // IndexDataFormat,
		  &m_vVertex[0],
		  // pVertexStreamZeroData,
		  sizeof(m_vVertex[0]) // VertexStreamZeroStride
		);
	}

  protected:
	std::vector<RageModelVertex> m_vVertex;
	std::vector<msTriangle> m_vTriangles;
};

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
RageDisplay_D3D::DrawQuadsInternal(const RageSpriteVertex v[], int iNumVerts)
{
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

	if (g_lastFVF != D3DFVF_RageSpriteVertex) {
		g_lastFVF = D3DFVF_RageSpriteVertex;
		g_pd3dDevice->SetFVF(D3DFVF_RageSpriteVertex);
	}

	SendCurrentMatrices();
	g_pd3dDevice->DrawIndexedPrimitiveUP(
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
RageDisplay_D3D::DrawQuadStripInternal(const RageSpriteVertex v[],
									   int iNumVerts)
{
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

	if (g_lastFVF != D3DFVF_RageSpriteVertex) {
		g_lastFVF = D3DFVF_RageSpriteVertex;
		g_pd3dDevice->SetFVF(D3DFVF_RageSpriteVertex);
	}

	SendCurrentMatrices();
	g_pd3dDevice->DrawIndexedPrimitiveUP(
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
RageDisplay_D3D::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[],
												int iNumVerts)
{
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

	if (g_lastFVF != D3DFVF_RageSpriteVertex) {
		g_lastFVF = D3DFVF_RageSpriteVertex;
		g_pd3dDevice->SetFVF(D3DFVF_RageSpriteVertex);
	}

	SendCurrentMatrices();
	g_pd3dDevice->DrawIndexedPrimitiveUP(
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
RageDisplay_D3D::DrawFanInternal(const RageSpriteVertex v[], int iNumVerts)
{
	if (g_lastFVF != D3DFVF_RageSpriteVertex) {
		g_lastFVF = D3DFVF_RageSpriteVertex;
		g_pd3dDevice->SetFVF(D3DFVF_RageSpriteVertex);
	}

	SendCurrentMatrices();
	g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN,
								  // PrimitiveType
								  iNumVerts - 2,
								  // PrimitiveCount,
								  v,
								  // pVertexStreamZeroData,
								  sizeof(RageSpriteVertex));
}

void
RageDisplay_D3D::DrawStripInternal(const RageSpriteVertex v[], int iNumVerts)
{
	if (g_lastFVF != D3DFVF_RageSpriteVertex) {
		g_lastFVF = D3DFVF_RageSpriteVertex;
		g_pd3dDevice->SetFVF(D3DFVF_RageSpriteVertex);
	}

	SendCurrentMatrices();
	g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,
								  // PrimitiveType
								  iNumVerts - 2,
								  // PrimitiveCount,
								  v,
								  // pVertexStreamZeroData,
								  sizeof(RageSpriteVertex));
}

void
RageDisplay_D3D::DrawTrianglesInternal(const RageSpriteVertex v[],
									   int iNumVerts)
{
	if (g_lastFVF != D3DFVF_RageSpriteVertex) {
		g_lastFVF = D3DFVF_RageSpriteVertex;
		g_pd3dDevice->SetFVF(D3DFVF_RageSpriteVertex);
	}

	SendCurrentMatrices();
	g_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,
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
	SendCurrentMatrices();

	/* If lighting is off, then the current material will have no effect. We
	 * want to still be able to color models with lighting off, so shove the
	 * material color in texture factor and modify the texture stage to use it
	 * instead of the vertex color (our models don't have vertex coloring
	 * anyway). */
	DWORD bLighting;
	g_pd3dDevice->GetRenderState(D3DRS_LIGHTING, &bLighting);

	if (bLighting == 0u) {
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	}

	p->Draw(iMeshIndex);

	if (bLighting == 0u) {
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
}

/* Use the default poly-based implementation.  D3D lines apparently don't
 * support AA with greater-than-one widths. */
/*
void RageDisplay_D3D::DrawLineStrip( const RageSpriteVertex v[], int iNumVerts,
float LineWidth )
{
	ASSERT( iNumVerts >= 2 );
	g_pd3dDevice->SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&LineWidth) );
// funky cast.  See D3DRENDERSTATETYPE doc g_pd3dDevice->SetVertexShader(
D3DFVF_RageSpriteVertex ); SendCurrentMatrices(); g_pd3dDevice->DrawPrimitiveUP(
		D3DPT_LINESTRIP, // PrimitiveType
		iNumVerts-1, // PrimitiveCount,
		v, // pVertexStreamZeroData,
		sizeof(RageSpriteVertex)
	);
	StatsAddVerts( iNumVerts );
}
*/

void
RageDisplay_D3D::ClearAllTextures()
{
	FOREACH_ENUM(TextureUnit, i)
	SetTexture(i, 0);
}

auto
RageDisplay_D3D::GetNumTextureUnits() -> int
{
	return g_DeviceCaps.MaxSimultaneousTextures;
}

void
RageDisplay_D3D::SetTexture(TextureUnit tu, intptr_t iTexture)
{
	//	g_DeviceCaps.MaxSimultaneousTextures = 1;
	if (tu >= static_cast<int>(g_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	if (iTexture == 0) {
		g_pd3dDevice->SetTexture(tu, nullptr);

		/* Intentionally commented out. Don't mess with texture stage state
		 * when just setting the texture. Model sets its texture modes before
		 * setting the final texture. */
		// g_pd3dDevice->SetTextureStageState( tu, D3DTSS_COLOROP,
		// D3DTOP_DISABLE );
	} else {
		auto* pTex = reinterpret_cast<IDirect3DTexture9*>(iTexture);
		g_pd3dDevice->SetTexture(tu, pTex);

		/* Intentionally commented out. Don't mess with texture stage state
		 * when just setting the texture. Model sets its texture modes before
		 * setting the final texture. */
		// g_pd3dDevice->SetTextureStageState( tu, D3DTSS_COLOROP,
		// D3DTOP_MODULATE );

		// Set palette (if any)
		SetPalette(iTexture);
	}
}

void
RageDisplay_D3D::SetTextureMode(TextureUnit tu, TextureMode tm)
{
	if (tu >= static_cast<int>(g_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	switch (tm) {
		case TextureMode_Modulate:
			// Use D3DTA_CURRENT instead of diffuse so that multitexturing works
			// properly.  For stage 0, D3DTA_CURRENT is the diffuse color.

			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLORARG2, D3DTA_CURRENT);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLOROP, D3DTOP_MODULATE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		case TextureMode_Add:
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLORARG2, D3DTA_CURRENT);
			g_pd3dDevice->SetTextureStageState(tu, D3DTSS_COLOROP, D3DTOP_ADD);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		case TextureMode_Glow:
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLORARG2, D3DTA_CURRENT);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
			g_pd3dDevice->SetTextureStageState(
			  tu, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			break;
		default:
			Locator::getLogger()->warn("RageDisplay_D3D::SetTextureMode called with invalid TextureMode");
	}
}

void
RageDisplay_D3D::SetTextureFiltering(TextureUnit tu, bool b)
{
	if (tu >= static_cast<int>(g_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	g_pd3dDevice->SetSamplerState(
	  tu, D3DSAMP_MINFILTER, b ? D3DTEXF_LINEAR : D3DTEXF_POINT);
	g_pd3dDevice->SetSamplerState(
	  tu, D3DSAMP_MAGFILTER, b ? D3DTEXF_LINEAR : D3DTEXF_POINT);
}

void
RageDisplay_D3D::SetBlendMode(BlendMode mode)
{
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

	if (mode == BLEND_INVERT_DEST) {
		g_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
	} else {
		g_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	}

	switch (mode) {
		case BLEND_NORMAL:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			break;
		case BLEND_ADD:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
			// This is not the right way to do BLEND_SUBTRACT.  This code is
			// only here to prevent crashing when someone tries to use it. -Kyz
		case BLEND_SUBTRACT:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			break;
		case BLEND_MODULATE:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		case BLEND_COPY_SRC:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			break;
			/* Effects currently missing in D3D: BLEND_ALPHA_MASK,
			 * BLEND_ALPHA_KNOCK_OUT These two may require DirectX9 since
			 * D3DRS_SRCALPHA and D3DRS_DESTALPHA don't seem to exist in DX8.
			 * -aj */
		case BLEND_ALPHA_MASK:
			// RGB: iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			// Alpha: iSourceAlpha = GL_ZERO; iDestAlpha = GL_SRC_ALPHA;

			g_pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA,
										 D3DBLEND_SRCALPHA);

			break;
		case BLEND_ALPHA_KNOCK_OUT:
			// RGB: iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			// Alpha: iSourceAlpha = GL_ZERO; iDestAlpha =
			// GL_ONE_MINUS_SRC_ALPHA;

			g_pd3dDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLENDALPHA,
										 D3DBLEND_INVSRCALPHA);

			break;
		case BLEND_ALPHA_MULTIPLY:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
			break;
		case BLEND_WEIGHTED_MULTIPLY:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
			break;
		case BLEND_INVERT_DEST:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		case BLEND_NO_EFFECT:
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			break;
		default:
			FAIL_M(ssprintf("Invalid BlendMode: %i", mode));
	}
}

auto
RageDisplay_D3D::IsZWriteEnabled() const -> bool
{
	DWORD b;
	g_pd3dDevice->GetRenderState(D3DRS_ZWRITEENABLE, &b);
	return b != 0;
}

void
RageDisplay_D3D::SetZBias(float f)
{
	D3DVIEWPORT9 viewData;
	g_pd3dDevice->GetViewport(&viewData);
	viewData.MinZ = SCALE(f, 0.0F, 1.0F, 0.05F, 0.0F);
	viewData.MaxZ = SCALE(f, 0.0F, 1.0F, 1.0F, 0.95F);
	g_pd3dDevice->SetViewport(&viewData);
}

auto
RageDisplay_D3D::IsZTestEnabled() const -> bool
{
	DWORD b;
	g_pd3dDevice->GetRenderState(D3DRS_ZFUNC, &b);
	return b != D3DCMP_ALWAYS;
}

void
RageDisplay_D3D::SetZWrite(bool b)
{
	g_pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, static_cast<DWORD>(b));
}

void
RageDisplay_D3D::SetZTestMode(ZTestMode mode)
{
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
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
	g_pd3dDevice->SetRenderState(D3DRS_ZFUNC, dw);
}

void
RageDisplay_D3D::ClearZBuffer()
{
	g_pd3dDevice->Clear(
	  0, nullptr, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0F, 0x00000000);
}

void
RageDisplay_D3D::SetTextureWrapping(TextureUnit tu, bool b)
{
	if (tu >= static_cast<int>(g_DeviceCaps.MaxSimultaneousTextures)) {
		// not supported
		return;
	}

	const int mode = b ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
	g_pd3dDevice->SetSamplerState(tu, D3DSAMP_ADDRESSU, mode);
	g_pd3dDevice->SetSamplerState(tu, D3DSAMP_ADDRESSV, mode);
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
	g_pd3dDevice->GetRenderState(D3DRS_LIGHTING, &bLighting);

	if (bLighting != 0u) {
		D3DMATERIAL9 mat;
		memcpy(&mat.Diffuse, diffuse, sizeof(float) * 4);
		memcpy(&mat.Ambient, ambient, sizeof(float) * 4);
		memcpy(&mat.Specular, specular, sizeof(float) * 4);
		memcpy(&mat.Emissive, emissive, sizeof(float) * 4);
		mat.Power = shininess;
		g_pd3dDevice->SetMaterial(&mat);
	} else {
		auto c = diffuse;
		c.r += emissive.r + ambient.r;
		c.g += emissive.g + ambient.g;
		c.b += emissive.b + ambient.b;
		RageVColor c2 = c;
		const auto c3 = *reinterpret_cast<DWORD*>(&c2);
		g_pd3dDevice->SetRenderState(D3DRS_TEXTUREFACTOR, c3);
	}
}

void
RageDisplay_D3D::SetLighting(bool b)
{
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, static_cast<DWORD>(b));
}

void
RageDisplay_D3D::SetLightOff(int index)
{
	g_pd3dDevice->LightEnable(index, 0);
}

void
RageDisplay_D3D::SetLightDirectional(int index,
									 const RageColor& ambient,
									 const RageColor& diffuse,
									 const RageColor& specular,
									 const RageVector3& dir)
{
	g_pd3dDevice->LightEnable(index, 1);

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

	g_pd3dDevice->SetLight(index, &light);
}

void
RageDisplay_D3D::SetCullMode(CullMode mode)
{
	switch (mode) {
		case CULL_BACK:
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
			break;
		case CULL_FRONT:
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			break;
		case CULL_NONE:
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
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
	if (g_mapRenderTargets.find(iTexHandle) != g_mapRenderTargets.end()) {
		delete g_mapRenderTargets[iTexHandle];
		g_mapRenderTargets.erase(iTexHandle);
		return;
	}

	// Delete palette (if any)
	if (g_TexResourceToPaletteIndex.find(iTexHandle) !=
		g_TexResourceToPaletteIndex.end()) {
		g_TexResourceToPaletteIndex.erase(
		  g_TexResourceToPaletteIndex.find(iTexHandle));
	}
	if (g_TexResourceToTexturePalette.find(iTexHandle) !=
		g_TexResourceToTexturePalette.end()) {
		g_TexResourceToTexturePalette.erase(
		  g_TexResourceToTexturePalette.find(iTexHandle));
	}
}

auto
RageDisplay_D3D::CreateTexture(RagePixelFormat pixfmt,
							   RageSurface* img,
							   bool /*bGenerateMipMaps*/) -> intptr_t
{
	HRESULT hr;
	IDirect3DTexture9* pTex;
	hr = g_pd3dDevice->CreateTexture(power_of_two(img->w),
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
							 GetErrorString(hr).c_str());
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

		ASSERT(g_TexResourceToTexturePalette.find(uTexHandle) ==
			   g_TexResourceToTexturePalette.end());
		g_TexResourceToTexturePalette[uTexHandle] = pal;
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
	g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, static_cast<DWORD>(b));
	g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0);
	g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
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

// Ported from OpenGL - xwidghet
class D3DRenderTarget_FramebufferObject : public RenderTarget
{
  public:
	D3DRenderTarget_FramebufferObject();
	~D3DRenderTarget_FramebufferObject() override;
	void Create(const RenderTargetParam& param,
				int& iTextureWidthOut,
				int& iTextureHeightOut) override;
	[[nodiscard]] auto GetTexture() const -> intptr_t override
	{
		return reinterpret_cast<intptr_t>(m_uTexHandle);
	}
	void StartRenderingTo() override;
	void FinishRenderingTo() override;

	[[nodiscard]] auto InvertY() const -> bool override { return true; }

  private:
	IDirect3DSurface9* m_iFrameBufferHandle;
	IDirect3DTexture9* m_uTexHandle;
	IDirect3DSurface9* m_iDepthBufferHandle;
};

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

auto
RageDisplay_D3D::CreateRenderTarget(const RenderTargetParam& param,
									int& iTextureWidthOut,
									int& iTextureHeightOut) -> intptr_t
{
	auto* pTarget = new D3DRenderTarget_FramebufferObject;

	pTarget->Create(param, iTextureWidthOut, iTextureHeightOut);

	const auto uTexture = pTarget->GetTexture();

	ASSERT(g_mapRenderTargets.find(uTexture) == g_mapRenderTargets.end());
	g_mapRenderTargets[uTexture] = pTarget;

	return uTexture;
}

auto
RageDisplay_D3D::GetRenderTarget() -> intptr_t
{
	for (const auto& g_mapRenderTarget : g_mapRenderTargets) {
		if (g_mapRenderTarget.second == g_pCurrentRenderTarget) {
			return g_mapRenderTarget.first;
		}
	}
	return 0;
}

void
RageDisplay_D3D::SetRenderTarget(intptr_t uTexHandle, bool bPreserveTexture)
{
	if (uTexHandle == 0) {
		g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		/* Pop matrixes affected by SetDefaultRenderStates. */
		DISPLAY->CameraPopMatrix();

		/* Reset the viewport. */
		D3DVIEWPORT9 viewData;
		g_pd3dDevice->GetViewport(&viewData);
		viewData.Width = GetActualVideoModeParams()->width;
		viewData.Height = GetActualVideoModeParams()->height;
		g_pd3dDevice->SetViewport(&viewData);

		if (g_pCurrentRenderTarget != nullptr) {
			g_pCurrentRenderTarget->FinishRenderingTo();
		}
		g_pCurrentRenderTarget = nullptr;
		g_pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, 0u);
		return;
	}

	/* If we already had a render target, disable it. */
	if (g_pCurrentRenderTarget != nullptr) {
		SetRenderTarget(0, true);
	}

	/* Enable the new render target. */
	ASSERT(g_mapRenderTargets.find(uTexHandle) != g_mapRenderTargets.end());
	auto* pTarget = g_mapRenderTargets[uTexHandle];
	pTarget->StartRenderingTo();
	g_pCurrentRenderTarget = pTarget;

	/* Set the viewport to the size of the render target. */
	D3DVIEWPORT9 viewData;
	g_pd3dDevice->GetViewport(&viewData);
	viewData.Width = pTarget->GetParam().iWidth;
	viewData.Height = pTarget->GetParam().iHeight;
	g_pd3dDevice->SetViewport(&viewData);

	/* If this render target implementation flips Y, compensate.   Inverting
	 * will switch the winding order. */
	if (g_bInvertY) {
		g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	}

	/* The render target may be in a different D3D context, so re-send
	 * state.  Push matrixes affected by SetDefaultRenderStates. */
	DISPLAY->CameraPushMatrix();
	SetDefaultRenderStates();
	SetZWrite(true);

	// Need to blend the render targets together, not sure why OpenGL doesn't
	// need this -xwidghet
	g_pd3dDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, 1u);

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

		if (FAILED(g_pd3dDevice->Clear(0, nullptr, iBit, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0F, 0x00000000))) {
			Locator::getLogger()->warn("Failed to clear render target");
		}
	}
}

void
RageDisplay_D3D::SetSphereEnvironmentMapping(TextureUnit tu, bool b)
{
	g_bSphereMapping[tu] = b;
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
