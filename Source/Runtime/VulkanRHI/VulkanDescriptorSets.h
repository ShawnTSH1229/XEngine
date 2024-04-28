#pragma once
#include <vector>
#include <vulkan\vulkan_core.h>
#include "Runtime\HAL\PlatformTypes.h"
#include "VulkanUtil.h"
#include "VulkanState.h"
#include "VulkanMemory.h"
#include "VulkanCommon.h"

#include <map>
#include <string>

class XVulkanDevice;
class XVulkanShader;

//TODO:
struct XVulkanTextureView
{
	XVulkanTextureView()
		: View(VK_NULL_HANDLE)
		, Image(VK_NULL_HANDLE)
		, ViewId(0)
	{
	}

	void Create(XVulkanDevice* Device, VkImage InImage, VkImageViewType ViewType, VkImageAspectFlags Aspect, VkFormat Format);
	VkImageView View;
	VkImage Image;
	uint32 ViewId;
private:
	void CreateImpl(XVulkanDevice* Device, VkImage InImage, VkImageViewType ViewType, VkImageAspectFlags Aspect, VkFormat Format);
};


// This container holds the actual VkWriteDescriptorSet structures; a Compute pipeline uses the arrays 'as-is', whereas a 
// Gfx PSO will have one big array and chunk it depending on the stage (eg Vertex, Pixel).
struct XVulkanDescriptorSetWriteContainer
{
	//std::vector<FVulkanHashableDescriptorInfo> HashableDescriptorInfo;
	std::vector<VkDescriptorImageInfo> DescriptorImageInfo;
	std::vector<VkDescriptorBufferInfo> DescriptorBufferInfo;
	std::vector<VkWriteDescriptorSet> DescriptorWrites;
	//std::vector<uint8> BindingToDynamicOffsetMap;
};

// Information for remapping descriptor sets when combining layouts
struct XDescriptorSetRemappingInfo
{
	~XDescriptorSetRemappingInfo();
	struct XSetInfo
	{
		std::vector<VkDescriptorType>	Types;
		uint16						NumImageInfos = 0;
		uint16						NumBufferInfos = 0;
	};
	XSetInfo SetInfo;
};

// This class encapsulates updating VkWriteDescriptorSet structures (but doesn't own them), and their flags for dirty ranges; it is intended
// to be used to access a sub-region of a long array of VkWriteDescriptorSet (ie FVulkanDescriptorSetWriteContainer)
class XVulkanDescriptorSetWriter
{
public:
	bool WriteImage(uint32 DescriptorIndex, const XVulkanTextureView* TextureView, VkImageLayout Layout)
	{
		return WriteTextureView<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(DescriptorIndex, TextureView, Layout);
	}

	bool WriteDynamicUniformBuffer(uint32 DescriptorIndex, const XVulkanAllocation& Allocation, VkDeviceSize Offset, VkDeviceSize Range)
	{
		return WriteBuffer<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC>(DescriptorIndex, Allocation, Offset, Range);
	}

	void SetDescriptorSet(VkDescriptorSet DescriptorSet)
	{
		for (uint32 Index = 0; Index < NumWrites; ++Index)
		{
			WriteDescriptors[Index].dstSet = DescriptorSet;
		}
	}

	uint32 SetupDescriptorWrites(
		const std::vector<VkDescriptorType>& Types, VkWriteDescriptorSet* InWriteDescriptors,
		VkDescriptorImageInfo* InImageInfo, VkDescriptorBufferInfo* InBufferInfo,
		const XVulkanSamplerState* DefaultSampler, const XVulkanTextureView* DefaultImageView);
protected:
	template <VkDescriptorType DescriptorType>
	bool WriteTextureView(uint32 DescriptorIndex, const XVulkanTextureView* TextureView, VkImageLayout Layout)
	{

		VkDescriptorImageInfo* ImageInfo = const_cast<VkDescriptorImageInfo*>(WriteDescriptors[DescriptorIndex].pImageInfo);
		bool bChanged = false;
		bChanged = CopyAndReturnNotEqual(ImageInfo->imageView, TextureView->View);
		bChanged |= CopyAndReturnNotEqual(ImageInfo->imageLayout, Layout);

		return bChanged;
	}

