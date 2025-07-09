#ifndef RAGE_VERTEX_SHADER_HANDLER_D3D_H
#define RAGE_VERTEX_SHADER_HANDLER_D3D_H

#include "RageUtil/Graphics/Shaders/RageShaderHandler.h"
#include <d3d9.h>

class RageVertexShaderHandler_D3D : public RageShaderHandler
{
  public:
	RageVertexShaderHandler_D3D(LPDIRECT3DDEVICE9 device);

  private:
  	std::unique_ptr<RageShader> CompileShader(
	  const std::string& path) override;
	const LPDIRECT3DDEVICE9 m_Device;
};

#endif
