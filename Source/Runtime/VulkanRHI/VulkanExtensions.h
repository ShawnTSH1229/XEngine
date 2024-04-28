#pragma once
#include "VulkanDevice.h"

class XVulkanExtensionBase
{
public:
	XVulkanExtensionBase(const ACHAR* InExtensionName)
		:ExtensionName(InExtensionName)
	{

	}

	inline const ACHAR* GetExtensionName() const { return ExtensionName; }

protected:
	const ACHAR* ExtensionName;
};


class XVulkanDeviceExtension : public XVulkanExtensionBase
{
public:
	XVulkanDeviceExtension(XVulkanDevice* InDevice, const ACHAR* InExtensionName)
		:XVulkanExtensionBase(InExtensionName),
		Device(InDevice)
	{

	}

	static std::vector<XVulkanDeviceExtension>& GetSupportedDeviceExtensions(XVulkanDevice* InDevice);

	virtual void PrePhysicalDeviceProperties(VkPhysicalDeviceProperties2KHR& PhysicalDeviceProperties2) {}
	virtual void PrePhysicalDeviceFeatures(VkPhysicalDeviceFeatures2KHR& PhysicalDeviceFeatures2) {}
	virtual void PreCreateDevice(VkDeviceCreateInfo& DeviceInfo) {}

protected:
	XVulkanDevice* Device;
	
};