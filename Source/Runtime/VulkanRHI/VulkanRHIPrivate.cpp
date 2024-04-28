#include "VulkanDevice.h"
#include "VulkanDescriptorSets.h"
XVulkanDescriptorSetsLayout::XVulkanDescriptorSetsLayout(XVulkanDevice* InDevice)
	:Device(InDevice)
{

}

void XVulkanDescriptorSetsLayout::Compile(XVulkanDescriptorSetLayoutMap& LayoutMap)
{
	//LayoutHandles.reserve(SetLayouts.size());
	
	//static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	
	//for (XSetLayout& Layout : SetLayouts)
	{
		VkDescriptorSetLayout* DesSetLayoutPtr = &LayoutHandle;

		uint32 LayoutHash = SetLayout.GetHash();
		auto iter = LayoutMap.find(LayoutHash);
		if (iter != LayoutMap.end())
		{
			*DesSetLayoutPtr = iter->second;
			return;
		}

		VkDescriptorSetLayoutCreateInfo DescriptorLayoutInfo = {};
		DescriptorLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		DescriptorLayoutInfo.bindingCount = SetLayout.LayoutBindings.size();
		DescriptorLayoutInfo.pBindings = SetLayout.LayoutBindings.data();

		VULKAN_VARIFY(vkCreateDescriptorSetLayout(Device->GetVkDevice(), &DescriptorLayoutInfo, nullptr, DesSetLayoutPtr));

		LayoutMap[LayoutHash] = *DesSetLayoutPtr;
	}

	DescriptorSetAllocateInfo = {};
	DescriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	
	DescriptorSetAllocateInfo.descriptorSetCount = 1;//static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	DescriptorSetAllocateInfo.pSetLayouts = &LayoutHandle;
}