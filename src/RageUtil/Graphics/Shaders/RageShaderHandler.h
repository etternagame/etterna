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
	// (lookupKey, isDefaultShader)
	using ShaderId = std::pair<size_t, bool>;

	bool IsShaderInCache(size_t shaderLookupKey) const;
	bool IsDefaultShaderInCache(size_t defaultShaderIndex) const;

	std::optional<ShaderId> CacheShaderFromPath(const std::string& path,
												bool useAsDefaultShader);
	bool TryRemoveShaderFromCache(size_t shaderLookupKey);
	bool TrySetActiveShader(ShaderId shaderId);

  protected:
	// CONVENTION (notes for myself): ownership of the return value must be
	// passed to the caller
	virtual std::optional<RageShader> CompileShader(
	  const std::string& path) = 0;

	// CONVENTION (notes for myself): the shader ownership is NOT passed to this
	// function; returns true if everything went OK
	virtual bool TrySetShaderForDevice(RageShader* shaderHandle) = 0;

  private:
	std::unordered_map<size_t, RageShader> m_ShaderCache;
	std::vector<RageShader> m_DefaultShaderCache;

	std::optional<ShaderId> m_CurrentShader;

	size_t GetShaderLookupKey(const std::string& path);
	std::hash<std::string> m_StringHasher;
};

#endif
