#pragma once
#include <vector>
#include "Runtime\HAL\PlatformTypes.h"
#include "vulkan\vulkan_core.h"
#include <Runtime\HAL\Mch.h>
#include "Runtime\Core\Template\XEngineTemplate.h"

class XMemoryManager;
class XVulkanDevice;
class XVulkanSubresourceAllocator;
class XFenceManager;

enum EVulkanAllocationType : uint8 {
	EVulkanAllocationEmpty,
	EVulkanAllocationPooledBuffer,
	EVulkanAllocationBuffer,
	EVulkanAllocationImage,
	EVulkanAllocationSize,
};

enum EVulkanAllocationMetaType : uint8
{
	EVulkanAllocationMetaUnknown,
	EVulkanAllocationMetaMultiBuffer,
	EVulkanAllocationMetaBufferStaging,
	EVulkanAllocationMetaImageRenderTarget,
	EVulkanAllocationMetaImageOther,
	EVulkanAllocationMetaUniformBuffer,
	EVulkanAllocationMetaBufferOther,
	EVulkanAllocationMetaSize,
};


enum class EType
{
	Image,
	Buffer,
};

class XVulkanEvictable
{

};

class XFence
{
public:
	XFence(XVulkanDevice* InDevice, XFenceManager* InOwner, bool bCreateSignaled);

	inline VkFence GetHandle() const
	{
		return Handle;
	}

	inline bool IsSignaled() const
	{
		return State == EState::Signaled;
	}

	XFenceManager* GetOwner()
	{
		return Owner;
	}

protected:
	VkFence Handle;

	enum class EState
	{
		// Initial state
		NotReady,

		// After GPU processed it
		Signaled,
	};

	EState State;

	XFenceManager* Owner;

	// Only owner can delete!
	~XFence()
	{
	}
	friend class XFenceManager;
};

class XFenceManager
{
public:
	XFenceManager()
		: Device(nullptr)
	{
	}

	XFenceManager(XVulkanDevice* InDevice)
		: Device(InDevice)
	{
	}

	~XFenceManager();

	XFence* AllocateFence(bool bCreateSignaled = false);

	inline bool IsFenceSignaled(XFence* Fence)
	{
		if (Fence->IsSignaled())
		{
			return true;
		}

		return CheckFenceState(Fence);
	}
	bool WaitForFence(XFence* Fence, uint64 TimeInNanoseconds);

	void ResetFence(XFence* Fence);
protected:
	bool CheckFenceState(XFence* Fence);
	XVulkanDevice* Device;

	std::vector<XFence*> FreeFences;
	std::vector<XFence*> UsedFences;
};
class XSemaphore
{
public:
	XSemaphore(XVulkanDevice* InDevice);
	XSemaphore(XVulkanDevice* InDevice, const VkSemaphore& InExternalSemaphore);
	virtual ~XSemaphore()
	{

	}

	inline VkSemaphore GetHandle() const
	{
		return SemaphoreHandle;
	}

	inline bool IsExternallyOwned() const
	{
		return bExternallyOwned;
	}

private:
	XVulkanDevice* Device;
	VkSemaphore SemaphoreHandle;
	bool bExternallyOwned;
};

class XDeviceMemoryAllocation
{
public:
	XDeviceMemoryAllocation()
		: MappedPointer(nullptr)
		, Handle(VK_NULL_HANDLE)
		, DeviceHandle(VK_NULL_HANDLE)
		, CanMapped(false)
	{
	}

	inline void* GetMappedPointer()
	{
		return MappedPointer;
	}

	inline VkDeviceMemory GetHandle() const
	{
		return Handle;
	}

	void* Map(VkDeviceSize Size, VkDeviceSize Offset);
	void Unmap();

	bool CanMapped;

private:
	void* MappedPointer;
	VkDeviceMemory Handle;
	VkDevice DeviceHandle;

	friend class XDeviceMemoryManager;
};

class XDeviceMemoryManager
{
public:
	XDeviceMemoryManager();
	~XDeviceMemoryManager();

	void Init(XVulkanDevice* Device);
	XDeviceMemoryAllocation* Alloc(VkDeviceSize AllocationSize, uint32 MemoryTypeIndex);

