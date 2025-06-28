#include "RageShaderHandler.h"

bool
RageShaderHandler::IsShaderInCache(size_t shaderLookupKey) const
{
	return m_ShaderCache.contains(shaderLookupKey);
}

bool
RageShaderHandler::IsDefaultShaderInCache(size_t defaultShaderIndex) const
{
	return defaultShaderIndex < m_DefaultShaderCache.size();
}

std::optional<RageShaderHandler::ShaderId>
RageShaderHandler::CacheShaderFromPath(const std::string& path,
										bool useAsDefaultShader)
{
	std::optional<RageShader> shader = m_CompileShader(path);
	if (!shader.has_value()) {
		return std::nullopt;
	}

	if (useAsDefaultShader) {
		m_DefaultShaderCache.emplace_back(*shader);
		return RageShaderHandler::ShaderId{ m_DefaultShaderCache.size() - 1,
											true };
	}

	size_t lookupKey = GetShaderLookupKey(path);
	m_ShaderCache.emplace(lookupKey, *shader);
	return RageShaderHandler::ShaderId{ lookupKey, true };
}

bool
RageShaderHandler::TryRemoveShaderFromCache(size_t shaderLookupKey)
{
	auto it = m_ShaderCache.find(shaderLookupKey);
	if (it == m_ShaderCache.end()) {
		return false;
	}

	m_ShaderCache.erase(it);
	return true;
}

bool
RageShaderHandler::TrySetActiveShader(ShaderId shaderId)
{
	auto& [shaderLookupKey, setDefaultShader] = shaderId;

	if ((setDefaultShader && !IsDefaultShaderInCache(shaderLookupKey)) ||
		(!setDefaultShader && !IsShaderInCache(shaderLookupKey))) {
		return false;
	}

	if (m_CurrentShader.has_value() && *m_CurrentShader == shaderId) {
		return true;
	}

	RageShader* neededShader = setDefaultShader
								 ? &m_DefaultShaderCache[shaderLookupKey]
								 : &m_ShaderCache[shaderLookupKey];

	// TODO: rollback to previous shader in case of fucky wucky?
	size_t setShaderResult = m_TrySetShaderForDevice(neededShader);
	if (setShaderResult == 0) {
		m_CurrentShader = shaderId;
		return true;
	}

	return false;
}

size_t
RageShaderHandler::GetShaderLookupKey(const std::string& path)
{
	return m_StringHasher(path);
}
