#pragma once
#include "vulkan\vulkan_core.h"
#include "Runtime\Core\PixelFormat.h"
#include <vector>
#include "VulkanResource.h"
#include "VulkanCommandBuffer.h"
class XVulkanDevice;
class XVulkanSwapChain;

class XVulkanViewport
{
public:
	XVulkanViewport(EPixelFormat& InOutPixelFormat, XVulkanDevice* VulkanDevice, uint32 InSizeX, uint32 InSizeY, void* InWindowHandle, VkInstance InInstance);
	~XVulkanViewport();
	void Prsent(XVulkanCommandListContext* Context, XVulkanCmdBuffer* CmdBuffer, XVulkanQueue* Queue, XVulkanQueue* PresentQueue);
	inline XVulkanTexture2D* GetCurrentBackTexture() { return BackBufferTextures[CurrentBackBuffer].get(); }
private:

	void* WindowHandle;
	VkInstance Instance;
	XVulkanDevice* Device;
	XVulkanSwapChain* VulkanSwapChain;
	
	uint32 CurrentBackBuffer;
	VkExtent2D SwapChainExtent;

	std::vector<std::shared_ptr<XVulkanTexture2D>>BackBufferTextures;
	std::vector<XVulkanTextureView> ImageViews;//Unused
	std::vector<VkImage> swapChainImages;

	std::vector<XSemaphore*> RenderingDoneSemaphores;

	XSemaphore* AcquiredSemaphore;

	uint32 AcquiredImageIndex;

	friend class VkHack;
};