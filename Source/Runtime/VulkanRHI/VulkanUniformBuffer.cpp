#include "VulkanPlatformRHI.h"
#include "VulkanResource.h"
#include "VulkanDevice.h"

XVulkanConstantBuffer::XVulkanConstantBuffer(XVulkanDevice* InDevice, uint32 InSize)
	:Device(InDevice)
	, Size(InSize)
{
	XMemoryManager& ResourceMgr = Device->GetMemoryManager();
	ResourceMgr.AllocateBufferPooled(Allocation, nullptr, Size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, EVulkanAllocationMetaUniformBuffer);
}

void XVulkanConstantBuffer::UpdateData(const void* data, uint32 size, uint32 offset_byte)
{
	XASSERT(offset_byte == 0);
	memcpy(Allocation.GetMappedPointer(Device), data, size);
}

std::shared_ptr<XRHIConstantBuffer> XVulkanPlatformRHI::RHICreateConstantBuffer(uint32 size)
{
	return std::make_shared<XVulkanConstantBuffer>(Device, size);
}