#include "VulkanResource.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"

void XVulkanTextureView::CreateImpl(XVulkanDevice* Device,VkImage InImage , VkImageViewType ViewType , VkImageAspectFlags Aspect, VkFormat Format)
{
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = InImage;

	createInfo.viewType = ViewType;
	createInfo.format = Format;

	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	createInfo.subresourceRange.aspectMask = Aspect;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	VULKAN_VARIFY(vkCreateImageView(Device->GetVkDevice(), &createInfo, nullptr, &View));
}

void XVulkanTextureView::Create(XVulkanDevice* Device, VkImage InImage, VkImageViewType ViewType, VkImageAspectFlags Aspect, VkFormat Format)
{
	CreateImpl(Device, InImage, ViewType, Aspect, Format);
	Image = InImage;
}

static VkShaderModule CreateShaderModule(XVulkanDevice* Device, XVulkanShader::XSpirvContainer& SpirvCode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = SpirvCode.SpirvCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(SpirvCode.SpirvCode.data());

	VkShaderModule shaderModule;
	VULKAN_VARIFY(vkCreateShaderModule(Device->GetVkDevice(), &createInfo, nullptr, &shaderModule));
	return shaderModule;
}

XVulkanShader::XVulkanShader(XVulkanDevice* InDevice, EShaderType InShaderType)
	:Device(InDevice)
{
}

VkShaderModule XVulkanShader::GetOrCreateHandle()
{
	return CreateShaderModule(Device, SpirvContainer);
}


VkShaderModule XVulkanShader::CreateHandle(const XGfxPipelineDesc& Desc, uint32 LayoutHash)
{
	VkShaderModule ShaderModule = CreateShaderModule(Device, SpirvContainer);
	ShaderModules[LayoutHash] = ShaderModule;
	return ShaderModule;
}

XVulkanShaderResourceView::XVulkanShaderResourceView(XVulkanDevice* InDevice, XVulkanResourceMultiBuffer* InSourceBuffer, uint32 InOffset)
{

#if RHI_RAYTRACING
	if (EnumHasAnyFlags(InSourceBuffer->GetUsage(), EBufferUsage::BUF_AccelerationStructure))
	{
		XASSERT(false);

		SourceRHIBuffer = std::shared_ptr<XRHIBuffer>(InSourceBuffer);

		VkAccelerationStructureCreateInfoKHR CreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		CreateInfo.buffer = InSourceBuffer->Buffer.VulkanHandle;
		CreateInfo.offset = InOffset;
		CreateInfo.size = InSourceBuffer->GetSize() - InOffset;
		CreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

		VkDevice NativeDevice = InDevice->GetVkDevice();
		VULKAN_VARIFY(VulkanExtension::vkCreateAccelerationStructureKHR(NativeDevice, &CreateInfo, nullptr, &AccelerationStructureHandle));
	}
	else
#endif
	{
		SourceStructuredBuffer = InSourceBuffer;
		Size = InSourceBuffer->GetSize() - InOffset;
		Offset = InOffset;
	}
}
