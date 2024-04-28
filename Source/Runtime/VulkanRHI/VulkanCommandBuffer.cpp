#include <vulkan\vulkan_core.h>
#include "VulkanCommandBuffer.h"
#include "VulkanQueue.h"
#include "VulkanPlatformRHI.h"
#include "VulkanDevice.h"
#include "VulkanRHIPrivate.h"

XVulkanCmdBuffer::XVulkanCmdBuffer(XVulkanDevice* InDevice, XVulkanCommandBufferPool* InCommandBufferPool)
	: Device(InDevice)
	, CmdBufferPool(InCommandBufferPool)
	, State(EState::NotAllocated)
{
	AllocMemory();
	Fence = Device->GetFenceManager().AllocateFence();
}

void XVulkanCmdBuffer::AllocMemory()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = CmdBufferPool->GetVkPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VULKAN_VARIFY(vkAllocateCommandBuffers(Device->GetVkDevice(), &allocInfo, &CommandBufferHandle));

	State = EState::ReadyForBegin;
}

void XVulkanCmdBuffer::BeginRenderPass(const XVulkanRenderTargetLayout* Layout, XVulkanRenderPass* RenderPass, XVulkanFramebuffer* Framebuffer)
{
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = RenderPass->GetRenderPass();
	renderPassInfo.framebuffer = Framebuffer->GetFramebuffer();
	renderPassInfo.renderArea = Framebuffer->GetRenderArea();
	
	std::vector<VkClearValue> clearValues;
	{
		VkClearValue ClearValue;
		ClearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues.push_back(ClearValue);
	}
	if (Layout->HashDS)
	{
		VkClearValue ClearValue;
		ClearValue.depthStencil = { 1.0f, 0 };
		clearValues.push_back(ClearValue);
	}

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(CommandBufferHandle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	if (CurrentDescriptorPoolSetContainer == nullptr)
	{
		AcquirePoolSetContainer();
	}
}

void XVulkanCmdBuffer::AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, XSemaphore* InWaitSemaphore)
{
	WaitFlags.push_back(InWaitFlags);
	WaitSemaphores.push_back(InWaitSemaphore);
}

bool XVulkanCmdBuffer::AcquirePoolSetAndDescriptorsIfNeeded(const XVulkanDescriptorSetsLayout* Layout, bool bNeedDescriptors, VkDescriptorSet* OutDescriptors)
{
	if (CurrentDescriptorPoolSetContainer == nullptr)
	{
		AcquirePoolSetContainer();
	}

	uint32 Hash = Layout->GetHash();
	auto iter = TypedDescriptorPoolSets.find(Hash);
	XVulkanTypedDescriptorPoolArray* FoundTypedArrray;
	if (iter == TypedDescriptorPoolSets.end())
	{
		FoundTypedArrray = CurrentDescriptorPoolSetContainer->AcquireTypedPoolArray(Layout);
		bNeedDescriptors = true;
	}
	else
	{
		FoundTypedArrray = iter->second;
	}

	if (bNeedDescriptors)
	{
		return FoundTypedArrray->AllocateDescriptorSets(Layout, OutDescriptors);
	}

	return true;
}

void XVulkanCmdBuffer::RefreshFenceStatus()
{
	if (State == EState::Submitted)
	{
		XFenceManager* FenceMgr = Fence->GetOwner();
		if (FenceMgr->IsFenceSignaled(Fence))
		{
			Fence->GetOwner()->ResetFence(Fence);
			State = EState::NeedReset;
		}
	}
}

void XVulkanCmdBuffer::Begin()
{
	if (State == EState::NeedReset)
	{
		vkResetCommandBuffer(CommandBufferHandle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	}
	State = EState::IsInsideBegin;

	VkCommandBufferBeginInfo CmdBufBeginInfo = {};
	CmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(CommandBufferHandle, &CmdBufBeginInfo);
}

void XVulkanCmdBuffer::End()
{
	VULKAN_VARIFY(vkEndCommandBuffer(GetHandle()));
	State = EState::HasEnded;
}

void XVulkanCmdBuffer::AcquirePoolSetContainer()
{
	CurrentDescriptorPoolSetContainer = &Device->GetDescriptorPoolsManager()->AcquirePoolSetContainer();
}

XVulkanCommandBufferPool::XVulkanCommandBufferPool(XVulkanDevice* InDevice, XVulkanCommandBufferManager* InVulkanCommandBufferManager)
	: Device(InDevice)
	, CmdBufferManager(InVulkanCommandBufferManager)

{
}

XVulkanCommandBufferPool::~XVulkanCommandBufferPool()
{
	for (auto iter = FreeCmdBuffers.begin(); iter != FreeCmdBuffers.end(); iter++)
	{
		delete* iter;
	}

	for (auto iter = CmdBuffers.begin(); iter != CmdBuffers.end(); iter++)
	{
		delete* iter;
	}
}

void XVulkanCommandBufferPool::Create(uint32 QueueFamilyIndexdex)
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = QueueFamilyIndexdex;

	VULKAN_VARIFY(vkCreateCommandPool(Device->GetVkDevice(), &poolInfo, nullptr, &CmdPool));
}

