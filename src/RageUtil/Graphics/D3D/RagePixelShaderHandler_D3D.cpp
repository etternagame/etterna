#include "RagePixelShaderHandler_D3D.h"
#include "RagePixelShader_D3D.h"

RagePixelShaderHandler_D3D::RagePixelShaderHandler_D3D(LPDIRECT3DDEVICE9 device)
  : RageShaderHandler(RageDisplayType::D3D, RageShaderType::Fragment)
  , m_Device(device)
{
}

std::optional<std::unique_ptr<RageShader>>
RagePixelShaderHandler_D3D::CompileShader(const std::string& path)
{
	auto shader = std::make_unique<RagePixelShader_D3D>(path);

	auto profile = D3DXGetPixelShaderProfile(m_Device);
	auto compilationResult = shader->Compile(profile);
	if (compilationResult != S_OK) {
		return std::nullopt;
	}

	auto creationResult = shader->CreateForDevice(m_Device, false);
	if (creationResult == nullptr) {
		return std::nullopt;
	}

	return shader;
}
