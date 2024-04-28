#include "VulkanPlatformRHI.h"
#include "VulkanQueue.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"

XVulkanQueue::XVulkanQueue(XVulkanDevice* VulkanDevice, uint32 InFamilyIndex)
	: Queue(VK_NULL_HANDLE)
	, FamilyIndex(InFamilyIndex)
	, Device(VulkanDevice)
{
	vkGetDeviceQueue(Device->GetVkDevice(), FamilyIndex, 0, &Queue);
}

void XVulkanQueue::Submit(XVulkanCmdBuffer* CmdBuffer, uint32 NumSignalSemaphores, VkSemaphore* SignalSemaphores)
{
	const VkCommandBuffer CmdBuffers[] = { CmdBuffer->GetHandle() };
	XFence* Fence = CmdBuffer->Fence;

	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = CmdBuffers;
	SubmitInfo.signalSemaphoreCount = NumSignalSemaphores;
	SubmitInfo.pSignalSemaphores = SignalSemaphores;

	std::vector<VkSemaphore> WaitSemaphores;
	if (CmdBuffer->WaitSemaphores.size() > 0)
	{
		WaitSemaphores.reserve((uint32)CmdBuffer->WaitSemaphores.size());
		for (XSemaphore* Semaphore : CmdBuffer->WaitSemaphores)
		{
			WaitSemaphores.push_back(Semaphore->GetHandle());
		}
		SubmitInfo.waitSemaphoreCount = (uint32)CmdBuffer->WaitSemaphores.size();
		SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
		SubmitInfo.pWaitDstStageMask = CmdBuffer->WaitFlags.data();
	}

	vkQueueSubmit(Queue, 1, &SubmitInfo, Fence->GetHandle());
	CmdBuffer->State = XVulkanCmdBuffer::EState::Submitted;
	CmdBuffer->MarkSemaphoresAsSubmitted();
	CmdBuffer->GetOwner()->RefreshFenceStatus(CmdBuffer);
	//Device->GetStagingManager().ProcessPendingFree(false, false);
}
