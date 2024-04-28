#include "Runtime\HAL\Mch.h"
#include "VulkanState.h"
#include "VulkanPlatformRHI.h"
#include "VulkanDevice.h"

static inline VkBlendOp BlendOpToVulkan(EBlendOperation InOp)
{
	switch (InOp)
	{
	case EBlendOperation::BO_Add:				return VK_BLEND_OP_ADD;
	default:
		XASSERT(false);
		break;
	}
	
	return VK_BLEND_OP_MAX_ENUM;
}

static inline VkBlendFactor BlendFactorToVulkan(EBlendFactor InFactor)
{
	switch (InFactor)
	{
	case EBlendFactor::BF_Zero:						return VK_BLEND_FACTOR_ZERO;
	case EBlendFactor::BF_One:						return VK_BLEND_FACTOR_ONE;
	case EBlendFactor::BF_SourceAlpha:				return VK_BLEND_FACTOR_SRC_ALPHA;
	case EBlendFactor::BF_InverseSourceAlpha:			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	default:
		XASSERT(false);
		break;
	}
	return VK_BLEND_FACTOR_MAX_ENUM;
}

XVulkanBlendState::XVulkanBlendState(const XBlendStateInitializerRHI& InInitializer)
{
	for (uint32 Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
	{
		const XBlendStateInitializerRHI::XRenderTarget& ColorTarget = InInitializer.RenderTargets[Index];
		VkPipelineColorBlendAttachmentState& BlendState = BlendStates[Index];
		memset(&BlendState, 0, sizeof(VkPipelineColorBlendAttachmentState));

		BlendState.colorBlendOp = BlendOpToVulkan(ColorTarget.RTColorBlendOp);
		BlendState.alphaBlendOp = BlendOpToVulkan(ColorTarget.RTAlphaBlendOp);

		BlendState.dstColorBlendFactor = BlendFactorToVulkan(ColorTarget.RTColorDestBlend);
		BlendState.dstAlphaBlendFactor = BlendFactorToVulkan(ColorTarget.RTAlphaDestBlend);

		BlendState.srcColorBlendFactor = BlendFactorToVulkan(ColorTarget.RTColorSrcBlend);
		BlendState.srcAlphaBlendFactor = BlendFactorToVulkan(ColorTarget.RTAlphaSrcBlend);

		BlendState.blendEnable =
			(ColorTarget.RTColorBlendOp != EBlendOperation::BO_Add || ColorTarget.RTColorDestBlend != EBlendFactor::BF_Zero || ColorTarget.RTColorSrcBlend != EBlendFactor::BF_One ||
				ColorTarget.RTAlphaBlendOp != EBlendOperation::BO_Add || ColorTarget.RTAlphaDestBlend != EBlendFactor::BF_Zero || ColorTarget.RTAlphaSrcBlend != EBlendFactor::BF_One) ? VK_TRUE : VK_FALSE;

		BlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
}

static inline VkCompareOp CompareOpToVulkan(ECompareFunction InOp)
{
	switch (InOp)
	{
	case ECompareFunction::CF_Greater:		return VK_COMPARE_OP_GREATER;
	case ECompareFunction::CF_Less:			return VK_COMPARE_OP_LESS;
	case ECompareFunction::CF_GreaterEqual:	return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case ECompareFunction::CF_Always:		return VK_COMPARE_OP_ALWAYS;
	default:
		XASSERT(false);
		break;
	}
	return VK_COMPARE_OP_MAX_ENUM;
}

XVulkanDepthStencilState::XVulkanDepthStencilState(const XDepthStencilStateInitializerRHI& InInitializer)
{
	DepthStencilState = {};
	DepthStencilState.depthWriteEnable = InInitializer.bEnableDepthWrite ? VK_TRUE : VK_FALSE;
	DepthStencilState.depthCompareOp = CompareOpToVulkan(InInitializer.DepthCompFunc);
	DepthStencilState.depthTestEnable = (InInitializer.DepthCompFunc != ECompareFunction::CF_Always || InInitializer.bEnableDepthWrite) ? VK_TRUE : VK_FALSE;
}

static inline VkCullModeFlags RasterizerCullModeToVulkan(EFaceCullMode InCullMode)
{
	switch (InCullMode)
	{
	case EFaceCullMode::FC_None:	return VK_CULL_MODE_NONE;
	case EFaceCullMode::FC_Front:	return VK_CULL_MODE_FRONT_BIT;
	case EFaceCullMode::FC_Back:	return VK_CULL_MODE_BACK_BIT;
	default:		
		XASSERT(false);
		break;
	}

	return VK_CULL_MODE_NONE;
}

XVulkanRasterizerState::XVulkanRasterizerState(const XRasterizationStateInitializerRHI& InInitializer)
{
	RasterizerState = {};
	RasterizerState.cullMode = RasterizerCullModeToVulkan(InInitializer.CullMode);

	RasterizerState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	RasterizerState.depthClampEnable = VK_FALSE;
	RasterizerState.rasterizerDiscardEnable = VK_FALSE;
	RasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
	RasterizerState.lineWidth = 1.0f;
	RasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	RasterizerState.depthBiasEnable = VK_FALSE;

	
}

static inline VkFilter FilterModeToVulkan(ESamplerFilter InFilterMode)
{
	switch (InFilterMode)
	{
	case ESamplerFilter::SF_Point:	return VK_FILTER_NEAREST;
	case ESamplerFilter::SF_Bilinear:	return VK_FILTER_LINEAR;
	default:
		XASSERT(false);
		break;
	}
}

XVulkanSamplerState::XVulkanSamplerState(XVulkanDevice* InDevice, const XSamplerStateInitializerRHI& InInitializer)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = FilterModeToVulkan(InInitializer.Filter);
	samplerInfo.minFilter = FilterModeToVulkan(InInitializer.Filter);
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VULKAN_VARIFY(vkCreateSampler(InDevice->GetVkDevice(), &samplerInfo, nullptr, &Sampler));
}

std::shared_ptr<XRHISamplerState> XVulkanPlatformRHI::RHICreateSamplerState(const XSamplerStateInitializerRHI& Initializer)
{
	return std::make_shared<XVulkanSamplerState>(Device, Initializer);
}

std::shared_ptr<XRHIRasterizationState> XVulkanPlatformRHI::RHICreateRasterizationStateState(const XRasterizationStateInitializerRHI& Initializer)
{
	return std::make_shared<XVulkanRasterizerState>(Initializer);
}

std::shared_ptr<XRHIDepthStencilState> XVulkanPlatformRHI::RHICreateDepthStencilState(const XDepthStencilStateInitializerRHI& Initializer)
{
	return std::make_shared<XVulkanDepthStencilState>(Initializer);
}

std::shared_ptr<XRHIBlendState> XVulkanPlatformRHI::RHICreateBlendState(const XBlendStateInitializerRHI& Initializer)
{
	return std::make_shared<XVulkanBlendState>(Initializer);
}


