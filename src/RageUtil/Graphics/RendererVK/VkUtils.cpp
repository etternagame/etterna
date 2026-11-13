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
#include <spirv_reflect.h>
#include <RageUtil/Graphics/RageDisplay.h>

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
	Locator::getLogger()->error("{}", message);
	throw std::runtime_error(message.c_str());
}

void
ThrowIfFail(vk::Result result, const std::source_location location)
{
	ThrowIfFail(static_cast<VkResult>(result), location);
}

static void
ThrowIfFail(
  SpvReflectResult result,
  const std::source_location location = std::source_location::current())
{
	if (result == SpvReflectResult::SPV_REFLECT_RESULT_SUCCESS) {
		return;
	}

	const std::string message = fmt::format(
	  "RendererVK failed: SPIRV-Reflect result {} at {}:{} in function {}",
	  static_cast<int>(result),
	  location.file_name(),
	  location.line(),
	  location.function_name());
	Locator::getLogger()->error("{}", message);
	throw std::runtime_error(message.c_str());
}

void
Fail(const std::source_location location)
{
	const std::string message =
	  fmt::format("RendererVK failed at {}:{} in function {}",
				  location.file_name(),
				  location.line(),
				  location.function_name());
	Locator::getLogger()->error("{}", message);
	throw std::runtime_error(message.c_str());
}

std::vector<uint32_t>
CompileShader(const std::string& sourceName,
			  shaderc_shader_kind shaderKind,
			  const std::string& source,
			  bool debug)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	options.SetTargetEnvironment(shaderc_target_env_vulkan,
								 shaderc_env_version_vulkan_1_3);

	if (debug) {
		options.SetGenerateDebugInfo();
	}

	auto result = compiler.CompileGlslToSpv(
	  source, shaderKind, sourceName.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		auto message = fmt::format("Vulkan GLSL shader compilation failed: {}",
								   result.GetErrorMessage());
		Locator::getLogger()->error("{}", message);
		sm_crash(message.c_str());
	}

	return { result.begin(), result.end() };
}

void
SpirvErrorCallback(void* userdata, const char* error)
{
	auto message =
	  fmt::format("Vulkan: SPIRV-Reflect failed with message: {}", error);
	Locator::getLogger()->error("{}", message);
}

void
PrintBlockReflectionInfo(const SpvReflectBlockVariable& variable,
						 std::string inside)
{
	Locator::getLogger()->info("Vulkan SPIRV-Reflect: found member {} (inside "
							   "{}): type {} size {} offset {}",
							   variable.name ? variable.name : "!unknown!",
							   inside,
							   variable.type_description->type_name
								 ? variable.type_description->type_name
								 : "!unknown!",
							   variable.size,
							   variable.offset);

	for (size_t i = 0; i < variable.member_count; i++) {
		PrintBlockReflectionInfo(variable.members[i],
								 variable.name ? variable.name : "!unknown!");
	}
}

void
GetShaderReflectionInfo(std::vector<uint32_t> spirvBlob)
{
	// turns out some versions of Vulkan SDK actually include
	// spirv-cross/spirv-reflect, but...
	// CMake can't find them (unless some things are hacked around)
	//
	// can't be bothered to patch the rest and deal
	// with OS specific SDK file layouts so here we go

	SpvReflectShaderModule module{};

	try {
		ThrowIfFail(spvReflectCreateShaderModule(
		  spirvBlob.size() * sizeof(uint32_t), spirvBlob.data(), &module));

		uint32_t pushConstantCount = 0;
		ThrowIfFail(spvReflectEnumeratePushConstants(
		  &module, &pushConstantCount, nullptr));

		std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
		ThrowIfFail(spvReflectEnumeratePushConstants(
		  &module, &pushConstantCount, &pushConstants[0]));

		SpvReflectBlockVariable* shaderArgsVariable = nullptr;
		for (const auto& variable : pushConstants) {
			if (variable->name == std::string("__shaderArgs")) {
				shaderArgsVariable = variable;
			}
		}

		if (shaderArgsVariable == nullptr) {
			Locator::getLogger()->error("Vulkan SPIRV-Reflect: failed to find "
										"the required __shaderArgs push "
										"constant field");
			Fail();
		}

		SpvReflectBlockVariable* vertexArgsVariable = nullptr;
		SpvReflectBlockVariable* fragmentArgsVariable = nullptr;

		for (size_t i = 0; i < shaderArgsVariable->member_count; i++) {
			if (shaderArgsVariable->members[i].name ==
				std::string("vertexArgs")) {
				vertexArgsVariable = &shaderArgsVariable->members[i];
			} else if (shaderArgsVariable->members[i].name ==
					   std::string("fragmentArgs")) {
				fragmentArgsVariable = &shaderArgsVariable->members[i];
			} else {
				Locator::getLogger()->error(
				  "Vulkan SPIRV-Reflect: __shaderArgs contains a stray member "
				  "{}",
				  shaderArgsVariable->members[i].name);
				Fail();
			}
		}

		if (vertexArgsVariable == nullptr || fragmentArgsVariable == nullptr) {
			Locator::getLogger()->error("Vulkan SPIRV-Reflect: __shaderArgs "
										"must have two buffer references "
										"(vertexArgs and fragmentArgs)");
			Fail();
		}

		if (!(vertexArgsVariable->type_description->type_flags &
			  (SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_REF |
			   SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT |
			   SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK))) {
			Locator::getLogger()->error(
			  "Vulkan SPIRV-Reflect: vertexArgs should be a buffer reference");
			Fail();
		}

		if (!(fragmentArgsVariable->type_description->type_flags &
			  (SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_REF |
			   SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT |
			   SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK))) {
			Locator::getLogger()->error(
			  "Vulkan SPIRV-Reflect: fragmentArgs should be a buffer reference");
			Fail();
		}

		PrintBlockReflectionInfo(*vertexArgsVariable, "__shaderArgs");
		PrintBlockReflectionInfo(*fragmentArgsVariable, "__shaderArgs");
	} catch (...) {
		spvReflectDestroyShaderModule(&module);
		throw;
	}

	spvReflectDestroyShaderModule(&module);
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

	auto shaderBlob = CompileShader("meow", shaderKind, contents, false);
	GetShaderReflectionInfo(
	  CompileShader("reflection_meow", shaderKind, contents, true));

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
