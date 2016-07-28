#include "vulkanDefinitions.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <iostream>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <cstring>

#include "mesh.h"
#include "assorted.h"
#include "texture.h"

//#define VULKAN_DEBUGGING

GLFWwindow* window;

VkResult result; //Global error variable
VkInstance vulkanInstance;
VkPhysicalDevice mainPhysicalDevice;
VkDevice logicalDevice;
VkSurfaceKHR vulkanSurface;
VkFormat colourFormat;
VkColorSpaceKHR colourSpace;
VkSwapchainKHR swapchain;
VkPhysicalDeviceMemoryProperties memoryProperties;
VkCommandPool commandPool;
VkQueue presentQueue;

std::vector<const char*> layers;
std::vector<const char*> extensions;
VkDeviceQueueCreateInfo queueInfo;
uint32_t queueCount = 0;
uint32_t presentQueueId;
std::vector<VkQueueFamilyProperties> queueProperties;
std::vector<const char*> deviceLayers;
std::vector<const char *> deviceExtensions;

VkExtent2D swapchainExtent;
uint32_t imageCount;
VkPresentModeKHR presentMode;

std::vector<Mesh> meshes;
std::vector<VkDescriptorSet> descriptorSets;
VkRenderPass renderPass;
VkPipeline simplepipeline;
VkPipeline normalpipeline;
VkPipelineLayout pipelineLayout;
std::vector<VkFramebuffer> frameBuffers;
std::vector<VkCommandBuffer> commandBuffers;

//Uniform buffer
struct UniformData
{
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 modelMatrix;
};

struct ShaderParts
{
    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfo;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
};

ShaderParts shader1;
ShaderParts shader2;
std::vector<MemoryBuffer> uniformBuffers;
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

std::vector<VkImage> swapchainImages;
std::vector<VkImageView> imageViews;
VkImageView depthImageView;
VkImage depthImage;
VkDeviceMemory depthImageMemory;

Mesh screenMesh;
ShaderParts screenShader;
VkPipeline screenpipeline;
VkPipelineLayout screenpipelineLayout;
UniformData screenQuadUniformData;
MemoryBuffer screenQuadUniformMemory;
VkDescriptorSet screenQuadDescriptorSet;
VkDescriptorSetLayout screenQuadDescriptorSetLayout;
struct FramebufferImage
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
};
struct FramebufferParts
{
    FramebufferImage colour;
    FramebufferImage depth;

    int width, height;
    VkFramebuffer framebuffer;
    VkSampler colourSampler;
} renderToFramebuffer;

VkDebugReportCallbackEXT debugcallback;

std::string FloattoStr(float a)
{
    std::stringstream ss;
    ss << a;
    return ss.str();
}

VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
    VkDebugReportFlagsEXT       flags,
    VkDebugReportObjectTypeEXT  objectType,
    uint64_t                    object,
    size_t                      location,
    int32_t                     messageCode,
    const char*                 pLayerPrefix,
    const char*                 pMessage,
    void*                       pUserData)
{
    if((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) == VK_DEBUG_REPORT_WARNING_BIT_EXT)
        return VK_TRUE;

    std::cerr << "VULKAN ";

    if((flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) == VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        std::cerr << "INFORMATION " << std::endl;
    if((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) == VK_DEBUG_REPORT_WARNING_BIT_EXT)
        std::cerr << "WARNING " << std::endl;
    if((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) == VK_DEBUG_REPORT_ERROR_BIT_EXT)
        std::cerr << "ERROR " << std::endl;

    std::cerr << "\"" << pMessage << "\"" << std::endl;
    return VK_FALSE;
}

VkResult loadShader(std::string shaderFilename, VkShaderModule * shaderModule)
{
    std::ifstream shaderStream(shaderFilename.c_str(), std::ios::binary);
    std::string shaderCode((std::istreambuf_iterator<char>(shaderStream)),
                           (std::istreambuf_iterator<char>()));

    VkShaderModuleCreateInfo shaderCreationInfo = {};
    shaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreationInfo.codeSize = shaderCode.size();
    shaderCreationInfo.pCode = (uint32_t *)shaderCode.c_str();

    return vkCreateShaderModule(logicalDevice, &shaderCreationInfo, NULL, shaderModule);
}

bool validationLayers()
{
    //Count validation layers to make sure they exist
    uint32_t layerCount = 0;
    result = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Instance layer count failed" << std::endl;
        return false;
    }
    else if(layerCount <= 0)
    {
        std::cout << "No validation layers found" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Validation layers found: " << layerCount << std::endl;
    }
    //Then retrieve them
    std::vector<VkLayerProperties> layersAvailable(layerCount);
    result = vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Instance layer retrieval failed" << std::endl;
        return false;
    }

    std::cout << "Available validation layers: " << std::endl;
    for(int i = 0; i < layerCount; i++)
    {
        std::cout << "Name: " << layersAvailable[i].layerName << std::endl;
    }

    //std::vector<const char*> layers;
    layers.push_back("VK_LAYER_LUNARG_standard_validation");
    bool missingLa = false;
    uint32_t missingLaId;
    for(int i = 0; i < layers.size(); i++)
    {
        bool found = false;
        for(int j = 0; j < layerCount; j++)
        {
            if(strcmp(layers[i], layersAvailable[j].layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            missingLa = true;
            missingLaId = i;
            break;
        }
    }
    if(missingLa)
    {
        std::cout << "Instance layer missing: " << layers[missingLaId] << std::endl;
        return false;
    }
    else
    {
        std::cout << "All instance layer found" << std::endl;
    }

    return true;
}

bool checkAndAddExtensions()
{
    uint32_t extensionCount = 0;
    uint32_t layerextensionCount = 0;
    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Instance extension count failed" << std::endl;
        return false;
    }
#ifdef VULKAN_DEBUGGING
    result = vkEnumerateInstanceExtensionProperties(layers[0], &layerextensionCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Instance layer extension count failed" << std::endl;
        return false;
    }
    else if(extensionCount <= 0)
    {
        std::cout << "No instance extensions found" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Instance extensions found: " << extensionCount << " + " << layerextensionCount << std::endl;
    }
#endif // VULKAN_DEBUGGING
    std::vector<VkExtensionProperties> extensionsAvailable(extensionCount + layerextensionCount);
    result = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionsAvailable.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Instance extension retrieval failed" << std::endl;
        return false;
    }
#ifdef VULKAN_DEBUGGING
    result = vkEnumerateInstanceExtensionProperties(layers[0], &layerextensionCount, &extensionsAvailable[extensionCount]);
    if(result != VK_SUCCESS)
    {
        std::cout << "Instance layer extension retrieval failed" << std::endl;
        return false;
    }
#endif // VULKAN_DEBUGGING

    //std::vector<const char*> extensions;
