#ifndef RAGE_DISPLAY_D3D_HELPERS_H
#define RAGE_DISPLAY_D3D_HELPERS_H

#include <d3d9.h>
#include <d3d9types.h>
#include <string>
#include <source_location>
#include <memory>
#include "RageUtil/Graphics/Shaders/RageShader.h"

namespace RageDisplay_D3D_Helpers {
std::string
GetErrorString(HRESULT hr);

void
LogHResultFailure(
  HRESULT hr,
  const std::source_location location = std::source_location::current());

constexpr std::string_view ShaderEntryPoint("main");

extern const D3DVERTEXELEMENT9 SpriteDeclaration[];
extern const D3DVERTEXELEMENT9 ModelDeclaration[];

std::unique_ptr<RageShader>
CompilePixelShaderFromPath(const std::string& path, LPDIRECT3DDEVICE9 device);

std::unique_ptr<RageShader>
CompileVertexShaderFromPath(const std::string& path, LPDIRECT3DDEVICE9 device);
}
#endif
