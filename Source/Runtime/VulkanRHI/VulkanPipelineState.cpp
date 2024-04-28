#include "VulkanContext.h"
#include "VulkanPipeline.h"
#include "VulkanPendingState.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"

void XVulkanCommandListContext::RHISetGraphicsPipelineState(XRHIGraphicsPSO* GraphicsState)
{
	PendingGfxState->SetGfxPipeline(static_cast<XVulkanRHIGraphicsPipelineState*>(GraphicsState));
	PendingGfxState->Bind(CmdBufferManager->GetActiveCmdBuffer()->GetHandle());
}

void XVulkanCommonPipelineDescriptorState::CreateDescriptorWriteInfos()
{
    const XVulkanSamplerState* DefaultSampler = Device->GetDefaultSampler();
    const XVulkanTextureView* DefaultImageView = Device->GetDefaultImageView();

    const auto& SetInfo = DescriptorSetsLayout->RemappingInfo.SetInfo;
    
    for(int32 Index = 0;Index< SetInfo.Types.size();Index++)
        DSWriteContainer.DescriptorWrites.push_back(VkWriteDescriptorSet{});
    for (int32 Index = 0; Index < SetInfo.NumImageInfos; Index++)
        DSWriteContainer.DescriptorImageInfo.emplace_back(VkDescriptorImageInfo{});
    for (int32 Index = 0; Index < SetInfo.NumBufferInfos; Index++)
        DSWriteContainer.DescriptorBufferInfo.emplace_back(VkDescriptorBufferInfo{});

    DSWriter.SetupDescriptorWrites(SetInfo.Types,
        DSWriteContainer.DescriptorWrites.data(), 
        DSWriteContainer.DescriptorImageInfo.data(), 
        DSWriteContainer.DescriptorBufferInfo.data(), 
        DefaultSampler, DefaultImageView);

}

XVulkanGraphicsPipelineDescriptorState::XVulkanGraphicsPipelineDescriptorState(XVulkanDevice* InDevice, XVulkanRHIGraphicsPipelineState* InGfxPipeline)
    :
    XVulkanCommonPipelineDescriptorState(InDevice),
    GfxPipeline(InGfxPipeline)
{
    DescriptorSetsLayout = InGfxPipeline->Layout->GetDescriptorSetsLayout();
    PipelineDescriptorInfo = &static_cast<XVulkanGfxLayout*>(InGfxPipeline->Layout)->GetGfxPipelineDescriptorInfo();

    CreateDescriptorWriteInfos();
}


