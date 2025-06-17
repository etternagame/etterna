#ifndef RAGE_PIXEL_SHADER_D3D_H
#define RAGE_PIXEL_SHADER_D3D_H

#include <d3d9.h>
#include <d3dx9.h>
#include <string>

class RagePixelShader_D3D
{
  public:
	[[nodiscard]] HRESULT Compile(const std::string& pixelShaderProfile);

	// caller is not responsible for releasing the shader
	// forceRefresh=true discards any previously created shader
	IDirect3DPixelShader9* CreateForDevice(LPDIRECT3DDEVICE9 device,
										   bool forceRefresh);

	RagePixelShader_D3D(const std::string& path)
	  : m_Path(path)
	{
		m_ShaderBuffer = nullptr;
		m_Shader = nullptr;
	}

	~RagePixelShader_D3D();

  private:
	LPD3DXBUFFER m_ShaderBuffer;
	IDirect3DPixelShader9* m_Shader;
	const std::string m_Path;
};

#endif
