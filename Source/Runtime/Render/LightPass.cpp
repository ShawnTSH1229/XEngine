#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "Runtime/RenderCore/CommonRenderRresource.h"

//XLightPass
class XLightPassVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XLightPassVS(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XLightPassVS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{
		CBV_View.Bind(Initializer.ShaderParameterMap, "cbView");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* ViewCB)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, CBV_View, ViewCB);
	}
	CBVParameterType CBV_View;
};

class XLightPassPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XLightPassPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XLightPassPS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{
		CBV_View.Bind(Initializer.ShaderParameterMap, "cbView");
		CBV_DefferedLight.Bind(Initializer.ShaderParameterMap, "cbLightPass");

		GBufferATexture.Bind(Initializer.ShaderParameterMap, "SceneTexturesStruct_GBufferATexture");
		GBufferBTexture.Bind(Initializer.ShaderParameterMap, "SceneTexturesStruct_GBufferBTexture");
		GBufferCTexture.Bind(Initializer.ShaderParameterMap, "SceneTexturesStruct_GBufferCTexture");
		GBufferDTexture.Bind(Initializer.ShaderParameterMap, "SceneTexturesStruct_GBufferDTexture");
		SceneDepthTexture.Bind(Initializer.ShaderParameterMap, "SceneTexturesStruct_SceneDepthTexture");
		ShadowMakTex.Bind(Initializer.ShaderParameterMap, "ShadowMaskTex");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* InCBV_View,
		XRHIConstantBuffer* InCBV_DefferedLight,

		XRHITexture* InGBufferATexture,
		XRHITexture* InGBufferBTexture,
		XRHITexture* InGBufferCTexture,
		XRHITexture* InGBufferDTexture,
		XRHITexture* InSceneDepthTexture,
		XRHITexture* InLightAttenuationTexture)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, CBV_View, InCBV_View);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, CBV_DefferedLight, InCBV_DefferedLight);

		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, GBufferATexture, InGBufferATexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, GBufferBTexture, InGBufferBTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, GBufferCTexture, InGBufferCTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, GBufferDTexture, InGBufferDTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, SceneDepthTexture, InSceneDepthTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, ShadowMakTex, InLightAttenuationTexture);
	}

	CBVParameterType CBV_View;
	CBVParameterType CBV_DefferedLight;

	TextureParameterType GBufferATexture;
	TextureParameterType GBufferBTexture;
	TextureParameterType GBufferCTexture;
	TextureParameterType GBufferDTexture;
	TextureParameterType SceneDepthTexture;
	TextureParameterType ShadowMakTex;

};

XLightPassVS::ShaderInfos XLightPassVS::StaticShaderInfos(
	"XLightPassVS", GET_SHADER_PATH("DeferredLightVertexShaders.hlsl"),
	"DeferredLightVertexMain", EShaderType::SV_Vertex, XLightPassVS::CustomConstrucFunc,
	XLightPassVS::ModifyShaderCompileSettings);
XLightPassPS::ShaderInfos XLightPassPS::StaticShaderInfos(
	"XLightPassPS", GET_SHADER_PATH("DeferredLightPixelShaders.hlsl"),
	"DeferredLightPixelMain", EShaderType::SV_Pixel, XLightPassPS::CustomConstrucFunc,
	XLightPassPS::ModifyShaderCompileSettings);

struct cbDefferedLight
{
	XVector3 LightDir;
	float padding0 = 0;
	XVector4 LightColorAndIntensityInLux;
};

class XDefferedLightResourece :public XRenderResource
{
public:
	std::shared_ptr<XRHIConstantBuffer>DefferedLightConstantBuffer;

	void InitRHI()override
	{
		DefferedLightConstantBuffer = RHICreateConstantBuffer(sizeof(cbDefferedLight));
	}
	void ReleaseRHI()override
	{

	}
};
TGlobalResource<XDefferedLightResourece>DefferedLightResourece;

void XDeferredShadingRenderer::LightPass(XRHICommandList& RHICmdList)
{
	cbDefferedLight cbDefferedLightIns;
	cbDefferedLightIns.LightDir = ShadowLightDir;
	cbDefferedLightIns.LightColorAndIntensityInLux = XVector4(MainLightColor.x, MainLightColor.y, MainLightColor.z, LightIntensity);
	DefferedLightResourece.DefferedLightConstantBuffer->UpdateData(&cbDefferedLightIns,sizeof(cbDefferedLight),0);

	XRHITexture* SceneColorRTs = SceneTargets.TextureSceneColorDeffered.get();
	XRHIRenderPassInfo RPInfos(1, &SceneColorRTs, ERenderTargetLoadAction::EClear, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RPInfos, "LightPass", sizeof("LightPass"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();;
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<XLightPassVS> VertexShader = GetGlobalShaderMapping()->GetShader<XLightPassVS>();
	TShaderReference<XLightPassPS> PixelShader = GetGlobalShaderMapping()->GetShader<XLightPassPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GFullScreenLayout.RHIVertexLayout.get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameter(RHICmdList, RViewInfo.ViewConstantBuffer.get());
	PixelShader->SetParameter(RHICmdList, RViewInfo.ViewConstantBuffer.get(), DefferedLightResourece.DefferedLightConstantBuffer.get(),
		SceneTargets.TextureGBufferA.get(), SceneTargets.TextureGBufferB.get(), SceneTargets.TextureGBufferC.get(), SceneTargets.TextureGBufferD.get(),
		SceneTargets.TextureDepthStencil.get(), SceneTargets.VSMShadowMaskTexture.get());

	RHICmdList.SetVertexBuffer(GFullScreenVertexRHI.RHIVertexBuffer.get(), 0, 0);
	RHICmdList.RHIDrawIndexedPrimitive(GFullScreenIndexRHI.RHIIndexBuffer.get(), 6, 1, 0, 0, 0);
	RHICmdList.RHIEndRenderPass();
}