	template <VkDescriptorType DescriptorType>
	bool WriteBuffer(uint32 DescriptorIndex, const XVulkanAllocation& Allocation, VkDeviceSize Offset, VkDeviceSize Range)
	{
		VkDescriptorBufferInfo* BufferInfo = const_cast<VkDescriptorBufferInfo*>(WriteDescriptors[DescriptorIndex].pBufferInfo);

		bool bChanged = false;
			bChanged = CopyAndReturnNotEqual(BufferInfo->buffer, Allocation.VulkanHandle);
			bChanged |= CopyAndReturnNotEqual(BufferInfo->offset, Offset);
			bChanged |= CopyAndReturnNotEqual(BufferInfo->range, Range);
		
		return bChanged;
	}
	
	uint32 NumWrites;

	// A view into someone else's descriptors
	VkWriteDescriptorSet* WriteDescriptors;
};

//TODO:Remove This
class XVulkanGfxPipelineDescriptorInfo
{
public:
	void Initialize(const XDescriptorSetRemappingInfo& InRemappingInfo);
	bool GetDescriptorSetAndBindingIndex(const EShaderType Stage, int32 ParameterIndex, uint8& OutDescriptorSet, uint32& OutBindingIndex) const;
	XDescriptorSetRemappingInfo DescriptorSetRemappingInfo;
};

// Information for the layout of descriptor sets; does not hold runtime objects
class XVulkanDescriptorSetsLayoutInfo
{
public:
	XVulkanDescriptorSetsLayoutInfo()
	{
		for (uint32 i = VK_DESCRIPTOR_TYPE_SAMPLER; i <= VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; ++i)
		{
			LayoutTypes[i] = 0;
		}
	}

	inline uint32 GetTypesUsed(VkDescriptorType Type) const
	{
		return LayoutTypes.find(static_cast<uint32>(Type))->second;
	}

	inline uint32 GetHash() const
	{
		return Hash;
	}

	struct XSetLayout
	{
		std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;

		inline void GenerateHash()
		{
			Hash = std::hash<std::string>{}(std::string((char*)LayoutBindings.data(), sizeof(VkDescriptorSetLayoutBinding) * LayoutBindings.size()));

		}

		uint32 GetHash()
		{
			return Hash;
		}

		uint32 Hash;
	};

	void FinalizeBindings_Gfx(XVulkanShader* VertexShader, XVulkanShader* PixelShader);
	//TODO:FindOrAddLayout
	XDescriptorSetRemappingInfo	RemappingInfo;
protected:
	std::map<uint32, uint32> LayoutTypes;

	XSetLayout SetLayout;//r.Vulkan.DescriptorSetLayoutMode

	uint32 Hash = 0;


	friend class XVulkanLayout;
	friend class XVulkanPipelineStateCacheManager;
};

using XVulkanDescriptorSetLayoutMap = std::map<uint32, VkDescriptorSetLayout>;


// The actual run-time descriptor set layouts
class XVulkanDescriptorSetsLayout : public XVulkanDescriptorSetsLayoutInfo
{
public:
	XVulkanDescriptorSetsLayout(XVulkanDevice* InDevice);

	void Compile(XVulkanDescriptorSetLayoutMap& LayoutMap);
	
	inline const VkDescriptorSetAllocateInfo& GetAllocateInfo() const
	{
		return DescriptorSetAllocateInfo;
	}

	VkDescriptorSetLayout LayoutHandle;
private:
	XVulkanDevice* Device;
	VkDescriptorSetAllocateInfo DescriptorSetAllocateInfo;
};


class XVulkanLayout
{
public:
	XVulkanLayout(XVulkanDevice* InDevice):DescriptorSetLayout(InDevice), Device(InDevice){}

	inline uint32 GetDescriptorSetLayoutHash() const
	{
		return DescriptorSetLayout.GetHash();
	}

	void Compile(XVulkanDescriptorSetLayoutMap& InDSetLayoutMap);

	const XVulkanDescriptorSetsLayout* GetDescriptorSetsLayout()
	{
		return &DescriptorSetLayout;
	}

	const VkPipelineLayout GetPipelineLayout()
	{
		return PipelineLayout;
	}
protected:
	XVulkanDescriptorSetsLayout	DescriptorSetLayout;
	XVulkanDevice* Device;
	VkPipelineLayout PipelineLayout;

	friend class XVulkanPipelineStateCacheManager;
};



class XVulkanGfxLayout : public XVulkanLayout
{
public:
	XVulkanGfxLayout(XVulkanDevice* InDevice) :XVulkanLayout(InDevice) {}

	inline const XVulkanGfxPipelineDescriptorInfo& GetGfxPipelineDescriptorInfo() const
	{
		return GfxPipelineDescriptorInfo;
	}
protected:
	XVulkanGfxPipelineDescriptorInfo		GfxPipelineDescriptorInfo;
	friend class XVulkanPipelineStateCacheManager;
};



