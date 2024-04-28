#pragma once
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/RHICommandList.h"
struct XSceneRenderTarget
{
	std::shared_ptr<XRHITexture2D> TextureGBufferA;
	std::shared_ptr<XRHITexture2D> TextureGBufferB;
	std::shared_ptr<XRHITexture2D> TextureGBufferC;
	std::shared_ptr<XRHITexture2D> TextureGBufferD;

	std::shared_ptr<XRHITexture2D> PhysicalShadowDepthTexture;
	std::shared_ptr<XRHITexture2D> PagetableInfos;
	std::shared_ptr<XRHITexture2D> VSMShadowMaskTexture;

	std::shared_ptr<XRHITexture2D> TextureDepthStencil;
	std::shared_ptr<XRHITexture2D> FurthestHZBOutput;

	std::shared_ptr<XRHITexture2D>TextureSceneColorDeffered;
	std::shared_ptr<XRHITexture2D>TextureSceneColorDefferedPingPong;

	std::shared_ptr<XRHITexture2D> TempShadowDepthTexture;
};