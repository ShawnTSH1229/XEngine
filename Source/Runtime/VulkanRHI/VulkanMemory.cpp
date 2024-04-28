#include "VulkanMemory.h"
#include "VulkanResource.h"
#include "VulkanDevice.h"
#include "VulkanRHIPrivate.h"
#include <Runtime\Core\Template\AlignmentTemplate.h>

enum
{
	GPU_ONLY_HEAP_PAGE_SIZE = 128 * 1024 * 1024,
	STAGING_HEAP_PAGE_SIZE = 32 * 1024 * 1024,
	ANDROID_MAX_HEAP_PAGE_SIZE = 16 * 1024 * 1024,
	ANDROID_MAX_HEAP_IMAGE_PAGE_SIZE = 16 * 1024 * 1024,
	ANDROID_MAX_HEAP_BUFFER_PAGE_SIZE = 4 * 1024 * 1024,

};
constexpr static uint32 PoolSize = 8192;
constexpr static uint32 BufferSize = 1 * 1024 * 1024;


//XDeviceMemoryAllocation:[BEGIN]
void* XDeviceMemoryAllocation::Map(VkDeviceSize Size, VkDeviceSize Offset)
{
	if (!MappedPointer)
	{
		VULKAN_VARIFY(vkMapMemory(DeviceHandle, Handle, Offset, Size, 0, &MappedPointer));
	}
	return MappedPointer;
}

void XDeviceMemoryAllocation::Unmap()
{
	vkUnmapMemory(DeviceHandle, Handle);
	MappedPointer = nullptr;
}
//XDeviceMemoryAllocation:[END]






//XDeviceMemoryManager:[BEGIN]
XDeviceMemoryManager::XDeviceMemoryManager()
{
}

XDeviceMemoryManager::~XDeviceMemoryManager()
{
	for (auto iter : HeapInfos)
	{
		for (auto iterHeap : iter.Allocations)
		{
			delete iterHeap;
		}
	}
}

void XDeviceMemoryManager::Init(XVulkanDevice* InDevice)
{
	Device = InDevice;
	vkGetPhysicalDeviceMemoryProperties(*Device->GetVkPhysicalDevice(), &MemoryProperties);
	HeapInfos.resize(MemoryProperties.memoryHeapCount);
}

