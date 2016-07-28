#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <limits>
#include <algorithm>
#include <iostream>

#include "vulkanDefinitions.h"
#include "assorted.h"

extern VkDevice logicalDevice;
extern VkPhysicalDeviceMemoryProperties memoryProperties;
extern VkCommandPool commandPool;
extern VkQueue presentQueue;

void Texture::destroy()
{
    vkDestroyImage(logicalDevice, textureImage, NULL);
    vkDestroyImageView(logicalDevice, textureView, NULL);
    vkDestroySampler(logicalDevice, sampler, NULL);
    vkFreeMemory(logicalDevice, textureImageMemory, NULL);
}

bool Texture::loadTexture(std::string filename)
{
    int width,height,channels;
    unsigned char *preloadedImage = stbi_load(filename.c_str(),&width,&height,&channels,STBI_rgb);
    std::vector<float> loadedImage(width * height * 3);
    for(int i = 0; i < width * height * 3; i++)
    {
        loadedImage[i] = (float) preloadedImage[i]/255;
    }

    VkImageCreateInfo textureCreateInfo = {};
    textureCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    textureCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    textureCreateInfo.format = VK_FORMAT_R32G32B32_SFLOAT;
    textureCreateInfo.extent = { width, height, 1 };
    textureCreateInfo.mipLevels = 1;
    textureCreateInfo.arrayLayers = 1;
    textureCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    textureCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    textureCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    textureCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    VkResult result = vkCreateImage(logicalDevice, &textureCreateInfo, NULL, &textureImage);

    VkMemoryRequirements textureMemoryRequirements = {};
    vkGetImageMemoryRequirements(logicalDevice, textureImage, &textureMemoryRequirements);

    VkMemoryAllocateInfo textureImageAllocateInfo = {};
    textureImageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    textureImageAllocateInfo.allocationSize = textureMemoryRequirements.size;

    uint32_t textureMemoryTypeBits = textureMemoryRequirements.memoryTypeBits;
    VkMemoryPropertyFlags tDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    for( uint32_t i = 0; i < 32; ++i)
    {
        VkMemoryType memoryType = memoryProperties.memoryTypes[i];
        if(textureMemoryTypeBits & 1)
        {
            if((memoryType.propertyFlags & tDesiredMemoryFlags) == tDesiredMemoryFlags)
            {
                textureImageAllocateInfo.memoryTypeIndex = i;
                break;
            }
        }
        textureMemoryTypeBits = textureMemoryTypeBits >> 1;
    }

    result = vkAllocateMemory(logicalDevice, &textureImageAllocateInfo, NULL, &textureImageMemory);
    result = vkBindImageMemory(logicalDevice, textureImage, textureImageMemory, 0);

    void *imageMapped;
    result = vkMapMemory(logicalDevice, textureImageMemory, 0, VK_WHOLE_SIZE, 0, &imageMapped );

    memcpy(imageMapped, loadedImage.data(), sizeof(float) * width * height * 3);

    VkMappedMemoryRange memoryRange = {};
    memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memoryRange.memory = textureImageMemory;
    memoryRange.offset = 0;
    memoryRange.size = VK_WHOLE_SIZE;
    result = vkFlushMappedMemoryRanges(logicalDevice, 1, &memoryRange);

    vkUnmapMemory(logicalDevice, textureImageMemory);


    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer copyCmd;
    vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &copyCmd);

	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(copyCmd, &cmdBufferBeginInfo);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;
    setImageLayout(copyCmd, textureImage, VK_IMAGE_ASPECT_COLOR_BIT,
                   VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   subresourceRange);

    vkEndCommandBuffer(copyCmd);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;
    vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(presentQueue);
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &copyCmd);


    VkImageViewCreateInfo textureImageViewCreateInfo = {};
    textureImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    textureImageViewCreateInfo.image = textureImage;
    textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    textureImageViewCreateInfo.format = VK_FORMAT_R32G32B32_SFLOAT;
    textureImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
                                              VK_COMPONENT_SWIZZLE_G,
                                              VK_COMPONENT_SWIZZLE_B,
                                              VK_COMPONENT_SWIZZLE_A };
    textureImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    textureImageViewCreateInfo.subresourceRange.levelCount = 1;
    textureImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    textureImageViewCreateInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(logicalDevice, &textureImageViewCreateInfo, NULL, &textureView);

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.minLod = 0;
    samplerCreateInfo.maxLod = 5;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    result = vkCreateSampler(logicalDevice, &samplerCreateInfo, NULL, &sampler);

    return true;
}

