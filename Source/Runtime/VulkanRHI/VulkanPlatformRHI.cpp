
#include "VulkanPlatformRHI.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanViewport.h"
#include "Runtime\Core\PixelFormat.h"
#include <Runtime\ApplicationCore\Application.h>
#include <set>

XVulkanPlatformRHI::XVulkanPlatformRHI()
{
    GPixelFormats[(int)EPixelFormat::FT_Unknown].PlatformFormat = DXGI_FORMAT_UNKNOWN;
    //GPixelFormats[(int)EPixelFormat::FT_R16G16B16A16_FLOAT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    GPixelFormats[(int)EPixelFormat::FT_R8G8B8A8_UNORM].PlatformFormat = VK_FORMAT_R8G8B8A8_UNORM;
    GPixelFormats[(int)EPixelFormat::FT_R8G8B8A8_UNORM_SRGB].PlatformFormat = VK_FORMAT_R8G8B8A8_SRGB;
    GPixelFormats[(int)EPixelFormat::FT_R24G8_TYPELESS].PlatformFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    //GPixelFormats[(int)EPixelFormat::FT_R11G11B10_FLOAT].PlatformFormat = DXGI_FORMAT_R11G11B10_FLOAT;
    //GPixelFormats[(int)EPixelFormat::FT_R16_FLOAT].PlatformFormat = DXGI_FORMAT_R16_FLOAT;
    //GPixelFormats[(int)EPixelFormat::FT_R32_UINT].PlatformFormat = DXGI_FORMAT_R32_UINT;
    //GPixelFormats[(int)EPixelFormat::FT_R32G32B32A32_UINT].PlatformFormat = DXGI_FORMAT_R32G32B32A32_UINT;
    //GPixelFormats[(int)EPixelFormat::FT_R32_TYPELESS].PlatformFormat = DXGI_FORMAT_R32_TYPELESS;
    GPixelFormats[(int)EPixelFormat::FT_B8G8A8A8_UNORM_SRGB].PlatformFormat = VK_FORMAT_B8G8R8A8_SRGB;
}

XVulkanPlatformRHI::~XVulkanPlatformRHI()
{
    delete VulkanViewport;
    delete Device;
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        func(Instance, DebugMessenger, nullptr);
    }
    vkDestroyInstance(Instance, nullptr);
}

void XVulkanPlatformRHI::Init()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const ACHAR*> Layers;
    std::vector<const ACHAR*> Extensions;
    GetInstanceLayersAndExtensions(Layers, Extensions);
    createInfo.enabledExtensionCount = Extensions.size();
    createInfo.ppEnabledExtensionNames = Extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(Layers.size());
    createInfo.ppEnabledLayerNames = Layers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = VkDebugCallback;
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    VULKAN_VARIFY(vkCreateInstance(&createInfo, nullptr, &Instance))
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) 
    {
        VULKAN_VARIFY(func(Instance, &debugCreateInfo, nullptr, &DebugMessenger))
    }
    else 
    {
        XASSERT(false);
    }

    SelectAndInitDevice();

    EPixelFormat BackBufferFormat = EPixelFormat::FT_B8G8A8A8_UNORM_SRGB;
    VulkanViewport = new XVulkanViewport(BackBufferFormat, Device, XApplication::Application->ClientWidth, XApplication::Application->ClientHeight, XApplication::Application->GetPlatformHandle(), Instance);

    GRHICmdList.SetContext(Device->GetGfxContex());
}

XRHITexture* XVulkanPlatformRHI::RHIGetCurrentBackTexture()
{
    return VulkanViewport->GetCurrentBackTexture();
}

bool IsDeviceSuitable(VkPhysicalDevice device) 
{  
    //get a gfx queue
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            return true;
        }
    }

    //swap chain support

    return false;
}

void XVulkanPlatformRHI::SelectAndInitDevice()
{
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &deviceCount, nullptr);
    XASSERT(deviceCount != 0);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(Instance, &deviceCount, devices.data());

    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
    for (const auto& device : devices) 
    {
        if (IsDeviceSuitable(device))
        {
            PhysicalDevice = device;
            break;
        }
    }

    Device = new XVulkanDevice(this, PhysicalDevice);
    Device->CreateDevice();
    Device->InitGPU();
}






