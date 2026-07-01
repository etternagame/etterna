#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __unix__
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "VkUtils.h"
#include <fmt/format.h>
#include <fstream>
#include <sstream>
#include "Core/Services/Locator.hpp"
#include <Etterna/Globals/global.h>
#include <shaderc/shaderc.hpp>
#include <RageUtil/File/RageFile.h>

void
ThrowIfFail(VkResult result, const std::source_location location)
{
	if (result == VK_SUCCESS) {
		return;
	}

	const std::string message =
	  fmt::format("RendererVK failed: VkResult {} at {}:{} in function {}",
				  static_cast<int>(result),
				  location.file_name(),
				  location.line(),
				  location.function_name());
	Locator::getLogger()->error(message);
	throw std::runtime_error(message.c_str());
}

void
ThrowIfFail(vk::Result result, const std::source_location location)
{
	ThrowIfFail(static_cast<VkResult>(result), location);
}

void
Fail(const std::source_location location)
{
	const std::string message =
	  fmt::format("RendererVK failed at {}:{} in function {}",
				  location.file_name(),
				  location.line(),
				  location.function_name());
	Locator::getLogger()->error(message);
	throw std::runtime_error(message.c_str());
}

std::vector<uint32_t>
CompileShader(const std::string& sourceName,
			  shaderc_shader_kind shaderKind,
			  const std::string& source)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	auto result = compiler.CompileGlslToSpv(
	  source, shaderKind, sourceName.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		auto message = fmt::format("Vulkan GLSL shader compilation failed: {}",
								   result.GetErrorMessage());
		Locator::getLogger()->error(message);
		sm_crash(message.c_str());
	}

	return { result.begin(), result.end() };
}

vk::raii::ShaderModule
LoadShaderFromFile(std::string path,
				   vk::raii::Device& device,
				   ShaderType shaderType)
{
	RageFile file;

	shaderc_shader_kind shaderKind = {};
	switch (shaderType) {
		case ShaderType_Vertex:
			shaderKind = shaderc_vertex_shader;
			break;
		case ShaderType_Fragment:
			shaderKind = shaderc_fragment_shader;
			break;
		default:
			assert(false && "Invalid shader type specified!");
	}

	if (!file.Open(path)) {
		throw std::runtime_error(file.GetError());
	}

	std::string contents;
	if (file.Read(contents) == -1) {
		throw std::runtime_error("Failed to read shader file at " + path);
	}

	auto shaderBlob = CompileShader("meow", shaderKind, contents);

	vk::ShaderModuleCreateInfo createInfo = {};
	createInfo.codeSize = shaderBlob.size() * sizeof(uint32_t);
	createInfo.pCode = shaderBlob.data();
	
	return vk::raii::ShaderModule(device, createInfo);
}

std::optional<uint32_t>
GetMemoryType(uint32_t typeBits,
			  vk::MemoryPropertyFlags neededProps,
			  vk::PhysicalDeviceMemoryProperties memoryProps)
{
	for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((memoryProps.memoryTypes[i].propertyFlags & neededProps) ==
				neededProps) {
				return i;
			}
		}
		typeBits >>= 1;
	}

	return std::nullopt;
}
