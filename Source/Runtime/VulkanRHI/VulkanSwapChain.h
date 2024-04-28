#pragma once
#include "vulkan\vulkan_core.h"
#include "VulkanMemory.h"
#include <vector>
//class XSemaphore;
class XVulkanDevice;
class XVulkanQueue;
class XVulkanSwapChain
{
public:
	XVulkanSwapChain(EPixelFormat& InOutPixelFormat ,XVulkanDevice* VulkanDevice, void* InWindowHandle, VkInstance InInstance , std::vector<VkImage>& OutImages , VkExtent2D& SwapChainExtent);
	~XVulkanSwapChain();
	void Present(XVulkanQueue* GfxQueue, XVulkanQueue* PresentQueue, XSemaphore* BackBufferRenderingDoneSemaphore);

protected:
	uint32 AcquireImageIndex(XSemaphore*& OutSemaphore);

	int32 SemaphoreIndex;
	uint32 CurrentImageIndex;
	std::vector<XSemaphore*> ImageAvailableSemaphore;

	std::vector<XFence*> ImageAcquiredFences;

	friend class  VkHack;

	XVulkanDevice* Device;
	void* WindowHandle; 
	VkInstance Instance;
	VkSurfaceKHR Surface;

	VkSwapchainKHR swapChain;

	friend class XVulkanViewport;
};