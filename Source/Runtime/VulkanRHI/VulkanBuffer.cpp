#include "VulkanPlatformRHI.h"
#include "VulkanResource.h"
#include "VulkanDevice.h"
#include "VulkanRHIPrivate.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include <map>

static std::map<XVulkanResourceMultiBuffer*, XPendingBufferLock> GPendingLockIBs;

VkBufferUsageFlags XVulkanResourceMultiBuffer::UEToVKBufferUsageFlags(EBufferUsage InUEUsage)
{
	VkBufferUsageFlags OutVkUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	if (((uint32)InUEUsage & (uint32)EBufferUsage::BUF_Vertex) != 0)
	{
		OutVkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if (((uint32)InUEUsage & (uint32)EBufferUsage::BUF_Index) != 0)
	{
		OutVkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}

	if (((uint32)InUEUsage & (uint32)EBufferUsage::BUF_StructuredBuffer) != 0)
	{
		OutVkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}

	if (((uint32)InUEUsage & (uint32)EBufferUsage::BUF_UnorderedAccess) != 0)
	{
		OutVkUsage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}

	if (((uint32)InUEUsage & (uint32)EBufferUsage::BUF_DrawIndirect) != 0)
	{
		OutVkUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}

	if (((uint32)InUEUsage & (uint32)EBufferUsage::BUF_ShaderResource) != 0)
	{
		OutVkUsage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
	}

	if (EnumHasAnyFlags(InUEUsage, EBufferUsage::BUF_AccelerationStructure))
	{
		OutVkUsage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
		OutVkUsage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	}

	OutVkUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	return OutVkUsage;
}


XVulkanResourceMultiBuffer::XVulkanResourceMultiBuffer(XVulkanDevice* InDevice,uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
	:XRHIBuffer(Stride, Size, Usage)
	, Device(InDevice)
{
	VkMemoryPropertyFlags BufferMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VkBufferUsageFlags BufferUsageFlags = UEToVKBufferUsageFlags(Usage);
	Device->GetMemoryManager().AllocateBufferPooled(Buffer, this, Size, BufferUsageFlags, BufferMemFlags, EVulkanAllocationMetaMultiBuffer);

	const void* DataSource = ResourceData.ResourceArray->GetResourceData();
	if (DataSource)
	{
		uint32 DataSize = ResourceData.ResourceArray->GetResourceDataSize();
		void* Data = Lock(EResourceLockMode::RLM_WriteOnly, DataSize, 0);
		memcpy(Data, DataSource, DataSize);
		InternalUnlock(Device->GetGfxContex(), &GPendingLockIBs[this], this);
		ResourceData.ResourceArray->ReleaseData();
	}
}

void XVulkanResourceMultiBuffer::InternalUnlock(XVulkanCommandListContext* Context, XPendingBufferLock* PendingLock, XVulkanResourceMultiBuffer* MultiBuffer)
{
	XVulkanCmdBuffer* Cmd = Context->GetCommandBufferManager()->GetActiveCmdBuffer();
	VkCommandBuffer CmdBuffer = Cmd->GetHandle();

	VkBufferCopy Region{};
	Region.size = PendingLock->Size;
	Region.dstOffset = PendingLock->Offset + MultiBuffer->Buffer.Offset;
	vkCmdCopyBuffer(CmdBuffer, PendingLock->StagingBuffer->Buffer, MultiBuffer->Buffer.VulkanHandle, 1, &Region);

	//MultiBuffer->GetParent()->GetStagingManager().ReleaseBuffer(Cmd, StagingBuffer);
	//vkDestroyBuffer(mVkHack.GetVkDevice(), stagingBuffer, nullptr);
	//vkFreeMemory(mVkHack.GetVkDevice(), stagingBufferMemory, nullptr);
}

void* XVulkanResourceMultiBuffer::Lock(EResourceLockMode LockMode, uint32 LockSize, uint32 Offset)
{
	XStagingBuffer* StagingBuffer = Device->GetStagingManager().AcquireBuffer(LockSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	void* Data = StagingBuffer->GetMappedPointer();

	XPendingBufferLock PendingBufferLock;
	PendingBufferLock.Size = LockSize;
	PendingBufferLock.Offset = Offset;
	PendingBufferLock.LockMode = LockMode;
	PendingBufferLock.StagingBuffer = StagingBuffer;
	GPendingLockIBs[this] = PendingBufferLock;
	return Data;
}

void XVulkanResourceMultiBuffer::UnLock(XVulkanCommandListContext* Context)
{
	XPendingBufferLock PendingBufferLock = GPendingLockIBs[this];
	XASSERT(PendingBufferLock.LockMode == EResourceLockMode::RLM_WriteOnly);
	InternalUnlock(Context, &PendingBufferLock, this);
}



std::shared_ptr<XRHIBuffer> XVulkanPlatformRHI::RHICreateBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return std::make_shared<XVulkanResourceMultiBuffer>(Device, Stride, Size, Usage, ResourceData);
}

std::shared_ptr<XRHIBuffer> XVulkanPlatformRHI::RHICreateVertexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return std::make_shared<XVulkanResourceMultiBuffer>(Device, Stride, Size, EBufferUsage((uint32)Usage | (uint32)EBufferUsage::BUF_Vertex), ResourceData);
}