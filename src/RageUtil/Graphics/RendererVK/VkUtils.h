#ifndef VK_UTILS_H
#define VK_UTILS_H

#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>
#include <deque>
#include <functional>
#include <source_location>
#include <span>
#include <optional>
#include <RageUtil/Misc/RageTypes.h>

void
ThrowIfFail(
  VkResult result,
  const std::source_location location = std::source_location::current());

void
ThrowIfFail(
  vk::Result result,
  const std::source_location location = std::source_location::current());

void
Fail(const std::source_location location = std::source_location::current());

vk::raii::ShaderModule
LoadShaderFromFile(std::string path,
				   vk::raii::Device& device,
				   ShaderType shaderType);

std::optional<uint32_t>
GetMemoryType(uint32_t typeBits,
			  vk::MemoryPropertyFlags neededProps,
			  vk::PhysicalDeviceMemoryProperties memoryProps);

template <typename T>
std::string
GetDetailedErrorString(vkb::Result<T>& result)
{
	std::string reason;
	auto& reasons = result.detailed_failure_reasons();
	
	for (int i = 0; i < reasons.size(); i++) {
		reason += reasons[i];
		if (i != reasons.size() - 1) {
			reason += " ; ";
		}
	}

	if (reasons.empty()) {
		reason = "No reason given";
	}

	return reason;
}

#endif
