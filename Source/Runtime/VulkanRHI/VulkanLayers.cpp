//TanGram:TODO:
//VK_LAYER_LUNARG_gfxreconstruct
//VK_LAYER_LUNARG_vktrace
//VK_LAYER_LUNARG_api_dump

//Only Get Required Extensions

#include <GLFW\glfw3.h>
#include "Runtime\VulkanRHI\VulkanPlatformRHI.h"

#define VULKAN_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

static bool FindlayerEnabled(std::vector<VkLayerProperties>& availableLayers, const ACHAR* layerName)
{
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerName, layerProperties.layerName) == 0) {
            layerFound = true;
            break;
        }
    }

    return layerFound;
}

static const std::vector<const ACHAR*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

void XVulkanPlatformRHI::GetInstanceLayersAndExtensions(std::vector<const ACHAR*>& OutLayer , std::vector<const ACHAR*>& OutExtension)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    XASSERT(FindlayerEnabled(availableLayers, VULKAN_VALIDATION_LAYER_NAME));
    OutLayer = validationLayers;

    uint32_t glfwExtensionCount = 0;
    const ACHAR** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const ACHAR*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#if RHI_RAYTRACING


#endif
    OutExtension = extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL XVulkanPlatformRHI::VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}