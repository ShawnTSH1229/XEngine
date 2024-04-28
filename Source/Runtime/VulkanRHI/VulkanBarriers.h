#pragma once
#include <map>
class XVulkanRenderPass;
class XVulkanDevice;
class XVulkanRenderTargetLayout;
class XVulkanCmdBuffer;
class XVulkanCommandListContext;
class XVulkanTextureView;
class XVulkanSurface;
class XVulkanFramebuffer
{
public:
	XVulkanFramebuffer(XVulkanDevice* Device, const XRHISetRenderTargetsInfo* InRTInfo, const XVulkanRenderTargetLayout* RTLayout, const XVulkanRenderPass* RenderPass);
	bool Matches(const XRHISetRenderTargetsInfo& RTInfo) const;
	const VkFramebuffer GetFramebuffer()const { return Framebuffer; }
	inline VkRect2D GetRenderArea() const { return RenderArea; }
private:
	VkFramebuffer Framebuffer;
	VkRect2D RenderArea;

	uint32 NumColorRenderTargets;

	uint32 NumColorAttachments;
	VkImage ColorRenderTargetImages[MaxSimultaneousRenderTargets];
	VkImage DepthStencilRenderTargetImage;

	std::vector<VkImageView>AttachmentViews;
	std::vector<XVulkanTextureView> VulkanTextureViews;
};

struct XVulkanImageLayout
{
	XVulkanImageLayout() {}
	XVulkanImageLayout(VkImageLayout MainLayoutIn) :MainLayout(MainLayoutIn) {}
	
	VkImageLayout MainLayout;
	std::vector<VkImageLayout>SubresLayouts;
	bool AreAllSubresourcesSameLayout() const
	{
		return SubresLayouts.size() == 0;
	}
	void Set(VkImageLayout Layout);
};

class XVulkanLayoutManager
{
public:
	XVulkanLayoutManager() 
	{

	}

	void SetGfxContex(XVulkanCommandListContext* GfxCtx)
	{
		GfxContex = GfxCtx;
	}

	~XVulkanLayoutManager();

	XVulkanFramebuffer* GetOrCreateFramebuffer(
		XVulkanDevice* InDevice,
		const XRHISetRenderTargetsInfo* RenderTargetsInfo, 
		const XVulkanRenderTargetLayout* RTLayout, 
		XVulkanRenderPass* RenderPass);

	void BeginRenderPass(XVulkanCmdBuffer* CmdBuffer, const XVulkanRenderTargetLayout* RTLayout, XVulkanRenderPass* RenderPass, XVulkanFramebuffer* Framebuffer);
	void EndRenderPass(XVulkanCmdBuffer* CmdBuffer);

	XVulkanRenderPass* GetOrCreateRenderPass(XVulkanDevice* InDevice, const XVulkanRenderTargetLayout* RTLayout);

	XVulkanImageLayout& GetOrAddFullLayout(const XVulkanSurface* Surface, VkImageLayout LayoutIfNotFound);
	XVulkanImageLayout& GetFullLayout(const XVulkanSurface* Surface);

	VkImageLayout FindLayout(VkImage Image) const
	{
		const XVulkanImageLayout& Layout = Layouts.find(Image)->second;
		return Layout.MainLayout;
	}
private:
	friend class VkHack;

	XVulkanCommandListContext* GfxContex;

	std::map<VkImage, XVulkanImageLayout> Layouts;
	struct XFramebufferList
	{
		std::vector<XVulkanFramebuffer*> Framebuffer;
	};
	std::map<uint32, XVulkanRenderPass*>RenderPasses;
	std::map<uint32, XFramebufferList*> Framebuffers;
};

struct XVulkanPipelineBarrier
{
	XVulkanPipelineBarrier() : SrcStageMask(0), DstStageMask(0),bHasMemoryBarrier(false)
	{
		mMemoryBarrier = {};
		mMemoryBarrier.sType= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	}
	void AddImageLayoutTransition(VkImage Image, VkImageAspectFlags AspectMask, const struct XVulkanImageLayout& SrcLayout, const struct XVulkanImageLayout& DstLayout);
	void AddImageLayoutTransition(VkImage Image, VkImageAspectFlags AspectMask, VkImageLayout SrcLayout, const XVulkanImageLayout& DstLayout);
	void AddImageLayoutTransition(VkImage Image, VkImageAspectFlags AspectMask, const XVulkanImageLayout& SrcLayout, VkImageLayout DstLayout);
	void AddImageLayoutTransition(VkImage Image, VkImageLayout SrcLayout, VkImageLayout DstLayout, const VkImageSubresourceRange& SubresourceRange);

	static VkImageSubresourceRange MakeSubresourceRange(VkImageAspectFlags AspectMask, uint32 FirstMip = 0, uint32 NumMips = VK_REMAINING_MIP_LEVELS, uint32 FirstLayer = 0, uint32 NumLayers = VK_REMAINING_ARRAY_LAYERS);

	bool bHasMemoryBarrier;
	std::vector<VkImageMemoryBarrier>ImageBarriers;

	VkMemoryBarrier mMemoryBarrier;
	VkPipelineStageFlags SrcStageMask, DstStageMask;

	void Execute(VkCommandBuffer CmdBuffer);
};