#ifndef RAGE_SHADER_HANDLER_H
#define RAGE_SHADER_HANDLER_H

#include "RageShader.h"
#include "RageShaderWeakRef.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>

// CONVENTION: ownership of the return value must be passed to the caller
using CompileShaderFromPathCallback = std::function<std::optional<RageShader>(const std::string&)>;

// CONVENTION: the shader ownership is NOT passed to the callback; returns true if everything went OK
using TrySetShaderForDeviceCallback = std::function<bool(RageShader*)>;

class RageShaderHandler
{
	// (lookupKey, isDefaultShader)
	using ShaderId = std::pair<size_t, bool>;

	RageShaderHandler(CompileShaderFromPathCallback compileShaderCallback,
					  TrySetShaderForDeviceCallback setShaderCallback);

	bool IsShaderInCache(size_t shaderLookupKey) const;
	bool IsDefaultShaderInCache(size_t defaultShaderIndex) const;

	std::optional<ShaderId> CacheShaderFromPath(const std::string& path,
											   bool useAsDefaultShader);
	bool TryRemoveShaderFromCache(size_t shaderLookupKey);
	bool TrySetActiveShader(ShaderId shaderId);

  private:
	std::unordered_map<size_t, RageShader> m_ShaderCache;
	std::vector<RageShader> m_DefaultShaderCache;

	std::optional<ShaderId> m_CurrentShader;

	const CompileShaderFromPathCallback m_CompileShader;
	const TrySetShaderForDeviceCallback m_TrySetShaderForDevice;

	size_t GetShaderLookupKey(const std::string& path);
	std::hash<std::string> m_StringHasher;
};

#endif
