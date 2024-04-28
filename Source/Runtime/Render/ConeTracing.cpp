#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"
#include "Runtime/RenderCore/CommonRenderRresource.h"

#include "VoxelizationScene.h"

#if USE_SVOGI

class XConeTracingPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XConeTracingPS(Initializer);
	}

	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XConeTracingPS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{
		cbView.Bind(Initializer.ShaderParameterMap, "cbView");
		cbVoxelization.Bind(Initializer.ShaderParameterMap, "cbVoxelization");
		NormalTexture.Bind(Initializer.ShaderParameterMap, "NormalTexture");
		SceneDepth.Bind(Initializer.ShaderParameterMap, "SceneDepth");

		SpaseVoxelOctreeR.Bind(Initializer.ShaderParameterMap, "SpaseVoxelOctreeR");
		IrradianceBrickBufferROnly.Bind(Initializer.ShaderParameterMap, "IrradianceBrickBufferROnly");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* cbVoxelizationIn,
		XRHIConstantBuffer* cbViewIn,
		XRHITexture* SceneDepthIn,
		XRHITexture* NormalTextureIn,
		XRHIShaderResourceView* SpaseVoxelOctreeRIn,
		XRHIShaderResourceView* IrradianceBrickBufferROnlyIn)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, cbVoxelization, cbVoxelizationIn);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, cbView, cbViewIn);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, SceneDepth, SceneDepthIn);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, NormalTexture, NormalTextureIn);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Pixel, SpaseVoxelOctreeR, SpaseVoxelOctreeRIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Pixel, IrradianceBrickBufferROnly, IrradianceBrickBufferROnlyIn);
	}

	SRVParameterType SpaseVoxelOctreeR;
	SRVParameterType IrradianceBrickBufferROnly;
	
	SRVParameterType SceneDepth;
	SRVParameterType NormalTexture;
	CBVParameterType cbVoxelization;
	CBVParameterType cbView;
};
XConeTracingPS::ShaderInfos XConeTracingPS::StaticShaderInfos(
	"XConeTracingPS", GET_SHADER_PATH("SVOConeTracing.hlsl"),
	"XConeTracingPS", EShaderType::SV_Pixel, XConeTracingPS::CustomConstrucFunc,
	XConeTracingPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::ConeTracingPass(XRHICommandList& RHICmdList, XRHITexture* TextureSceneColorDest)
{
	XRHIRenderPassInfo RPInfos(1, &TextureSceneColorDest, ERenderTargetLoadAction::ELoad, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RPInfos, "XConeTracingPS", sizeof("XConeTracingPS"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);


	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<true, EBlendOperation::BO_Add, EBlendFactor::BF_One, EBlendFactor::BF_One>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<RFullScreenQuadVS> VertexShader = GetGlobalShaderMapping()->GetShader<RFullScreenQuadVS>();
	TShaderReference<XConeTracingPS> PixelShader = GetGlobalShaderMapping()->GetShader<XConeTracingPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GFullScreenLayout.RHIVertexLayout.get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);
	PixelShader->SetParameter(RHICmdList, 
		SVOGIResourece.VoxelScenePSCBuffer.get(),
		RViewInfo.ViewConstantBuffer.get(), 
		SceneTargets.TextureDepthStencil.get(), 
		SceneTargets.TextureGBufferA.get(),
		GetRHISRVFromTexture(SVOGIResourece.SpaseVoxelOctree.get()),
		GetRHISRVFromTexture(SVOGIResourece.BrickBufferPingPong[SVOGIResourece.PingPongIndex % 2])
		);
	SVOGIResourece.PingPongIndex++;

	RHICmdList.SetVertexBuffer(GFullScreenVertexRHI.RHIVertexBuffer.get(), 0, 0);
	RHICmdList.RHIDrawIndexedPrimitive(GFullScreenIndexRHI.RHIIndexBuffer.get(), 6, 1, 0, 0, 0);
	RHICmdList.RHIEndRenderPass();
}

#endif