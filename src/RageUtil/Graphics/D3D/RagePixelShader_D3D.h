#ifndef RAGE_PIXEL_SHADER_D3D_H
#define RAGE_PIXEL_SHADER_D3D_H

#include "RageUtil/Graphics/Shaders/RageShader.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <string>

class RagePixelShader_D3D : public RageShader
{
  public:
	// caller is not responsible for releasing the shader
	// returns old shader if it exists AND forceRefresh=false
	IDirect3DPixelShader9* GetShaderForDevice(LPDIRECT3DDEVICE9 device,
											  bool forceRefresh);

	RagePixelShader_D3D(const std::string& path)
	  : m_Path(path)
	{
		m_ShaderBuffer = nullptr;
		m_Shader = nullptr;
	}

	~RagePixelShader_D3D();

  private:
	HRESULT Compile(const std::string& pixelShaderProfile);

	IDirect3DPixelShader9* CreateForDevice(LPDIRECT3DDEVICE9 device,
										   bool forceRefresh);

	void ReleaseBuffers();

	LPD3DXBUFFER m_ShaderBuffer;
	IDirect3DPixelShader9* m_Shader;
	const std::string m_Path;
};

#endif