XDeviceMemoryAllocation* XDeviceMemoryManager::Alloc(VkDeviceSize AllocationSize, uint32 MemoryTypeIndex)
{
	VkDeviceMemory Handle;

	VkMemoryAllocateInfo Info = {};
	Info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	Info.allocationSize = AllocationSize;
	Info.memoryTypeIndex = MemoryTypeIndex;
	VULKAN_VARIFY(vkAllocateMemory(Device->GetVkDevice(), &Info, nullptr, &Handle));

	XDeviceMemoryAllocation* NewAllocation = new XDeviceMemoryAllocation();
	NewAllocation->Handle = Handle;
	NewAllocation->DeviceHandle = Device->GetVkDevice();

	uint32 HeapIndex = MemoryProperties.memoryTypes[MemoryTypeIndex].heapIndex;
	HeapInfos[HeapIndex].Allocations.push_back(NewAllocation);

	NewAllocation->CanMapped = VKHasAllFlags(MemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	return NewAllocation;
}

VkResult XDeviceMemoryManager::GetMemoryTypeFromProperties(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32* OutTypeIndex)
{
	for (uint32 i = 0; i < MemoryProperties.memoryTypeCount && TypeBits; i++)
	{
		if ((TypeBits & 1) == 1)
		{
			if ((MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
			{
				*OutTypeIndex = i;
				return VK_SUCCESS;
			}
		}
		TypeBits >>= 1;
	}
	return VK_ERROR_FEATURE_NOT_PRESENT;
}
//XDeviceMemoryManager:[END]




//XVulkanAllocation:[BEGIN]
void XVulkanAllocation::Init(XVulkanDevice* InDevice, EVulkanAllocationType InType, EVulkanAllocationMetaType InMetaType, VkBuffer InVkBuffer, uint32 AlignedOffset, uint32 InAllocatorIndex)
{
	Device = InDevice;

	VulkanHandle = InVkBuffer;
	Offset = AlignedOffset;

	MetaType = InMetaType;
	Type = InType;

	AllocatorIndex = InAllocatorIndex;
}

XVulkanSubresourceAllocator* XVulkanAllocation::GetSubresourceAllocator(XVulkanDevice* Device) const
{
	return Device->GetMemoryManager().GetSubresourceAllocator(AllocatorIndex);
}

void* XVulkanAllocation::GetMappedPointer(XVulkanDevice* Device)
{
	XVulkanSubresourceAllocator* Allocator = GetSubresourceAllocator(Device);
	uint8* pMappedPointer = (uint8*)Allocator->GetMappedPointer();
	XASSERT(pMappedPointer != nullptr);
	return Offset + pMappedPointer;
}

void XVulkanAllocation::BindBuffer(XVulkanDevice* Device, VkBuffer Buffer)
{
	VkDeviceMemory MemoryHandle = GetDeviceMemoryHandle(Device);
	VkResult Result = vkBindBufferMemory(Device->GetVkDevice(), Buffer, MemoryHandle, Offset);
}

void XVulkanAllocation::BindImage(XVulkanDevice* Device, VkImage Image)
{
	VkDeviceMemory MemoryHandle = GetDeviceMemoryHandle(Device);
	VULKAN_VARIFY(vkBindImageMemory(Device->GetVkDevice(), Image, MemoryHandle, Offset));
}

VkDeviceMemory XVulkanAllocation::GetDeviceMemoryHandle(XVulkanDevice* Device) const
{
	XVulkanSubresourceAllocator* Allocator = GetSubresourceAllocator(Device);
	return Allocator->GetMemoryAllocation()->GetHandle();
}

//void* XVulkanAllocation::Lock(EResourceLockMode LockMode, uint32 Size, uint32 Offset)
//{
//	void* Data = nullptr;
//	if (LockMode == EResourceLockMode::RLM_WriteOnly)
//	{
//		XStagingBuffer* StagingBuffer = Device->GetStagingManager().AcquireBuffer(Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
//		Data = StagingBuffer->GetMappedPointer();
//	}
//	return Data;
//}
//
//void XVulkanAllocation::Unlock()
//{
//}
//XVulkanAllocation:[END]


//XVulkanSubresourceAllocator:[BEGIN]

void XVulkanSubresourceAllocator::XRange::AllocateFromEntry(std::vector<XRange>& Ranges, int32 Index, uint32 SizeToAllocate)
{
	XRange& Entry = Ranges[Index];
	if (SizeToAllocate < Entry.Size)
	{
		Entry.Size -= SizeToAllocate;
		Entry.Offset += SizeToAllocate;
	}
}

XVulkanSubresourceAllocator::XVulkanSubresourceAllocator(XVulkanDevice* InDevice, EVulkanAllocationType InType, VkBuffer InBuffer, uint32 InBufferSize, XDeviceMemoryAllocation* InDeviceMemoryAllocation)
	: Device(InDevice)
	, Type(InType)
	, Buffer(InBuffer)
	, MaxSize(InBufferSize)
	, MemoryAllocation(InDeviceMemoryAllocation)
{
	AllocatorIndex = -1;

	XRange FullRange;
	FullRange.Offset = 0;
	FullRange.Size = MaxSize;
	FreeList.push_back(FullRange);
}

bool XVulkanSubresourceAllocator::TryAllocate(XVulkanAllocation& OutAllocation, XVulkanEvictable* Owner, uint32 InSize, uint32 Alignment, EVulkanAllocationMetaType MetaType)
{
	for (int32 Index = 0; Index < FreeList.size(); ++Index)
	{
		XRange& Entry = FreeList[Index];
		uint32 AllocatedOffset = Entry.Offset;
		uint32 AlignedOffset = Align(Entry.Offset, Alignment);
		uint32 AlignmentAdjustment = AlignedOffset - Entry.Offset;
		uint32 AllocatedSize = AlignmentAdjustment + InSize;
		if (AllocatedSize <= Entry.Size)
		{
			XRange::AllocateFromEntry(FreeList, Index, AllocatedSize);
			OutAllocation.Init(Device, Type, MetaType, Buffer, AlignedOffset, AllocatorIndex);
			return true;
		}
	}
	return false;
}
//XVulkanSubresourceAllocator:[BEGIN]


//XVulkanResourceHeap:[BEGIN]
XVulkanResourceHeap::XVulkanResourceHeap(XMemoryManager* InOwner, uint32 InMemoryTypeIndex, uint32 InOverridePageSize)
	: Owner(InOwner)
	, MemoryTypeIndex(InMemoryTypeIndex)
{

}

bool XVulkanResourceHeap::AllocateResource(XVulkanAllocation& OutAllocation, XVulkanEvictable* AllocationOwner, EType Type, uint32 Size, uint32 Alignment, EVulkanAllocationMetaType MetaType, bool bMapAllocation)
{
	XVulkanPageSizeBucket MemoryBucket;
	uint32 BucketId = GetPageSizeBucket(MemoryBucket, Type, Size);

	std::vector<XVulkanSubresourceAllocator*>& UsedPages = ActivePages[BucketId];

	uint32 AllocationSize;
	if (Size < MemoryBucket.PageSize)
	{
		for (int32 Index = 0; Index < UsedPages.size(); ++Index)
		{
			XVulkanSubresourceAllocator* Page = UsedPages[Index];
			if (Page->TryAllocate(OutAllocation, AllocationOwner, Size, Alignment, MetaType))
			{
				return true;
			}
		}
		AllocationSize = (std::max)(Size, MemoryBucket.PageSize);
	}

	XDeviceMemoryManager& DeviceMemoryManager = Owner->GetDevice()->GetDeviceMemoryManager();
	XDeviceMemoryAllocation* DeviceMemoryAllocation = DeviceMemoryManager.Alloc(AllocationSize, MemoryTypeIndex);

	if (bMapAllocation)
	{
		DeviceMemoryAllocation->Map(AllocationSize, 0);
	}

	EVulkanAllocationType AllocationType = (Type == EType::Image) ? EVulkanAllocationType::EVulkanAllocationImage : EVulkanAllocationType::EVulkanAllocationBuffer;
	XVulkanSubresourceAllocator* Page = new XVulkanSubresourceAllocator(Owner->GetDevice(), AllocationType, VkBuffer(), AllocationSize, DeviceMemoryAllocation);
	Owner->RegisterSubresourceAllocator(Page);
	Page->BucketId = BucketId;
	ActivePages[BucketId].push_back(Page);
	return Page->TryAllocate(OutAllocation, AllocationOwner, Size, Alignment, MetaType);

}

uint32 XVulkanResourceHeap::GetPageSizeBucket(XVulkanPageSizeBucket& BucketOut, EType Type, uint32 AllocationSize)
{
	uint32 Mask = 0;
	Mask |= (Type == EType::Image) ? XVulkanPageSizeBucket::BUCKET_MASK_IMAGE : 0;
	Mask |= (Type == EType::Buffer) ? XVulkanPageSizeBucket::BUCKET_MASK_BUFFER : 0;
	for (XVulkanPageSizeBucket& B : PageSizeBuckets)
	{
		if (Mask == (B.BucketMask & Mask) && AllocationSize <= B.AllocationMax)
		{
			BucketOut = B;
			return &B - &PageSizeBuckets[0];
		}
	}
	return 0xffffffff;
}
//XVulkanResourceHeap:[END]


XMemoryManager::XMemoryManager(XVulkanDevice* InDevice, XDeviceMemoryManager* InDeviceMemoryManager)
	: Device(InDevice)
	, DeviceMemoryManager(InDeviceMemoryManager)
{
}

XMemoryManager::~XMemoryManager()
{
	for (auto iter : UsedBufferAllocations)
	{
		delete iter;
	}

	for (auto iter : AllBufferAllocations)
	{
		delete iter;
	}

	for (auto iter : ResourceTypeHeaps)
	{
		delete iter;
	}
}

void XMemoryManager::RegisterSubresourceAllocator(XVulkanSubresourceAllocator* SubresourceAllocator)
{
	SubresourceAllocator->AllocatorIndex = AllBufferAllocations.size();
	AllBufferAllocations.push_back(SubresourceAllocator);
}

void XMemoryManager::Init()
{
	const VkPhysicalDeviceMemoryProperties& MemoryProperties = DeviceMemoryManager->GetMemoryProperties();
	const uint32 TypeBits = (1 << MemoryProperties.memoryTypeCount) - 1;
	ResourceTypeHeaps.resize(MemoryProperties.memoryTypeCount);

	// Upload heap
	{
		uint32 TypeIndex;
		VULKAN_VARIFY(DeviceMemoryManager->GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &TypeIndex));
		ResourceTypeHeaps[TypeIndex] = new XVulkanResourceHeap(this, TypeIndex, STAGING_HEAP_PAGE_SIZE);

		auto& Buckets = ResourceTypeHeaps[TypeIndex]->PageSizeBuckets;
		XVulkanPageSizeBucket Bucket0 = { STAGING_HEAP_PAGE_SIZE, STAGING_HEAP_PAGE_SIZE, XVulkanPageSizeBucket::BUCKET_MASK_IMAGE | XVulkanPageSizeBucket::BUCKET_MASK_BUFFER };
		XVulkanPageSizeBucket Bucket1 = { UINT64_MAX, 0, XVulkanPageSizeBucket::BUCKET_MASK_IMAGE | XVulkanPageSizeBucket::BUCKET_MASK_BUFFER };
		Buckets.push_back(Bucket0);
		Buckets.push_back(Bucket1);
	}

	//Main Heap
	{
		uint32 Index;
		VULKAN_VARIFY(DeviceMemoryManager->GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &Index));

		for (Index = 0; Index < MemoryProperties.memoryTypeCount; ++Index)
		{
			if (!ResourceTypeHeaps[Index])
			{
				ResourceTypeHeaps[Index] = new XVulkanResourceHeap(this, Index);
				auto& PageSizeBuckets = ResourceTypeHeaps[Index]->PageSizeBuckets;

				XVulkanPageSizeBucket BucketImage = { UINT64_MAX, (uint32)ANDROID_MAX_HEAP_IMAGE_PAGE_SIZE, XVulkanPageSizeBucket::BUCKET_MASK_IMAGE };
				XVulkanPageSizeBucket BucketBuffer = { UINT64_MAX, (uint32)ANDROID_MAX_HEAP_BUFFER_PAGE_SIZE, XVulkanPageSizeBucket::BUCKET_MASK_BUFFER };
				PageSizeBuckets.push_back(BucketImage);
				PageSizeBuckets.push_back(BucketBuffer);
			}
		}
	}
}

bool XMemoryManager::AllocateBufferMemory(XVulkanAllocation& OutAllocation, VkBuffer InBuffer, EVulkanAllocationFlags Flags, uint32 InForceMinAlignment)
{
	VkBufferMemoryRequirementsInfo2 BufferMemoryRequirementsInfo = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };
	VkMemoryDedicatedRequirements DedicatedRequirements = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS };
	VkMemoryRequirements2 MemoryRequirements = { VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 };
	
	BufferMemoryRequirementsInfo.buffer = InBuffer;
	MemoryRequirements.pNext = &DedicatedRequirements;

	vkGetBufferMemoryRequirements2(Device->GetVkDevice(), &BufferMemoryRequirementsInfo, &MemoryRequirements);
	MemoryRequirements.memoryRequirements.alignment = std::max<VkDeviceSize>(MemoryRequirements.memoryRequirements.alignment, (VkDeviceSize)InForceMinAlignment);

	AllocateBufferMemory(OutAllocation, nullptr, MemoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, EVulkanAllocationMetaType::EVulkanAllocationMetaImageOther);

	if (EnumHasAllFlags(Flags, EVulkanAllocationFlags::AutoBind))
	{
		VkBindBufferMemoryInfo BindBufferMemoryInfo = { VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO };
		BindBufferMemoryInfo.buffer = InBuffer;
		BindBufferMemoryInfo.memory = OutAllocation.GetDeviceMemoryHandle(Device);
		BindBufferMemoryInfo.memoryOffset = OutAllocation.Offset;

		VULKAN_VARIFY(vkBindBufferMemory2(Device->GetVkDevice(), 1, &BindBufferMemoryInfo));
	}
	return true;
}

