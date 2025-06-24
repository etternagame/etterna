/* RageShaderWeakRef - weak reference to a shader */

#ifndef RAGE_SHADER_REFERENCE_H
#define RAGE_SHADER_REFERENCE_H

#include <string>
#include "Etterna/Models/Misc/EnumHelper.h"

enum class RageDisplayType
{
	D3D,
	OGL,
	Invalid,
};
LuaDeclareType(RageDisplayType);

enum class RageShaderType
{
	Fragment,
	Vertex,
	Invalid,
};
LuaDeclareType(RageShaderType);

//class LuaClass;

// TODO: Lua bindings so it's easy to pass these thingies around
class RageShaderWeakRef
{
  public:
	RageShaderWeakRef();

	RageShaderWeakRef(const std::string& path,
						const size_t pathHash,
						RageDisplayType displayType,
						RageShaderType shaderType);

	const std::string m_Path;
	const size_t m_PathHash;
	const RageDisplayType m_DisplayType;
	const RageShaderType m_ShaderType;
	//std::unique_ptr<LuaClass> m_pLuaInstance;
};

#endif
