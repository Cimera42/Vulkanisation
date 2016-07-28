#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct Texture
{
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureView;
    VkSampler sampler;

    void destroy();
    bool loadTexture(std::string filename);
    bool loadTextureArray(std::vector<std::string> filenames);
};

#endif // TEXTURE_H_INCLUDED
