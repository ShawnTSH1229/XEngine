#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RenderCore/CommonRenderRresource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

class XFinalPassPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XFinalPassPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XFinalPassPS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{
		FullScreenMap.Bind(Initializer.ShaderParameterMap, "FullScreenMap");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHITexture* InTexture)
	{
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, FullScreenMap, InTexture);
	}
	TextureParameterType    FullScreenMap;
};
XFinalPassPS::ShaderInfos XFinalPassPS::StaticShaderInfos(
	"XFinalPassPS", GET_SHADER_PATH("FinalPassShader.hlsl"),
	"PS", EShaderType::SV_Pixel, XFinalPassPS::CustomConstrucFunc,
	XFinalPassPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::PresentPass(XRHICommandList& RHICmdList , XRHITexture* TexSrc)
{
	XRHITexture* BackTex = RHIGetCurrentBackTexture();
	XRHIRenderPassInfo RPInfos(1, &BackTex, ERenderTargetLoadAction::EClear, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RPInfos, "FinalPass", sizeof("FinalPass"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();;
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<RFullScreenQuadVS> SSRVertexShader = GetGlobalShaderMapping()->GetShader<RFullScreenQuadVS>();
	TShaderReference<XFinalPassPS> SSRPixelShader = GetGlobalShaderMapping()->GetShader<XFinalPassPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = SSRVertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = SSRPixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GFullScreenLayout.RHIVertexLayout.get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);
	SSRPixelShader->SetParameter(RHICmdList, TexSrc);

	RHICmdList.SetVertexBuffer(GFullScreenVertexRHI.RHIVertexBuffer.get(), 0, 0);
	RHICmdList.RHIDrawIndexedPrimitive(GFullScreenIndexRHI.RHIIndexBuffer.get(), 6, 1, 0, 0, 0);
	RHICmdList.RHIEndRenderPass();
}