#ifdef VULKAN_DEBUGGING
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // = "VK_EXT_debug_report"
#endif // VULKAN_DEBUGGING
    uint32_t glfwExtensionCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for(int i = 0; i < glfwExtensionCount; i++)
    {
        extensions.push_back(glfwExtensions[i]);
    }

    bool missingEx = false;
    uint32_t missingExId;
    for(int i = 0; i < extensions.size(); i++)
    {
        bool found = false;
        for(int j = 0; j < extensionCount; j++)
        {
            if(strcmp(extensions[i], extensionsAvailable[j].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            missingEx = true;
            missingExId = i;
            break;
        }
    }
    if(missingEx)
    {
        std::cout << "Instance extension missing: " << extensions[missingExId] << std::endl;
        return false;
    }
    else
    {
        std::cout << "All instance extensions found" << std::endl;
    }

    return true;
}

bool initDebugging()
{
    LOAD_IFUNCTION(vulkanInstance, vkCreateDebugReportCallbackEXT);
    LOAD_IFUNCTION(vulkanInstance, vkDebugReportMessageEXT);

    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext       = NULL;
    callbackCreateInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                     VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                     VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = &MyDebugReportCallback;
    callbackCreateInfo.pUserData   = NULL;

    /* Register the callback */
    //VkDebugReportCallbackEXT debugcallback;
    result = vkCreateDebugReportCallbackEXT(vulkanInstance, &callbackCreateInfo, NULL, &debugcallback);
    if(result != VK_SUCCESS)
    {
        std::cout << "Debug reporting creation failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Debug reporting created" << std::endl;
    }

    return true;
}

bool destroyDebugging()
{
    LOAD_IFUNCTION(vulkanInstance, vkDestroyDebugReportCallbackEXT);
    vkDestroyDebugReportCallbackEXT(vulkanInstance, debugcallback, NULL);

    return true;
}

bool deviceQueue()
{
    //Count device queues
    //uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(mainPhysicalDevice, &queueCount, NULL);
    if(queueCount < 1)
    {
        std::cout << "No device queues" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Physical device queue count: " << queueCount << std::endl;
    }

    //Get device queues after counting
    bool found = false;
    //uint32_t presentQueueId;
    queueProperties.resize(queueCount);
    //std::vector<VkQueueFamilyProperties> queueProperties(queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mainPhysicalDevice, &queueCount, queueProperties.data());
    for(uint32_t i = 0; i < queueCount; ++i)
    {
        VkBool32 supportsPresent;
        vkGetPhysicalDeviceSurfaceSupportKHR(mainPhysicalDevice, i, vulkanSurface, &supportsPresent);

        if((queueProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
        {
            if(supportsPresent == VK_TRUE)
            {
                presentQueueId = i;
                found = true;
                break;
            }
        }
    }
    if(found == false)
    {
        std::cout << "Physical device does not support presenting" << std::endl;
        return false;
    }

    vkGetPhysicalDeviceMemoryProperties(mainPhysicalDevice, &memoryProperties);
    static float priorities[] = { 1.0f }; //0.0-1.0 weighting
    queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; //VkStructureType
    queueInfo.pNext = NULL;                                       //const void
    queueInfo.flags = 0;                                          //VKDeviceQueueCreateFlags
    queueInfo.queueFamilyIndex = presentQueueId;                               //uint32_t
    queueInfo.queueCount = 1;                                     //uint32_t
    queueInfo.pQueuePriorities = priorities;                  //const float*

    return true;
}

bool physicalDevice()
{
    //Count number of Vulkan Supported GPUs
    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Device count failed (" << result << ")" << std::endl;
        return false;
    }
    else if(deviceCount < 1)
    {
        std::cout << "No GPU installed?" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Device count: " << deviceCount << std::endl;
    }
    //Get actual devices after checking for existence
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    result = vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, physicalDevices.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Device retrieval failed (" << result << ")" << std::endl;
        return false;
    }
    std::string deviceTypes[5] = {"VK_PHYSICAL_DEVICE_TYPE_OTHER",
                                  "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU",
                                  "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU",
                                  "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU",
                                  "VK_PHYSICAL_DEVICE_TYPE_CPU",};
    mainPhysicalDevice = physicalDevices[0];
    VkPhysicalDeviceProperties physicalProperties = {};
    vkGetPhysicalDeviceProperties(mainPhysicalDevice, &physicalProperties);
    std::cout <<    "Device Name: " << physicalProperties.deviceName << std::endl;
    std::cout <<    "Device Type: " << deviceTypes[physicalProperties.deviceType] << std::endl;
    std::cout << "Driver Version: " << physicalProperties.driverVersion << std::endl;
    std::cout <<    "API Version: " << VK_VERSION_MAJOR(physicalProperties.apiVersion) << "."
                                    << VK_VERSION_MINOR(physicalProperties.apiVersion) << "."
                                    << VK_VERSION_PATCH(physicalProperties.apiVersion) << std::endl;

    VkPhysicalDeviceFeatures physicalFeatures = {};
    vkGetPhysicalDeviceFeatures(mainPhysicalDevice, &physicalFeatures);

    if(!deviceQueue())
        return false;

    return true;
}

bool deviceValidationLayers()
{
    //Count validation layers to make sure they exist
    uint32_t devicelayerCount = 0;
    result = vkEnumerateDeviceLayerProperties(mainPhysicalDevice, &devicelayerCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Device layer count failed" << std::endl;
        return false;
    }
    else if(devicelayerCount <= 0)
    {
        std::cout << "No device validation layers found" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Device validation layers found: " << devicelayerCount << std::endl;
    }
    //Then retrieve them
    std::vector<VkLayerProperties> devicelayersAvailable(devicelayerCount);
    result = vkEnumerateDeviceLayerProperties(mainPhysicalDevice, &devicelayerCount, devicelayersAvailable.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Device layer retrieval failed" << std::endl;
        return false;
    }

    //std::vector<const char*> deviceLayers;
    deviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
    bool missingDeLa = false;
    uint32_t missingDeLaId;
    for(int i = 0; i < deviceLayers.size(); i++)
    {
        bool found = false;
        for(int j = 0; j < devicelayerCount; j++)
        {
            if(strcmp(deviceLayers[i], devicelayersAvailable[j].layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            missingDeLa = true;
            missingDeLaId = i;
            break;
        }
    }
    if(missingDeLa)
    {
        std::cout << "Instance layer missing: " << deviceLayers[missingDeLaId] << std::endl;
        return false;
    }
    else
    {
        std::cout << "All instance layer found" << std::endl;
    }

    return true;
}

bool doDeviceExtensions()
{
    uint32_t deviceextensionCount = 0;
    uint32_t layerdeviceextensionCount = 0;
    result = vkEnumerateDeviceExtensionProperties(mainPhysicalDevice, NULL, &deviceextensionCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Device extension count failed" << std::endl;
        return false;
    }
#ifdef VULKAN_DEBUGGING
    result = vkEnumerateDeviceExtensionProperties(mainPhysicalDevice, deviceLayers[0], &layerdeviceextensionCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Device layer extension count failed" << std::endl;
        return false;
    }
    else if(deviceextensionCount <= 0)
    {
        std::cout << "No device extensions found" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Device extensions found: " << deviceextensionCount << " + " << layerdeviceextensionCount << std::endl;
    }
#endif // VULKAN_DEBUGGING
    std::vector<VkExtensionProperties> deviceextensionsAvailable(deviceextensionCount + layerdeviceextensionCount);
    result = vkEnumerateDeviceExtensionProperties(mainPhysicalDevice, NULL, &deviceextensionCount, deviceextensionsAvailable.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Device extension retrieval failed" << std::endl;
        return false;
    }
#ifdef VULKAN_DEBUGGING
    result = vkEnumerateDeviceExtensionProperties(mainPhysicalDevice, deviceLayers[0], &layerdeviceextensionCount, &deviceextensionsAvailable[deviceextensionCount]);
    if(result != VK_SUCCESS)
    {
        std::cout << "Device layer extension retrieval failed" << std::endl;
        return false;
    }
#endif // VULKAN_DEBUGGING

    //std::vector<const char *> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    bool missingDeEx = false;
    uint32_t missingDeExId;
    for(int i = 0; i < deviceExtensions.size(); i++)
    {
        bool found = false;
        for(int j = 0; j < deviceextensionCount; j++)
        {
            if(strcmp(deviceExtensions[i], deviceextensionsAvailable[j].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            missingDeEx = true;
            missingDeExId = i;
            break;
        }
    }
    if(missingDeEx)
    {
        std::cout << "Device extension missing: " << extensions[missingDeExId] << std::endl;
        return false;
    }
    else
    {
        std::cout << "All device extensions found" << std::endl;
    }

    return true;
}

bool surfaceFormats()
{
    //Count number of supported colour formats
    uint32_t formatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(mainPhysicalDevice, vulkanSurface, &formatCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Format count failed (" << result << ")" << std::endl;
        return false;
    }
    else if(formatCount < 1)
    {
        std::cout << "No available formats" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Format count: " << formatCount << std::endl;
    }
    //Get formats after checking number of them
    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(mainPhysicalDevice, vulkanSurface, &formatCount, surfaceFormats.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Format retrieval failed (" << result << ")" << std::endl;
        return false;
    }

    //Choose colour format and colour space
    //Format = size of components, compression
    //Space  = range of colours/gamut
    if(formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        colourFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        colourFormat = surfaceFormats[0].format;
    }
    colourSpace = surfaceFormats[0].colorSpace;

    //Get resolution of surface
    VkSurfaceCapabilitiesKHR capabilities = {};
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mainPhysicalDevice, vulkanSurface, &capabilities);
    if(result != VK_SUCCESS)
    {
        std::cout << "Surface capability check failed (" << result << ")" << std::endl;
        return false;
    }

    swapchainExtent = {};
    if (capabilities.currentExtent.width == -1 || capabilities.currentExtent.height == -1) {
        swapchainExtent.width = 640;
        swapchainExtent.height = 480;
    }
    else
    {
        swapchainExtent = capabilities.currentExtent;
    }

    if(capabilities.maxImageCount < 1)
    {
        std::cout << "Surface does not support images?" << std::endl;
        return false;
    }
    //VkExtent2D swapchainExtent = {};
    //uint32_t imageCount = capabilities.minImageCount + 1;
    imageCount = capabilities.minImageCount + 1;
    if(imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform = capabilities.currentTransform;
    if( capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    //Count present modes
    uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(mainPhysicalDevice, vulkanSurface, &presentModeCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Present mode count failed (" << result << ")" << std::endl;
        return false;
    }
    else if(presentModeCount < 1)
    {
        std::cout << "No available present modes" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Present mode count: " << presentModeCount << std::endl;
    }
    //Get present modes after checking number of them
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(mainPhysicalDevice, vulkanSurface, &presentModeCount, presentModes.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Present mode retrieval failed (" << result << ")" << std::endl;
        return false;
    }

    //Check for specific present mode
    //If not found, a default will be used
    presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32_t i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }
    std::string presentModesAll[4] = {"VK_PRESENT_MODE_IMMEDIATE_KHR",
                                      "VK_PRESENT_MODE_MAILBOX_KHR",
                                      "VK_PRESENT_MODE_FIFO_KHR",
                                      "VK_PRESENT_MODE_FIFO_RELAXED_KHR",};
    std::cout << "Present mode used: " << presentModesAll[presentMode] << std::endl;

    return true;
}

VkCommandBuffer offscreenCommandBuffer;
bool createOffscreenCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
    commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocationInfo.commandPool = commandPool;
    commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocationInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocationInfo, &offscreenCommandBuffer);
    if(result != VK_SUCCESS)
    {
        std::cout << "Offscreen command buffer could not be allocated" << std::endl;
        return false;
    }
    else
        std::cout << "Offscreen command buffer allocated" << std::endl;

    VkClearValue clearValue[] = {{0.25f,0.35f,0.5f,1.0f}, {1.0, 0.0}};
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea = {0, 0, renderToFramebuffer.width, renderToFramebuffer.height};
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValue;
    renderPassBeginInfo.framebuffer = renderToFramebuffer.framebuffer;

    VkDeviceSize offsets = {0};

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkViewport viewport = {0, 0, swapchainExtent.width, swapchainExtent.height, 0, 1};
    VkRect2D scissor = {0, 0, swapchainExtent.width, swapchainExtent.height};

    vkBeginCommandBuffer(offscreenCommandBuffer, &beginInfo);
    vkCmdBeginRenderPass(offscreenCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
        vkCmdSetViewport(offscreenCommandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(offscreenCommandBuffer, 0, 1, &scissor);

        vkCmdBindPipeline(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, simplepipeline);

        for(int j = 0; j < meshes.size(); j++)
        {
            vkCmdBindDescriptorSets(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[j], 0, NULL);
            vkCmdBindVertexBuffers(offscreenCommandBuffer, 0, 1, &meshes[j].vertexBuffer.buffer, &offsets);
            vkCmdBindIndexBuffer(offscreenCommandBuffer, meshes[j].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(offscreenCommandBuffer, meshes[j].indices.size(), 1,0,0,1);
        }

        vkCmdBindPipeline(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normalpipeline);

        for(int j = 0; j < meshes.size(); j++)
        {
            vkCmdBindDescriptorSets(offscreenCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[j], 0, NULL);
            vkCmdBindVertexBuffers(offscreenCommandBuffer, 0, 1, &meshes[j].vertexBuffer.buffer, &offsets);
            vkCmdBindIndexBuffer(offscreenCommandBuffer, meshes[j].indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(offscreenCommandBuffer, meshes[j].indices.size(), 1,0,0,1);
        }

    vkCmdEndRenderPass(offscreenCommandBuffer);
    result = vkEndCommandBuffer(offscreenCommandBuffer);
    if(result != VK_SUCCESS)
    {
        std::cout << "Command buffer could not be created and filled" << std::endl;
        return false;
    }
    else
        std::cout << "Command buffer created and filled" << std::endl;

    return true;
}

bool createCommandBuffers()
{
    commandBuffers.resize(frameBuffers.size());

    VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
    commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocationInfo.commandPool = commandPool;
    commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocationInfo.commandBufferCount = commandBuffers.size();

    result = vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocationInfo, commandBuffers.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Command buffers could not be allocated" << std::endl;
        return false;
    }
    else
        std::cout << "Command buffers allocated" << std::endl;

    VkClearValue clearValue[] = {{0.25f,0.35f,0.25f,1.0f}, {1.0, 0.0}};
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea = {0, 0, swapchainExtent.width, swapchainExtent.height};
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValue;

    VkDeviceSize offsets = {0};

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkViewport viewport = {0, 0, swapchainExtent.width, swapchainExtent.height, 0, 1};
    VkRect2D scissor = {0, 0, swapchainExtent.width, swapchainExtent.height};

    for(int i = 0; i < commandBuffers.size(); i++)
    {
        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

        renderPassBeginInfo.framebuffer = frameBuffers[i];

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
            vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);
            vkCmdSetScissor(commandBuffers[i], 0, 1, &scissor);

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, screenpipeline);

            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, screenpipelineLayout, 0, 1, &screenQuadDescriptorSet, 0, NULL);
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &screenMesh.vertexBuffer.buffer, &offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], screenMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffers[i], screenMesh.indices.size(), 1,0,0,1);

        vkCmdEndRenderPass(commandBuffers[i]);
        result = vkEndCommandBuffer(commandBuffers[i]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Command buffer could not be created and filled: " << i << std::endl;
            return false;
        }
        else
            std::cout << "Command buffer created and filled: " << i << std::endl;
    }

    return true;
}

bool createSwapchain()
{
    //Swapchain parameters
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;  //VkStructureType
    swapchainCreateInfo.surface = vulkanSurface;                              //VkSurfaceKHR
    swapchainCreateInfo.minImageCount = imageCount;                           //uint32_t
    swapchainCreateInfo.imageFormat = colourFormat;                           //VkFormat
    swapchainCreateInfo.imageColorSpace = colourSpace;                        //VkColorSpaceKHR
    swapchainCreateInfo.imageExtent = swapchainExtent;                        //VkExtent2D
    swapchainCreateInfo.imageArrayLayers = 1;                                 //uint32_t
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;     //VkImageUsageFlags
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;         //VkSharingMode
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; //VkSurfaceTransformFlagBitsKHR
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;   //VkCompositeAlphaFlagBitsKHR
    swapchainCreateInfo.presentMode = presentMode;                            //VkPresentModeKHR
    //Create swapchain using parameters
    result = vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, NULL, &swapchain);
    if(result != VK_SUCCESS)
    {
        std::cout << "Swapchain creation failed (" << result << ")" << std::endl;
        return false;
    }

    return true;
}

