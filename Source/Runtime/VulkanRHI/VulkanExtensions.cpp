#include "VulkanExtensions.h"

template <typename ExistingChainType, typename NewStructType>
static void AddToPNext(ExistingChainType& Existing, NewStructType& Added)
{
	Added.pNext = (void*)Existing.pNext;
	Existing.pNext = (void*)&Added;
}


class XVulkanAccelerationExtensionStructExtension : public XVulkanDeviceExtension
{
public:
	XVulkanAccelerationExtensionStructExtension(XVulkanDevice* InDevice)
		:XVulkanDeviceExtension(InDevice, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
	{

	}

	virtual void PrePhysicalDeviceProperties(VkPhysicalDeviceProperties2KHR& PhysicalDeviceProperties2)final override 
	{
#if RHI_RAYTRACING
		const XVulkanDeviceExtensionProperties& RayTracingProperties = Device->GetDeviceExtensionProperties();
		VkPhysicalDeviceAccelerationStructurePropertiesKHR& AccelerationStructure = const_cast<VkPhysicalDeviceAccelerationStructurePropertiesKHR&>(RayTracingProperties.AccelerationStructure);
		AccelerationStructure = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };
		AddToPNext(PhysicalDeviceProperties2, AccelerationStructure);
#endif	
	}

	virtual void PrePhysicalDeviceFeatures(VkPhysicalDeviceFeatures2KHR& PhysicalDeviceFeatures2)final override 
	{
		AccelerationStructureFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
		AddToPNext(PhysicalDeviceFeatures2, AccelerationStructureFeatures);
	}

	virtual void PreCreateDevice(VkDeviceCreateInfo& DeviceCreateInfo) final override
	{
		AddToPNext(DeviceCreateInfo, AccelerationStructureFeatures);
	}

private:
	VkPhysicalDeviceAccelerationStructureFeaturesKHR AccelerationStructureFeatures;
};

class XVulkanayTracingPipelineExtension : public XVulkanDeviceExtension
{
public:
	XVulkanayTracingPipelineExtension(XVulkanDevice* InDevice)
		:XVulkanDeviceExtension(InDevice, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
	{

	}

	virtual void PrePhysicalDeviceProperties(VkPhysicalDeviceProperties2KHR& PhysicalDeviceProperties2)final override
	{
#if RHI_RAYTRACING
		const XVulkanDeviceExtensionProperties& RayTracingProperties = Device->GetDeviceExtensionProperties();
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR& RayTracingPipeline = const_cast<VkPhysicalDeviceRayTracingPipelinePropertiesKHR&>(RayTracingProperties.RayTracingPipeline);
		RayTracingPipeline = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		AddToPNext(PhysicalDeviceProperties2, RayTracingPipeline);
#endif
	}

	virtual void PrePhysicalDeviceFeatures(VkPhysicalDeviceFeatures2KHR& PhysicalDeviceFeatures2)final override
	{
		RayTracingPipelineFeature = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
		AddToPNext(PhysicalDeviceFeatures2, RayTracingPipelineFeature);
	}

	virtual void PreCreateDevice(VkDeviceCreateInfo& DeviceCreateInfo) final override
	{
		AddToPNext(DeviceCreateInfo, RayTracingPipelineFeature);
	}

private:
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR RayTracingPipelineFeature;
};

class XVulkanEXTDescriptorBufferExtension : public XVulkanDeviceExtension
{
public:
	XVulkanEXTDescriptorBufferExtension(XVulkanDevice* InDevice)
		:XVulkanDeviceExtension(InDevice, VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME)
	{

	}

	virtual void PrePhysicalDeviceProperties(VkPhysicalDeviceProperties2KHR& PhysicalDeviceProperties2)final override
	{
#if RHI_RAYTRACING
		const XVulkanDeviceExtensionProperties& DeviceExtensionProperties = Device->GetDeviceExtensionProperties();
		VkPhysicalDeviceDescriptorBufferPropertiesEXT& DescriptorBufferProperties = const_cast<VkPhysicalDeviceDescriptorBufferPropertiesEXT&>(DeviceExtensionProperties.DescriptorBufferProps);
		DescriptorBufferProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		AddToPNext(PhysicalDeviceProperties2, DescriptorBufferProperties);
#endif
	}

	virtual void PrePhysicalDeviceFeatures(VkPhysicalDeviceFeatures2KHR& PhysicalDeviceFeatures2)final override
	{
		DescriptorBufferFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
		AddToPNext(PhysicalDeviceFeatures2, DescriptorBufferFeatures);
	}

	virtual void PreCreateDevice(VkDeviceCreateInfo& DeviceCreateInfo) final override
	{
		AddToPNext(DeviceCreateInfo, DescriptorBufferFeatures);
	}

private:
	VkPhysicalDeviceDescriptorBufferFeaturesEXT DescriptorBufferFeatures;
};

static std::vector<XVulkanDeviceExtension> vkDeviceExtensions;
static bool bInit = false;
std::vector<XVulkanDeviceExtension>& XVulkanDeviceExtension::GetSupportedDeviceExtensions(XVulkanDevice* InDevice)
{
	if (bInit == false)
	{
		vkDeviceExtensions.push_back(XVulkanAccelerationExtensionStructExtension(InDevice));
		vkDeviceExtensions.push_back(XVulkanayTracingPipelineExtension(InDevice));
	}
	return vkDeviceExtensions;
};