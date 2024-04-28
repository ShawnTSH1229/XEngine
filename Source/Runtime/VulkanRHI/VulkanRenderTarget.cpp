#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanRHIPrivate.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResource.h"


XVulkanRenderTargetLayout::XVulkanRenderTargetLayout(const XGraphicsPSOInitializer& Initializer)
{
	RenderPassFullHash = 42;
	RenderPassCompatibleHash = 42;
	bool bSetExtent = false;
	int32 Index = 0;
	for (; Index < Initializer.RTNums; Index++)
	{
		//PSO不需要LoadStore信息 RenderPass需要LS信息??

		VkAttachmentDescription CurrDesc{};
		CurrDesc.format = VkFormat(GPixelFormats[(int)Initializer.RT_Format[Index]].PlatformFormat);
		XASSERT(CurrDesc.format == VK_FORMAT_B8G8R8A8_SRGB);
		CurrDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		CurrDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		CurrDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		CurrDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		THashCombine(RenderPassFullHash, CurrDesc.loadOp);
		THashCombine(RenderPassFullHash, CurrDesc.storeOp);
		ColorAttachCount++;
		THashCombine(RenderPassCompatibleHash, CurrDesc.format);
		Desc[Index] = CurrDesc;
	}

	VkImageLayout DepthStencilLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (Initializer.DS_Format != EPixelFormat::FT_Unknown)
	{
		//RenderPass
		//PSO不需要LoadStore信息 RenderPass需要LS信息??

		//XASSERT(false);
		VkAttachmentDescription CurrDesc{};
		CurrDesc.format = VkFormat(GPixelFormats[(int)Initializer.DS_Format].PlatformFormat);
		CurrDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		CurrDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		CurrDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		CurrDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		Desc[Index] = CurrDesc;
		THashCombine(RenderPassFullHash, CurrDesc.loadOp);
		THashCombine(RenderPassFullHash, CurrDesc.storeOp);
		THashCombine(RenderPassCompatibleHash, CurrDesc.format);
		//ColorAttachCount++;
		HashDS = true;
	
	}
	THashCombine(RenderPassFullHash, RenderPassCompatibleHash);
}

XVulkanRenderTargetLayout::XVulkanRenderTargetLayout(XVulkanDevice& InDevice, const XRHIRenderPassInfo& RPInfo, VkImageLayout CurrentDSLayout)
{
	RenderPassFullHash = 42;
	RenderPassCompatibleHash = 42;
	bool bSetExtent = false;
	int32 Index = 0;
	for (;Index < MaxSimultaneousRenderTargets; Index++)
	{
		if (RPInfo.RenderTargets[Index].RenderTarget == nullptr)
		{
			break;
		}

		XVulkanTextureBase* Texture = GetVulkanTextureFromRHITexture(RPInfo.RenderTargets[Index].RenderTarget);

		if (!bSetExtent)
		{
			bSetExtent = true;
			Extent2D.width = Texture->Surface.Width;
			Extent2D.height = Texture->Surface.Height;
		}
		else
		{
			XASSERT(Extent2D.width == Texture->Surface.Width);
			XASSERT(Extent2D.height = Texture->Surface.Height);
		}

		VkAttachmentDescription CurrDesc{};
		CurrDesc.format = VkFormat(GPixelFormats[(int)RPInfo.RenderTargets[Index].RenderTarget->GetFormat()].PlatformFormat);
		XASSERT(CurrDesc.format == VK_FORMAT_B8G8R8A8_SRGB);
		CurrDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		CurrDesc.loadOp = RenderTargetLoadActionToVulkan(RPInfo.RenderTargets[Index].LoadAction);
		CurrDesc.storeOp = RenderTargetStoreActionToVulkan(RPInfo.RenderTargets[Index].StoreAction);
		CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//CurrDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//CurrDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		XASSERT_TEMP(false);
		CurrDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
		CurrDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		THashCombine(RenderPassFullHash, CurrDesc.loadOp);
		THashCombine(RenderPassFullHash, CurrDesc.storeOp);

		ColorAttachCount++;

		THashCombine(RenderPassCompatibleHash, CurrDesc.format);

		Desc[Index] = CurrDesc;
	}

	if (RPInfo.DepthStencilRenderTarget.DepthStencilTarget)
	{
		VkAttachmentDescription CurrDesc{};
		CurrDesc.format = VkFormat(GPixelFormats[(int)RPInfo.DepthStencilRenderTarget.DepthStencilTarget->GetFormat()].PlatformFormat);
		CurrDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		CurrDesc.loadOp = RenderTargetLoadActionToVulkan(RPInfo.DepthStencilRenderTarget.LoadAction);
		CurrDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		CurrDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		CurrDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		CurrDesc.initialLayout = CurrentDSLayout;
		CurrDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		Desc[Index] = CurrDesc;
		THashCombine(RenderPassFullHash, CurrDesc.loadOp);
		THashCombine(RenderPassFullHash, CurrDesc.storeOp);
		THashCombine(RenderPassCompatibleHash, CurrDesc.format);
		HashDS = true;
	}
;
	THashCombine(RenderPassFullHash, RenderPassCompatibleHash);
}


