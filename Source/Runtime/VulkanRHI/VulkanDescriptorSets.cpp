#include "VulkanDescriptorSets.h"
#include "VulkanResource.h"
#include "VulkanCommon.h"
#include "VulkanLoader.h"
#include "VulkanDevice.h"

static inline uint8 GetIndexForDescritorType(VkDescriptorType DescriptorType)
{
    switch (DescriptorType)
    {
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:return VulkanBindless::BindlesssStorageBufferSet;
    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:return VulkanBindless::BindlessAccelerationStructureSet;
    }
    XASSERT(false);

    return VulkanBindless::None;
}

static inline uint32 GetDescriptorTypeSize(XVulkanDevice* Device, VkDescriptorType DescriptorType)
{
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& DescriptorBufferProperties = Device->GetDeviceExtensionProperties().DescriptorBufferProps;

    switch (DescriptorType)
    {
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return DescriptorBufferProperties.robustStorageBufferDescriptorSize;
    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return DescriptorBufferProperties.accelerationStructureDescriptorSize;
    default:
        XASSERT(false);
    }
    return 0;
}

static inline VkDescriptorType GetDescriptorTypeFromSetIndex(uint8 SetIndex)
{
    switch (SetIndex)
    {
    case VulkanBindless::None:          return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    case VulkanBindless::BindlesssStorageBufferSet:          return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case VulkanBindless::BindlessAccelerationStructureSet:  return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    default: XASSERT(false);
    }
    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

void XVulkanDescriptorSetsLayoutInfo::FinalizeBindings_Gfx(XVulkanShader* VertexShader, XVulkanShader* PixelShader)
{
    if (VertexShader->ResourceCount.NumCBV)
    {
        for (int32 Index = 0; Index < VertexShader->ResourceCount.NumCBV; Index++)
        {
            RemappingInfo.SetInfo.Types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            RemappingInfo.SetInfo.NumBufferInfos++;
            VkDescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = RemappingInfo.SetInfo.Types.size() - 1;
            uboLayoutBinding.descriptorCount = 1;//?
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.pImmutableSamplers = nullptr;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            SetLayout.LayoutBindings.push_back(uboLayoutBinding);

            LayoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]++;
        }
    }

    if (PixelShader->ResourceCount.NumCBV)
    {
        for (int32 Index = 0; Index < PixelShader->ResourceCount.NumCBV; Index++)
        {
            RemappingInfo.SetInfo.Types.push_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            RemappingInfo.SetInfo.NumBufferInfos++;
            VkDescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = RemappingInfo.SetInfo.Types.size() - 1;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.pImmutableSamplers = nullptr;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            SetLayout.LayoutBindings.push_back(uboLayoutBinding);

            LayoutTypes[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]++;
        }
    }

    //COMBINED IMAGE AND SAMPLER!!

    if (PixelShader->ResourceCount.NumSRV)
    {
        for (int32 Index = 0; Index < PixelShader->ResourceCount.NumSRV; Index++)
        {
            RemappingInfo.SetInfo.Types.push_back(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
            RemappingInfo.SetInfo.NumImageInfos++;
            VkDescriptorSetLayoutBinding textureLayoutBinding{};
            textureLayoutBinding.binding = RemappingInfo.SetInfo.Types.size() - 1;
            textureLayoutBinding.descriptorCount = 1;
            textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            textureLayoutBinding.pImmutableSamplers = nullptr;
            textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            SetLayout.LayoutBindings.push_back(textureLayoutBinding);

            LayoutTypes[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE]++;
        }
    }

    if (PixelShader->ResourceCount.NumSampler)
    {
        for (int32 Index = 0; Index < PixelShader->ResourceCount.NumSampler; Index++)
        {
            RemappingInfo.SetInfo.Types.push_back(VK_DESCRIPTOR_TYPE_SAMPLER);
            RemappingInfo.SetInfo.NumImageInfos++;
            VkDescriptorSetLayoutBinding samplerBinding{};
            samplerBinding.binding = RemappingInfo.SetInfo.Types.size() - 1;
            samplerBinding.descriptorCount = 1;
            samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerBinding.pImmutableSamplers = nullptr;
            samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            SetLayout.LayoutBindings.push_back(samplerBinding);

            LayoutTypes[VK_DESCRIPTOR_TYPE_SAMPLER]++;
        }
    }

    SetLayout.GenerateHash();
    Hash = SetLayout.Hash;
}

XVulkanDescriptorPoolSetContainer& XVulkanDescriptorPoolsManager::AcquirePoolSetContainer()
{
    for (int32 Index = 0; Index < PoolSets.size(); ++Index)
    {
        auto* PoolSet = PoolSets[Index];
        if (PoolSet->IsUnused())
        {
            PoolSet->SetUsed(true);
            return *PoolSet;
        }
    }

    XVulkanDescriptorPoolSetContainer* PoolSet = new XVulkanDescriptorPoolSetContainer(Device);
    PoolSets.push_back(PoolSet);
    return *PoolSet;
}

void XVulkanGfxPipelineDescriptorInfo::Initialize(const XDescriptorSetRemappingInfo& InRemappingInfo)
{
    DescriptorSetRemappingInfo = InRemappingInfo;
}

bool XVulkanGfxPipelineDescriptorInfo::GetDescriptorSetAndBindingIndex(const EShaderType Stage, int32 ParameterIndex, uint8& OutDescriptorSet, uint32& OutBindingIndex) const
{
    return false;
}

XDescriptorSetRemappingInfo::~XDescriptorSetRemappingInfo()
{
    int32 ForDebug;
}

XVulkanBindlessDescriptorManager::XVulkanBindlessDescriptorManager(XVulkanDevice* InDevice)
    :Device(InDevice)
{
    memset(BufferBindingInfo, 0, sizeof(VkDescriptorBufferBindingInfoEXT) * VulkanBindless::NumBindLessSet);
    for (uint32 Index = VulkanBindless::None + 1; Index < VulkanBindless::NumBindLessSet; Index++)
    {
        BufferIndices[Index] = Index;
    }
}

void XVulkanBindlessDescriptorManager::UpdateBuffer(XRHIDescriptorHandle DescriptorHandle, VkBuffer Buffer, VkDeviceSize BufferOffset, VkDeviceSize BufferSize)
{
    VkBufferDeviceAddressInfo BufferInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    BufferInfo.buffer = Buffer;
    const VkDeviceAddress BufferAddress = VulkanExtension::vkGetBufferDeviceAddressKHR(Device->GetVkDevice(), &BufferInfo);
    UpdateBuffer(DescriptorHandle, BufferAddress + BufferOffset, BufferSize);
}

void XVulkanBindlessDescriptorManager::UpdateBuffer(XRHIDescriptorHandle DescriptorHandle, VkDeviceAddress BufferAddress, VkDeviceSize BufferSize)
{
    VkDescriptorAddressInfoEXT AddressInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
    AddressInfo.address = BufferAddress;
    AddressInfo.range = BufferSize;

    VkDescriptorDataEXT DescriptorData;
    DescriptorData.pStorageBuffer = &AddressInfo;
    UpdateDescriptor(DescriptorHandle, DescriptorData);
}

void XVulkanBindlessDescriptorManager::UpdateDescriptor(XRHIDescriptorHandle DescriptorHandle, VkDescriptorDataEXT DescriptorData)
{
    const uint8 SetIndex = DescriptorHandle.Type;
    BindlessSetState& State = BindlessSetStates[SetIndex];
    const uint32 ByteOffset = DescriptorHandle.Index * State.DescriptorSize;

    VkDescriptorGetInfoEXT Info = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
    Info.type = State.DescriptorType;
    Info.data = DescriptorData;

    VulkanExtension::vkGetDescriptorEXT(Device->GetVkDevice(), &Info, State.DescriptorSize, &State.DebugDescriptors[ByteOffset]);
    memcpy(State.MappedPointer, &State.DebugDescriptors[ByteOffset], State.DescriptorSize);
}

void XVulkanBindlessDescriptorManager::Unregister(XRHIDescriptorHandle DescriptorHandle)
{
    if (DescriptorHandle.IsValid())
    {
        const uint8 SetIndex = DescriptorHandle.Type;
        BindlessSetState& State = BindlessSetStates[SetIndex];
        const uint32 PreviousHead = State.FreeListHead;
        State.FreeListHead = DescriptorHandle.Index;
        const uint32 ByteOffset = DescriptorHandle.Index * State.DescriptorSize;
        uint32* NewSlot = (uint32*)(&State.DebugDescriptors[ByteOffset]);
        memset(NewSlot, 0, State.DescriptorSize);
        *NewSlot = PreviousHead;
    }
}

void XVulkanBindlessDescriptorManager::Init()
{
    const VkDevice DeviceHandle = Device->GetVkDevice();
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& DescriptorBufferProperties = Device->GetDeviceExtensionProperties().DescriptorBufferProps;

    {
        VkDescriptorSetLayoutCreateInfo EmptyDescriptorSetLayoutCreateInfo;
        EmptyDescriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        VULKAN_VARIFY(vkCreateDescriptorSetLayout(DeviceHandle, &EmptyDescriptorSetLayoutCreateInfo, nullptr, &EmptyDescriptorSetLayout));
    }

    {
        auto InitBindlessState = [&](VkDescriptorType DescriptorType, BindlessSetState& OutState)
        {
            OutState.DescriptorType = DescriptorType;
            OutState.DescriptorSize = GetDescriptorTypeSize(Device,DescriptorType);
            OutState.MaxDescriptorCount = 256;
        };

        auto CreateDescriptorSetlayout = [&](const BindlessSetState& State)
        {
            XASSERT(State.DescriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM);

            VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding;
            DescriptorSetLayoutBinding.binding = 0;
            DescriptorSetLayoutBinding.descriptorType = State.DescriptorType;
            DescriptorSetLayoutBinding.descriptorCount = State.MaxDescriptorCount;
            DescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
            DescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

            const VkDescriptorBindingFlags DescriptorBindingFlags = 0;

            VkDescriptorSetLayoutBindingFlagsCreateInfo DescriptorSetLayoutBindingFlagsCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
            DescriptorSetLayoutBindingFlagsCreateInfo.bindingCount = 1;
            DescriptorSetLayoutBindingFlagsCreateInfo.pBindingFlags = &DescriptorBindingFlags;

            VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            DescriptorSetLayoutCreateInfo.pBindings = &DescriptorSetLayoutBinding;
            DescriptorSetLayoutCreateInfo.bindingCount = 1;
            DescriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
            DescriptorSetLayoutCreateInfo.pNext = &DescriptorSetLayoutBindingFlagsCreateInfo;

            VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
            VULKAN_VARIFY(vkCreateDescriptorSetLayout(DeviceHandle, &DescriptorSetLayoutCreateInfo, nullptr, &DescriptorSetLayout));
            return DescriptorSetLayout;
        };

        auto CreateDescriptorBuffer = [&](BindlessSetState& InOutState, VkDescriptorBufferBindingInfoEXT& OutBufferBindingInfo)
        {
            const VkBufferUsageFlags BufferUsageFlags =
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

            const uint32 DescriptorBufferSize = InOutState.DescriptorSize * InOutState.MaxDescriptorCount;
            InOutState.DebugDescriptors.resize(DescriptorBufferSize);

            VkDeviceSize LayoutSizeInBytes = 0;
            VulkanExtension::vkGetDescriptorSetLayoutSizeEXT(DeviceHandle, InOutState.DescriptorSetLayout, &LayoutSizeInBytes);
            XASSERT(LayoutSizeInBytes == InOutState.DescriptorSize * InOutState.MaxDescriptorCount);
            static_assert(VulkanBindless::NumBindLessSet == 3);//sampler type

            // Create descriptor buffer
            {
                VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                BufferCreateInfo.size = DescriptorBufferSize;
                BufferCreateInfo.usage = BufferUsageFlags;
                VULKAN_VARIFY(vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &InOutState.BufferHandle));
            }

            // Allocate buffer memory, bind and map
            {
                VkMemoryRequirements BufferMemoryReqs;
                memset(&BufferMemoryReqs, 0, sizeof(VkMemoryRequirements));
                vkGetBufferMemoryRequirements(DeviceHandle, InOutState.BufferHandle, &BufferMemoryReqs);
               

                uint32 MemoryTypeIndex = 0;
                VULKAN_VARIFY(Device->GetDeviceMemoryManager().GetMemoryTypeFromProperties(BufferMemoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &MemoryTypeIndex));

                VkMemoryAllocateFlagsInfo FlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
                FlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

                VkMemoryAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                AllocateInfo.allocationSize = BufferMemoryReqs.size;
                AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
                AllocateInfo.pNext = &FlagsInfo;

                VULKAN_VARIFY(vkAllocateMemory(DeviceHandle, &AllocateInfo, nullptr, &InOutState.MemoryHandle));
                VULKAN_VARIFY(vkBindBufferMemory(DeviceHandle, InOutState.BufferHandle, InOutState.MemoryHandle, 0));
                VULKAN_VARIFY(vkMapMemory(DeviceHandle, InOutState.MemoryHandle, 0, VK_WHOLE_SIZE, 0, (void**)&InOutState.MappedPointer));
                memset(InOutState.MappedPointer, 0, AllocateInfo.allocationSize);
            }

            // Setup the binding info
            {
                VkBufferDeviceAddressInfo AddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
                AddressInfo.buffer = InOutState.BufferHandle;

                OutBufferBindingInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT };
                OutBufferBindingInfo.address = VulkanExtension::vkGetBufferDeviceAddressKHR(DeviceHandle, &AddressInfo);
                OutBufferBindingInfo.usage = BufferUsageFlags;
            }
            return DescriptorBufferSize;
        };

        uint32 TotalResourceDescriptorBufferSize = 0;
        for (uint32 SetIndex = VulkanBindless::None + 1; SetIndex < VulkanBindless::NumBindLessSet; SetIndex++)
        {
            BindlessSetState& State = BindlessSetStates[SetIndex];
            InitBindlessState(GetDescriptorTypeFromSetIndex(SetIndex), State);
            static_assert(VulkanBindless::NumBindLessSet == 3);
            State.DescriptorSetLayout = CreateDescriptorSetlayout(State);
            TotalResourceDescriptorBufferSize += CreateDescriptorBuffer(State, BufferBindingInfo[SetIndex]);
        }
    }

    // Now create the single pipeline layout used by everything
    {
        VkDescriptorSetLayout DescriptorSetLayouts[VulkanBindless::NumBindLessSet];
        for (int32 LayoutIndex = VulkanBindless::None + 1; LayoutIndex < VulkanBindless::NumBindLessSet; ++LayoutIndex)
        {
            const BindlessSetState& State = BindlessSetStates[LayoutIndex];
            DescriptorSetLayouts[LayoutIndex] = State.DescriptorSetLayout;
        }

        VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        PipelineLayoutCreateInfo.setLayoutCount = VulkanBindless::NumBindLessSet;
        PipelineLayoutCreateInfo.pSetLayouts = DescriptorSetLayouts;
        VULKAN_VARIFY(vkCreatePipelineLayout(DeviceHandle, &PipelineLayoutCreateInfo, nullptr, &BindlessPipelineLayout));
    }

}

uint32 XVulkanBindlessDescriptorManager::GetFreeResourceStateIndex(BindlessSetState& State)
{
    if (State.FreeListHead != -1 && State.CurDescriptorCount >= State.MaxDescriptorCount)
    {
        const uint32 FreeIndex = State.FreeListHead;
        const uint32 ByteOffset = State.FreeListHead * State.DescriptorSize;
        uint32* NextSlot = (uint32*)(&State.DebugDescriptors[ByteOffset]);
        State.FreeListHead = *NextSlot;
        return FreeIndex;
    }

    const uint32 ResourceIndex = State.CurDescriptorCount++;
    return ResourceIndex;
}

XRHIDescriptorHandle XVulkanBindlessDescriptorManager::ReserveDescriptor(VkDescriptorType DescriptorType)
{
    const uint8 SetIndex = GetIndexForDescritorType(DescriptorType);
    BindlessSetState& State = BindlessSetStates[SetIndex];
    const uint32 ResourceIndex = GetFreeResourceStateIndex(State);
    return XRHIDescriptorHandle(SetIndex, ResourceIndex);
}
