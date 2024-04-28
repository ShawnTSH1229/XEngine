#include "VulkanPendingState.h"
#include "VulkanRHIPrivate.h"
#include "VulkanDevice.h"
#include "VulaknHack.h"

XVulkanDescriptorPool::XVulkanDescriptorPool(XVulkanDevice* InDevice, const XVulkanDescriptorSetsLayout* InLayout, uint32 MaxSetsAllocations)
    : Device(InDevice)
    , Layout(InLayout)
    , DescriptorPool(VK_NULL_HANDLE)
{
    MaxDescriptorSets = MaxSetsAllocations;

    std::vector<VkDescriptorPoolSize>Types;
    for (uint32 TypeIndex = VK_DESCRIPTOR_TYPE_SAMPLER; TypeIndex <= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++TypeIndex)
    {
        VkDescriptorType DescriptorType = (VkDescriptorType)TypeIndex;
        uint32 NumTypesUsed = Layout->GetTypesUsed(DescriptorType);
        if (NumTypesUsed > 0)
        {
            VkDescriptorPoolSize Type = {};
            Type.type = DescriptorType;
            Type.descriptorCount = NumTypesUsed * MaxSetsAllocations;
            Types.push_back(Type);
        }
    }


    VkDescriptorPoolCreateInfo PoolInfo = {};
    PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolInfo.poolSizeCount = Types.size();
    PoolInfo.pPoolSizes = Types.data();
    PoolInfo.maxSets = MaxDescriptorSets;

    VULKAN_VARIFY(vkCreateDescriptorPool(Device->GetVkDevice(), &PoolInfo, nullptr, &DescriptorPool));
}

bool XVulkanDescriptorPool::AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VkDescriptorSet* OutSets)
{
    VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo = InDescriptorSetAllocateInfo;
    DescriptorSetAllocateInfo.descriptorPool = DescriptorPool;

    return VK_SUCCESS == vkAllocateDescriptorSets(Device->GetVkDevice(), &DescriptorSetAllocateInfo, OutSets);
}

XVulkanDescriptorPool* XVulkanTypedDescriptorPoolArray::PushNewPool()
{
    // Max number of descriptor sets layout allocations
    const uint32 MaxSetsAllocationsBase = 32;
    // Allow max 128 setS per pool (32 << 2)
    const uint32 MaxSetsAllocations = MaxSetsAllocationsBase << 2u;

    XVulkanDescriptorPool* NewPool = new XVulkanDescriptorPool(Device, Layout, MaxSetsAllocations);

    PoolArray.push_back(NewPool);

    return NewPool;

}

XVulkanTypedDescriptorPoolArray::~XVulkanTypedDescriptorPoolArray()
{
    for (auto iter : PoolArray)
    {
        delete iter;
    }
}

bool XVulkanTypedDescriptorPoolArray::AllocateDescriptorSets(const XVulkanDescriptorSetsLayout* Layout, VkDescriptorSet* OutSets)
{
    const VkDescriptorSetLayout LayoutHandle = Layout->LayoutHandle;

    for (int32 Index = 0; Index < (std::min)(PoolArray.size(), std::size_t(32)); Index++)
    {
        if (PoolArray[Index]->AllocateDescriptorSets(Layout->GetAllocateInfo(), OutSets) == false)
        {
            PushNewPool();
        }
        else
        {
            return true;
        }
    }

    return true;

}


XVulkanDescriptorPoolSetContainer::~XVulkanDescriptorPoolSetContainer()
{
    for (auto iter : TypedDescriptorPools)
    {
        if (iter.second)
        {
            delete iter.second;
        }
    }
}

XVulkanTypedDescriptorPoolArray* XVulkanDescriptorPoolSetContainer::AcquireTypedPoolArray(const XVulkanDescriptorSetsLayout* Layout)
{
    const uint32 Hash = Layout->GetHash();

    auto iter = TypedDescriptorPools.find(Hash);
    if (iter == TypedDescriptorPools.end())
    {
        XVulkanTypedDescriptorPoolArray* TypedPool = new XVulkanTypedDescriptorPoolArray(Device, Layout);
        TypedDescriptorPools[Hash] = TypedPool;
        return TypedPool;
    }

    return iter->second;
}

void XVulkanPendingGfxState::SetGfxPipeline(XVulkanRHIGraphicsPipelineState* InGfxPipeline)
{
    bool bChanged = false;
    if (InGfxPipeline != CurrentPipeline)
    {
        CurrentPipeline = InGfxPipeline;
        auto iter = PipelineStates.find(InGfxPipeline);
        if (iter != PipelineStates.end())
        {
            CurrentState = iter->second;
        }
        else
        {
            CurrentState = new XVulkanGraphicsPipelineDescriptorState(Device, InGfxPipeline);
            PipelineStates[CurrentPipeline] = CurrentState;
        }
        bChanged = true;
    }

    if (bChanged)
    {
        //CurrentState->Reset();
    }
}

void XVulkanPendingGfxState::PrepareForDraw(XVulkanCmdBuffer* CmdBuffer)
{
    UpdateDynamicStates(CmdBuffer);

   bool bHasDescriptorSets = CurrentState->UpdateDescriptorSets(nullptr, CmdBuffer);

    if (bHasDescriptorSets)
    {
        CurrentState->BindDescriptorSets(CmdBuffer->GetHandle());
    }

    //TODO:BindMultiVertexBuffer
    VkDeviceSize Offset = PendingStreams[0].BufferOffset;
    vkCmdBindVertexBuffers(CmdBuffer->GetHandle(), 0, 1, &PendingStreams[0].Stream, &Offset);
}

void XVulkanPendingGfxState::SetTextureForStage(EShaderType ShaderType, uint32 ParameterIndex, const XVulkanTextureBase* TextureBase, VkImageLayout Layout)
{
    CurrentState->SetTexture(0, ParameterIndex, TextureBase, Layout);
}


bool XVulkanGraphicsPipelineDescriptorState::UpdateDescriptorSets(XVulkanCommandListContext* CmdListContext, XVulkanCmdBuffer* CmdBuffer)
{
    const bool bNeedsWrite = bIsResourcesDirty || VkHack::TempMakeDirty();
    if (bNeedsWrite == false)
    {
        return false;
    }

    if (!CmdBuffer->AcquirePoolSetAndDescriptorsIfNeeded(DescriptorSetsLayout, true, &DescriptorSetHandle))
    {
        return false;
    }

    DSWriter.SetDescriptorSet(DescriptorSetHandle);

    vkUpdateDescriptorSets(Device->GetVkDevice(), DSWriteContainer.DescriptorWrites.size(), DSWriteContainer.DescriptorWrites.data(), 0, nullptr);
    bIsResourcesDirty = false;
    return true;
}

void XVulkanGraphicsPipelineDescriptorState::SetTexture(uint8 DescriptorSet, uint32 BindingIndex, const XVulkanTextureBase* TextureBase, VkImageLayout Layout)
{
    // If the texture doesn't support sampling, then we read it through a UAV
    MarkDirty(DSWriter.WriteImage(BindingIndex, &TextureBase->DefaultView, Layout));
}

void XVulkanGraphicsPipelineDescriptorState::SetUniformBuffer(uint8 DescriptorSet, uint32 BindingIndex, const XVulkanConstantBuffer* UniformBuffer)
{
    MarkDirty(DSWriter.WriteDynamicUniformBuffer(BindingIndex, UniformBuffer->Allocation, UniformBuffer->GetOffset(),UniformBuffer->GetSize()));
}
