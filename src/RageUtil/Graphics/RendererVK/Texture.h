#ifndef RENDERER_VK_TEXTURE_H
#define RENDERER_VK_TEXTURE_H

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>
#include <RageUtil/Graphics/RageSurface.h>

struct Texture
{
	enum
	{
		Wrapping = 0b01,
		Filtering = 0b10,
		PossibleSamplerCount = 4,
		MaxTextures = 1 << 15,
	};
	VmaAllocation allocation = nullptr;
	VmaAllocator allocator = nullptr;
	vk::Image image = {};
	vk::ImageView view = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
	bool initialized = false;
	vk::ImageLayout currentLayout = vk::ImageLayout::eUndefined;
};

#endif
