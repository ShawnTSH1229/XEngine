#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanRHIPrivate.h"
#include "VulkanBarriers.h"

#include "VulkanResource.h"
#include "VulkanCommandBuffer.h"
#include "VulkanPendingState.h"

XVulkanLayoutManager XVulkanCommandListContext::GlobalLayoutManager;

void VulkanSetImageLayout(VkCommandBuffer CmdBuffer, VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout, const VkImageSubresourceRange& SubresourceRange)
{
	XVulkanPipelineBarrier Barrier;
	Barrier.AddImageLayoutTransition(Image, OldLayout, NewLayout, SubresourceRange);
	Barrier.Execute(CmdBuffer);
}

XVulkanLayoutManager::~XVulkanLayoutManager()
{
	for (auto iter = RenderPasses.begin(); iter != RenderPasses.end(); iter++)
	{
		delete iter->second;
	}
}

XVulkanFramebuffer* XVulkanLayoutManager::GetOrCreateFramebuffer(XVulkanDevice* InDevice, const XRHISetRenderTargetsInfo* RenderTargetsInfo, const XVulkanRenderTargetLayout* RTLayout, XVulkanRenderPass* RenderPass)
{
	static uint32 HashHackTemp = 0;
	HashHackTemp++;
	HashHackTemp = HashHackTemp % 2;

	uint32 RTLayoutHash = RTLayout->GetRenderPassCompatibleHash() + HashHackTemp;

	auto iter = Framebuffers.find(RTLayoutHash);
	XFramebufferList* FramebufferList = nullptr;
	if (iter != Framebuffers.end())
	{
		XASSERT_TEMP(false);
		for (int32 Index = 0; Index < iter->second->Framebuffer.size(); Index++)
		{
			if (iter->second->Framebuffer[Index]->Matches(*RenderTargetsInfo))
			{
				return iter->second->Framebuffer[Index];
			}
		}
	}
	
	FramebufferList = new XFramebufferList;
	Framebuffers[RTLayoutHash] = FramebufferList;
	XVulkanFramebuffer* Framebuffer = new XVulkanFramebuffer(InDevice, RenderTargetsInfo, RTLayout, RenderPass);
	FramebufferList->Framebuffer.push_back(Framebuffer);
	return Framebuffer;
}

void XVulkanLayoutManager::BeginRenderPass(XVulkanCmdBuffer* CmdBuffer, const XVulkanRenderTargetLayout* RTLayout, XVulkanRenderPass* RenderPass, XVulkanFramebuffer* Framebuffer)
{
	CmdBuffer->BeginRenderPass(RTLayout, RenderPass, Framebuffer);
	const VkExtent2D& Extents = RTLayout->GetExtent2D();
	GfxContex->GetPendingGfxState()->SetViewport(0, 0, 0, Extents.width, Extents.height, 1);
}

void XVulkanLayoutManager::EndRenderPass(XVulkanCmdBuffer* CmdBuffer)
{
	CmdBuffer->EndRenderPass();
}

XVulkanRenderPass* XVulkanLayoutManager::GetOrCreateRenderPass(XVulkanDevice* InDevice, const XVulkanRenderTargetLayout* RTLayout)
{
	const uint32 RenderPassHash = RTLayout->GetRenderPassFullHash();
	auto iter = RenderPasses.find(RenderPassHash);
	if (iter != RenderPasses.end())
	{
		return iter->second;
	}

	XVulkanRenderPass* RenderPass = new XVulkanRenderPass(InDevice, RTLayout);
	RenderPasses[RenderPassHash] = RenderPass;
	return RenderPass;
}

XVulkanImageLayout& XVulkanLayoutManager::GetOrAddFullLayout(const XVulkanSurface* Surface, VkImageLayout LayoutIfNotFound)
{
	auto iter = Layouts.find(Surface->Image);
	if (iter != Layouts.end())
	{
		return iter->second;
	}
	Layouts[Surface->Image] = XVulkanImageLayout(LayoutIfNotFound);
	return Layouts[Surface->Image];
}

XVulkanImageLayout& XVulkanLayoutManager::GetFullLayout(const XVulkanSurface* Surface)
{
	auto iter = Layouts.find(Surface->Image);
	if (iter != Layouts.end())
	{
		return iter->second;
	}
	else
	{
		XASSERT(false);
	}
}


bool XVulkanFramebuffer::Matches(const XRHISetRenderTargetsInfo& RTInfo) const
{
	bool bMatch = false;
	for (int32 Index = 0; Index < RTInfo.NumColorRenderTargets; Index++)
	{
		const VkImage Image = GetVulkanTextureFromRHITexture(RTInfo.ColorRenderTarget[Index].Texture)->Surface.Image;
		if (ColorRenderTargetImages[Index] != Image)
		{
			return false;
		}
	}
	XASSERT_TEMP(false);
	return true;
}

