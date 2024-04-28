#include "VulkanContext.h"
#include "VulkanPlatformRHI.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRHIPrivate.h"
#include "VulkanDevice.h"
#include "VulkanViewport.h"
#include "VulkanPendingState.h"

XVulkanCommandListContext::XVulkanCommandListContext(XVulkanPlatformRHI* InRHI, XVulkanDevice* InDevice, XVulkanQueue* InQueue)
	: RHI(InRHI)
	, Device(InDevice)
	, Queue(InQueue)
{
	GlobalLayoutManager.SetGfxContex(this);
	CmdBufferManager = new XVulkanCommandBufferManager(Device, this);
	PendingGfxState = new XVulkanPendingGfxState(Device);
}

void XVulkanCommandListContext::RHIEndFrame()
{
	RHI->GetVulkanViewport()->Prsent(this, CmdBufferManager->GetActiveCmdBuffer(), Queue, Queue);
}

XVulkanCommandListContext::~XVulkanCommandListContext()
{
	delete CmdBufferManager;
}

void XVulkanCommandListContext::Execute()
{
	vkEndCommandBuffer(CmdBufferManager->GetActiveCmdBuffer()->GetHandle());
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = CmdBufferManager->GetActiveCmdBuffer()->GetHandlePtr();
	vkQueueSubmit(GetQueue()->GetVkQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(GetQueue()->GetVkQueue());
}

void XVulkanCommandListContext::OpenCmdList()
{
	vkResetCommandBuffer(CmdBufferManager->GetActiveCmdBuffer()->GetHandle(), /*VkCommandBufferResetFlagBits*/ 0);
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(CmdBufferManager->GetActiveCmdBuffer()->GetHandle(), &beginInfo);
}

void XVulkanCommandListContext::CloseCmdList()
{
	vkEndCommandBuffer(CmdBufferManager->GetActiveCmdBuffer()->GetHandle());
}
