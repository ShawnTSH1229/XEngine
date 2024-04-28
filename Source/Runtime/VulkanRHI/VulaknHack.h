#pragma once
#include <vulkan\vulkan_core.h>
#include <vector>
class XVulkanTextureView;
class VkHack
{
public:
	VkDevice GetVkDevice();
	VkPhysicalDevice GetVkPhyDevice();
	VkExtent2D GetBkBufferExtent();
	std::vector<VkImageView>& GetBkImageViews();
	uint32 GetgraphicsFamilyIndex();
	VkQueue GetVkQueue();
	VkSwapchainKHR GetVkSwapChain();
	VkCommandPool GetCmdPool();
	const VkCommandBuffer* GetCmdBuffer();
	VkRenderPass GetVkRenderPas();
	void TempPresent();
	VkPipeline GetVkPipeline();
	VkPipelineLayout GetVkPipelineLayout();
	VkDescriptorSetLayout GetVkDescriptorSetLayout();

	VkImageView GetTexView(XRHITexture2D* Tex);

	static bool TempMakeDirty()
	{
		return true;
	}
};
;
