#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef __unix__
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "PersistentBuffer.h"
#include "VkUtils.h"

void
PersistentBuffer::Init(VmaAllocator allocator,
					   const vk::BufferCreateInfo& createInfo,
					   const VmaAllocationCreateInfo& allocInfo)
{
	this->allocator = allocator;
	ThrowIfFail(
	  vmaCreateBuffer(allocator,
					  &static_cast<const VkBufferCreateInfo&>(createInfo),
					  &allocInfo,
					  &this->buffer,
					  &this->allocation,
					  &this->allocInfo));
}

void
PersistentBuffer::Destroy()
{
	if (buffer == VK_NULL_HANDLE) {
		return;
	}

	vmaDestroyBuffer(allocator, buffer, allocation);

	buffer = VK_NULL_HANDLE;
	allocation = VK_NULL_HANDLE;
	allocInfo = {};
	allocator = nullptr;
	gpuAddress = 0;
}

vk::Buffer
PersistentBuffer::Get() const
{
	return vk::Buffer(buffer);
}

void*
PersistentBuffer::GetMappedData() const
{
	return allocInfo.pMappedData;
}
