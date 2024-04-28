#include "PlatformRHI.h"
#include "Runtime/HAL/Mch.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/RHICommandList.h"

#include "Runtime/VulkanRHI/VulkanPlatformRHI.h"
#include "Runtime/D3D12RHI/D3D12PlatformRHI.h"

XPlatformRHI* GPlatformRHI = nullptr;
XRHICommandList GRHICmdList;
bool GIsRHIInitialized = false;

void RHIRelease()
{
	delete GPlatformRHI;
}

void RHIInit(uint32 Width, uint32 Height , bool bUseDX12)
{
	if (GPlatformRHI == nullptr)
	{
		if (bUseDX12)
		{
			GPlatformRHI = new XD3D12PlatformRHI();
		}
		else
		{
			GPlatformRHI = new XVulkanPlatformRHI();
		}
		
		GPlatformRHI->Init();
	}
	
#if USE_DX12
	GRHICmdList.ReseizeViewport(Width, Height);
#endif
	GRHICmdList.Open();
	XRenderResource::InitRHIForAllResources();

	GRHICmdList.Execute();
	XASSERT(GIsRHIInitialized == false);
	
	GIsRHIInitialized = true;
}

#include "Runtime\VulkanRHI\VulaknHack.h"
#include "Runtime\VulkanRHI\VulkanPlatformRHI.h"
#include "Runtime\VulkanRHI\VulkanDevice.h"
#include "Runtime\VulkanRHI\VulkanViewport.h"
#include "Runtime\VulkanRHI\VulkanSwapChain.h"
#include "Runtime\VulkanRHI\VulkanCommandBuffer.h"
#include "Runtime\VulkanRHI\VulkanRHIPrivate.h"

VkDevice VkHack::GetVkDevice()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->Device->Device;
}

VkPhysicalDevice VkHack::GetVkPhyDevice()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->Device->Gpu;
}

VkExtent2D VkHack::GetBkBufferExtent()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->VulkanViewport->SwapChainExtent;
}

std::vector<VkImageView>& VkHack::GetBkImageViews()
{
	static std::vector<VkImageView> Views(2);
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	Views[0] = VkRHI->VulkanViewport->ImageViews[0].View;
	Views[1] = VkRHI->VulkanViewport->ImageViews[1].View;
	return Views;
}

uint32 VkHack::GetgraphicsFamilyIndex()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->Device->GfxQueueIndex;
}

VkQueue VkHack::GetVkQueue()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->Device->GfxQueue->Queue;
}

VkSwapchainKHR VkHack::GetVkSwapChain()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->VulkanViewport->VulkanSwapChain->swapChain;
}

VkCommandPool VkHack::GetCmdPool()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->Device->GfxContext->CmdBufferManager->Pool.CmdPool;
}

const VkCommandBuffer* VkHack::GetCmdBuffer()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return &VkRHI->Device->GfxContext->CmdBufferManager->Pool.CmdBuffers[0]->CommandBufferHandle;
}

VkRenderPass VkHack::GetVkRenderPas()
{
	for (
		auto iter = XVulkanCommandListContext::GlobalLayoutManager.RenderPasses.begin(); 
		iter != XVulkanCommandListContext::GlobalLayoutManager.RenderPasses.end(); 
		iter++)
	{
		return iter->second->GetRenderPass();
	}
	XASSERT(false);
	return VkRenderPass();
}

void VkHack::TempPresent()
{
	//XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	//VkRHI->GetVulkanViewport()->Prsent();
	return;
}

VkPipeline VkHack::GetVkPipeline()
{
	XVulkanPlatformRHI* VkRHI = (XVulkanPlatformRHI*)GPlatformRHI;
	return VkRHI->Device->PipelineStateCache->GraphicsPSOMap.begin()->second->VulkanPipeline;
}



//VkPipelineLayout VkHack::GetVkPipelineLayout()
//{
//	return VkPipelineLayout();
//}
