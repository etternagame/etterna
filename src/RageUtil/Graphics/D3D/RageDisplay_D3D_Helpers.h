#ifndef RAGE_DISPLAY_D3D_HELPERS_H
#define RAGE_DISPLAY_D3D_HELPERS_H

#include <d3d9.h>
#include <d3d9types.h>
#include <string>

#define D3DFVF_RageSpriteVertex                                                \
	(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define D3DFVF_RageModelVertex (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)

namespace RageDisplay_D3D_Helpers {
std::string
GetErrorString(HRESULT hr);

constexpr std::string_view ShaderEntryPoint("main");

extern const D3DVERTEXELEMENT9 SpriteDeclaration[];
extern const D3DVERTEXELEMENT9 ModelDeclaration[];
}
#endif