void XVulkanPipelineBarrier::AddImageLayoutTransition(VkImage Image, VkImageAspectFlags AspectMask, const XVulkanImageLayout& SrcLayout, const XVulkanImageLayout& DstLayout)
{
	if (SrcLayout.AreAllSubresourcesSameLayout())
	{
		AddImageLayoutTransition(Image, AspectMask, SrcLayout.MainLayout, DstLayout);
	}
	else if (DstLayout.AreAllSubresourcesSameLayout())
	{
		AddImageLayoutTransition(Image, AspectMask, SrcLayout, DstLayout.MainLayout);
	}
	else
	{
		XASSERT(false);
	}
}

void XVulkanPipelineBarrier::AddImageLayoutTransition(VkImage Image, VkImageAspectFlags AspectMask, VkImageLayout SrcLayout, const XVulkanImageLayout& DstLayout)
{
	if (DstLayout.AreAllSubresourcesSameLayout())
	{
		AddImageLayoutTransition(Image, SrcLayout, DstLayout.MainLayout, MakeSubresourceRange(AspectMask));
	}
	else
	{
		XASSERT(false);
	}
}

void XVulkanPipelineBarrier::AddImageLayoutTransition(VkImage Image, VkImageAspectFlags AspectMask, const XVulkanImageLayout& SrcLayout, VkImageLayout DstLayout)
{
	if (SrcLayout.AreAllSubresourcesSameLayout())
	{
		AddImageLayoutTransition(Image, SrcLayout.MainLayout, DstLayout, MakeSubresourceRange(AspectMask));
	}
	else
	{
		XASSERT(false);
	}
}

static VkPipelineStageFlags GetVkStageFlagsForLayout(VkImageLayout Layout)
{
	VkPipelineStageFlags Flags = 0;

	switch (Layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		Flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		Flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		Flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
	case VK_IMAGE_LAYOUT_UNDEFINED:
		Flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		Flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	default:XASSERT(false);
	}
	return Flags;
}
static VkAccessFlags GetVkAccessMaskForLayout(VkImageLayout Layout)
{
	VkAccessFlags Flags = 0;

	switch (Layout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		Flags = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		Flags = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		Flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		Flags = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		Flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
	case VK_IMAGE_LAYOUT_UNDEFINED:
		Flags = 0;
		break;
	default:XASSERT(false);
	}
	return Flags;
}

static void SetupImageBarrier(VkImageMemoryBarrier& ImgBarrier, VkImage Image, VkAccessFlags SrcAccessFlags, VkAccessFlags DstAccessFlags, VkImageLayout SrcLayout, VkImageLayout DstLayout, const VkImageSubresourceRange& SubresRange)
{
	ImgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	ImgBarrier.pNext = nullptr;
	ImgBarrier.srcAccessMask = SrcAccessFlags;
	ImgBarrier.dstAccessMask = DstAccessFlags;
	ImgBarrier.oldLayout = SrcLayout;
	ImgBarrier.newLayout = DstLayout;
	ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	ImgBarrier.image = Image;
	ImgBarrier.subresourceRange = SubresRange;
}

void XVulkanPipelineBarrier::AddImageLayoutTransition(VkImage Image, VkImageLayout SrcLayout, VkImageLayout DstLayout, const VkImageSubresourceRange& SubresourceRange)
{
	SrcStageMask |= GetVkStageFlagsForLayout(SrcLayout);
	DstStageMask |= GetVkStageFlagsForLayout(DstLayout);
	
	VkAccessFlags SrcAccessFlags = GetVkAccessMaskForLayout(SrcLayout);
	VkAccessFlags DstAccessFlags = GetVkAccessMaskForLayout(DstLayout);

	ImageBarriers.push_back(VkImageMemoryBarrier{});
	SetupImageBarrier(ImageBarriers[ImageBarriers.size() - 1], Image, SrcAccessFlags, DstAccessFlags, SrcLayout, DstLayout, SubresourceRange);
}

VkImageSubresourceRange XVulkanPipelineBarrier::MakeSubresourceRange(VkImageAspectFlags AspectMask, uint32 FirstMip, uint32 NumMips, uint32 FirstLayer, uint32 NumLayers)
{
	VkImageSubresourceRange Range;
	Range.aspectMask = AspectMask;
	Range.baseMipLevel = FirstMip;
	Range.levelCount = NumMips;
	Range.baseArrayLayer = FirstLayer;
	Range.layerCount = NumLayers;
	return Range;
}

void XVulkanPipelineBarrier::Execute(VkCommandBuffer CmdBuffer)
{
	if (bHasMemoryBarrier || ImageBarriers.size() != 0)
	{
		vkCmdPipelineBarrier(CmdBuffer, SrcStageMask, DstStageMask, 0, bHasMemoryBarrier ? 1 : 0, &mMemoryBarrier, 0, nullptr, ImageBarriers.size(), ImageBarriers.data());
	}
}

void XVulkanImageLayout::Set(VkImageLayout Layout)
{
	MainLayout = Layout;
}
