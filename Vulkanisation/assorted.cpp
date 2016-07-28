#include "assorted.h"

#include <iostream> //cout
#include <cstring> //memcpy
#include "vulkanDefinitions.h"

extern VkDevice logicalDevice;
extern VkPhysicalDeviceMemoryProperties memoryProperties;

void MemoryBuffer::destroy()
{
    vkDestroyBuffer(logicalDevice, buffer, NULL);
    vkFreeMemory(logicalDevice, bufferMemory, NULL);
}

uint32_t getMemoryTypeIndex(uint32_t inMemType, VkMemoryPropertyFlags desiredFlags)
{
    uint32_t memoryTypeBits = inMemType;
    VkMemoryPropertyFlags desiredMemoryFlags = desiredFlags;
    for(uint32_t i = 0; i < 32; ++i)
    {
        VkMemoryType memoryType = memoryProperties.memoryTypes[i];
        if(memoryTypeBits & 1)
        {
            if((memoryType.propertyFlags & desiredMemoryFlags) == desiredMemoryFlags)
            {
                return i;
            }
        }
        memoryTypeBits = memoryTypeBits >> 1;
    }
    return 0;
}

bool createBuffer(VkDeviceSize memSize, VkBufferUsageFlagBits usageFlags, const void* data, MemoryBuffer *buffer)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = memSize;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(logicalDevice, &bufferInfo, NULL,
                             &buffer->buffer);
    if(result != VK_SUCCESS)
    {
        std::cout << "Buffer creation failed (" << result << ")" << std::endl;
        return false;
    }

    VkMemoryRequirements bufferMemoryRequirements = {};
    vkGetBufferMemoryRequirements(logicalDevice, buffer->buffer,
                                   &bufferMemoryRequirements);

    VkMemoryAllocateInfo bufferAllocateInfo = {};
    bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    bufferAllocateInfo.allocationSize = bufferMemoryRequirements.size;
    bufferAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(bufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    result = vkAllocateMemory(logicalDevice, &bufferAllocateInfo, NULL, &buffer->bufferMemory);
    if(result != VK_SUCCESS)
    {
        std::cout << "Buffer memory allocation failed (" << result << ")" << std::endl;
        return false;
    }

    void *mapped;
    result = vkMapMemory(logicalDevice, buffer->bufferMemory, 0, VK_WHOLE_SIZE, 0, &mapped);
    if(result != VK_SUCCESS)
    {
        std::cout << "Memory buffer mapping failed (" << result << ")" << std::endl;
        return false;
    }

    memcpy(mapped, data, memSize);
    vkUnmapMemory(logicalDevice, buffer->bufferMemory);

    result = vkBindBufferMemory(logicalDevice, buffer->buffer, buffer->bufferMemory, 0);
    if(result != VK_SUCCESS)
    {
        std::cout << "Memory buffer bind failed (" << result << ")" << std::endl;
        return false;
    }

    return true;
}

//Taken from VulkanTools
void setImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange)
	{
		// Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = NULL;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)

		// Undefined layout
		// Only allowed as initial layout!
		// Make sure any writes to the image have been finished
		if (oldImageLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		// Old layout is color attachment
		// Make sure any writes to the color buffer have been finished
		if (oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}

		// Old layout is depth/stencil attachment
		// Make sure any writes to the depth/stencil buffer have been finished
		if (oldImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}

		// Old layout is transfer source
		// Make sure any reads from the image have been finished
		if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		// Old layout is shader read (sampler, input attachment)
		// Make sure any shader reads from the image have been finished
		if (oldImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		// Target layouts (new)

		// New layout is transfer destination (copy, blit)
		// Make sure any copyies to the image have been finished
		if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		// New layout is transfer source (copy, blit)
		// Make sure any reads from and writes to the image have been finished
		if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		// New layout is color attachment
		// Make sure any writes to the color buffer hav been finished
		if (newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		}

		// New layout is depth attachment
		// Make sure any writes to depth/stencil buffer have been finished
		if (newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}

		// New layout is shader read (sampler, input attachment)
		// Make sure any writes to the image have been finished
		if (newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		}

		// Put barrier on top
		VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageFlags,
			destStageFlags,
			0,
			0, NULL,
			0, NULL,
			1, &imageMemoryBarrier);
}
