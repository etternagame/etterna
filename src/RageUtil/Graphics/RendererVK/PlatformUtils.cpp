#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __unix__
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include "PlatformUtils.h"

#ifdef _WIN32
#include "archutils/Win32/GraphicsWindow.h"
#endif
#ifdef __unix__
#include "archutils/Unix/X11Helper.h"
#endif

vkb::Result<vkb::Instance>
CreateInstance(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback)
{
	vkb::InstanceBuilder builder;
	auto instanceResult =
	  builder
#ifdef VKDEBUG
		.request_validation_layers(true)
		.use_default_debug_messenger()
		.add_validation_feature_enable(
		  VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT)
		.add_validation_feature_enable(
		  VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT)
		.add_validation_feature_enable(
		  VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
		.set_debug_callback(debugCallback)
#endif
		.require_api_version(1, 3, 0)
		.enable_extension(VK_KHR_SURFACE_EXTENSION_NAME)
#ifdef _WIN32
		.enable_extension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME)
		.enable_extension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME)
#endif
#ifdef __unix__
		.enable_extension(VK_KHR_XLIB_SURFACE_EXTENSION_NAME)
#endif
#ifdef __APPLE__
		.enable_extension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)
		.enable_extension(
		  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)
#endif
		.build();

	return instanceResult;
}

vk::raii::SurfaceKHR
CreateSurfaceKHR(const vk::raii::Instance& instance)
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = GraphicsWindow::GetHwnd();
	createInfo.hinstance = GetModuleHandle(nullptr);
	return instance.createWin32SurfaceKHR(createInfo);
#endif
#ifdef __unix__
	VkXlibSurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.dpy = X11Helper::Dpy;
	createInfo.window = X11Helper::Win;

	return instance.createXlibSurfaceKHR(createInfo);
#endif
#ifdef __APPLE__
	// TODO: use vkCreateMacOSSurfaceMVK or vkCreateMetalSurfaceEXT?
	return nullptr;
#endif
}
