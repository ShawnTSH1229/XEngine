#include "VulkanDevice.h"
#include "VulkanExtensions.h"
#include "VulkanPipeline.h"
#include "VulkanLoader.h"

std::vector<const ACHAR*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if RHI_RAYTRACING
    ,VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
#endif
};

XVulkanDevice::XVulkanDevice(XVulkanPlatformRHI* InRHI, VkPhysicalDevice InGpu)
	: Device(VK_NULL_HANDLE)
	, RHI(InRHI)
	, Gpu(InGpu)
    , GfxQueue(nullptr)
    , PresentQueue(nullptr)
    , MemoryManager(this, &DeviceMemoryManager)
    , FenceManager(this)
{
    PipelineStateCache = new XVulkanPipelineStateCacheManager(this);
    DescriptorPoolsManager = new XVulkanDescriptorPoolsManager(this);
}

XVulkanDevice::~XVulkanDevice()
{
    delete PipelineStateCache;
    delete GfxContext;
    delete GfxQueue;
    delete DescriptorPoolsManager;
    delete DefaultImage;
    vkDestroyDevice(Device, nullptr);
}

void XVulkanDevice::InitGPU()
{
    GfxContext = new XVulkanCommandListContext(RHI, this, GfxQueue);
   
    
    // Setup default resource
    {
        XSamplerStateInitializerRHI Default(SF_Point);
        DefaultSampler = RHI->RHICreateSamplerState(Default);

        XVulkanEvictable HackHere;

        DefaultImage = new XVulkanSurface(&HackHere, this, EPixelFormat::FT_R8G8B8A8_UNORM, 1, 1, VK_IMAGE_VIEW_TYPE_2D, ETextureCreateFlags::TexCreate_ShaderResource, 1, nullptr, 0);
        DefaultTextureView.Create(this, DefaultImage->Image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, VK_FORMAT_R8G8B8A8_UNORM);
    }

    //BindlessDescriptorManager = new XVulkanBindlessDescriptorManager(this);
    //BindlessDescriptorManager->Init();
}

void XVulkanDevice::CreateDevice()
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(Gpu, &queueFamilyCount, queueFamilies.data());

    int32 GfxQueueFamilyIndex = -1;
    int32 ComputeQueueFamilyIndex = -1;
    int32 TransferQueueFamilyIndex = -1;
    for (int32 Index = 0; Index < queueFamilies.size(); Index++)
    {
        if (queueFamilies[Index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            GfxQueueFamilyIndex = Index;
            break;
        }
    }

    GfxQueueIndex = GfxQueueFamilyIndex;
    float queuePriority = 1.0f;

    auto& DeviceExtensionArray = XVulkanDeviceExtension::GetSupportedDeviceExtensions(this);
    
    {
        VkPhysicalDeviceFeatures2 PhysicalDeviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        for (auto iter : DeviceExtensionArray)
        {
            iter.PrePhysicalDeviceFeatures(PhysicalDeviceFeatures2);
        }
        vkGetPhysicalDeviceFeatures2(Gpu, &PhysicalDeviceFeatures2);
    }


    {
        VkPhysicalDeviceProperties2 PhysicalDeviceProperties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        for (auto iter : DeviceExtensionArray)
        {
            iter.PrePhysicalDeviceProperties(PhysicalDeviceProperties2);
        }
        vkGetPhysicalDeviceProperties2(Gpu, &PhysicalDeviceProperties2);
    }
    

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = GfxQueueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;


    for (auto iter : DeviceExtensionArray)
    {
        deviceExtensions.push_back(iter.GetExtensionName());
        iter.PreCreateDevice(createInfo);
    }

    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledLayerCount = 0;

    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

   

    VULKAN_VARIFY(vkCreateDevice(Gpu, &createInfo, nullptr, &Device));
    GfxQueue = new XVulkanQueue(this, GfxQueueFamilyIndex);
    
    DeviceMemoryManager.Init(this);
    StagingManager.Init(this);
    MemoryManager.Init();

    VulkanExtension::InitExtensionFunction(Device);
    GRHIRayTracingScratchBufferAlignment = GetDeviceExtensionProperties().AccelerationStructure.minAccelerationStructureScratchOffsetAlignment;
    GRHIRayTracingAccelerationStructureAlignment = 256;
}

namespace VulkanExtension
{
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
    PFN_vkGetDescriptorEXT vkGetDescriptorEXT = nullptr;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
    PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT = nullptr;
}

void VulkanExtension::InitExtensionFunction(VkDevice Device)
{
    vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(Device, "vkGetAccelerationStructureBuildSizesKHR");
    vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(Device, "vkGetBufferDeviceAddressKHR");
    vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(Device, "vkCreateAccelerationStructureKHR");
    vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(Device, "vkGetAccelerationStructureDeviceAddressKHR");
    vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(Device, "vkCmdBuildAccelerationStructuresKHR");
    vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(Device, "vkCreateRayTracingPipelinesKHR");
    vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(Device, "vkGetRayTracingShaderGroupHandlesKHR");
    vkGetDescriptorEXT = (PFN_vkGetDescriptorEXT)vkGetDeviceProcAddr(Device, "vkGetDescriptorEXT");
    vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(Device, "vkCmdTraceRaysKHR");
    vkGetDescriptorSetLayoutSizeEXT = (PFN_vkGetDescriptorSetLayoutSizeEXT)vkGetDeviceProcAddr(Device, "vkGetDescriptorSetLayoutSizeEXT");
}