	VkPhysicalDeviceMemoryProperties GetMemoryProperties() { return MemoryProperties; }
	VkResult GetMemoryTypeFromProperties(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32* OutTypeIndex);
private:

	struct XHeapInfo
	{
		VkDeviceSize UsedSize;
		VkDeviceSize PeakSize;
		std::vector<XDeviceMemoryAllocation*> Allocations;

		XHeapInfo() :
			UsedSize(0),
			PeakSize(0)
		{
		}
	};

	//MemoryProperties.memoryHeapCount
	std::vector<XHeapInfo> HeapInfos;

	//VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001,
	//VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002,
	VkPhysicalDeviceMemoryProperties MemoryProperties;

	XVulkanDevice* Device;
};


// Holds a reference to -any- vulkan gpu allocation
class XVulkanAllocation
{
public:
	void Init(XVulkanDevice* InDevice, EVulkanAllocationType Type, EVulkanAllocationMetaType MetaType, VkBuffer InVkBuffer, uint32 AlignedOffset, uint32 InAllocatorIndex);

	XVulkanSubresourceAllocator* GetSubresourceAllocator(XVulkanDevice* Device) const;
	void* GetMappedPointer(XVulkanDevice* Device);

	void BindBuffer(XVulkanDevice* Device, VkBuffer Buffer);
	void BindImage(XVulkanDevice* Device, VkImage Image);
	VkDeviceMemory GetDeviceMemoryHandle(XVulkanDevice* Device) const;
	//void* Lock(EResourceLockMode LockMode, uint32 Size, uint32 Offset);
	//void Unlock();


	//Buffer Infomation
	VkBuffer VulkanHandle;
	uint32 Offset;
	uint16 AllocatorIndex = 0;

	//Device and allocation
	XVulkanDevice* Device;
	XDeviceMemoryAllocation* MemoryAllocation;

	//Type
	EVulkanAllocationMetaType MetaType = EVulkanAllocationMetaUnknown;
	uint8 Type : 7;
};

class XVulkanSubresourceAllocator
{
public:
	struct XRange
	{
		uint32 Offset;
		uint32 Size;

		static void AllocateFromEntry(std::vector<XRange>& Ranges, int32 Index, uint32 SizeToAllocate);
	};

	XVulkanSubresourceAllocator(XVulkanDevice* InDevice, EVulkanAllocationType InType,  VkBuffer InBuffer, uint32 InBufferSize, XDeviceMemoryAllocation* InDeviceMemoryAllocation);
	
	bool TryAllocate(XVulkanAllocation& OutAllocation, XVulkanEvictable* Owner, uint32 Size, uint32 Alignment, EVulkanAllocationMetaType MetaType);
	
	inline void* GetMappedPointer()
	{
		return MemoryAllocation->GetMappedPointer();
	}

	XDeviceMemoryAllocation* GetMemoryAllocation()
	{
		return MemoryAllocation;
	}

	uint32 GetAllocatorIndex()
	{
		return AllocatorIndex;
	}

	VkMemoryPropertyFlags MemoryPropertyFlags;//
	VkBufferUsageFlags BufferUsageFlags;//

	uint32 MaxSize;
	uint8 BucketId;//

	uint32 AllocatorIndex;
private:
	XDeviceMemoryAllocation* MemoryAllocation;

	XVulkanDevice* Device;
	EVulkanAllocationType Type;
	VkBuffer Buffer;

	std::vector<XRange> FreeList;
};

struct XVulkanPageSizeBucket
{
	uint64 AllocationMax;
	uint32 PageSize;
	enum
	{
		BUCKET_MASK_IMAGE = 0x1,
		BUCKET_MASK_BUFFER = 0x2,
	};
	uint32 BucketMask;
};

// A set of Device Allocations (Heap Pages) for a specific memory type.
class XVulkanResourceHeap
{
public:
	XVulkanResourceHeap(XMemoryManager* InOwner, uint32 InMemoryTypeIndex, uint32 InOverridePageSize = 0);
	bool AllocateResource(XVulkanAllocation& OutAllocation, XVulkanEvictable* AllocationOwner, EType Type, uint32 Size, uint32 Alignment, EVulkanAllocationMetaType MetaType, bool bMapAllocation);
	uint32 GetPageSizeBucket(XVulkanPageSizeBucket& BucketOut, EType Type, uint32 AllocationSize);

