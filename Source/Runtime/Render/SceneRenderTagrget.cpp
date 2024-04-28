#include "DeferredShadingRenderer.h"
#include "VirtualShadowMap.h"
#include "ShadowMap_Old.h"

void XDeferredShadingRenderer::SceneTagetGen()
{
	SceneTargets.TextureGBufferA = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R16G16B16A16_FLOAT
		, ETextureCreateFlags(TexCreate_RenderTargetable), 1
		, nullptr);

	SceneTargets.TextureGBufferB = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R16G16B16A16_FLOAT
		, ETextureCreateFlags(TexCreate_RenderTargetable), 1
		, nullptr);

	SceneTargets.TextureGBufferC = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R16G16B16A16_FLOAT
		, ETextureCreateFlags(TexCreate_RenderTargetable), 1
		, nullptr);

	SceneTargets.TextureGBufferD = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R8G8B8A8_UNORM
		, ETextureCreateFlags(TexCreate_RenderTargetable), 1
		, nullptr);

	SceneTargets.PhysicalShadowDepthTexture = RHICreateTexture2D(PhysicalShadowDepthTextureSize, PhysicalShadowDepthTextureSize, 1, false, false,
		EPixelFormat::FT_R32_UINT, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);

	SceneTargets.PagetableInfos = RHICreateTexture2D(VirtualTileWidthNum, VirtualTileWidthNum, 1, false, false, EPixelFormat::FT_R32G32B32A32_UINT
		, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);
	
	SceneTargets.VSMShadowMaskTexture = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R16_FLOAT
		, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1
		, nullptr);

	SceneTargets.TextureDepthStencil = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R24G8_TYPELESS, ETextureCreateFlags(TexCreate_DepthStencilTargetable | TexCreate_ShaderResource)
		, 1, nullptr);

	SceneTargets.TempShadowDepthTexture = RHICreateTexture2D(ShadowMapDepthTextureSize, ShadowMapDepthTextureSize, 1, false, false,
		EPixelFormat::FT_R24G8_TYPELESS, ETextureCreateFlags(TexCreate_DepthStencilTargetable | TexCreate_ShaderResource)
		, 1, nullptr);

	SceneTargets.FurthestHZBOutput = RHICreateTexture2D(512, 512, 1, false, false, EPixelFormat::FT_R16_FLOAT
		, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 5, nullptr);

	SceneTargets.TextureSceneColorDeffered = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R16G16B16A16_FLOAT
		, ETextureCreateFlags(TexCreate_RenderTargetable), 1
		, nullptr);

	SceneTargets.TextureSceneColorDefferedPingPong = RHICreateTexture2D(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1, false, false,
		EPixelFormat::FT_R16G16B16A16_FLOAT, ETextureCreateFlags(TexCreate_RenderTargetable), 1, nullptr);
}