void XMemoryManager::AllocateBufferPooled(XVulkanAllocation& OutAllocation, XVulkanEvictable* AllocationOwner, uint32 Size, VkBufferUsageFlags BufferUsage, VkMemoryPropertyFlags MemoryPropertyFlags, EVulkanAllocationMetaType MetaType)
{
	for (int32 Index = 0; Index < UsedBufferAllocations.size(); ++Index)
	{
		XVulkanSubresourceAllocator* SubresourceAllocator = UsedBufferAllocations[Index];
		if ((SubresourceAllocator->BufferUsageFlags & BufferUsage) == BufferUsage &&
			(SubresourceAllocator->MemoryPropertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags)
		{
			uint32 Alignment = 0;
			XASSERT_TEMP(false);//Align
			if (SubresourceAllocator->TryAllocate(OutAllocation, AllocationOwner, Size, Alignment, MetaType))
			{
				return;
			}
		}
	}

	uint32 BufferSize = (std::max)(Size, uint32(1024 * 1024));
	VkBuffer Buffer;
	VkBufferCreateInfo BufferCreateInfo = {};
	BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	BufferCreateInfo.size = BufferSize;
	BufferCreateInfo.usage = BufferUsage;
	VULKAN_VARIFY(vkCreateBuffer(Device->GetVkDevice(), &BufferCreateInfo, nullptr, &Buffer));

	VkMemoryRequirements MemReqs;
	vkGetBufferMemoryRequirements(Device->GetVkDevice(), Buffer, &MemReqs);

	uint32 Alignment = (uint32)MemReqs.alignment;
	uint32 MemoryTypeIndex;
	VULKAN_VARIFY(Device->GetDeviceMemoryManager().GetMemoryTypeFromProperties(MemReqs.memoryTypeBits, MemoryPropertyFlags, &MemoryTypeIndex));

	XDeviceMemoryAllocation* DeviceMemoryAllocation = DeviceMemoryManager->Alloc(BufferSize, MemoryTypeIndex);
	vkBindBufferMemory(Device->GetVkDevice(), Buffer, DeviceMemoryAllocation->GetHandle(), 0);

	if (DeviceMemoryAllocation->CanMapped)
	{
		DeviceMemoryAllocation->Map(BufferSize, 0);
	}

	XVulkanSubresourceAllocator* SubresourceAllocator = new XVulkanSubresourceAllocator(Device, EVulkanAllocationPooledBuffer,  Buffer, BufferSize, DeviceMemoryAllocation);
	UsedBufferAllocations.push_back(SubresourceAllocator);
	RegisterSubresourceAllocator(SubresourceAllocator);
	bool Result = SubresourceAllocator->TryAllocate(OutAllocation, AllocationOwner, Size, Alignment, MetaType);
	XASSERT(Result = true);
}

bool XMemoryManager::AllocateImageMemory(XVulkanAllocation& Allocation, XVulkanEvictable* AllocationOwner, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, EVulkanAllocationMetaType MetaType)
{
	uint32 TypeIndex = 0;
	VkResult Result = DeviceMemoryManager->GetMemoryTypeFromProperties(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, &TypeIndex);
	bool bMapped = VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	return ResourceTypeHeaps[TypeIndex]->AllocateResource(Allocation, AllocationOwner, EType::Image, MemoryReqs.size, MemoryReqs.alignment, MetaType, bMapped);
}

bool XMemoryManager::AllocateBufferMemory(XVulkanAllocation& OutAllocation, XVulkanEvictable* AllocationOwner, const VkMemoryRequirements2& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, EVulkanAllocationMetaType MetaType)
{
	uint32 TypeIndex = 0;
	VkResult Result = DeviceMemoryManager->GetMemoryTypeFromProperties(MemoryReqs.memoryRequirements.memoryTypeBits, MemoryPropertyFlags, &TypeIndex);
	bool bMapped = VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	return ResourceTypeHeaps[TypeIndex]->AllocateResource(OutAllocation, AllocationOwner, EType::Buffer, MemoryReqs.memoryRequirements.size, MemoryReqs.memoryRequirements.alignment, MetaType, bMapped);
}

XStagingBuffer::XStagingBuffer(XVulkanDevice* InDevice)
	:Device(InDevice)
{
}

void* XStagingBuffer::GetMappedPointer()
{
	return Allocation.GetMappedPointer(Device);
}


XStagingManager::~XStagingManager()
{
	for (auto iter : UsedStagingBuffers)
	{
		delete iter;
	}
}

void XStagingManager::Init(XVulkanDevice* InDevice)
{
	Device = InDevice;
}

XStagingBuffer* XStagingManager::AcquireBuffer(uint32 Size, VkBufferUsageFlags InUsageFlags, VkMemoryPropertyFlagBits InMemoryReadFlags)
{
	if (InMemoryReadFlags == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
	{
		XASSERT(false);
	}
	if ((InUsageFlags & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) != 0)
	{
		InUsageFlags |= (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	XStagingBuffer* StagingBuffer = new XStagingBuffer(Device);
	VkBufferCreateInfo StagingBufferCreateInfo = {};
	StagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	StagingBufferCreateInfo.size = Size;
	StagingBufferCreateInfo.usage = InUsageFlags;
	VULKAN_VARIFY(vkCreateBuffer(Device->GetVkDevice(), &StagingBufferCreateInfo, nullptr, &StagingBuffer->Buffer));

	VkBufferMemoryRequirementsInfo2 BufferMemoryRequirementsInfo = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2 };
	BufferMemoryRequirementsInfo.buffer = StagingBuffer->Buffer;

	VkMemoryRequirements2 MemReqs;
	vkGetBufferMemoryRequirements2(Device->GetVkDevice(), &BufferMemoryRequirementsInfo, &MemReqs);

	VkMemoryPropertyFlags readTypeFlags = InMemoryReadFlags;
	Device->GetMemoryManager().AllocateBufferMemory(StagingBuffer->Allocation, StagingBuffer, MemReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | readTypeFlags, EVulkanAllocationMetaBufferStaging);
	StagingBuffer->Allocation.BindBuffer(Device, StagingBuffer->Buffer);
	UsedStagingBuffers.push_back(StagingBuffer);
	return StagingBuffer;
}

XSemaphore::XSemaphore(XVulkanDevice* InDevice)
	:Device(InDevice),
	SemaphoreHandle(VK_NULL_HANDLE),
	bExternallyOwned(false)
{
	VkSemaphoreCreateInfo CreateInfo = {};
	CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VULKAN_VARIFY(vkCreateSemaphore(Device->GetVkDevice(), &CreateInfo, nullptr, &SemaphoreHandle));
}

XSemaphore::XSemaphore(XVulkanDevice* InDevice, const VkSemaphore& InExternalSemaphore)
	:Device(InDevice), 
	SemaphoreHandle(InExternalSemaphore),
	bExternallyOwned(true)
{

}

XFenceManager::~XFenceManager()
{
	for (auto iter = FreeFences.begin(); iter != FreeFences.end(); iter++)
	{
		if (*iter)
			delete* iter;
	}
	for (auto iter = UsedFences.begin(); iter != UsedFences.end(); iter++)
	{
		if (*iter)
			delete* iter;
	}
}

XFence* XFenceManager::AllocateFence(bool bCreateSignaled)
{
	if (FreeFences.size() != 0)
	{
		XFence* Fence = FreeFences[0];
		FreeFences.erase(FreeFences.begin());
		UsedFences.push_back(Fence);

		if (bCreateSignaled)
		{
			Fence->State = XFence::EState::Signaled;
		}
		return Fence;
	}

	XFence* NewFence = new XFence(Device, this, bCreateSignaled);
	UsedFences.push_back(NewFence);
	return NewFence;
}

bool XFenceManager::WaitForFence(XFence* Fence, uint64 TimeInNanoseconds)
{
	XASSERT(Fence->State == XFence::EState::NotReady);
	VkResult Result = vkWaitForFences(Device->GetVkDevice(), 1, &Fence->Handle, true, TimeInNanoseconds);
	switch (Result)
	{
	case VK_SUCCESS:
		Fence->State = XFence::EState::Signaled;
		return true;
	case VK_TIMEOUT:
		break;
	default:
		XASSERT(false);
		break;
	}

	return false;
}
void XFenceManager::ResetFence(XFence* Fence)
{
	if (Fence->State != XFence::EState::NotReady)
	{
		vkResetFences(Device->GetVkDevice(), 1, &Fence->Handle);
		Fence->State = XFence::EState::NotReady;
	}
}

bool XFenceManager::CheckFenceState(XFence* Fence)
{
	VkResult Result = vkGetFenceStatus(Device->GetVkDevice(), Fence->Handle);
	switch (Result)
	{
	case VK_SUCCESS:
		Fence->State = XFence::EState::Signaled;
		return true;

	case VK_NOT_READY:
		break;

	default:
		XASSERT(false);
		break;
	}

	return false;
}

XFence::XFence(XVulkanDevice* InDevice, XFenceManager* InOwner, bool bCreateSignaled)
	: State(bCreateSignaled ? XFence::EState::Signaled : XFence::EState::NotReady)
	, Owner(InOwner)
{
	VkFenceCreateInfo Info = {};
	Info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	Info.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
	VULKAN_VARIFY(vkCreateFence(InDevice->GetVkDevice(), &Info, nullptr, &Handle));
}