class XVulkanDescriptorPool
{
public:
	XVulkanDescriptorPool(XVulkanDevice* InDevice, const XVulkanDescriptorSetsLayout* Layout, uint32 MaxSetsAllocations);
	bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VkDescriptorSet* OutSets);
private:
	XVulkanDevice* Device;
	const XVulkanDescriptorSetsLayout* Layout;
	VkDescriptorPool DescriptorPool;

	uint32 MaxDescriptorSets;
};



class XVulkanTypedDescriptorPoolArray
{
	
	XVulkanDescriptorPool* PushNewPool();
public:

	XVulkanTypedDescriptorPoolArray(XVulkanDevice* InDevice, const XVulkanDescriptorSetsLayout* InLayout)
		: Device(InDevice)
		, Layout(InLayout)
	{
		PushNewPool();
	};

	~XVulkanTypedDescriptorPoolArray();

	bool AllocateDescriptorSets(const XVulkanDescriptorSetsLayout* Layout, VkDescriptorSet* OutSets);
private:
	XVulkanDevice* Device;
	const XVulkanDescriptorSetsLayout* Layout;
	std::vector<XVulkanDescriptorPool*> PoolArray;
};


class XVulkanDescriptorPoolSetContainer
{
public:
	XVulkanDescriptorPoolSetContainer(XVulkanDevice* InDevice)
		:Device(InDevice)
		, bUsed(false)
	{

	}

	~XVulkanDescriptorPoolSetContainer();

	inline void SetUsed(bool bInUsed)
	{
		bUsed = bInUsed;
	}

	inline bool IsUnused() const
	{
		return !bUsed;
	}

	XVulkanTypedDescriptorPoolArray* AcquireTypedPoolArray(const XVulkanDescriptorSetsLayout* Layout);

private:
	bool bUsed;
	XVulkanDevice* Device;

	std::map<uint32, XVulkanTypedDescriptorPoolArray*>TypedDescriptorPools;
};

class XVulkanDescriptorPoolsManager
{
public:
	XVulkanDescriptorPoolsManager(XVulkanDevice* InDevice)
		:Device(InDevice)
	{
		
	}

	XVulkanDescriptorPoolSetContainer& AcquirePoolSetContainer();
private:
	std::vector<XVulkanDescriptorPoolSetContainer*>PoolSets;
	XVulkanDevice* Device;
};

class XVulkanBindlessDescriptorManager
{
public:
	XVulkanBindlessDescriptorManager(XVulkanDevice* InDevice);

	struct BindlessSetState
	{
		VkDescriptorType DescriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

		uint32 MaxDescriptorCount = 0;
		uint32 CurDescriptorCount = 1;//0 is null buffer

		uint32 FreeListHead = -1;
		uint32 DescriptorSize = 0;

		uint8* MappedPointer = nullptr;

		std::vector<uint8>DebugDescriptors;

		VkDeviceMemory MemoryHandle;
		VkBuffer BufferHandle;
		VkDescriptorSetLayout DescriptorSetLayout;
	};

	using XUniformBufferDescriptorArrays = std::array<VkDescriptorAddressInfoEXT, uint32(EShaderType::SV_ShaderCount)>;

	void UpdateBuffer(XRHIDescriptorHandle DescriptorHandle, VkBuffer Buffer, VkDeviceSize BufferOffset, VkDeviceSize BufferSize);
	void UpdateBuffer(XRHIDescriptorHandle DescriptorHandle, VkDeviceAddress BufferAddress, VkDeviceSize BufferSize);
	void UpdateDescriptor(XRHIDescriptorHandle DescriptorHandle, VkDescriptorDataEXT DescriptorData);

	void Unregister(XRHIDescriptorHandle DescriptorHandle);
	void Init();

	VkDescriptorBufferBindingInfoEXT BufferBindingInfo[VulkanBindless::NumBindLessSet];
	BindlessSetState BindlessSetStates[VulkanBindless::NumBindLessSet];

	uint32 BufferIndices[VulkanBindless::NumBindLessSet];

	uint32 GetFreeResourceStateIndex(BindlessSetState& State);
	XRHIDescriptorHandle ReserveDescriptor(VkDescriptorType DescriptorType);

	VkDescriptorSetLayout EmptyDescriptorSetLayout;

	VkPipelineLayout BindlessPipelineLayout;

	XVulkanDevice* Device;
};