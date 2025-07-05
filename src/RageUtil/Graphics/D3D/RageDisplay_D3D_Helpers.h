#ifndef RAGE_DISPLAY_D3D_HELPERS_H
#define RAGE_DISPLAY_D3D_HELPERS_H

#include <d3d9.h>
#include <string>

namespace RageDisplay_D3D_Helpers {
std::string
GetErrorString(HRESULT hr);

constexpr std::string_view ShaderEntryPoint("main");
}
#endif
