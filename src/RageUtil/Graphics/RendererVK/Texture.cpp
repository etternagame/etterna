#include "Texture.h"

void
Texture::InitImageBuffer()
{
	VmaAllocationCreateInfo textureAllocInfo = {};
	textureAllocInfo.flags =
	  VMA_ALLOCATION_CREATE_MAPPED_BIT |
	  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	textureAllocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	textureAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	vk::BufferCreateInfo textureInfo = {};
	textureInfo.size = width * height * sizeof(uint32_t);
	textureInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
	imageBuffer.Init(allocator, textureInfo, textureAllocInfo);
}

void
Texture::DestroyImageBuffer()
{
	imageBuffer.Destroy();
}
