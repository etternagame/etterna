#ifndef RAGE_SHADER_HANDLER_H
#define RAGE_SHADER_HANDLER_H

#include "RageShader.h"
#include "RageShaderWeakRef.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>

class RageShaderHandler
{
public:
	RageShaderHandler(RageDisplayType displayType, RageShaderType shaderType);

	bool IsShaderInCache(size_t shaderLookupKey) const;
	bool IsDefaultShaderInCache(size_t shaderLookupKey) const;

	std::optional<RageShaderWeakRef> GetOrCreateShaderFromPath(const std::string& path,
												bool useAsDefaultShader);
	bool TryRemoveShaderFromCache(RageShaderWeakRef shader);
	bool TrySetActiveShader(RageShaderWeakRef shader);
	
	// CONVENTION (notes for myself): ownership is not passed to the caller
	RageShader* GetCurrentShader();

  protected:
	// CONVENTION (notes for myself): ownership of the return value must be
	// passed to the caller
	virtual std::optional<std::unique_ptr<RageShader>> CompileShader(
	  const std::string& path) = 0;

  private:
	std::unordered_map<size_t, std::unique_ptr<RageShader>> m_ShaderCache;
	std::unordered_map<size_t, std::unique_ptr<RageShader>>
	  m_DefaultShaderCache;

	const RageDisplayType m_DisplayType;
	const RageShaderType m_ShaderType;

	RageShader* m_CurrentShader;

	size_t GetShaderLookupKey(const std::string& path);
	std::hash<std::string> m_StringHasher;
};

#endif
