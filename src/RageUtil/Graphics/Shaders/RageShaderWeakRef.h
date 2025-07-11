/* RageShaderWeakRef - weak reference to a shader (used for lookups by
 * RageShaderHandler) */

#ifndef RAGE_SHADER_REFERENCE_H
#define RAGE_SHADER_REFERENCE_H

#include <string>
#include "RageUtil/Misc/RageTypes.h"
#include "Etterna/Models/Lua/LuaBinding.h"
#include "RageShader.h"

// TODO: Lua bindings so it's easy to pass these thingies around
// ALSO NOTE: it's likely much better to use compile-time constants and such to
// load default shaders
// TODO: these things should not be invalid (i guess)
class RageShaderWeakRef
{
  public:
	RageShaderWeakRef();

	RageShaderWeakRef(RageShader* shader,
					  size_t lookupKey,
					  RageDisplayType displayType,
					  RageShaderType shaderType,
					  bool isDefault);
	RageShaderWeakRef(const RageShaderWeakRef& otherWeakRef);
	RageShaderWeakRef& operator=(const RageShaderWeakRef& otherWeakRef);

	bool IsDestroyed() const;

	RageShader* GetShader() const;
	size_t GetLookupKey() const;
	RageDisplayType GetDisplayType() const;
	RageShaderType GetShaderType() const;
	bool IsDefault() const;

	// LunaRageShaderWeakRef territory
	void PushSelf(lua_State* L);
	std::unique_ptr<LuaClass> m_pLuaInstance;

  private:
	RageShader* m_Shader;
	size_t m_LookupKey;
	RageDisplayType m_DisplayType;
	RageShaderType m_ShaderType;
	bool m_IsDefault;
};

#endif
