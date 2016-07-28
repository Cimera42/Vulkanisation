#ifndef ASSORTED_H_INCLUDED
#define ASSORTED_H_INCLUDED

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

struct MemoryBuffer
{
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    void destroy();
};

uint32_t getMemoryTypeIndex(uint32_t inMemType, VkMemoryPropertyFlags desiredFlags);
bool createBuffer(VkDeviceSize memSize, VkBufferUsageFlagBits usageFlags, const void* data, MemoryBuffer *buffer);
void setImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange);
#endif // ASSORTED_H_INCLUDED