bool createCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;      //VkStructureType
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = presentQueueId;

    result = vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, NULL, &commandPool);
    if(result != VK_SUCCESS)
    {
        std::cout << "Command pool creation failed (" << result << ")" << std::endl;
        return false;
    }

    return true;
}

bool doSwapchainImages()
{
    //Get images from swapchain
    //Count images
    uint32_t swapchainImageCount;
    result = vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImageCount, NULL);
    if(result != VK_SUCCESS)
    {
        std::cout << "Swapchain image count failed (" << result << ")" << std::endl;
        return false;
    }
    else if(swapchainImageCount < 1)
    {
        std::cout << "No swapchain images" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Swapchain image count: " << swapchainImageCount << std::endl;
    }
    //Retrieve images after checking count
    //std::vector<VkImage> swapchainImages(swapchainImageCount);
    swapchainImages.resize(swapchainImageCount);
    //std::vector<VkImageView> imageViews(swapchainImageCount);
    imageViews.resize(swapchainImageCount);
    result = vkGetSwapchainImagesKHR(logicalDevice, swapchain, &swapchainImageCount, swapchainImages.data());
    if(result != VK_SUCCESS)
    {
        std::cout << "Swapchain image retrieval failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Swapchain image retrieval success" << std::endl;
    }

    //Set image views
    //Image views are how you access textures in shaders
    //Set up view parameters
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;        //VkStructureType
    imageViewCreateInfo.pNext = NULL;                                            //const void
    imageViewCreateInfo.format = colourFormat;                                   //VkFormat
    imageViewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R,                    //VkComponentMapping
                                  VK_COMPONENT_SWIZZLE_G,
                                  VK_COMPONENT_SWIZZLE_B,
                                  VK_COMPONENT_SWIZZLE_A};
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                        //VkImageViewType
    imageViewCreateInfo.flags = 0;                                               //VkImageViewCreateFlags

    for(uint32_t i = 0; i < swapchainImageCount; i++)
    {
        imageViewCreateInfo.image = swapchainImages[i];

        // create the image view:
        result = vkCreateImageView(logicalDevice, &imageViewCreateInfo, NULL, &imageViews[i]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Image view could not be created: " << i << " (" << result << ")" << std::endl;
            return false;
        }
    }

    return true;
}

