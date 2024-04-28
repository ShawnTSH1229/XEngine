#pragma once
#include <Runtime\HAL\PlatformTypes.h>
#include "VulkanContext.h"
#include "VulkanMemory.h"

class XVulkanCommandBufferPool;
class XVulkanDescriptorPoolSetContainer;

class XVulkanCmdBuffer
{
public:
	XVulkanCmdBuffer(XVulkanDevice* InDevice, XVulkanCommandBufferPool* InCommandBufferPool);
	void AllocMemory();
	void BeginRenderPass(const XVulkanRenderTargetLayout* Layout,XVulkanRenderPass* RenderPass, XVulkanFramebuffer* Framebuffer);
	VkCommandBuffer GetHandle() { return CommandBufferHandle; }
	VkCommandBuffer* GetHandlePtr() { return &CommandBufferHandle; }
	void AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, XSemaphore* InWaitSemaphore);
	bool AcquirePoolSetAndDescriptorsIfNeeded(const class XVulkanDescriptorSetsLayout* Layout, bool bNeedDescriptors, VkDescriptorSet* OutDescriptors);
	void RefreshFenceStatus();
	void EndRenderPass()
	{
		vkCmdEndRenderPass(CommandBufferHandle);
		State = EState::IsInsideBegin;
	}

	void Begin();
	void End();

	enum class EState : uint8
	{
		ReadyForBegin,
		IsInsideBegin,
		IsInsideRenderPass,
		HasEnded,
		Submitted,
		NotAllocated,
		NeedReset,
	};

	inline bool IsOutsideRenderPass() const
	{
		return State == EState::IsInsideBegin;
	}

	inline bool IsSubmitted() const
	{
		return State == EState::Submitted;
	}

	inline bool HasBegun() const
	{
		return State == EState::IsInsideBegin || State == EState::IsInsideRenderPass;
	}

	std::vector<XSemaphore*> WaitSemaphores;
	std::vector<VkPipelineStageFlags> WaitFlags;
	XFence* Fence;
	EState State;

	void MarkSemaphoresAsSubmitted()
	{
		WaitFlags.clear();
		WaitSemaphores.clear();
	}
	XVulkanCommandBufferPool* GetOwner()
	{
		return CmdBufferPool;
	}

private:
	void AcquirePoolSetContainer();


	friend class VkHack;
	VkCommandBuffer CommandBufferHandle;
	XVulkanDevice* Device;
	XVulkanCommandBufferPool* CmdBufferPool;

	XVulkanDescriptorPoolSetContainer* CurrentDescriptorPoolSetContainer = nullptr;

	std::map<uint32, class XVulkanTypedDescriptorPoolArray*> TypedDescriptorPoolSets;
};

class XVulkanCommandBufferPool
{
public:
	XVulkanCommandBufferPool(XVulkanDevice* InDevice, XVulkanCommandBufferManager* InVulkanCommandBufferManager);
	~XVulkanCommandBufferPool();
	void Create(uint32 InQueueFamilyIndexdex);
	XVulkanCmdBuffer* CreateCmdBuffer();
	VkCommandPool GetVkPool();
	void RefreshFenceStatus(XVulkanCmdBuffer* SkipCmdBuffer = nullptr);
private:
	friend class VkHack;
	friend class XVulkanCommandBufferManager;

	std::vector<XVulkanCmdBuffer*> CmdBuffers;
	std::vector<XVulkanCmdBuffer*> FreeCmdBuffers;

	XVulkanDevice* Device;
	XVulkanCommandBufferManager* CmdBufferManager;
	VkCommandPool CmdPool;;
};
class XVulkanCommandBufferManager
{
public:
	XVulkanCommandBufferManager(XVulkanDevice* InDevice, XVulkanCommandListContext* InContext);
	XVulkanCmdBuffer* GetActiveCmdBuffer() { return ActiveCmdBuffer; }
	XVulkanCmdBuffer* GetUploadCmdBuffer();
	void SubmitUploadCmdBuffer(uint32 NumSignalSemaphores = 0, VkSemaphore* SignalSemaphores = nullptr);
	
	//must perform PrepareForNewActiveCommandBuffer after SubmitActiveCmdBuffer
	void SubmitActiveCmdBuffer(std::vector<XSemaphore*> SignalSemaphores);
	void SubmitActiveCmdBuffer()
	{
		std::vector<XSemaphore*> Vec;
		SubmitActiveCmdBuffer(Vec);
	}
	
	void PrepareForNewActiveCommandBuffer();

	void SubmitActiveCmdBufferFromPresent(XSemaphore* SignalSemaphore = nullptr);
private:
	friend class VkHack;

	XVulkanDevice* Device;
	XVulkanCommandListContext* Context;
	XVulkanCommandBufferPool Pool;
	XVulkanQueue* Queue;
	XVulkanCmdBuffer* ActiveCmdBuffer;
	XVulkanCmdBuffer* UploadCmdBuffer;

	/** This semaphore is used to prevent overlaps between the (current) graphics cmdbuf and next upload cmdbuf. */
	XSemaphore* ActiveCmdBufferSemaphore;
};