XVulkanCmdBuffer* XVulkanCommandBufferPool::CreateCmdBuffer()
{
	for (auto iter = FreeCmdBuffers.begin(); iter != FreeCmdBuffers.end(); iter++)
	{
		XVulkanCmdBuffer* CmdBuffer = *iter;
		FreeCmdBuffers.erase(iter);
		CmdBuffers.push_back(CmdBuffer);
		return CmdBuffer;
	}

	XVulkanCmdBuffer* CmdBuffer = new XVulkanCmdBuffer(Device, this);
	CmdBuffers.push_back(CmdBuffer);
	return CmdBuffer;
}

VkCommandPool XVulkanCommandBufferPool::GetVkPool()
{
	return CmdPool;
}

void XVulkanCommandBufferPool::RefreshFenceStatus(XVulkanCmdBuffer* SkipCmdBuffer)
{
	for (int32 Index = 0; Index < CmdBuffers.size(); ++Index)
	{
		XVulkanCmdBuffer* CmdBuffer = CmdBuffers[Index];
		if (CmdBuffer != SkipCmdBuffer)
		{
			CmdBuffer->RefreshFenceStatus();
		}
	}
}

XVulkanCommandBufferManager::XVulkanCommandBufferManager(XVulkanDevice* InDevice, XVulkanCommandListContext* InContext)
	: Device(InDevice)
	, Context(InContext)
	, Pool(InDevice, this)
	, Queue(InContext->GetQueue())
	, UploadCmdBuffer(nullptr)
{

	Pool.Create(Queue->GetFamilyIndex());
	ActiveCmdBuffer = Pool.CreateCmdBuffer();
}

XVulkanCmdBuffer* XVulkanCommandBufferManager::GetUploadCmdBuffer()
{
	if (!UploadCmdBuffer)
	{
		for (int32 Index = 0; Index < Pool.CmdBuffers.size(); ++Index)
		{
			XVulkanCmdBuffer* CmdBuffer = Pool.CmdBuffers[Index];
			CmdBuffer->RefreshFenceStatus();
			if (CmdBuffer->State == XVulkanCmdBuffer::EState::ReadyForBegin || CmdBuffer->State == XVulkanCmdBuffer::EState::NeedReset)
			{
				UploadCmdBuffer = CmdBuffer;
				UploadCmdBuffer->Begin();
				return UploadCmdBuffer;
			}
		}
		UploadCmdBuffer = Pool.CreateCmdBuffer();
		UploadCmdBuffer->Begin();
	}
	return UploadCmdBuffer;
}

void XVulkanCommandBufferManager::SubmitUploadCmdBuffer(uint32 NumSignalSemaphores, VkSemaphore* SignalSemaphores)
{
	if (!UploadCmdBuffer->IsSubmitted() && UploadCmdBuffer->HasBegun())
	{
		UploadCmdBuffer->End();
		Queue->Submit(UploadCmdBuffer, NumSignalSemaphores, SignalSemaphores);
	}
	UploadCmdBuffer = nullptr;
}

void XVulkanCommandBufferManager::PrepareForNewActiveCommandBuffer()
{
	for (int32 Index = 0; Index < Pool.CmdBuffers.size(); ++Index)
	{
		XVulkanCmdBuffer* CmdBuffer = Pool.CmdBuffers[Index];
		CmdBuffer->RefreshFenceStatus();

		{
			if (CmdBuffer->State == XVulkanCmdBuffer::EState::ReadyForBegin || CmdBuffer->State == XVulkanCmdBuffer::EState::NeedReset)
			{
				ActiveCmdBuffer = CmdBuffer;
				ActiveCmdBuffer->Begin();
				return;
			}
			else
			{
				XASSERT(CmdBuffer->State == XVulkanCmdBuffer::EState::Submitted);
			}
		}
	}

	ActiveCmdBuffer = Pool.CreateCmdBuffer();
	ActiveCmdBuffer->Begin();
}

void XVulkanCommandBufferManager::SubmitActiveCmdBuffer(std::vector<XSemaphore*> SignalSemaphores)
{
	std::vector<VkSemaphore> SemaphoreHandles;
	SemaphoreHandles.reserve(SignalSemaphores.size() + 1);
	for (XSemaphore* Semaphore : SignalSemaphores)
	{
		SemaphoreHandles.push_back(Semaphore->GetHandle());
	}

	if (!ActiveCmdBuffer->IsSubmitted() && ActiveCmdBuffer->HasBegun())
	{
		if (!ActiveCmdBuffer->IsOutsideRenderPass())
		{
			ActiveCmdBuffer->EndRenderPass();
		}

		ActiveCmdBuffer->End();
		Queue->Submit(ActiveCmdBuffer, SemaphoreHandles.size(), SemaphoreHandles.data());
	}

	ActiveCmdBuffer = nullptr;
	if (ActiveCmdBufferSemaphore != nullptr)
	{
		delete ActiveCmdBufferSemaphore;
		ActiveCmdBufferSemaphore = nullptr;
	}
}

void XVulkanCommandBufferManager::SubmitActiveCmdBufferFromPresent(XSemaphore* SignalSemaphore)
{
	if (SignalSemaphore)
	{
		Queue->Submit(ActiveCmdBuffer, SignalSemaphore->GetHandle());
	}
	else
	{
		Queue->Submit(ActiveCmdBuffer, 0, 0);
	}
}


