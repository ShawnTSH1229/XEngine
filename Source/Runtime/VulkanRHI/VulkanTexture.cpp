#include "VulkanResource.h"
#include "VulkanPlatformRHI.h"
#include "VulkanDevice.h"
#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"

void XVulkanSurface::SetInitialImageState(XVulkanCommandListContext& Context, VkImageLayout InitialLayout, bool bClear)
{
	XVulkanCmdBuffer* CmdBuffer = Context.GetCommandBufferManager()->GetUploadCmdBuffer();

	VkImageSubresourceRange SubresourceRange = XVulkanPipelineBarrier::MakeSubresourceRange(FullAspectMask);

	VkImageLayout CurrentLayout;
	if (bClear)
	{
		VulkanSetImageLayout(CmdBuffer->GetHandle(), Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubresourceRange);
		if (FullAspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
		{
			VkClearColorValue Color = {};
			vkCmdClearColorImage(CmdBuffer->GetHandle(), Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &Color, 1, &SubresourceRange);
		}
		else
		{
			VkClearDepthStencilValue Value = {};
			vkCmdClearDepthStencilImage(CmdBuffer->GetHandle(), Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &Value, 1, &SubresourceRange);
		}
		CurrentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}
	else
	{
		CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	if (InitialLayout != CurrentLayout && InitialLayout != VK_IMAGE_LAYOUT_UNDEFINED)
	{
		VulkanSetImageLayout(CmdBuffer->GetHandle(), Image, CurrentLayout, InitialLayout, SubresourceRange);
	}
}

XVulkanSurface::XVulkanSurface(XVulkanDevice* InDevice, EPixelFormat InFormat, uint32 InWidth, uint32 InHeight, VkImageViewType	InViewType)
	: Device(InDevice)
	, PixelFormat(InFormat)
	, ViewType(InViewType)
	, Width(InWidth)
	, Height(InHeight)
{
	ViewFormat = VkFormat(GPixelFormats[(int)PixelFormat].PlatformFormat);

	FullAspectMask = GetAspectMaskFromFormat(InFormat, true, true);
	ViewFormat = ToVkTextureFormat(InFormat, false);
	VkImageType ImageType;
	switch (InViewType)
	{
	case VkImageViewType::VK_IMAGE_VIEW_TYPE_2D:
		ImageType = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	default:
		XASSERT(false);
	};

	VkImageCreateInfo ImageCreateInfo{};
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = ImageType;
	ImageCreateInfo.extent.width = Width;
	ImageCreateInfo.extent.height = Height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.format = ViewFormat;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	ImageCreateInfo.usage = 0;
	ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	ImageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

	VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VULKAN_VARIFY(vkCreateImage(Device->GetVkDevice(), &ImageCreateInfo, nullptr, &Image));

	vkGetImageMemoryRequirements(Device->GetVkDevice(), Image, &MemoryRequirements);
}

XVulkanSurface::XVulkanSurface(XVulkanDevice* InDevice, EPixelFormat InFormat, uint32 InWidth, uint32 InHeight, VkImageViewType	InViewType, VkImage InImage)
	: Device(InDevice)
	, PixelFormat(InFormat)
	, ViewType(InViewType)
	, Image(InImage)
	, Width(InWidth)
	, Height(InHeight)
{
	ViewFormat = VkFormat(GPixelFormats[(int)PixelFormat].PlatformFormat);
}

XVulkanSurface::XVulkanSurface(XVulkanEvictable* Owner, XVulkanDevice* InDevice, EPixelFormat InFormat, uint32 Width, uint32 Height, VkImageViewType InViewType, ETextureCreateFlags Flag, uint32 NumMipsIn, uint8* TexData, uint32 DataSize)
	: Device(InDevice)
	, PixelFormat(InFormat)
	, ViewType(InViewType)
	, Width(Width)
	, Height(Height)
{
	FullAspectMask = GetAspectMaskFromFormat(InFormat, true, true);
	ViewFormat = ToVkTextureFormat(InFormat, ((Flag & ETextureCreateFlags::TexCreate_SRGB) != 0));
	VkImageType ImageType;
	switch (InViewType)
	{
	case VkImageViewType::VK_IMAGE_VIEW_TYPE_2D:
		ImageType = VkImageType::VK_IMAGE_TYPE_2D;
		break;
	default:
		XASSERT(false);
	};

	VkImageCreateInfo ImageCreateInfo{};
	ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ImageCreateInfo.imageType = ImageType;
	ImageCreateInfo.extent.width = Width;
	ImageCreateInfo.extent.height = Height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = NumMipsIn;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.format = ViewFormat;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	ImageCreateInfo.usage = 0;
	ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	ImageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	
	VkImageLayout InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (((Flag & ETextureCreateFlags::TexCreate_RenderTargetable) != 0))
	{
		InitialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	else if (((Flag & ETextureCreateFlags::TexCreate_DepthStencilTargetable) != 0))
	{
		InitialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	else if (((Flag & ETextureCreateFlags::TexCreate_UAV) != 0))
	{
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}
	else
	{
		InitialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//ImageCreateInfo.initialLayout = InitialLayout;

	VULKAN_VARIFY(vkCreateImage(Device->GetVkDevice(), &ImageCreateInfo, nullptr, &Image));

	vkGetImageMemoryRequirements(Device->GetVkDevice(), Image, &MemoryRequirements);

	const bool bRenderTarget =
		((Flag & ETextureCreateFlags::TexCreate_RenderTargetable) != 0)
		|| ((Flag & ETextureCreateFlags::TexCreate_DepthStencilTargetable) != 0);
		
	const bool bUAV = ((Flag & ETextureCreateFlags::TexCreate_UAV) != 0);
	
	VkMemoryPropertyFlags MemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	EVulkanAllocationMetaType MetaType = (bRenderTarget || bUAV) ? EVulkanAllocationMetaImageRenderTarget : EVulkanAllocationMetaImageOther;
	Device->GetMemoryManager().AllocateImageMemory(Allocation, Owner, MemoryRequirements, MemoryFlags, MetaType);
	Allocation.BindImage(Device, Image);

	SetInitialImageState(*Device->GetGfxContex(), InitialLayout, false);
	auto& Ret = InDevice->GetGfxContex()->GetLayoutManager().GetOrAddFullLayout(this, InitialLayout);
	Ret.MainLayout = InitialLayout;
}

void XVulkanSurface::InternalLockWrite(XVulkanCommandListContext* Context, XVulkanSurface* Surface, const VkBufferImageCopy& Region, XStagingBuffer* StagingBuffer)
{
	XVulkanCmdBuffer* Cmd = Context->GetCommandBufferManager()->GetUploadCmdBuffer();
	VkCommandBuffer CmdBuffer = Cmd->GetHandle();

	XVulkanImageLayout& TrackedTextureLayout = Context->GetLayoutManager().GetOrAddFullLayout(Surface, VK_IMAGE_LAYOUT_UNDEFINED);
	XVulkanImageLayout TransferTextureLayout = TrackedTextureLayout;
	TransferTextureLayout.Set(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	{
		XVulkanPipelineBarrier Barrier;
		Barrier.AddImageLayoutTransition(Surface->Image, Region.imageSubresource.aspectMask, TrackedTextureLayout, TransferTextureLayout);
		Barrier.Execute(CmdBuffer);
	}
	vkCmdCopyBufferToImage(CmdBuffer, StagingBuffer->Buffer, Surface->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

	TrackedTextureLayout.Set(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	{
		XVulkanPipelineBarrier Barrier;
		Barrier.AddImageLayoutTransition(Surface->Image, Region.imageSubresource.aspectMask, TransferTextureLayout, TrackedTextureLayout);
		Barrier.Execute(CmdBuffer);
	}

	//Surface->Device->GetStagingManager().ReleaseBuffer(CmdBuffer, StagingBuffer);
	Context->GetCommandBufferManager()->SubmitUploadCmdBuffer();
}



XVulkanTextureBase::XVulkanTextureBase(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType	InViewType)
	:Surface(Device,Format, Width, Height, InViewType)
{
	
}

XVulkanTextureBase::XVulkanTextureBase(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType InViewType, VkImage InImage)
	:Surface(Device, Format, Width, Height, InViewType, InImage)
{
	
}

XVulkanTextureBase::XVulkanTextureBase(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType InViewType, ETextureCreateFlags Flag, uint32 NumMipsIn, uint8* TexData, uint32 DataSize)
	:Surface(this, Device, Format, Width, Height, VK_IMAGE_VIEW_TYPE_2D, Flag, NumMipsIn, TexData, DataSize)
{
	DefaultView.Create(Device, Surface.Image, VK_IMAGE_VIEW_TYPE_2D, Surface.FullAspectMask, Surface.ViewFormat);

	if (TexData == nullptr)
	{
		return;
	}

	XStagingBuffer* StagingBuffer = Device->GetStagingManager().AcquireBuffer(DataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	void* Data = StagingBuffer->GetMappedPointer();
	memcpy(Data, TexData, DataSize);
	//CreateInfo.BulkData->Discard();

	VkBufferImageCopy Region{};
	Region.bufferOffset = 0;
	Region.bufferRowLength = Surface.Width;
	Region.bufferImageHeight = Surface.Height;

	Region.imageSubresource.mipLevel = 0;
	Region.imageSubresource.baseArrayLayer = 0;
	Region.imageSubresource.layerCount = 1;
	Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	Region.imageExtent.width = Region.bufferRowLength;
	Region.imageExtent.height = Region.bufferImageHeight;
	Region.imageExtent.depth = 1;

	XVulkanSurface::InternalLockWrite(Device->GetGfxContex(), &Surface, Region, StagingBuffer);

}

XVulkanTexture2D::XVulkanTexture2D(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType	InViewType)
	: XRHITexture2D(Format)
	, XVulkanTextureBase(Device, Format, Width, Height, InViewType)
{

}

XVulkanTexture2D::XVulkanTexture2D(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, VkImageViewType InViewType, VkImage InImage)
	: XRHITexture2D(Format)
	, XVulkanTextureBase(Device, Format, Width, Height, InViewType, InImage)
{
}

XVulkanTexture2D::XVulkanTexture2D(XVulkanDevice* Device, EPixelFormat Format, uint32 Width, uint32 Height, ETextureCreateFlags Flag, uint32 NumMipsIn, uint8* TexData, uint32 DataSize)
	: XRHITexture2D(Format)
	, XVulkanTextureBase(Device, Format, Width, Height, VK_IMAGE_VIEW_TYPE_2D, Flag, NumMipsIn, TexData, DataSize)
{
}

std::shared_ptr<XRHITexture2D> XVulkanPlatformRHI::RHICreateTexture2D2(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray, bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size)
{
	return std::make_shared<XVulkanTexture2D>(Device, Format, width, height, flag, NumMipsIn, tex_data, Size);
}
