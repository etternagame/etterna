#include "RageShaderWeakRef.h"
#include "Etterna/Models/Lua/LuaBinding.h"

RageShaderWeakRef::RageShaderWeakRef()
  : RageShaderWeakRef("",
						0,
						RageDisplayType::Invalid,
						RageShaderType::Invalid)
{
}

RageShaderWeakRef::RageShaderWeakRef(const std::string& path,
										 const size_t pathHash,
										 RageDisplayType displayType,
										 RageShaderType shaderType)
  : m_Path(path)
  , m_PathHash(pathHash)
  , m_DisplayType(displayType)
  , m_ShaderType(shaderType)
{
}
