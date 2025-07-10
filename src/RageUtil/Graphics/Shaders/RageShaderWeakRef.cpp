#include "RageShaderWeakRef.h"
#include "RageUtil/Graphics/RageDisplay.h"

RageShaderWeakRef::RageShaderWeakRef()
  : RageShaderWeakRef(nullptr,
					  0,
					  RageDisplayType::Invalid,
					  RageShaderType::Invalid,
					  false)
{
}

RageShaderWeakRef::RageShaderWeakRef(RageShader* shader,
									 size_t lookupKey,
									 RageDisplayType displayType,
									 RageShaderType shaderType,
									 bool isDefault)
  : m_LookupKey(lookupKey)
  , m_DisplayType(displayType)
  , m_ShaderType(shaderType)
  , m_IsDefault(isDefault)
  , m_Shader(shader)
{
	m_pLuaInstance = std::make_unique<LuaClass>();
}

RageShaderWeakRef::RageShaderWeakRef(const RageShaderWeakRef& otherWeakRef)
{
	m_LookupKey = otherWeakRef.m_LookupKey;
	m_DisplayType = otherWeakRef.m_DisplayType;
	m_ShaderType = otherWeakRef.m_ShaderType;
	m_IsDefault = otherWeakRef.m_IsDefault;
	m_Shader = otherWeakRef.m_Shader;
}

RageShaderWeakRef&
RageShaderWeakRef::operator=(const RageShaderWeakRef& otherWeakRef)
{
	m_LookupKey = otherWeakRef.m_LookupKey;
	m_DisplayType = otherWeakRef.m_DisplayType;
	m_ShaderType = otherWeakRef.m_ShaderType;
	m_IsDefault = otherWeakRef.m_IsDefault;
	m_Shader = otherWeakRef.m_Shader;

	return *this;
}

bool
RageShaderWeakRef::IsDestroyed() const
{
	return m_ShaderType == RageShaderType::Invalid || DISPLAY->IsShaderInCache(*this);
}

RageShader*
RageShaderWeakRef::GetShader() const
{
	return m_Shader;
}

size_t
RageShaderWeakRef::GetLookupKey() const
{
	return m_LookupKey;
}

RageDisplayType
RageShaderWeakRef::GetDisplayType() const
{
	return m_DisplayType;
}

RageShaderType
RageShaderWeakRef::GetShaderType() const
{
	return m_ShaderType;
}

bool
RageShaderWeakRef::IsDefault() const
{
	return m_IsDefault;
}

// maybe move this to LunaRageShaderWeakRef.cpp or something if the macrohell
// doesn't unleash
#pragma region Lua wrapper

class LunaRageShaderWeakRef : public Luna<RageShaderWeakRef>
{
	// TODO: stuffs?
};

LUA_REGISTER_INSTANCED_BASE_CLASS(RageShaderWeakRef)

#pragma endregion
