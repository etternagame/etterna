/* RageShaderHandler - a cache & lifetime manager for compiled shaders */
#ifndef RAGE_SHADER_HANDLER_H
#define RAGE_SHADER_HANDLER_H

#include "RageShader.h"
#include "RageShaderWeakRef.h"
#include <unordered_map>
#include <memory>
#include <functional>
#include <queue>
#include <optional>

using RageCompiledShaderFactory =
  std::function<std::unique_ptr<RageShader>(const std::string&)>;

class RageShaderHandler
{
  public:
	RageShaderHandler(RageDisplayType displayType,
					  RageShaderType shaderType,
					  RageCompiledShaderFactory shaderFactory);

	bool IsShaderInCache(size_t shaderLookupKey) const;
	bool IsDefaultShaderInCache(size_t shaderLookupKey) const;

	RageShaderWeakRef GetOrCreateShaderFromPath(const std::string& path,
												bool useAsDefaultShader);
	bool TryRemoveShaderFromCache(RageShaderWeakRef shader);
	bool TrySetActiveShader(RageShader* shader);

	// CONVENTION (notes for myself): ownership is not passed to the caller
	RageShader* GetCurrentShader();

	void PushUniform(const RageUniformCollection& uniform);
	std::optional<RageUniformCollection> TryPopUniform();

  private:
	std::unordered_map<size_t, std::unique_ptr<RageShader>> m_ShaderCache;
	std::unordered_map<size_t, std::unique_ptr<RageShader>>
	  m_DefaultShaderCache;

	// NOTE: a lot of copies?
	std::queue<RageUniformCollection> m_UniformQueue;

	const RageDisplayType m_DisplayType;
	const RageShaderType m_ShaderType;
	const RageCompiledShaderFactory m_ShaderFactory;

	RageShader* m_CurrentShader;

	size_t GetShaderLookupKey(const std::string& path);
	std::hash<std::string> m_StringHasher;
};

#endif
