#pragma once
#include <vulkan\vulkan_core.h>
#include "Runtime\RHI\RHI.h"
#include "Runtime\RHI\RHIResource.h"
class XVulkanDevice;
class XVulkanSamplerState : public XRHISamplerState
{
public:
	XVulkanSamplerState(XVulkanDevice* InDevice, const XSamplerStateInitializerRHI& InInitializer);

	VkSampler Sampler;
};

class XVulkanBlendState : public XRHIBlendState
{
public:
	XVulkanBlendState(const XBlendStateInitializerRHI& InInitializer);
	VkPipelineColorBlendAttachmentState BlendStates[MaxSimultaneousRenderTargets];
};

class XVulkanDepthStencilState : public XRHIDepthStencilState
{
public:
	XVulkanDepthStencilState(const XDepthStencilStateInitializerRHI& InInitializer);
	VkPipelineDepthStencilStateCreateInfo DepthStencilState;
};

class XVulkanRasterizerState : public XRHIRasterizationState
{
public:
	XVulkanRasterizerState(const XRasterizationStateInitializerRHI& InInitializer);
	VkPipelineRasterizationStateCreateInfo RasterizerState;
};