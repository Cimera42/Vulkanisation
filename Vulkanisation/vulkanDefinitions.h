#ifndef VULKANDEFINITIONS_H_INCLUDED
#define VULKANDEFINITIONS_H_INCLUDED

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define DECLARE_FUNCTION(funcName) PFN_ ## funcName funcName = NULL
#define EXTERN_DECLARE_FUNCTION(funcName) extern PFN_ ## funcName funcName
#define LOAD_FUNCTION(funcName) funcName = (PFN_ ## funcName) glfwGetInstanceProcAddress(NULL, #funcName)
#define LOAD_IFUNCTION(instance, funcName) PFN_ ## funcName funcName = (PFN_ ## funcName) glfwGetInstanceProcAddress(instance, #funcName)

void loadFunctions();

//Pointer to function, loaded from DLL by GLFW
    EXTERN_DECLARE_FUNCTION(vkCreateInstance);
    EXTERN_DECLARE_FUNCTION(vkDestroyInstance);
    EXTERN_DECLARE_FUNCTION(vkEnumeratePhysicalDevices);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceProperties);
    EXTERN_DECLARE_FUNCTION(vkCreateDevice);
    EXTERN_DECLARE_FUNCTION(vkDestroyDevice);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
    EXTERN_DECLARE_FUNCTION(vkDestroySurfaceKHR);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR);
    EXTERN_DECLARE_FUNCTION(vkCreateSwapchainKHR);
    EXTERN_DECLARE_FUNCTION(vkDestroySwapchainKHR);
    EXTERN_DECLARE_FUNCTION(vkGetSwapchainImagesKHR);
    EXTERN_DECLARE_FUNCTION(vkCmdPipelineBarrier);
    EXTERN_DECLARE_FUNCTION(vkCreateImageView);
    EXTERN_DECLARE_FUNCTION(vkDestroyImageView);
    EXTERN_DECLARE_FUNCTION(vkCreateFramebuffer);
    EXTERN_DECLARE_FUNCTION(vkDestroyFramebuffer);
    EXTERN_DECLARE_FUNCTION(vkAcquireNextImageKHR);
    EXTERN_DECLARE_FUNCTION(vkQueuePresentKHR);
    EXTERN_DECLARE_FUNCTION(vkCreateCommandPool);
    EXTERN_DECLARE_FUNCTION(vkDestroyCommandPool);
    EXTERN_DECLARE_FUNCTION(vkAllocateCommandBuffers);
    EXTERN_DECLARE_FUNCTION(vkFreeCommandBuffers);
    EXTERN_DECLARE_FUNCTION(vkBeginCommandBuffer);
    EXTERN_DECLARE_FUNCTION(vkEndCommandBuffer);
    EXTERN_DECLARE_FUNCTION(vkQueueSubmit);
    EXTERN_DECLARE_FUNCTION(vkCreateFence);
    EXTERN_DECLARE_FUNCTION(vkDestroyFence);
    EXTERN_DECLARE_FUNCTION(vkGetDeviceQueue);
    EXTERN_DECLARE_FUNCTION(vkWaitForFences);
    EXTERN_DECLARE_FUNCTION(vkResetFences);
    EXTERN_DECLARE_FUNCTION(vkResetCommandBuffer);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceMemoryProperties);
    EXTERN_DECLARE_FUNCTION(vkCreateImage);
    EXTERN_DECLARE_FUNCTION(vkDestroyImage);
    EXTERN_DECLARE_FUNCTION(vkGetImageMemoryRequirements);
    EXTERN_DECLARE_FUNCTION(vkAllocateMemory);
    EXTERN_DECLARE_FUNCTION(vkBindImageMemory);
    EXTERN_DECLARE_FUNCTION(vkCreateRenderPass);
    EXTERN_DECLARE_FUNCTION(vkDestroyRenderPass);
    EXTERN_DECLARE_FUNCTION(vkCreateBuffer);
    EXTERN_DECLARE_FUNCTION(vkDestroyBuffer);
    EXTERN_DECLARE_FUNCTION(vkGetBufferMemoryRequirements);
    EXTERN_DECLARE_FUNCTION(vkMapMemory);
    EXTERN_DECLARE_FUNCTION(vkUnmapMemory);
    EXTERN_DECLARE_FUNCTION(vkBindBufferMemory);
    EXTERN_DECLARE_FUNCTION(vkCreateShaderModule);
    EXTERN_DECLARE_FUNCTION(vkDestroyShaderModule);
    EXTERN_DECLARE_FUNCTION(vkCreatePipelineLayout);
    EXTERN_DECLARE_FUNCTION(vkDestroyPipelineLayout);
    EXTERN_DECLARE_FUNCTION(vkCreateGraphicsPipelines);
    EXTERN_DECLARE_FUNCTION(vkDestroyPipeline);
    EXTERN_DECLARE_FUNCTION(vkCreateSemaphore);
    EXTERN_DECLARE_FUNCTION(vkCmdBeginRenderPass);
    EXTERN_DECLARE_FUNCTION(vkCmdBindPipeline);
    EXTERN_DECLARE_FUNCTION(vkCmdSetViewport);
    EXTERN_DECLARE_FUNCTION(vkCmdSetScissor);
    EXTERN_DECLARE_FUNCTION(vkCmdBindVertexBuffers);
    EXTERN_DECLARE_FUNCTION(vkCmdBindIndexBuffer);
    EXTERN_DECLARE_FUNCTION(vkCmdDraw);
    EXTERN_DECLARE_FUNCTION(vkCmdDrawIndexed);
    EXTERN_DECLARE_FUNCTION(vkCmdEndRenderPass);
    EXTERN_DECLARE_FUNCTION(vkDestroySemaphore);
    EXTERN_DECLARE_FUNCTION(vkEnumerateInstanceLayerProperties);
    EXTERN_DECLARE_FUNCTION(vkEnumerateInstanceExtensionProperties);
    EXTERN_DECLARE_FUNCTION(vkEnumerateDeviceLayerProperties);
    EXTERN_DECLARE_FUNCTION(vkEnumerateDeviceExtensionProperties);
    EXTERN_DECLARE_FUNCTION(vkFreeMemory);
    EXTERN_DECLARE_FUNCTION(vkCmdBindDescriptorSets);
    EXTERN_DECLARE_FUNCTION(vkCreateDescriptorSetLayout);
    EXTERN_DECLARE_FUNCTION(vkCreateDescriptorPool);
    EXTERN_DECLARE_FUNCTION(vkAllocateDescriptorSets);
    EXTERN_DECLARE_FUNCTION(vkUpdateDescriptorSets);
    EXTERN_DECLARE_FUNCTION(vkDestroyDescriptorSetLayout);
    EXTERN_DECLARE_FUNCTION(vkDestroyDescriptorPool);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceFormatProperties);
    EXTERN_DECLARE_FUNCTION(vkGetImageSubresourceLayout);
    EXTERN_DECLARE_FUNCTION(vkCreateSampler);
    EXTERN_DECLARE_FUNCTION(vkQueueWaitIdle);
    EXTERN_DECLARE_FUNCTION(vkFlushMappedMemoryRanges);
    EXTERN_DECLARE_FUNCTION(vkDestroySampler);
    EXTERN_DECLARE_FUNCTION(vkCmdPushConstants);
    EXTERN_DECLARE_FUNCTION(vkCmdCopyBufferToImage);
    EXTERN_DECLARE_FUNCTION(vkDeviceWaitIdle);
    EXTERN_DECLARE_FUNCTION(vkGetPhysicalDeviceFeatures);

#endif // VULKANDEFINITIONS_H_INCLUDED
