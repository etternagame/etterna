#include "RageShaderHandler.h"

RageShaderHandler::RageShaderHandler(RageDisplayType displayType,
									 RageShaderType shaderType)
  : m_DisplayType(displayType)
  , m_ShaderType(shaderType)
{
	m_CurrentShader = nullptr;
}

bool
RageShaderHandler::IsShaderInCache(size_t shaderLookupKey) const
{
	return m_ShaderCache.contains(shaderLookupKey);
}

bool
RageShaderHandler::IsDefaultShaderInCache(size_t shaderLookupKey) const
{
	return m_DefaultShaderCache.contains(shaderLookupKey);
}

std::optional<RageShaderWeakRef>
RageShaderHandler::CacheShaderFromPath(const std::string& path,
									   bool useAsDefaultShader)
{
	std::optional<std::unique_ptr<RageShader>> shader = CompileShader(path);
	if (!shader.has_value()) {
		return std::nullopt;
	}

	size_t lookupKey = GetShaderLookupKey(path);

	std::unordered_map<size_t, std::unique_ptr<RageShader>>& cache =
	  useAsDefaultShader ? m_DefaultShaderCache : m_ShaderCache;
	cache.emplace(lookupKey, std::move(*shader));

	return RageShaderWeakRef(lookupKey, m_DisplayType, m_ShaderType, useAsDefaultShader);
}

bool
RageShaderHandler::TryRemoveShaderFromCache(RageShaderWeakRef shader)
{
	if (shader.IsDefault()) {
		return false;
	}

	auto it = m_ShaderCache.find(shader.GetLookupKey());
	if (it == m_ShaderCache.end()) {
		return false;
	}

	m_ShaderCache.erase(it);
	return true;
}

bool
RageShaderHandler::TrySetActiveShader(RageShaderWeakRef shader)
{
	size_t shaderLookupKey = shader.GetLookupKey();
	bool setDefaultShader = shader.IsDefault();

	if ((setDefaultShader && !IsDefaultShaderInCache(shaderLookupKey)) ||
		(!setDefaultShader && !IsShaderInCache(shaderLookupKey))) {
		return false;
	}

	m_CurrentShader = setDefaultShader
						? m_DefaultShaderCache[shaderLookupKey].get()
						: m_ShaderCache[shaderLookupKey].get();
	return true;
}

size_t
RageShaderHandler::GetShaderLookupKey(const std::string& path)
{
	return m_StringHasher(path);
}

RageShader*
RageShaderHandler::GetCurrentShader()
{
	return m_CurrentShader;
}