bool doDepthImage()
{
    VkImageCreateInfo depthImageCreateInfo = {};
    depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    depthImageCreateInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;                          // notice me senpai!
    depthImageCreateInfo.extent = { swapchainExtent.width, swapchainExtent.height, 1 };
    depthImageCreateInfo.mipLevels = 1;
    depthImageCreateInfo.arrayLayers = 1;
    depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;                       // notice me senpai!
    depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;   // notice me senpai!
    depthImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    depthImageCreateInfo.queueFamilyIndexCount = 0;
    depthImageCreateInfo.pQueueFamilyIndices = NULL;
    depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;             // notice me senpai!

    result = vkCreateImage(logicalDevice, &depthImageCreateInfo, NULL, &depthImage);
    if(result != VK_SUCCESS)
    {
        std::cout << "Depth image could not be created (" << result << ")" << std::endl;
        return false;
    }

    VkMemoryRequirements memoryRequirements = {};
    vkGetImageMemoryRequirements(logicalDevice, depthImage, &memoryRequirements);

    VkMemoryAllocateInfo imageAllocateInfo = {};
    imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    imageAllocateInfo.allocationSize = memoryRequirements.size;
    imageAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    depthImageMemory = {};
    result = vkAllocateMemory(logicalDevice, &imageAllocateInfo, NULL, &depthImageMemory);
    if(result != VK_SUCCESS)
    {
        std::cout << "Image memory could not be allocated (" << result << ")" << std::endl;
        return false;
    }

    result = vkBindImageMemory(logicalDevice, depthImage, depthImageMemory, 0 );
    if(result != VK_SUCCESS)
    {
        std::cout << "Depth image could not be bound (" << result << ")" << std::endl;
        return false;
    }

    VkImageViewCreateInfo depthImageViewCreateInfo = {};
    depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthImageViewCreateInfo.image = depthImage;
    depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewCreateInfo.format = depthImageCreateInfo.format;
    depthImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                       VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    depthImageViewCreateInfo.subresourceRange.levelCount = 1;
    depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    depthImageViewCreateInfo.subresourceRange.layerCount = 1;

    result = vkCreateImageView(logicalDevice, &depthImageViewCreateInfo, NULL, &depthImageView);
    if(result != VK_SUCCESS)
    {
        std::cout << "Depth image view could not be bound (" << result << ")" << std::endl;
        return false;
    }

    return true;
}


bool createRenderPass()
{
    VkAttachmentDescription passAttachments[2] = { };
    passAttachments[0].format = colourFormat;
    passAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    passAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    passAttachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    passAttachments[1].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    passAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    passAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    passAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    passAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    passAttachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    passAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colourAttachmentReference = {};
    colourAttachmentReference.attachment = 0;
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = passAttachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;

    result = vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, NULL, &renderPass);
    if(result != VK_SUCCESS)
    {
        std::cout << "Render pass creation failed (" << result << ")" << std::endl;
        return false;
    }

    return true;
}

bool createFramebuffers()
{
    VkImageView frameBufferAttachments[2];
    frameBufferAttachments[1] = depthImageView;

    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = 2;  // must be equal to the attachment count on render pass
    frameBufferCreateInfo.pAttachments = frameBufferAttachments;
    frameBufferCreateInfo.width = swapchainExtent.width;
    frameBufferCreateInfo.height = swapchainExtent.height;
    frameBufferCreateInfo.layers = 1;

    // create a framebuffer per swap chain imageView:
    //std::vector<VkFramebuffer> frameBuffers(imageCount);
    frameBuffers.resize(imageCount);
    for( uint32_t i = 0; i < imageCount; ++i )
    {
        frameBufferAttachments[0] = imageViews[i];
        result = vkCreateFramebuffer(logicalDevice, &frameBufferCreateInfo,
                                      NULL, &frameBuffers[i]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Framebuffer creation failed: " << i << "(" << result << ")" << std::endl;
            return false;
        }
    }

    return true;
}

bool loadModels()
{
    //LOAD MESH HERE
    Mesh testMesh;
    Mesh testMesh2;
    Mesh testMesh3;
    if(!testMesh.loadModel("models/org.fbx"))
    {
        std::cout << "Model 1 failed to load" << std::endl;
    }
    else
        meshes.push_back(testMesh);
    if(!testMesh2.loadModel("models/flier.fbx"))
    {
        std::cout << "Model 2 failed to load" << std::endl;
    }
    else
        meshes.push_back(testMesh2);
    if(!testMesh3.loadModel("models/rani.fbx"))
    {
        std::cout << "Model 3 failed to load" << std::endl;
    }
    else
        meshes.push_back(testMesh3);

    //Mesh screenMesh;
    std::vector<glm::vec3> quadVertices;
    quadVertices.push_back(glm::vec3(-1,1,0));  //TL
    quadVertices.push_back(glm::vec3(1,1,0));   //TR
    quadVertices.push_back(glm::vec3(-1,-1,0)); //BL
    quadVertices.push_back(glm::vec3(1,-1,0));  //BR
    std::vector<glm::vec2> quadUvs;
    quadUvs.push_back(glm::vec2(0,0));
    quadUvs.push_back(glm::vec2(1,0));
    quadUvs.push_back(glm::vec2(0,1));
    quadUvs.push_back(glm::vec2(1,1));
    std::vector<glm::vec3> quadNormals;
    quadNormals.push_back(glm::vec3(0,1,0));
    quadNormals.push_back(glm::vec3(0,1,0));
    quadNormals.push_back(glm::vec3(0,1,0));
    quadNormals.push_back(glm::vec3(0,1,0));
    std::vector<unsigned int> quadIndices;
    quadIndices.push_back(0);
    quadIndices.push_back(1);
    quadIndices.push_back(3);
    quadIndices.push_back(3);
    quadIndices.push_back(2);
    quadIndices.push_back(0);
    if(!screenMesh.loadWithVectors(quadVertices, quadUvs, quadNormals, quadIndices))
    {
        std::cout << "Screen quad failed to load" << std::endl;
    }

    return true;
}

