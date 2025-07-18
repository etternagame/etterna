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

RageShaderWeakRef
RageShaderHandler::GetOrCreateShaderFromPath(const std::string& path,
									   bool useAsDefaultShader)
{
	std::unordered_map<size_t, std::unique_ptr<RageShader>>& cache =
	  useAsDefaultShader ? m_DefaultShaderCache : m_ShaderCache;

	size_t lookupKey = GetShaderLookupKey(path);
	auto it = cache.find(lookupKey);
	if (it != cache.end()) {
		return RageShaderWeakRef(it->second.get(),
								 lookupKey,
								 m_DisplayType,
								 m_ShaderType,
								 useAsDefaultShader);
	}

	std::unique_ptr<RageShader> shader = CompileShader(path);
	if (shader == nullptr) {
		return RageShaderWeakRef();
	}

	RageShader* rawShader = shader.get();
	cache.emplace(lookupKey, std::move(shader));

	return RageShaderWeakRef(rawShader,
							 lookupKey,
							 m_DisplayType,
							 m_ShaderType,
							 useAsDefaultShader);
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
RageShaderHandler::TrySetActiveShader(RageShader* shader)
{
	if (shader == nullptr) {
		return false;
	}

	m_CurrentShader = shader;
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

void
RageShaderHandler::PushUniform(const RageUniformCollection& uniform)
{
	if (uniform.boolData.empty() && uniform.floatData.empty() &&
		uniform.intData.empty()) {
		return;
	}
	m_UniformQueue.push(uniform);
}

std::optional<RageUniformCollection>
RageShaderHandler::TryPopUniform()
{
	auto uniform =
	  m_UniformQueue.empty() ? std::nullopt : std::make_optional(m_UniformQueue.front());
	return uniform;
}
