#pragma once
#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResource.h"

class XVulkanCommonPipelineDescriptorState
{
public:
	XVulkanCommonPipelineDescriptorState(XVulkanDevice* InDevice)
		:Device(InDevice)
	{

	}

	void CreateDescriptorWriteInfos();
protected:

	inline void Bind(VkCommandBuffer CmdBuffer, VkPipelineLayout PipelineLayout, VkPipelineBindPoint BindPoint)
	{
		vkCmdBindDescriptorSets(CmdBuffer, BindPoint, PipelineLayout, 0,
			1, &DescriptorSetHandle, (uint32)DynamicOffsets.size(), DynamicOffsets.data());
	}

	XVulkanDescriptorSetWriter DSWriter;
	XVulkanDescriptorSetWriteContainer DSWriteContainer;

	const XVulkanDescriptorSetsLayout* DescriptorSetsLayout = nullptr;
	
	VkDescriptorSet DescriptorSetHandle;

	//TODO:Dynamic Uniform Buffer
	std::vector<uint32> DynamicOffsets;

	XVulkanDevice* Device;
};

class XVulkanGraphicsPipelineDescriptorState : XVulkanCommonPipelineDescriptorState
{
public:
	XVulkanGraphicsPipelineDescriptorState(XVulkanDevice* InDevice, XVulkanRHIGraphicsPipelineState* InGfxPipeline);

	bool UpdateDescriptorSets(XVulkanCommandListContext* CmdListContext, XVulkanCmdBuffer* CmdBuffer);

	inline const XVulkanGfxPipelineDescriptorInfo& GetGfxPipelineDescriptorInfo() const
	{
		return *PipelineDescriptorInfo;
	}

	void SetTexture(uint8 DescriptorSet, uint32 BindingIndex, const XVulkanTextureBase* TextureBase, VkImageLayout Layout);
	
	void SetUniformBuffer(uint8 DescriptorSet, uint32 BindingIndex, const XVulkanConstantBuffer* UniformBuffer);
	
	inline void BindDescriptorSets(VkCommandBuffer CmdBuffer)
	{
		Bind(CmdBuffer, GfxPipeline->Layout->GetPipelineLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS);
	}

	inline void MarkDirty(bool bDirty)
	{
		bIsResourcesDirty |= bDirty;
		bIsDSetsKeyDirty |= bDirty;
	}
private:
	const XVulkanGfxPipelineDescriptorInfo* PipelineDescriptorInfo;

	XVulkanRHIGraphicsPipelineState* GfxPipeline;

	bool bIsResourcesDirty = true;
	mutable bool bIsDSetsKeyDirty = true;
};