	static constexpr int32 MAX_BUCKETS = 5;
	std::vector<XVulkanPageSizeBucket>PageSizeBuckets;
private:
	XMemoryManager* Owner;
	uint16 MemoryTypeIndex;
	std::vector<XVulkanSubresourceAllocator*> ActivePages[MAX_BUCKETS];
};

enum class EVulkanAllocationFlags : uint16
{
	None = 0x0000,

	HostVisible = 0x0001,    // Will be written from CPU, will likely contain the HOST_VISIBLE + HOST_COHERENT flags

	Dedicated = 0x0008,	  // Will not share a memory block with other resources

	AutoBind = 0x0080,	  // Will automatically bind the allocation to the supplied VkBuffer/VkImage, avoids an extra lock to bind separately
};

ENUM_CLASS_FLAGS(EVulkanAllocationFlags);

//inline           EVulkanAllocationFlags& operator|=(EVulkanAllocationFlags& Lhs, EVulkanAllocationFlags Rhs) { return Lhs = (EVulkanAllocationFlags)((__underlying_type(EVulkanAllocationFlags))Lhs | (__underlying_type(EVulkanAllocationFlags))Rhs); } 
//inline constexpr EVulkanAllocationFlags  operator| (EVulkanAllocationFlags  Lhs, EVulkanAllocationFlags Rhs) { return (EVulkanAllocationFlags)((__underlying_type(EVulkanAllocationFlags))Lhs | (__underlying_type(EVulkanAllocationFlags))Rhs); }

class XMemoryManager
{
public:
	XMemoryManager(XVulkanDevice* InDevice, XDeviceMemoryManager* InDeviceMemoryManager);
	~XMemoryManager();
	void RegisterSubresourceAllocator(XVulkanSubresourceAllocator* SubresourceAllocator);

	void Init();

	bool AllocateBufferMemory(XVulkanAllocation& OutAllocation, VkBuffer InBuffer, EVulkanAllocationFlags Flags, uint32 InForceMinAlignment = -1);

	bool AllocateBufferMemory(
		XVulkanAllocation& OutAllocation, 
		XVulkanEvictable* AllocationOwner, 
		const VkMemoryRequirements2& MemoryReqs, 
		VkMemoryPropertyFlags MemoryPropertyFlags, 
		EVulkanAllocationMetaType MetaType);

	void AllocateBufferPooled(
		XVulkanAllocation& OutAllocation,
		XVulkanEvictable* Owner, uint32 Size,
		VkBufferUsageFlags BufferUsage,
		VkMemoryPropertyFlags MemoryPropertyFlags,
		EVulkanAllocationMetaType MetaType);

	bool AllocateImageMemory(
		XVulkanAllocation& Allocation, 
		XVulkanEvictable* AllocationOwner, 
		const VkMemoryRequirements& MemoryReqs,
		VkMemoryPropertyFlags MemoryPropertyFlags, 
		EVulkanAllocationMetaType MetaType);

	inline XVulkanSubresourceAllocator* GetSubresourceAllocator(const uint32 AllocatorIndex)
	{
		return AllBufferAllocations[AllocatorIndex];
	}

	XVulkanDevice* GetDevice()
	{
		return Device;
	}

	std::vector<XVulkanSubresourceAllocator*>UsedBufferAllocations;
	std::vector<XVulkanSubresourceAllocator*> AllBufferAllocations;
private:
	XDeviceMemoryManager* DeviceMemoryManager;
	XVulkanDevice* Device;
	std::vector<XVulkanResourceHeap*>ResourceTypeHeaps;
};

class XStagingBuffer : public XVulkanEvictable
{
public:
	XStagingBuffer(XVulkanDevice* InDevice);
	void* GetMappedPointer();
	VkBuffer Buffer;
	XVulkanAllocation Allocation;
private:
	XVulkanDevice* Device;
};

class XStagingManager
{
public:
	XStagingManager() :Device(nullptr) {}
	~XStagingManager();
	void Init(XVulkanDevice* InDevice);
	XStagingBuffer* AcquireBuffer(uint32 Size, VkBufferUsageFlags InUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VkMemoryPropertyFlagBits InMemoryReadFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
private:
	std::vector<XStagingBuffer*>UsedStagingBuffers;
	XVulkanDevice* Device;
};













