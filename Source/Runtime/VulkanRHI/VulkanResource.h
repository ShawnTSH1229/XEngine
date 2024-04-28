#pragma once
#include <map>
#include <vulkan\vulkan_core.h>
#include <Runtime\HAL\Mch.h>
#include <Runtime\HAL\PlatformTypes.h>
#include "Runtime\RHI\RHIResource.h"
#include "Runtime\RenderCore\ShaderCore.h"
#include "VulkanPipeline.h"
#include "VulkanMemory.h"

class XVulkanDevice;


class XVulkanVertexLayout :public XRHIVertexLayout
{
public:
	XRHIVertexLayoutArray VertexElements;
	explicit XVulkanVertexLayout(const XRHIVertexLayoutArray& InVertexElements) :
		VertexElements(InVertexElements) {}
};

class XVulkanSurface
{
public:
	XVulkanSurface(XVulkanDevice* Device, EPixelFormat Format, uint32 Width , uint32 Height ,VkImageViewType	InViewType);
	
	// Constructor for externally owned Image
	XVulkanSurface(XVulkanDevice* InDevice, EPixelFormat InFormat, uint32 Width, uint32 Height, VkImageViewType	ViewType ,VkImage InImage);
	XVulkanSurface(XVulkanEvictable* Owner, XVulkanDevice* InDevice, EPixelFormat InFormat, uint32 Width, uint32 Height, VkImageViewType	InViewType, ETextureCreateFlags Flag, uint32 NumMipsIn, uint8* TexData, uint32 DataSize);

	inline VkImageViewType GetViewType() const { return ViewType; }

	static void InternalLockWrite(class XVulkanCommandListContext* Context, XVulkanSurface* Surface, const VkBufferImageCopy& Region, XStagingBuffer* StagingBuffer);

	EPixelFormat PixelFormat;
	XVulkanDevice* Device;

	VkFormat ViewFormat;
	VkImage Image;

	uint32 Width;
	uint32 Height;

	VkImageAspectFlags FullAspectMask;
	VkMemoryRequirements MemoryRequirements;
private:
	void SetInitialImageState(XVulkanCommandListContext& Context, VkImageLayout InitialLayout, bool bClear);

	XVulkanAllocation Allocation;
	VkImageViewType	ViewType;
};

class XVulkanTextureBase :public XVulkanEvictable
{
public:
	XVulkanTextureBase(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType	InViewType);
	XVulkanTextureBase(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType	InViewType, VkImage InImage);

	XVulkanTextureBase(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType	InViewType, ETextureCreateFlags Flag, uint32 NumMipsIn, uint8* TexData, uint32 DataSize);

	XVulkanSurface Surface;
	XVulkanTextureView DefaultView;
};

class XVulkanTexture2D : public XRHITexture2D, public XVulkanTextureBase
{
public:
	XVulkanTexture2D(XVulkanDevice* Device, EPixelFormat Forma, uint32 Width, uint32 Height, VkImageViewType	InViewType);
	XVulkanTexture2D(XVulkanDevice* Device, EPixelFormat Forma, uint32 Width, uint32 Height, VkImageViewType	InViewType, VkImage InImage);
	
	XVulkanTexture2D(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, ETextureCreateFlags Flag, uint32 NumMipsIn, uint8* TexData, uint32 DataSize);

	virtual void* GetTextureBaseRHI()
	{
		return static_cast<XVulkanTextureBase*>(this);
	}
};

inline XVulkanTextureBase* GetVulkanTextureFromRHITexture(XRHITexture* Texture)
{
	if (!Texture) { return NULL; }
	XVulkanTextureBase* Result = ((XVulkanTextureBase*)Texture->GetTextureBaseRHI()); XASSERT(Result);
	return Result;
}

class XVulkanShader
{
public:
	XVulkanShader(XVulkanDevice* InDevice, EShaderType InShaderType);
	
	class XSpirvContainer
	{
	public:
		friend class XVulkanShader;
		std::vector<uint8>	SpirvCode;
	} SpirvContainer;

	XShaderResourceCount ResourceCount;
	std::string EntryName;


	std::string GetEntryPoint()
	{
		return EntryName;
	}

	VkShaderModule GetOrCreateHandle();

