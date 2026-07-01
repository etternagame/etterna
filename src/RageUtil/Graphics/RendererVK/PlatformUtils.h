#ifndef RENDERER_VK_PLATFORM_UTILS_H
#define RENDERER_VK_PLATFORM_UTILS_H

#ifdef DEBUG
#define VKDEBUG 1
#endif
#ifdef _DEBUG
#define VKDEBUG 1
#endif

#include <vulkan/vulkan_raii.hpp>
#include <VkBootstrap.h>

vk::raii::SurfaceKHR
CreateSurfaceKHR(const vk::raii::Instance& instance);

vkb::Result<vkb::Instance>
CreateInstance(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback);

#endif