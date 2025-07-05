#include "RageDisplay_D3D_Helpers.h"
#include "Core/Services/Locator.hpp"
#include "RageUtil/Utils/RageUtil.h"

// The DXGetErrorStringW function comes from the DirectX Error Library  (See
// https://walbourn.github.io/wheres-dxerr-lib/ )
//--------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
const WCHAR* WINAPI
DXGetErrorStringW(_In_ HRESULT hr)
{
#define CHK_ERRA(hrchk)                                                        \
	case hrchk:                                                                \
		return L## #hrchk;

#define HRESULT_FROM_WIN32b(x)                                                 \
	((HRESULT)(x) <= 0 ? ((HRESULT)(x))                                        \
					   : ((HRESULT)(((x) & 0x0000FFFF) |                       \
									(FACILITY_WIN32 << 16) | 0x80000000)))

#define CHK_ERR_WIN32A(hrchk)                                                  \
	case HRESULT_FROM_WIN32b(hrchk):                                           \
	case hrchk:                                                                \
		return L## #hrchk;

	switch (hr) {
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
		// CHK_ERRA(D3DERR_WASSTILLDRAWING)
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

		default:
			return L"Unknown error.";
	}
}

auto
RageDisplay_D3D_Helpers::GetErrorString(HRESULT hr) -> std::string
{
	Locator::getLogger()->warn("RageDisplay_D3D::GetErrorString() - HR {}",
							   static_cast<long>(hr));
	const wchar_t* msg = DXGetErrorStringW(hr);
	if (msg) {
		auto r = WStringToString(std::wstring(msg));
		if (r.compare("Unknown error.") == 0) {
			return fmt::format("Unknown error: HR {}", static_cast<long>(hr));
		} else {
			return r;
		}
	} else {
		return fmt::format("Failed to read error: HR {}",
						   static_cast<long>(hr));
	}
}
