#include "RageVertexShaderHandler_D3D.h"
#include "RageVertexShader_D3D.h"

RageVertexShaderHandler_D3D::RageVertexShaderHandler_D3D(LPDIRECT3DDEVICE9 device)
  : RageShaderHandler(RageDisplayType::D3D, RageShaderType::Vertex)
  , m_Device(device)
{
}

std::optional<std::unique_ptr<RageShader>>
RageVertexShaderHandler_D3D::CompileShader(const std::string& path)
{
	auto shader = std::make_unique<RageVertexShader_D3D>(path);

	auto profile = D3DXGetVertexShaderProfile(m_Device);
	auto creationResult = shader->GetShaderForDevice(m_Device, false);
	if (creationResult == nullptr) {
		return std::nullopt;
	}

	return shader;
}
