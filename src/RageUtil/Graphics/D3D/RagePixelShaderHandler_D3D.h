#ifndef RAGE_PIXEL_SHADER_HANDLER_D3D_H
#define RAGE_PIXEL_SHADER_HANDLER_D3D_H

#include "RageUtil/Graphics/Shaders/RageShaderHandler.h"
#include <d3d9.h>

class RagePixelShaderHandler_D3D : public RageShaderHandler
{
public:
    RagePixelShaderHandler_D3D(LPDIRECT3DDEVICE9 device);
    std::optional<RageShader> CompileShader(
	  const std::string& path) override;

private:
    const LPDIRECT3DDEVICE9 m_Device;
};

#endif