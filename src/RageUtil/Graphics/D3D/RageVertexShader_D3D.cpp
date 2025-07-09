#include "RageVertexShader_D3D.h"
#include "Core/Services/Locator.hpp"
#include "RageDisplay_D3D_Helpers.h"

[[nodiscard]] HRESULT
RageVertexShader_D3D::Compile(const std::string& vertexShaderProfile)
{
	LPD3DXBUFFER errorBuffer = nullptr;
	auto result = D3DXCompileShaderFromFile(m_Path.c_str(),
											nullptr,
											nullptr,
	  RageDisplay_D3D_Helpers::ShaderEntryPoint.data(),
											vertexShaderProfile.c_str(),
											0,
											&m_ShaderBuffer,
											&errorBuffer,
											nullptr);

	if (result != D3D_OK) {
		Locator::getLogger()->warn("RageVertexShader_D3D D3DXCompileShaderFromFile "
								   "failed for {} - {} (error buffer: {})",
								   m_Path,
		  RageDisplay_D3D_Helpers::GetErrorString(result),
		  (char*)errorBuffer->GetBufferPointer());
		errorBuffer->Release();
		m_ShaderBuffer = nullptr;
	}

	return result;
}

IDirect3DVertexShader9*
RageVertexShader_D3D::CreateForDevice(LPDIRECT3DDEVICE9 device, bool forceRefresh)
{
	if (m_Shader != nullptr && !forceRefresh) {
		return m_Shader;
	}

	IDirect3DVertexShader9* shader = nullptr;
	auto result = device->CreateVertexShader(
	  (const DWORD*)m_ShaderBuffer->GetBufferPointer(), &shader);

	if (result != D3D_OK) {
		Locator::getLogger()->warn(
		  "RageVertexShader_D3D "
		  "CreateForDevice failed for {} - {}",
		  m_Path,
		  RageDisplay_D3D_Helpers::GetErrorString(result));
		return nullptr;
	}

	return shader;
}

IDirect3DVertexShader9*
RageVertexShader_D3D::GetShaderForDevice(LPDIRECT3DDEVICE9 device,
										bool forceRefresh)
{
	if (!forceRefresh && m_Shader != nullptr) {
		return m_Shader;
	}

	ReleaseBuffers();

	const std::string shaderProfile = D3DXGetVertexShaderProfile(device);
	const auto result = Compile(shaderProfile);
	if (FAILED(result)) {
		return nullptr;
	}

	m_Shader = CreateForDevice(device, forceRefresh);
	return m_Shader;
}

void
RageVertexShader_D3D::ReleaseBuffers()
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

RageVertexShader_D3D::~RageVertexShader_D3D()
{
	ReleaseBuffers();
}