bool Texture::loadTextureArray(std::vector<std::string> filenames)
{
    std::vector<float> loadedImages;
    std::vector<int> widths(filenames.size());
    std::vector<int> heights(filenames.size());
    int totalSize = 0;
    for(int j = 0; j < filenames.size(); j++)
    {
        int width,height,channels;
        unsigned char *preloadedImage = stbi_load(filenames[j].c_str(),&width,&height,&channels,STBI_rgb);
        widths[j] = width;
        heights[j] = height;
        for(int i = 0; i < width * height * 3; i++)
        {
            loadedImages.push_back((float) preloadedImage[i]/255);
        }
        totalSize += width*height*3;
    }


    VkMemoryAllocateInfo stagingMemoryAllocateInfo = {};
    stagingMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements stagingMemoryRequirements;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

	VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(float) * totalSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(logicalDevice, &bufferCreateInfo, NULL, &stagingBuffer);

    vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &stagingMemoryRequirements);
    stagingMemoryAllocateInfo.allocationSize = stagingMemoryRequirements.size;
    stagingMemoryAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    result = vkAllocateMemory(logicalDevice, &stagingMemoryAllocateInfo, NULL, &stagingMemory);
    result = vkBindBufferMemory(logicalDevice, stagingBuffer, stagingMemory, 0);

    void* stagingMapped;
    result = vkMapMemory(logicalDevice, stagingMemory, 0, stagingMemoryRequirements.size, 0, &stagingMapped);
    memcpy(stagingMapped, loadedImages.data(), sizeof(float) * totalSize);
    vkUnmapMemory(logicalDevice, stagingMemory);

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    int offset = 0;
    for(int i = 0; i < filenames.size(); i++)
    {
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = 0;
        bufferCopyRegion.imageSubresource.baseArrayLayer = i;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = widths[i];
        bufferCopyRegion.imageExtent.height = heights[i];
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;
        bufferCopyRegions.push_back(bufferCopyRegion);

        offset += sizeof(float) * widths[i] * heights[i] * 3;
    }


    VkImageCreateInfo textureCreateInfo = {};
    textureCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    textureCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    textureCreateInfo.format = VK_FORMAT_R32G32B32_SFLOAT;
    textureCreateInfo.extent = {*std::max_element(widths.begin(), widths.end()),
                                *std::max_element(heights.begin(), heights.end()), 1};
    textureCreateInfo.mipLevels = 1;
    textureCreateInfo.arrayLayers = filenames.size();
    textureCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    textureCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    textureCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    textureCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    textureCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    result = vkCreateImage(logicalDevice, &textureCreateInfo, NULL, &textureImage);

    VkMemoryRequirements textureMemoryRequirements = {};
    vkGetImageMemoryRequirements(logicalDevice, textureImage, &textureMemoryRequirements);

    VkMemoryAllocateInfo textureImageAllocateInfo = {};
    textureImageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    textureImageAllocateInfo.allocationSize = textureMemoryRequirements.size;
    textureImageAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(textureMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = vkAllocateMemory(logicalDevice, &textureImageAllocateInfo, NULL, &textureImageMemory);
    result = vkBindImageMemory(logicalDevice, textureImage, textureImageMemory, 0);


	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;

    VkCommandBuffer copyCmd;
    vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, &copyCmd);

	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(copyCmd, &cmdBufferBeginInfo);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = filenames.size();
    setImageLayout(copyCmd, textureImage, VK_IMAGE_ASPECT_COLOR_BIT,
                   VK_IMAGE_LAYOUT_PREINITIALIZED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   subresourceRange);

    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, textureImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           bufferCopyRegions.size(), bufferCopyRegions.data());

    setImageLayout(copyCmd, textureImage, VK_IMAGE_ASPECT_COLOR_BIT,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                   subresourceRange);

    vkEndCommandBuffer(copyCmd);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCmd;
    vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(presentQueue);
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &copyCmd);


    VkImageViewCreateInfo textureImageViewCreateInfo = {};
    textureImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    textureImageViewCreateInfo.image = textureImage;
    textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    textureImageViewCreateInfo.format = VK_FORMAT_R32G32B32_SFLOAT;
    textureImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
                                              VK_COMPONENT_SWIZZLE_G,
                                              VK_COMPONENT_SWIZZLE_B,
                                              VK_COMPONENT_SWIZZLE_A };
    textureImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    textureImageViewCreateInfo.subresourceRange.levelCount = 1;
    textureImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    textureImageViewCreateInfo.subresourceRange.layerCount = filenames.size();

    result = vkCreateImageView(logicalDevice, &textureImageViewCreateInfo, NULL, &textureView);

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.minLod = 0;
    samplerCreateInfo.maxLod = 5;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    result = vkCreateSampler(logicalDevice, &samplerCreateInfo, NULL, &sampler);

    vkFreeMemory(logicalDevice, stagingMemory, NULL);
    vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);

    return true;
}
