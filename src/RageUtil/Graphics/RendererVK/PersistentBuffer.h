#ifndef RENDERER_VK_BUFFER_HELPER_H
#define RENDERER_VK_BUFFER_HELPER_H

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

struct PersistentBuffer
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	VmaAllocationInfo allocInfo = {};
	VmaAllocator allocator = nullptr;
	uint64_t gpuAddress = 0;

	void Init(VmaAllocator allocator,
			  const vk::BufferCreateInfo& createInfo,
			  const VmaAllocationCreateInfo& allocInfo);

	vk::Buffer Get() const;

	void* GetMappedData() const;
};

#endif
