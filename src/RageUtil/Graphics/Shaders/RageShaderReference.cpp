#include "RageShaderReference.h"
#include "Etterna/Models/Lua/LuaBinding.h"

RageShaderReference::RageShaderReference()
  : RageShaderReference("",
						0,
						RageDisplayType::Invalid,
						RageShaderType::Invalid)
{
}

RageShaderReference::RageShaderReference(const std::string& path,
										 const size_t pathHash,
										 RageDisplayType displayType,
										 RageShaderType shaderType)
  : m_Path(path)
  , m_PathHash(pathHash)
  , m_DisplayType(displayType)
  , m_ShaderType(shaderType)
{
}
