#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "ShadowMap_Old.h"

TGlobalResource<XShadowMapResourece_Old>ShadowMapResourece_Old;

class XShadowMapVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XShadowMapVS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XShadowMapVS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		cbPerObject.Bind(Initializer.ShaderParameterMap, "cbPerObject");
		LightProjectMatrix.Bind(Initializer.ShaderParameterMap, "LightProjectMatrix");
	}

	void SetcbPerObject(XRHICommandList& RHICommandList,XRHIConstantBuffer* cbPerObjectIn)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, cbPerObject, cbPerObjectIn);
	}
	void SetLightProjectMatrix(XRHICommandList& RHICommandList, XRHIConstantBuffer* LightProjectMatrixIn)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, LightProjectMatrix, LightProjectMatrixIn);
	}
	CBVParameterType cbPerObject;
	CBVParameterType LightProjectMatrix;

};
XShadowMapVS::ShaderInfos XShadowMapVS::StaticShaderInfos(
	"XShadowMapVS", GET_SHADER_PATH("ShadowProjection_Old.hlsl"),
	"VS", EShaderType::SV_Vertex, XShadowMapVS::CustomConstrucFunc,
	XShadowMapVS::ModifyShaderCompileSettings);


class XShadowMapPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XShadowMapPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XShadowMapPS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer) {}
};
XShadowMapPS::ShaderInfos XShadowMapPS::StaticShaderInfos(
	"XShadowMapPS", GET_SHADER_PATH("ShadowProjection_Old.hlsl"),
	"PS", EShaderType::SV_Pixel, XShadowMapPS::CustomConstrucFunc,
	XShadowMapPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::ShadowMapGenPass(XRHICommandList& RHICmdList)
{
	XBoundSphere TtempSceneBoundingSphere;
	TtempSceneBoundingSphere.Center = XVector3(0, 0, 0);
	TtempSceneBoundingSphere.Radius = 16.0f;
	float l = TtempSceneBoundingSphere.Center.x - TtempSceneBoundingSphere.Radius;
	float r = TtempSceneBoundingSphere.Center.x + TtempSceneBoundingSphere.Radius;
	float t = TtempSceneBoundingSphere.Center.y + TtempSceneBoundingSphere.Radius;
	float b = TtempSceneBoundingSphere.Center.y - TtempSceneBoundingSphere.Radius;
	float f = TtempSceneBoundingSphere.Radius * 2;
	float n = 0.1f;

	ShadowLightDir.Normalize();
	XVector3 LightPos = TtempSceneBoundingSphere.Center + ShadowLightDir * TtempSceneBoundingSphere.Radius * 1.0;
	XVector3 UpTemp(0, 1, 0);

	XMatrix TempLightViewMat = XMatrix::CreateMatrixLookAtLH(LightPos, TtempSceneBoundingSphere.Center, UpTemp);
	XMatrix TempLightProjMat = XMatrix::CreateOrthographicOffCenterLH(l, r, b, t, f, n);
	XMatrix TempLightViewProjMat = TempLightViewMat * TempLightProjMat;
	ShadowMapResourece_Old.ShadowViewProjectionCBStructIns.ShadowViewProjection = TempLightViewProjMat;
	ShadowMapResourece_Old.ShadowViewProjectionCBStructIns.ShadowViewProjectionInv = TempLightViewProjMat.Invert();
	ShadowMapResourece_Old.ShadowViewProjectionCB->UpdateData(&ShadowMapResourece_Old.ShadowViewProjectionCBStructIns, sizeof(ShadowViewProjectionCBStruct), 0);

	XRHIRenderPassInfo RPInfos(0, nullptr, ERenderTargetLoadAction::ENoAction, SceneTargets.TempShadowDepthTexture.get(), EDepthStencilLoadAction::EClear);
	RHICmdList.RHIBeginRenderPass(RPInfos, "XShadowMapPS", sizeof("XShadowMapPS"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, ECompareFunction::CF_Greater>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<XShadowMapVS> VertexShader = GetGlobalShaderMapping()->GetShader<XShadowMapVS>();
	TShaderReference<XShadowMapPS> PixelShader = GetGlobalShaderMapping()->GetShader<XShadowMapPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = DefaultVertexFactory.GetLayout(ELayoutType::Layout_Default).get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);

	VertexShader->SetLightProjectMatrix(RHICmdList,ShadowMapResourece_Old.ShadowViewProjectionCB.get());
	for (uint32 i = 0; i < RenderGeos.size(); i++)
	{
		std::shared_ptr<GGeomertry>& GeoInsPtr = RenderGeos[i];
		VertexShader->SetcbPerObject(RHICmdList,  GeoInsPtr->GetPerObjectVertexCBuffer().get());
		RHICmdList.SetVertexBuffer(GeoInsPtr->GetGVertexBuffer()->GetRHIVertexBuffer().get(), 0, 0);
		RHICmdList.RHIDrawIndexedPrimitive(GeoInsPtr->GetGIndexBuffer()->GetRHIIndexBuffer().get(), GeoInsPtr->GetIndexCount(), 1, 0, 0, 0);
	}
	RHICmdList.RHIEndRenderPass();
}