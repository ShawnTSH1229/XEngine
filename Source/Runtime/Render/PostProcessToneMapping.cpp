#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"
#include "Runtime/RenderCore/CommonRenderRresource.h"


class XToneMappingPassPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XToneMappingPassPS(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XToneMappingPassPS(const XShaderInitlizer& Initializer)
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
XToneMappingPassPS::ShaderInfos XToneMappingPassPS::StaticShaderInfos(
	"XToneMappingPassPS", GET_SHADER_PATH("ToneMapping.hlsl"),
	"ToneMapping_PS", EShaderType::SV_Pixel, XToneMappingPassPS::CustomConstrucFunc,
	XToneMappingPassPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::PostProcessToneMapping(XRHICommandList& RHICmdList, XRHITexture* TextureSceneColorSrc, XRHITexture* TextureSceneColorDest)
{
	XRHIRenderPassInfo RPInfos(1, &TextureSceneColorDest, ERenderTargetLoadAction::ELoad, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RPInfos, "PostProcess_ToneMapping", sizeof("PostProcess_ToneMapping"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<RFullScreenQuadVS> VertexShader = GetGlobalShaderMapping()->GetShader<RFullScreenQuadVS>();
	TShaderReference<XToneMappingPassPS> PixelShader = GetGlobalShaderMapping()->GetShader<XToneMappingPassPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GFullScreenLayout.RHIVertexLayout.get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);
	PixelShader->SetParameter(RHICmdList, TextureSceneColorSrc);

	RHICmdList.SetVertexBuffer(GFullScreenVertexRHI.RHIVertexBuffer.get(), 0, 0);
	RHICmdList.RHIDrawIndexedPrimitive(GFullScreenIndexRHI.RHIIndexBuffer.get(), 6, 1, 0, 0, 0);
	RHICmdList.RHIEndRenderPass();
}