void XVulkanCommandListContext::RHIEndRenderPass()
{
	XVulkanCmdBuffer* CmdBuffer = CmdBufferManager->GetActiveCmdBuffer();
	GlobalLayoutManager.EndRenderPass(CmdBuffer);
}

void XVulkanCommandListContext::RHIBeginRenderPass(const XRHIRenderPassInfo& InInfo, const char* InName, uint32 Size)
{
	XVulkanCmdBuffer* CmdBuffer = CmdBufferManager->GetActiveCmdBuffer();

	VkImageLayout CurrentDSLayout;
	XRHITexture* DSTexture = InInfo.DepthStencilRenderTarget.DepthStencilTarget;
	if (DSTexture)
	{
		XVulkanSurface& Surface = static_cast<XVulkanTexture2D*>(DSTexture)->Surface;
		CurrentDSLayout = GlobalLayoutManager.GetFullLayout(&Surface).MainLayout;
	}
	else
	{
		CurrentDSLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	XVulkanRenderTargetLayout RTLayout(*Device, InInfo, CurrentDSLayout);
	XVulkanRenderPass* RenderPass = GlobalLayoutManager.GetOrCreateRenderPass(Device, &RTLayout);
	XRHISetRenderTargetsInfo RTInfo;
	InInfo.ConvertToRenderTargetsInfo(RTInfo);
	XVulkanFramebuffer* Framebuffer = GlobalLayoutManager.GetOrCreateFramebuffer(Device, &RTInfo, &RTLayout, RenderPass);
	GlobalLayoutManager.BeginRenderPass(CmdBuffer, &RTLayout, RenderPass, Framebuffer);

}

void XVulkanCommandListContext::Transition(XRHITransitionInfo TransitionInfo)
{
	XASSERT(TransitionInfo.Type == XRHITransitionInfo::EType::BVH);
	XASSERT(TransitionInfo.AccessBefore == ERHIAccess::BVHWrite);
	XASSERT(TransitionInfo.AccessAfter == ERHIAccess::BVHRead);

	VkMemoryBarrier Barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	Barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	Barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	VkPipelineStageFlags StageFlag = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	XVulkanCmdBuffer* CmdBuffer = CmdBufferManager->GetActiveCmdBuffer();
	vkCmdPipelineBarrier(CmdBuffer->GetHandle(), StageFlag, StageFlag, 0, 1, &Barrier, 0, nullptr, 0, nullptr);
}

XVulkanRenderPass* XVulkanCommandListContext::PrepareRenderPassForPSOCreation(const XGraphicsPSOInitializer& Initializer)
{
	XVulkanRenderTargetLayout RTLayout(Initializer);
	return GlobalLayoutManager.GetOrCreateRenderPass(Device, &RTLayout);;
}