	VkShaderModule GetOrCreateHandle(const XGfxPipelineDesc& Desc, uint32 LayoutHash)
	{
		auto iter = ShaderModules.find(LayoutHash);;
		if (iter != ShaderModules.end())
		{
			return iter->second;
		}

		return CreateHandle(Desc, LayoutHash);
	}

	XVulkanDevice* Device;
	std::map<uint32, VkShaderModule> ShaderModules;
protected:
	VkShaderModule CreateHandle(const XGfxPipelineDesc& Desc, uint32 LayoutHash);
};

class XVulkanVertexShader : public XRHIVertexShader , public XVulkanShader
{
public:
	XVulkanVertexShader(XVulkanDevice* InDevice, EShaderType ShaderTypeIn)
		:XVulkanShader(InDevice, EShaderType::SV_Vertex) {}
	enum
	{
		ShaderTypeStatic = EShaderType::SV_Vertex
	};
};

class XVulkanPixelShader : public XRHIPixelShader, public XVulkanShader
{
public:
	XVulkanPixelShader(XVulkanDevice* InDevice, EShaderType ShaderTypeIn)
		:XVulkanShader(InDevice, EShaderType::SV_Pixel) {}

	enum
	{
		ShaderTypeStatic = EShaderType::SV_Pixel
	};
};

class XVulkanRayTracingShader : public XRHIRayTracingShader, public XVulkanShader
{
public:
	XVulkanRayTracingShader(XVulkanDevice* InDevice, EShaderType ShaderTypeIn)
		: XRHIRayTracingShader(ShaderTypeIn),
		XVulkanShader(InDevice, ShaderTypeIn) {}

	enum
	{
		ShaderTypeStatic = EShaderType::SV_ShaderCount
	};
	
};



//TODO XVulkanResourceMultiBuffer
class XVulkanResourceMultiBuffer : public XRHIBuffer ,public XVulkanEvictable
{
public:
	XVulkanResourceMultiBuffer(XVulkanDevice* Device,uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData);
	
	void* Lock(EResourceLockMode LockMode, uint32 LockSize, uint32 Offset);
	void UnLock(class XVulkanCommandListContext* Context);
	
	static VkBufferUsageFlags UEToVKBufferUsageFlags(EBufferUsage InUEUsage);
	
#if RHI_RAYTRACING
	VkDeviceAddress GetDeviceAddress() const;
#endif

	XVulkanAllocation Buffer;
	VkBufferUsageFlags BufferUsageFlags;
	static void InternalUnlock(class XVulkanCommandListContext* Context, class XPendingBufferLock* PendingLock, XVulkanResourceMultiBuffer* MultiBuffer);
private:
	XVulkanDevice* Device;

};

class XVulkanConstantBuffer : public XRHIConstantBuffer
{
public:
	XVulkanConstantBuffer(XVulkanDevice* Device, uint32 Size);

	void UpdateData(const void* data, uint32 size, uint32 offset_byte)override;

	const uint64 GetSize()const
	{
		return Size;
	}

	const uint64 GetOffset()const
	{
		return Allocation.Offset;
	}

	XVulkanAllocation Allocation;

	XVulkanDevice* Device;
	uint64 Size;
};

class XVulkanShaderFactory
{
public:
	~XVulkanShaderFactory();

	template <typename ShaderType>
	ShaderType* CreateShader(XArrayView<uint8> Code, XVulkanDevice* Device, EShaderType InShaderType);

	template <typename ShaderType>
	ShaderType* LookupShader(uint64 ShaderKey, EShaderType InShaderType) const;

private:
	std::map<uint64,XVulkanShader*>MapToVkShader[(uint32)EShaderType::SV_ShaderCount];
	
};

class XVulkanShaderResourceView : public XRHIShaderResourceView
{
public:
	XVulkanShaderResourceView(XVulkanDevice* Device, XVulkanResourceMultiBuffer* InSourceBuffer, uint32 InOffset = 0);

	XVulkanResourceMultiBuffer* SourceStructuredBuffer = nullptr;
	uint32 Size = 0;
	uint32 Offset = 0;

	std::shared_ptr<XRHIBuffer> SourceRHIBuffer;

#if RHI_RAYTRACING
	VkAccelerationStructureKHR AccelerationStructureHandle = VK_NULL_HANDLE;
#endif // VULKAN_RHI_RAYTRACING
};
