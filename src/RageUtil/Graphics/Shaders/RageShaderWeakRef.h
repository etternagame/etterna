/* RageShaderWeakRef - weak reference to a shader (used for lookups by RageShaderHandler) */

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

// class LuaClass;

// TODO: Lua bindings so it's easy to pass these thingies around
class RageShaderWeakRef
{
  public:
	RageShaderWeakRef();

	RageShaderWeakRef(size_t lookupKey,
					  RageDisplayType displayType,
					  RageShaderType shaderType);
	RageShaderWeakRef(const RageShaderWeakRef& otherWeakRef);
	RageShaderWeakRef& operator=(const RageShaderWeakRef& otherWeakRef);

	size_t GetLookupKey() const;
	RageDisplayType GetDisplayType() const;
	RageShaderType GetShaderType() const;

  private:
	size_t m_LookupKey;
	RageDisplayType m_DisplayType;
	RageShaderType m_ShaderType;
	// std::unique_ptr<LuaClass> m_pLuaInstance;
};

#endif
