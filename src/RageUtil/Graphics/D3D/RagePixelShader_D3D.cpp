#include "RagePixelShader_D3D.h"
#include "Core/Services/Locator.hpp"
#include "RageDisplay_D3D_Helpers.h"

[[nodiscard]] HRESULT
RagePixelShader_D3D::Compile(const std::string& pixelShaderProfile)
{
	LPD3DXBUFFER errorBuffer = nullptr;
	auto result = D3DXCompileShaderFromFile(m_Path.c_str(),
											nullptr,
											nullptr,
	  RageDisplay_D3D_Helpers::ShaderEntryPoint.data(),
											pixelShaderProfile.c_str(),
											0,
											&m_ShaderBuffer,
											&errorBuffer,
											nullptr);

	if (result != D3D_OK) {
		Locator::getLogger()->warn("RagePixelShader_D3D D3DXCompileShaderFromFile "
								   "failed for {} - {} (error buffer: {})",
								   m_Path,
		  RageDisplay_D3D_Helpers::GetErrorString(result),
		  (char*)errorBuffer->GetBufferPointer());
		errorBuffer->Release();
	}

	return result;
}

IDirect3DPixelShader9*
RagePixelShader_D3D::CreateForDevice(LPDIRECT3DDEVICE9 device, bool forceRefresh)
{
	if (m_Shader != nullptr && !forceRefresh) {
		return m_Shader;
	}

	IDirect3DPixelShader9* shader = nullptr;
	auto result = device->CreatePixelShader(
	  (const DWORD*)m_ShaderBuffer->GetBufferPointer(), &shader);

	if (result != D3D_OK) {
		Locator::getLogger()->warn(
		  "RagePixelShader_D3D "
		  "CreateForDevice failed for {} - {}",
		  m_Path,
		  RageDisplay_D3D_Helpers::GetErrorString(result));
		return nullptr;
	}

	if (m_Shader != nullptr) {
		m_Shader->Release();
	}

	m_Shader = shader;

	return shader;
}

RagePixelShader_D3D::~RagePixelShader_D3D()
{
	if (m_ShaderBuffer != nullptr) {
		m_ShaderBuffer->Release();
		m_ShaderBuffer = nullptr;
	}
	if (m_Shader != nullptr) {
		m_Shader->Release();
		m_Shader = nullptr;
	}
}
