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
#include <spirv_cross/spirv_cross_c.h>
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

void
ThrowIfFail(
  spvc_result result,
  const std::source_location location = std::source_location::current())
{
	if (result == spvc_result::SPVC_SUCCESS) {
		return;
	}

	const std::string message = fmt::format(
	  "RendererVK failed: SPIRV compiler result {} at {}:{} in function {}",
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
	  fmt::format("Vulkan: SPIRV reflection failed with message: {}", error);
	Locator::getLogger()->error("{}", message);
}

void
GetShaderReflectionInfo(std::vector<uint32_t> spirvBlob)
{
	// turns out some versions of Vulkan SDK actually include spirv-cross, but..
	// CMake can't find them (unless some things are hacked around)
	// aaaand even if spirv-cross is vendored, there might be conflicts which
	// surface only at runtime so let's use the C API instead (can't be bothered
	// to fix whatever instability the C++ has)

	spvc_context context = nullptr;

	try {
		ThrowIfFail(spvc_context_create(&context));
		spvc_context_set_error_callback(context, SpirvErrorCallback, nullptr);

		spvc_parsed_ir ir = nullptr;
		ThrowIfFail(spvc_context_parse_spirv(
		  context, spirvBlob.data(), spirvBlob.size(), &ir));

		spvc_compiler glslCompiler = nullptr;
		ThrowIfFail(
		  spvc_context_create_compiler(context,
									   SPVC_BACKEND_GLSL,
									   ir,
									   SPVC_CAPTURE_MODE_TAKE_OWNERSHIP,
									   &glslCompiler));

		spvc_resources resources = nullptr;
		ThrowIfFail(
		  spvc_compiler_create_shader_resources(glslCompiler, &resources));

		const spvc_reflected_resource* resourceList = nullptr;
		size_t resourceListSize = 0;
		ThrowIfFail(spvc_resources_get_resource_list_for_type(
		  resources,
		  spvc_resource_type::SPVC_RESOURCE_TYPE_PUSH_CONSTANT,
		  &resourceList,
		  &resourceListSize));

		const spvc_reflected_resource* shaderArgsResource = nullptr;
		for (size_t i = 0; i < resourceListSize; i++) {
			if (std::string(resourceList[i].name) == "__shaderArgs") {
				shaderArgsResource = &resourceList[i];
			}
		}

		if (shaderArgsResource == nullptr) {
			Locator::getLogger()->error(
			  "Vulkan SPIRV: failed to find the required __shaderArgs push "
			  "constant field");
			Fail();
		}

		spvc_type shaderArgsBaseTypeHandle = spvc_compiler_get_type_handle(
		  glslCompiler, shaderArgsResource->base_type_id);

		spvc_basetype shaderArgsBaseType =
		  spvc_type_get_basetype(shaderArgsBaseTypeHandle);

		if (shaderArgsBaseType != SPVC_BASETYPE_STRUCT) {
			Locator::getLogger()->error(
			  "Vulkan SPIRV: __shaderArgs must be a struct");
			Fail();
		}

		spvc_type shaderArgsTypeHandle = spvc_compiler_get_type_handle(
		  glslCompiler, shaderArgsResource->base_type_id);

		size_t memberCount =
		  spvc_type_get_num_member_types(shaderArgsTypeHandle);

		spvc_type vertexArgsType = nullptr;
		spvc_type fragmentArgsType = nullptr;

		for (size_t i = 0; i < memberCount; i++) {
			spvc_type_id memberId =
			  spvc_type_get_member_type(shaderArgsTypeHandle, i);
			spvc_type memberTypeHandle =
			  spvc_compiler_get_type_handle(glslCompiler, memberId);

			std::string memberName = spvc_compiler_get_member_name(
			  glslCompiler, shaderArgsResource->base_type_id, i);
			SpvStorageClass memberStorageClass =
			  spvc_type_get_storage_class(memberTypeHandle);

			if (memberStorageClass !=
				SpvStorageClass::SpvStorageClassPhysicalStorageBuffer) {
				Locator::getLogger()->error("Vulkan SPIRV: __shaderArgs member "
											"{} is not a buffer reference",
											memberName);
				Fail();
			}

			if (memberName == "vertexArgs") {
				vertexArgsType = memberTypeHandle;
			} else if (memberName == "fragmentArgs") {
				fragmentArgsType = memberTypeHandle;
			} else {
				Locator::getLogger()->error(
				  "Vulkan SPIRV: __shaderArgs contains a stray member {}",
				  memberName);
				Fail();
			}
		}

		if (vertexArgsType == nullptr || fragmentArgsType == nullptr) {
			Locator::getLogger()->error(
			  "Vulkan SPIRV: __shaderArgs must have two buffer references "
			  "(vertexArgs and fragmentArgs)");
			Fail();
		}

		spvc_basetype vertexArgsBaseType =
		  spvc_type_get_basetype(vertexArgsType);

		if (vertexArgsBaseType != SPVC_BASETYPE_STRUCT) {
			Locator::getLogger()->error("Vulkan SPIRV: vertexArgs must be a "
										"buffer reference of basetype struct");
			Fail();
		}

		spvc_basetype fragmentArgsBaseType =
		  spvc_type_get_basetype(fragmentArgsType);

		if (fragmentArgsBaseType != SPVC_BASETYPE_STRUCT) {
			Locator::getLogger()->error("Vulkan SPIRV: fragmentArgs must be a "
										"buffer reference of basetype struct");
			Fail();
		}

		size_t vertexArgsCount = spvc_type_get_num_member_types(vertexArgsType);
		for (size_t i = 0; i < vertexArgsCount; i++) {
			std::string memberName = spvc_compiler_get_member_name(
			  glslCompiler, spvc_type_get_base_type_id(vertexArgsType), i);
			Locator::getLogger()->debug(
			  "Vulkan SPIRV: vertexArgs has member {}", memberName);
		}

		size_t fragmentArgsCount =
		  spvc_type_get_num_member_types(fragmentArgsType);
		for (size_t i = 0; i < fragmentArgsCount; i++) {
			std::string memberName = spvc_compiler_get_member_name(
			  glslCompiler, spvc_type_get_base_type_id(fragmentArgsType), i);
			Locator::getLogger()->debug(
			  "Vulkan SPIRV: fragmentArgs has member {}", memberName);
		}
	} catch (...) {
		if (context != nullptr) {
			spvc_context_destroy(context);
			context = nullptr;
		}
		throw;
	}
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
	GetShaderReflectionInfo(CompileShader(
	  "reflection_meow", shaderKind, contents, true));

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