bool loadShaders()
{
    static VkVertexInputBindingDescription vertexBindingDescription = {};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(Vertex);
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //Simple model shader
    {
        shader1.shaderModules.resize(2);
        result = loadShader("./shaders/simple.vert.spv", &shader1.shaderModules[0]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Vertex shader creation failed (" << result << ")" << std::endl;
            return false;
        }
        result = loadShader("./shaders/simple.frag.spv", &shader1.shaderModules[1]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Fragment shader creation failed (" << result << ")" << std::endl;
            return false;
        }

        shader1.stageCreateInfo.resize(2);
        shader1.stageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader1.stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader1.stageCreateInfo[0].module = shader1.shaderModules[0];
        shader1.stageCreateInfo[0].pName = "main";        // shader entry point function name
        shader1.stageCreateInfo[0].pSpecializationInfo = NULL;

        shader1.stageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader1.stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shader1.stageCreateInfo[1].module = shader1.shaderModules[1];
        shader1.stageCreateInfo[1].pName = "main";        // shader entry point function name
        shader1.stageCreateInfo[1].pSpecializationInfo = NULL;


        static std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptionSimple(4);
        vertexAttributeDescriptionSimple[0].location = 0;
        vertexAttributeDescriptionSimple[0].binding = 0;
        vertexAttributeDescriptionSimple[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributeDescriptionSimple[0].offset = 0;

        vertexAttributeDescriptionSimple[1].location = 1;
        vertexAttributeDescriptionSimple[1].binding = 0;
        vertexAttributeDescriptionSimple[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributeDescriptionSimple[1].offset = sizeof(glm::vec3);

        vertexAttributeDescriptionSimple[2].location = 2;
        vertexAttributeDescriptionSimple[2].binding = 0;
        vertexAttributeDescriptionSimple[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributeDescriptionSimple[2].offset = sizeof(glm::vec3) + sizeof(glm::vec2);

        vertexAttributeDescriptionSimple[3].location = 3;
        vertexAttributeDescriptionSimple[3].binding = 0;
        vertexAttributeDescriptionSimple[3].format = VK_FORMAT_R32_SINT;
        vertexAttributeDescriptionSimple[3].offset = sizeof(glm::vec3)*2 + sizeof(glm::vec2);

        shader1.vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        shader1.vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        shader1.vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        shader1.vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionSimple.size();
        shader1.vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptionSimple.data();

        std::cout << "Simple shader parts created" << std::endl;
    }

    //Normals line view shader
    {
        shader2.shaderModules.resize(3);
        result = loadShader("./shaders/normal.vert.spv", &shader2.shaderModules[0]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Vertex shader creation failed (" << result << ")" << std::endl;
            return false;
        }
        result = loadShader("./shaders/normal.geom.spv", &shader2.shaderModules[1]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Geometry shader creation failed (" << result << ")" << std::endl;
            return false;
        }
        result = loadShader("./shaders/normal.frag.spv", &shader2.shaderModules[2]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Fragment shader creation failed (" << result << ")" << std::endl;
            return false;
        }

        shader2.stageCreateInfo.resize(3);
        shader2.stageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader2.stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shader2.stageCreateInfo[0].module = shader2.shaderModules[0];
        shader2.stageCreateInfo[0].pName = "main";        // shader entry point function name
        shader2.stageCreateInfo[0].pSpecializationInfo = NULL;

        shader2.stageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader2.stageCreateInfo[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        shader2.stageCreateInfo[1].module = shader2.shaderModules[1];
        shader2.stageCreateInfo[1].pName = "main";        // shader entry point function name
        shader2.stageCreateInfo[1].pSpecializationInfo = NULL;

        shader2.stageCreateInfo[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader2.stageCreateInfo[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shader2.stageCreateInfo[2].module = shader2.shaderModules[2];
        shader2.stageCreateInfo[2].pName = "main";        // shader entry point function name
        shader2.stageCreateInfo[2].pSpecializationInfo = NULL;


        static std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptionNormal(2);
        vertexAttributeDescriptionNormal[0].location = 0;
        vertexAttributeDescriptionNormal[0].binding = 0;
        vertexAttributeDescriptionNormal[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributeDescriptionNormal[0].offset = 0;

        vertexAttributeDescriptionNormal[1].location = 1;
        vertexAttributeDescriptionNormal[1].binding = 0;
        vertexAttributeDescriptionNormal[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributeDescriptionNormal[1].offset = sizeof(glm::vec3) + sizeof(glm::vec2);

        shader2.vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        shader2.vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        shader2.vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        shader2.vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionNormal.size();
        shader2.vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptionNormal.data();

        std::cout << "Normals shader parts created" << std::endl;
    }

    //Screen quad shader
    {
        screenShader.shaderModules.resize(2);
        result = loadShader("./shaders/screen.vert.spv", &screenShader.shaderModules[0]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Vertex shader creation failed (" << result << ")" << std::endl;
            return false;
        }
        result = loadShader("./shaders/screen.frag.spv", &screenShader.shaderModules[1]);
        if(result != VK_SUCCESS)
        {
            std::cout << "Fragment shader creation failed (" << result << ")" << std::endl;
            return false;
        }

        screenShader.stageCreateInfo.resize(2);
        screenShader.stageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        screenShader.stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        screenShader.stageCreateInfo[0].module = screenShader.shaderModules[0];
        screenShader.stageCreateInfo[0].pName = "main";        // shader entry point function name
        screenShader.stageCreateInfo[0].pSpecializationInfo = NULL;

        screenShader.stageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        screenShader.stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        screenShader.stageCreateInfo[1].module = screenShader.shaderModules[1];
        screenShader.stageCreateInfo[1].pName = "main";        // shader entry point function name
        screenShader.stageCreateInfo[1].pSpecializationInfo = NULL;

        static std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptionScreen(2);
        vertexAttributeDescriptionScreen[0].location = 0;
        vertexAttributeDescriptionScreen[0].binding = 0;
        vertexAttributeDescriptionScreen[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributeDescriptionScreen[0].offset = 0;

        vertexAttributeDescriptionScreen[1].location = 1;
        vertexAttributeDescriptionScreen[1].binding = 0;
        vertexAttributeDescriptionScreen[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertexAttributeDescriptionScreen[1].offset = sizeof(glm::vec3);

        screenShader.vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        screenShader.vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
        screenShader.vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
        screenShader.vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionScreen.size();
        screenShader.vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptionScreen.data();

        std::cout << "Screen shader parts created" << std::endl;
    }

    return true;
}

bool doDescriptors()
{
    //Descriptor pool
    {
        VkDescriptorPoolSize typeCounts[2];
        typeCounts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        typeCounts[0].descriptorCount = 10;
        typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        typeCounts[1].descriptorCount = 10;

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = NULL;
        descriptorPoolInfo.poolSizeCount = 2;
        descriptorPoolInfo.pPoolSizes = typeCounts;
        descriptorPoolInfo.maxSets = 20;

        result = vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, NULL, &descriptorPool);
        if(result != VK_SUCCESS)
        {
            std::cout << "Descriptor pool creation failed (" << result << ")" << std::endl;
            return false;
        }
    }

    //Meshes
    {
        std::vector<VkDescriptorSetLayoutBinding> descriptorlayoutBinding(2);
        descriptorlayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorlayoutBinding[0].binding = 0;
        descriptorlayoutBinding[0].descriptorCount = 1;
        descriptorlayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
        descriptorlayoutBinding[0].pImmutableSamplers = NULL;

        descriptorlayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorlayoutBinding[1].binding = 1;
        descriptorlayoutBinding[1].descriptorCount = 1;
        descriptorlayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        descriptorlayoutBinding[1].pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
        descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayoutCreateInfo.pNext = NULL;
        descriptorLayoutCreateInfo.bindingCount = 2;
        descriptorLayoutCreateInfo.pBindings = descriptorlayoutBinding.data();

        descriptorSetLayouts.resize(meshes.size());
        for(int i = 0; i < meshes.size(); i++)
        {
            result = vkCreateDescriptorSetLayout(logicalDevice, &descriptorLayoutCreateInfo, NULL, &descriptorSetLayouts[i]);
        }

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = descriptorSetLayouts.data();

        //std::vector<VkDescriptorSet> descriptorSets(meshes.size());
        descriptorSets.resize(meshes.size());
        for(int i = 0; i < meshes.size(); i++)
        {
            result = vkAllocateDescriptorSets(logicalDevice, &allocInfo, &descriptorSets[i]);
        }

        std::vector<VkDescriptorBufferInfo> uniformDescriptorInfos(meshes.size());
        for(int i = 0; i < meshes.size(); i++)
        {
            uniformDescriptorInfos[i].buffer = uniformBuffers[i].buffer;
            uniformDescriptorInfos[i].offset = 0;
            uniformDescriptorInfos[i].range = sizeof(UniformData);
        }

        std::vector<VkDescriptorImageInfo> descriptorImageInfos(meshes.size());
        for(int i = 0; i < meshes.size(); i++)
        {
            if(meshes[i].textured)
            {
                descriptorImageInfos[i].sampler = meshes[i].tex.sampler;
                descriptorImageInfos[i].imageView = meshes[i].tex.textureView;
                descriptorImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            }
        }

        std::vector<std::vector<VkWriteDescriptorSet> > writeDescriptorSets(meshes.size());
        for(int i = 0; i < meshes.size(); i++)
        {
            writeDescriptorSets[i].resize(2);
            // Binding 0 : Uniform buffer
            writeDescriptorSets[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[i][0].dstBinding = 0;
            writeDescriptorSets[i][0].dstSet = descriptorSets[i];
            writeDescriptorSets[i][0].descriptorCount = 1;
            writeDescriptorSets[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSets[i][0].pBufferInfo = &uniformDescriptorInfos[i];
            // Binding 1 : Image sampler
            writeDescriptorSets[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[i][1].dstBinding = 1;
            writeDescriptorSets[i][1].dstSet = descriptorSets[i];
            writeDescriptorSets[i][1].descriptorCount = 1;
            writeDescriptorSets[i][1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[i][1].pImageInfo = &descriptorImageInfos[i];
            vkUpdateDescriptorSets(logicalDevice, 2, writeDescriptorSets[i].data(), 0, NULL);
        }
    }

    //Screen quad
    {
        std::vector<VkDescriptorSetLayoutBinding> screenQuadDescriptorlayoutBinding(2);
        screenQuadDescriptorlayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        screenQuadDescriptorlayoutBinding[0].binding = 0;
        screenQuadDescriptorlayoutBinding[0].descriptorCount = 1;
        screenQuadDescriptorlayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
        screenQuadDescriptorlayoutBinding[0].pImmutableSamplers = NULL;

        screenQuadDescriptorlayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        screenQuadDescriptorlayoutBinding[1].binding = 1;
        screenQuadDescriptorlayoutBinding[1].descriptorCount = 1;
        screenQuadDescriptorlayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        screenQuadDescriptorlayoutBinding[1].pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo screenQuadDescriptorLayoutCreateInfo = {};
        screenQuadDescriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        screenQuadDescriptorLayoutCreateInfo.pNext = NULL;
        screenQuadDescriptorLayoutCreateInfo.bindingCount = 2;
        screenQuadDescriptorLayoutCreateInfo.pBindings = screenQuadDescriptorlayoutBinding.data();

        //VkDescriptorSetLayout screenQuadDescriptorSetLayout;
        result = vkCreateDescriptorSetLayout(logicalDevice, &screenQuadDescriptorLayoutCreateInfo, NULL, &screenQuadDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &screenQuadDescriptorSetLayout;

        //VkDescriptorSet screenQuadDescriptorSet;
        result = vkAllocateDescriptorSets(logicalDevice, &allocInfo, &screenQuadDescriptorSet);

        VkDescriptorBufferInfo screenQuadUniformDescriptorInfo;
        screenQuadUniformDescriptorInfo.buffer = screenQuadUniformMemory.buffer;
        screenQuadUniformDescriptorInfo.offset = 0;
        screenQuadUniformDescriptorInfo.range = sizeof(UniformData);

        VkDescriptorImageInfo screenQuadImageDescriptorInfo;
        screenQuadImageDescriptorInfo.sampler = renderToFramebuffer.colourSampler;
        screenQuadImageDescriptorInfo.imageView = renderToFramebuffer.colour.imageView;
        screenQuadImageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        std::vector<VkWriteDescriptorSet> writeDescriptorSet(2);
        // Binding 0 : Uniform buffer
        writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet[0].dstBinding = 0;
        writeDescriptorSet[0].dstSet = screenQuadDescriptorSet;
        writeDescriptorSet[0].descriptorCount = 1;
        writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet[0].pBufferInfo = &screenQuadUniformDescriptorInfo;
        // Binding 1 : Image sampler
        writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet[1].dstBinding = 1;
        writeDescriptorSet[1].dstSet = screenQuadDescriptorSet;
        writeDescriptorSet[1].descriptorCount = 1;
        writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet[1].pImageInfo = &screenQuadImageDescriptorInfo;
        vkUpdateDescriptorSets(logicalDevice, 2, writeDescriptorSet.data(), 0, NULL);
    }

    return true;
}

bool createPipeline()
{
    VkPipelineLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
    layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutCreateInfo.pushConstantRangeCount = 0;//1;
    layoutCreateInfo.pPushConstantRanges = NULL;//&pushConstantRange;

    result = vkCreatePipelineLayout(logicalDevice, &layoutCreateInfo, NULL, &pipelineLayout);
    if(result != VK_SUCCESS)
    {
        std::cout << "Pipeline layout creation failed (" << result << ")" << std::endl;
        return false;
    }
    // vertex topology config:
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = swapchainExtent.width;
    viewport.height = swapchainExtent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;

    VkRect2D scissors = {};
    scissors.offset = {0, 0};
    scissors.extent = {swapchainExtent.width, swapchainExtent.height};

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissors;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0;
    rasterizationState.depthBiasClamp = 0;
    rasterizationState.depthBiasSlopeFactor = 0;
    rasterizationState.lineWidth = 1;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.minSampleShading = 0;
    multisampleState.pSampleMask = NULL;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    VkStencilOpState noOPStencilState = {};
    noOPStencilState.failOp = VK_STENCIL_OP_KEEP;
    noOPStencilState.passOp = VK_STENCIL_OP_KEEP;
    noOPStencilState.depthFailOp = VK_STENCIL_OP_KEEP;
    noOPStencilState.compareOp = VK_COMPARE_OP_ALWAYS;
    noOPStencilState.compareMask = 0;
    noOPStencilState.writeMask = 0;
    noOPStencilState.reference = 0;

    VkPipelineDepthStencilStateCreateInfo depthState = {};
    depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthState.depthTestEnable = VK_TRUE;
    depthState.depthWriteEnable = VK_TRUE;
    depthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthState.depthBoundsTestEnable = VK_FALSE;
    depthState.stencilTestEnable = VK_FALSE;
    depthState.front = noOPStencilState;
    depthState.back = noOPStencilState;
    depthState.minDepthBounds = 0;
    depthState.maxDepthBounds = 0;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask = 0xf;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachmentState;
    colorBlendState.blendConstants[0] = 0.0;
    colorBlendState.blendConstants[1] = 0.0;
    colorBlendState.blendConstants[2] = 0.0;
    colorBlendState.blendConstants[3] = 0.0;

    VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicState;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = shader1.shaderModules.size();
    pipelineCreateInfo.pStages = shader1.stageCreateInfo.data();
    pipelineCreateInfo.pVertexInputState = &shader1.vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pTessellationState = NULL;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pDepthStencilState = &depthState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = NULL;
    pipelineCreateInfo.basePipelineIndex = 0;

    result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL,
                                       &simplepipeline);
    if(result != VK_SUCCESS)
    {
        std::cout << "Simple pipeline creation failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Simple pipeline created" << std::endl;
    }

    pipelineCreateInfo.stageCount = shader2.shaderModules.size();
    pipelineCreateInfo.pStages = shader2.stageCreateInfo.data();
    pipelineCreateInfo.pVertexInputState = &shader2.vertexInputStateCreateInfo;
    result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL,
                                       &normalpipeline);
    if(result != VK_SUCCESS)
    {
        std::cout << "Normals pipeline creation failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Normals pipeline created" << std::endl;
    }

    VkPipelineLayoutCreateInfo screenlayoutCreateInfo = {};
    screenlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    screenlayoutCreateInfo.setLayoutCount = 1;
    screenlayoutCreateInfo.pSetLayouts = &screenQuadDescriptorSetLayout;
    screenlayoutCreateInfo.pushConstantRangeCount = 0;//1;
    screenlayoutCreateInfo.pPushConstantRanges = NULL;//&pushConstantRange;

    result = vkCreatePipelineLayout(logicalDevice, &screenlayoutCreateInfo, NULL, &screenpipelineLayout);
    if(result != VK_SUCCESS)
    {
        std::cout << "Pipeline layout creation failed (" << result << ")" << std::endl;
        return false;
    }

    pipelineCreateInfo.layout = screenpipelineLayout;
    pipelineCreateInfo.stageCount = screenShader.shaderModules.size();
    pipelineCreateInfo.pStages = screenShader.stageCreateInfo.data();
    pipelineCreateInfo.pVertexInputState = &screenShader.vertexInputStateCreateInfo;
    result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL,
                                       &screenpipeline);
    if(result != VK_SUCCESS)
    {
        std::cout << "Screen pipeline creation failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Screen pipeline created" << std::endl;
    }

    return true;
}

bool loadFramebuffer()
{
    renderToFramebuffer.width = 640;
    renderToFramebuffer.height = 480;

    VkFormat fbColourFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat fbDepthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    //Colour attachment
    {
        VkImageCreateInfo colourImageCreateInfo = {};
        colourImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        colourImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        colourImageCreateInfo.format = fbColourFormat;
        colourImageCreateInfo.extent.width = renderToFramebuffer.width;
        colourImageCreateInfo.extent.height = renderToFramebuffer.height;
        colourImageCreateInfo.extent.depth = 1;
        colourImageCreateInfo.mipLevels = 1;
        colourImageCreateInfo.arrayLayers = 1;
        colourImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        colourImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        // We will sample directly from the color attachment
        colourImageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        result = vkCreateImage(logicalDevice, &colourImageCreateInfo, NULL, &renderToFramebuffer.colour.image);
        if(result != VK_SUCCESS)
        {
            std::cout << "Framebuffer colour image could not be created (" << result << ")" << std::endl;
            return false;
        }

        VkMemoryRequirements colourmemoryRequirements = {};
        vkGetImageMemoryRequirements(logicalDevice, renderToFramebuffer.colour.image, &colourmemoryRequirements);

        VkMemoryAllocateInfo imageAllocateInfo = {};
        imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        imageAllocateInfo.allocationSize = colourmemoryRequirements.size;
        imageAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(colourmemoryRequirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(logicalDevice, &imageAllocateInfo, NULL, &renderToFramebuffer.colour.memory);
        if(result != VK_SUCCESS)
        {
            std::cout << "Colour image memory could not be allocated (" << result << ")" << std::endl;
            return false;
        }
        result = vkBindImageMemory(logicalDevice, renderToFramebuffer.colour.image, renderToFramebuffer.colour.memory, 0 );
        if(result != VK_SUCCESS)
        {
            std::cout << "Colour image memory could not be bound (" << result << ")" << std::endl;
            return false;
        }

        VkImageViewCreateInfo colourImageViewCreateInfo = {};
        colourImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colourImageViewCreateInfo.image = renderToFramebuffer.colour.image;
        colourImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colourImageViewCreateInfo.format = fbColourFormat;
        colourImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colourImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        colourImageViewCreateInfo.subresourceRange.levelCount = 1;
        colourImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        colourImageViewCreateInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(logicalDevice, &colourImageViewCreateInfo, NULL, &renderToFramebuffer.colour.imageView);
        if(result != VK_SUCCESS)
        {
            std::cout << "Depth image view could not be bound (" << result << ")" << std::endl;
            return false;
        }

        VkSamplerCreateInfo colourSamplerCreateInfo = {};
        colourSamplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		colourSamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		colourSamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		colourSamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		colourSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		colourSamplerCreateInfo.addressModeV = colourSamplerCreateInfo.addressModeU;
		colourSamplerCreateInfo.addressModeW = colourSamplerCreateInfo.addressModeU;
		colourSamplerCreateInfo.mipLodBias = 0.0f;
		colourSamplerCreateInfo.maxAnisotropy = 0;
		colourSamplerCreateInfo.minLod = 0.0f;
		colourSamplerCreateInfo.maxLod = 1.0f;
		colourSamplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        result = vkCreateSampler(logicalDevice, &colourSamplerCreateInfo, NULL, &renderToFramebuffer.colourSampler);
        if(result != VK_SUCCESS)
        {
            std::cout << "Colour image sampler could not be created (" << result << ")" << std::endl;
            return false;
        }
    }

    //Depth attachment
    {
        VkImageCreateInfo depthImageCreateInfo = {};
        depthImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageCreateInfo.format = fbDepthFormat;
        depthImageCreateInfo.extent.width = renderToFramebuffer.width;
        depthImageCreateInfo.extent.height = renderToFramebuffer.height;
        depthImageCreateInfo.extent.depth = 1;
        depthImageCreateInfo.mipLevels = 1;
        depthImageCreateInfo.arrayLayers = 1;
        depthImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        result = vkCreateImage(logicalDevice, &depthImageCreateInfo, NULL, &renderToFramebuffer.depth.image);
        if(result != VK_SUCCESS)
        {
            std::cout << "Framebuffer depth image could not be created (" << result << ")" << std::endl;
            return false;
        }

        VkMemoryRequirements depthmemoryRequirements = {};
        vkGetImageMemoryRequirements(logicalDevice, renderToFramebuffer.depth.image, &depthmemoryRequirements);

        VkMemoryAllocateInfo imageAllocateInfo = {};
        imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        imageAllocateInfo.allocationSize = depthmemoryRequirements.size;
        imageAllocateInfo.memoryTypeIndex = getMemoryTypeIndex(depthmemoryRequirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(logicalDevice, &imageAllocateInfo, NULL, &renderToFramebuffer.depth.memory);
        if(result != VK_SUCCESS)
        {
            std::cout << "Depth image memory could not be allocated (" << result << ")" << std::endl;
            return false;
        }
        result = vkBindImageMemory(logicalDevice, renderToFramebuffer.depth.image, renderToFramebuffer.depth.memory, 0 );
        if(result != VK_SUCCESS)
        {
            std::cout << "Depth image memory could not be bound (" << result << ")" << std::endl;
            return false;
        }

        VkImageViewCreateInfo depthViewCreateInfo = {};
        depthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewCreateInfo.image = renderToFramebuffer.depth.image;
		depthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		depthViewCreateInfo.format = fbDepthFormat;
		depthViewCreateInfo.flags = 0;
		depthViewCreateInfo.subresourceRange = {};
		depthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthViewCreateInfo.subresourceRange.baseMipLevel = 0;
		depthViewCreateInfo.subresourceRange.levelCount = 1;
		depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        depthViewCreateInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(logicalDevice, &depthViewCreateInfo, NULL, &renderToFramebuffer.depth.imageView);
        if(result != VK_SUCCESS)
        {
            std::cout << "Depth image view could not be created (" << result << ")" << std::endl;
            return false;
        }
    }

    VkImageView attachments[2];
    attachments[0] = renderToFramebuffer.colour.imageView;
    attachments[1] = renderToFramebuffer.depth.imageView;

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = 2;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = renderToFramebuffer.width;
    framebufferCreateInfo.height = renderToFramebuffer.height;
    framebufferCreateInfo.layers = 1;

    result = vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, NULL, &renderToFramebuffer.framebuffer);
    if(result != VK_SUCCESS)
    {
        std::cout << "Framebuffer could not be created (" << result << ")" << std::endl;
        return false;
    }

    return true;
}

int main()
{
    std::cout << "First Line of Program" << std::endl;
    if (!glfwInit())
        return -1;

    int fps = 0;
    double now = glfwGetTime();
    float delta = 0;
    float lastFrame = glfwGetTime();

    if (glfwVulkanSupported())
    {
        std::cout << "Vulkan Supported, continuing." << std::endl;
    }

    loadFunctions();

    #ifdef VULKAN_DEBUGGING
    if(!validationLayers())
        return false;
    #endif // VULKAN_DEBUGGING

    if(!checkAndAddExtensions())
        return false;

    VkApplicationInfo applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; //VkStructureType
    applicationInfo.pNext = NULL;                               //const void
    applicationInfo.pApplicationName = "VulkanLearning";        //const char
    applicationInfo.applicationVersion = 1;                     //uint32_t
    applicationInfo.pEngineName = "TPending";                   //const char
    applicationInfo.engineVersion = 1;                          //uint32_t
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 5);      //uint32_t

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
#ifdef VULKAN_DEBUGGING
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instanceCreateInfo.ppEnabledLayerNames = layers.data();
#endif // VULKAN_DEBUGGING

    result = vkCreateInstance(&instanceCreateInfo, NULL, &vulkanInstance);
    if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        std::cout << "Driver issue (" << result << ")" << std::endl;
        return false;
    }
    else if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateInstance failed (" << result << ")" << std::endl;
        return false;
    }
    else{
        std::cout << "vkCreateInstance success" << std::endl;
    }

#ifdef VULKAN_DEBUGGING
    if(!initDebugging())
        return false;
#endif // VULKAN_DEBUGGING

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(640,480, "Vulkanisation", NULL, NULL);
    if (!window)
    {
        std::cout << "Window creation failed" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Window created successfully" << std::endl;
    }

    //Create surfaces
    result = glfwCreateWindowSurface(vulkanInstance, window, NULL, &vulkanSurface);
    if(result != VK_SUCCESS)
    {
        std::cout << "Surface creation failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Surface created successfully" << std::endl;
    }

    if(!physicalDevice())
        return false;

#ifdef VULKAN_DEBUGGING
    if(!deviceValidationLayers())
        return false;
#endif // VULKAN_DEBUGGING

    if(!doDeviceExtensions())
        return false;

    std::cout << "1" << std::endl;

    VkPhysicalDeviceFeatures enabledFeatures = {};
    enabledFeatures.geometryShader = VK_TRUE;
    std::cout << "2" << std::endl;

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;       //VkStructureType
    deviceInfo.pNext = NULL;                                       //const void
    deviceInfo.flags = 0;                                          //VkDeviceCreateFlags
    deviceInfo.queueCreateInfoCount = 1;                           //uint32_t
    deviceInfo.pQueueCreateInfos = &queueInfo;                     //const VkDeviceQueueCreateInfo*
    deviceInfo.pEnabledFeatures = &enabledFeatures;                 //const VkPhysicalDeviceFeatures*
    deviceInfo.enabledExtensionCount = deviceExtensions.size();   //uint32_t
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data(); //const char* const* (array of char arrays/strings)
#ifdef VULKAN_DEBUGGING
    deviceInfo.enabledLayerCount = deviceLayers.size();   //uint32_t
    deviceInfo.ppEnabledLayerNames = deviceLayers.data(); //const char* const* (array of char arrays/strings)
#endif // VULKAN_DEBUGGING
    std::cout << "3" << std::endl;

    //Logical device is used to interface with physical devices
    result = vkCreateDevice(mainPhysicalDevice, &deviceInfo, NULL, &logicalDevice);
    std::cout << "4" << std::endl;
    if(result != VK_SUCCESS)
    {
        std::cout << "Logical device creation failed (" << result << ")" << std::endl;
        return false;
    }
    else
    {
        std::cout << "Logical device created successfully" << std::endl;
    }

    vkGetDeviceQueue(logicalDevice, presentQueueId, 0, &presentQueue);

    if(!surfaceFormats())
        return false;

    if(!createSwapchain())
        return false;

    if(!createCommandPool())
        return false;

    if(!doSwapchainImages())
        return false;

    if(!doDepthImage())
        return false;

    if(!createRenderPass())
        return false;

    if(!createFramebuffers())
        return false;

    if(!loadModels())
        return false;

    if(!loadFramebuffer())
        return false;

    if(!loadShaders())
        return false;

    UniformData uniformData;
    uniformData.projectionMatrix = glm::perspective(glm::radians(90.0f), (float)swapchainExtent.width/(float)swapchainExtent.height, 0.1f, 100.0f);
    uniformData.viewMatrix = glm::lookAt(glm::vec3(0,0,-5), glm::vec3(0,0,0), glm::vec3(0,1,0));
    uniformData.modelMatrix = glm::mat4();
    uniformData.modelMatrix = glm::rotate(uniformData.modelMatrix, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    uniformData.modelMatrix = glm::rotate(uniformData.modelMatrix, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    uniformData.modelMatrix = glm::rotate(uniformData.modelMatrix, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    uniformBuffers.resize(meshes.size());
    for(int i = 0; i < meshes.size(); i++)
    {
        createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &uniformData
                        ,&uniformBuffers[i]);
    }

    //UniformData screenQuadUniformData;
    screenQuadUniformData.projectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
    //screenQuadUniformData.viewMatrix = glm::lookAt(glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(0,1,0));
    //MemoryBuffer screenQuadUniformMemory;
    createBuffer(sizeof(UniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &screenQuadUniformData
                        ,&screenQuadUniformMemory);

    if(!doDescriptors())
        return false;

    if(!createPipeline())
        return false;

    VkSemaphore imageAvailableSemaphore, offscreenRenderingCompleteSemaphore, renderingCompleteSemaphore;
    VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0};
    vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, NULL, &imageAvailableSemaphore);
    vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, NULL, &offscreenRenderingCompleteSemaphore);
    vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, NULL, &renderingCompleteSemaphore);

    if(!createCommandBuffers())
        return false;

    if(!createOffscreenCommandBuffer())
        return false;

    float camPitch = 0, camYaw = 0;
    glm::vec3 camPos = glm::vec3(0,0,-5);
    glm::vec3 camForward, camRight, camUp;

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS)
    {
        delta = (glfwGetTime() - lastFrame);
        lastFrame = glfwGetTime();

        double x,y;
        glfwGetCursorPos(window, &x, &y);
        camYaw -= (x-640/2)/10*(3.14/180);
        camPitch -= (y-480/2)/10*(3.14/180);
        glfwSetCursorPos(window, 640/2, 480/2);
        camForward = glm::vec3(cos(camPitch) * sin(camYaw),
                                 sin(camPitch),
                                 cos(camPitch) * cos(camYaw));
        camRight = glm::vec3(sin(camYaw - 3.14f/2.0f),
                    0,
                    cos(camYaw - 3.14f/2.0f));
        camUp = glm::cross(camRight, camForward);

        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camPos += camForward*delta*5.0f;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camPos -= camForward*delta*5.0f;
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camPos -= camRight*delta*5.0f;
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camPos += camRight*delta*5.0f;
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camPos += camUp*delta*5.0f;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camPos -= camUp*delta*5.0f;
        uniformData.viewMatrix = glm::lookAt(camPos, camPos + camForward, camUp);

        //Bind uniform buffer
        uniformData.modelMatrix = glm::mat4();
        uniformData.modelMatrix = glm::rotate(uniformData.modelMatrix, glm::radians(125.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        uniformData.modelMatrix = glm::rotate(uniformData.modelMatrix, glm::radians((float) (sin(glfwGetTime())+1)*45), glm::vec3(1.0f, 0.0f, 0.0f));
            void *uniformmapped;
            result = vkMapMemory(logicalDevice, uniformBuffers[0].bufferMemory, 0, VK_WHOLE_SIZE, 0, &uniformmapped);
            memcpy(uniformmapped, &uniformData, sizeof(UniformData));
            vkUnmapMemory(logicalDevice, uniformBuffers[0].bufferMemory);

        uniformData.modelMatrix = glm::mat4();
        float time = (float)glfwGetTime();
        uniformData.modelMatrix = glm::translate(uniformData.modelMatrix, glm::vec3(-15,0,0));
        uniformData.modelMatrix = glm::translate(uniformData.modelMatrix, glm::vec3(5*glm::cos(time),0,5*glm::sin(time)));
        uniformData.modelMatrix = glm::rotate(uniformData.modelMatrix, -time, glm::vec3(0.0f, 1.0f, 0.0f));
            //void *uniformmapped;
            result = vkMapMemory(logicalDevice, uniformBuffers[1].bufferMemory, 0, VK_WHOLE_SIZE, 0, &uniformmapped);
            memcpy(uniformmapped, &uniformData, sizeof(UniformData));
            vkUnmapMemory(logicalDevice, uniformBuffers[1].bufferMemory);

        uniformData.modelMatrix = glm::mat4();
        uniformData.modelMatrix = glm::translate(uniformData.modelMatrix, glm::vec3(-5,0,2));
            //void *uniformmapped;
            result = vkMapMemory(logicalDevice, uniformBuffers[2].bufferMemory, 0, VK_WHOLE_SIZE, 0, &uniformmapped);
            memcpy(uniformmapped, &uniformData, sizeof(UniformData));
            vkUnmapMemory(logicalDevice, uniformBuffers[2].bufferMemory);


        uint32_t nextImageIdx;
        vkAcquireNextImageKHR(logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(),
                                imageAvailableSemaphore, VK_NULL_HANDLE, &nextImageIdx);

        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo;
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &offscreenCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &offscreenRenderingCompleteSemaphore;
        submitInfo.pNext = NULL;

        result = vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if(result != VK_SUCCESS)
        {
            std::cout << "Draw queue could not be submitted" << std::endl;
            return false;
        }

        submitInfo.pCommandBuffers = &commandBuffers[nextImageIdx];
        submitInfo.pWaitSemaphores = &offscreenRenderingCompleteSemaphore;
        submitInfo.pSignalSemaphores = &renderingCompleteSemaphore;
        result = vkQueueSubmit(presentQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if(result != VK_SUCCESS)
        {
            std::cout << "Draw queue could not be submitted" << std::endl;
            return false;
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderingCompleteSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pResults = NULL;
        presentInfo.pImageIndices = &nextImageIdx;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);
        if(result != VK_SUCCESS)
        {
            std::cout << "Presenting failed" << std::endl;
            return false;
        }
        //else
        //    std::cout << "Presenting success" << std::endl;

        glfwPollEvents();
        //Simple fps counter
        fps++;
        if(glfwGetTime() > now + 1.0f)
        {
            std::string fpsString = "fps: ";
            fpsString += FloattoStr(fps);
            glfwSetWindowTitle(window, fpsString.c_str());
            //std::cout << "Frametime:" << 1000.0f/fps << " FPS:" << fps << std::endl;
            now = glfwGetTime();
            fps = 0;
        }
    }
    //Wait for swapchains etc, to be idle before trying to delete
    //Deleting while in use causes error
    vkDeviceWaitIdle(logicalDevice);

    //Destruction
    vkDestroySemaphore(logicalDevice, imageAvailableSemaphore, NULL);
    vkDestroySemaphore(logicalDevice, offscreenRenderingCompleteSemaphore, NULL);
    vkDestroySemaphore(logicalDevice, renderingCompleteSemaphore, NULL);

    screenMesh.deleteModel();
    for(int i = 0; i < screenShader.shaderModules.size(); i++)
    {
        vkDestroyShaderModule(logicalDevice, screenShader.shaderModules[i], NULL);
    }
    vkDestroyPipeline(logicalDevice, screenpipeline, NULL);
    vkDestroyPipelineLayout(logicalDevice, screenpipelineLayout, NULL);
    screenQuadUniformMemory.destroy();
    vkDestroyDescriptorSetLayout(logicalDevice, screenQuadDescriptorSetLayout, NULL);

    vkDestroyFramebuffer(logicalDevice, renderToFramebuffer.framebuffer, NULL);
    vkDestroySampler(logicalDevice, renderToFramebuffer.colourSampler, NULL);
    vkDestroyImage(logicalDevice, renderToFramebuffer.colour.image, NULL);
    vkDestroyImageView(logicalDevice, renderToFramebuffer.colour.imageView, NULL);
    vkFreeMemory(logicalDevice, renderToFramebuffer.colour.memory, NULL);
    vkDestroyImage(logicalDevice, renderToFramebuffer.depth.image, NULL);
    vkDestroyImageView(logicalDevice, renderToFramebuffer.depth.imageView, NULL);
    vkFreeMemory(logicalDevice, renderToFramebuffer.depth.memory, NULL);

    for(uint32_t i = 0; i < imageViews.size(); i++)
    {
        vkDestroyImageView(logicalDevice, imageViews[i], NULL);
    }
    for(uint32_t i = 0; i < frameBuffers.size(); i++)
    {
        vkDestroyFramebuffer(logicalDevice, frameBuffers[i], NULL);
    }
    for(uint32_t i = 0; i < descriptorSetLayouts.size(); i++)
    {
        vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayouts[i], NULL);
    }
    vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);
    vkDestroyRenderPass(logicalDevice, renderPass, NULL);
    vkDestroyPipeline(logicalDevice, simplepipeline, NULL);
    vkDestroyPipeline(logicalDevice, normalpipeline, NULL);
    vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);
    for(int i = 0; i < shader1.shaderModules.size(); i++)
    {
        vkDestroyShaderModule(logicalDevice, shader1.shaderModules[i], NULL);
    }
    for(int i = 0; i < shader2.shaderModules.size(); i++)
    {
        vkDestroyShaderModule(logicalDevice, shader2.shaderModules[i], NULL);
    }
    vkDestroyImage(logicalDevice, depthImage, NULL);
    vkDestroyImageView(logicalDevice, depthImageView, NULL);
    for(int i = 0; i < uniformBuffers.size(); i++)
    {
        uniformBuffers[i].destroy();
    }
    vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), commandBuffers.data());
    for(int i = 0; i < meshes.size(); i++)
    {
        meshes[i].deleteModel();
    }
    vkFreeMemory(logicalDevice, depthImageMemory, NULL);
    vkDestroyCommandPool(logicalDevice, commandPool, NULL);
    vkDestroySwapchainKHR(logicalDevice, swapchain, NULL);
    vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, NULL);
    vkDestroyDevice(logicalDevice, NULL);
#ifdef VULKAN_DEBUGGING
    destroyDebugging();
#endif // VULKAN_DEBUGGING
    vkDestroyInstance(vulkanInstance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
