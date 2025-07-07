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
	return shaderLookupKey < m_DefaultShaderCache.size();
}

std::optional<RageShaderWeakRef>
RageShaderHandler::CacheShaderFromPath(const std::string& path,
									   bool useAsDefaultShader)
{
	std::optional<RageShader> shader = CompileShader(path);
	if (!shader.has_value()) {
		return std::nullopt;
	}

	if (useAsDefaultShader) {
		m_DefaultShaderCache.emplace_back(*shader);
		return RageShaderWeakRef(
		  m_DefaultShaderCache.size() - 1, m_DisplayType, m_ShaderType, true);
	}

	size_t lookupKey = GetShaderLookupKey(path);
	m_ShaderCache.emplace(lookupKey, *shader);
	return RageShaderWeakRef(lookupKey, m_DisplayType, m_ShaderType, false);
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

	RageShader* neededShader = setDefaultShader
								 ? &m_DefaultShaderCache[shaderLookupKey]
								 : &m_ShaderCache[shaderLookupKey];

	if (neededShader == nullptr) {
		return false;
	}

	m_CurrentShader = neededShader;
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
