#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

class ShadowMaskGenCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new ShadowMaskGenCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	ShadowMaskGenCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		cbView.Bind(Initializer.ShaderParameterMap, "cbView");
		cbShadowViewInfo.Bind(Initializer.ShaderParameterMap, "cbShadowViewInfo");

		VsmShadowMaskTex.Bind(Initializer.ShaderParameterMap, "VsmShadowMaskTex");

		GBufferNormal.Bind(Initializer.ShaderParameterMap, "GBufferNormal");
		SceneDepthInput.Bind(Initializer.ShaderParameterMap, "SceneDepthInput");
		PagetableInfos.Bind(Initializer.ShaderParameterMap, "PagetableInfos");
		PhysicalShadowDepthTexture.Bind(Initializer.ShaderParameterMap, "PhysicalShadowDepthTexture");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* IncbView,
		XRHIConstantBuffer* IncbShadowViewInfo,

		XRHIUnorderedAcessView* InVsmShadowMaskTex,

		XRHIShaderResourceView* InGBufferNormal,
		XRHIShaderResourceView* InSceneDepthInput,
		XRHIShaderResourceView* InPagetableInfos,
		XRHIShaderResourceView* InPhysicalShadowDepthTexture)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbView, IncbView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbShadowViewInfo, IncbShadowViewInfo);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VsmShadowMaskTex, InVsmShadowMaskTex);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, GBufferNormal, InGBufferNormal);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthInput, InSceneDepthInput);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, PagetableInfos, InPagetableInfos);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, PhysicalShadowDepthTexture, InPhysicalShadowDepthTexture);
	}

	CBVParameterType cbView;
	CBVParameterType cbShadowViewInfo;

	UAVParameterType VsmShadowMaskTex;

	SRVParameterType GBufferNormal;
	SRVParameterType SceneDepthInput;
	SRVParameterType PagetableInfos;
	SRVParameterType PhysicalShadowDepthTexture;
};
ShadowMaskGenCS::ShaderInfos ShadowMaskGenCS::StaticShaderInfos(
	"ShadowMaskGenCS", GET_SHADER_PATH("ShadowMaskGenCS.hlsl"),
	"ShadowMaskGenCS", EShaderType::SV_Compute, ShadowMaskGenCS::CustomConstrucFunc,
	ShadowMaskGenCS::ModifyShaderCompileSettings);

struct ShadowMaskGenStruct
{
	XMatrix LightViewProjectMatrix;
	XVector3 ShadowLightDir;
	float padding0 = 0;
};

class XShadowMaskGenResourece :public XRenderResource
{
public:
	std::shared_ptr<XRHIConstantBuffer>ShadowMaskGenConstantBuffer;

	void InitRHI()override
	{
		ShadowMaskGenConstantBuffer = RHICreateConstantBuffer(sizeof(ShadowMaskGenStruct));
	}
	void ReleaseRHI()override
	{

	}
};
TGlobalResource<XShadowMaskGenResourece>ShadowMaskGenResourece;

void XDeferredShadingRenderer::ShadowMaskGenerate(XRHICommandList& RHICmdList)
{
	ShadowLightDir.Normalize();
	ShadowMaskGenStruct ShadowMaskGenIns;
	ShadowMaskGenIns.LightViewProjectMatrix = LightViewProjMat;
	ShadowMaskGenIns.ShadowLightDir = ShadowLightDir;

	ShadowMaskGenResourece.ShadowMaskGenConstantBuffer->UpdateData(&ShadowMaskGenIns,sizeof(ShadowMaskGenStruct),0);

	RHICmdList.RHIEventBegin(1, "ShadowMaskGenCS", sizeof("ShadowMaskGenCS"));
	TShaderReference<ShadowMaskGenCS> Shader = GetGlobalShaderMapping()->GetShader<ShadowMaskGenCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);

	Shader->SetParameters(RHICmdList, RViewInfo.ViewConstantBuffer.get(), 
		ShadowMaskGenResourece.ShadowMaskGenConstantBuffer.get(),
		GetRHIUAVFromTexture(SceneTargets.VSMShadowMaskTexture.get()),
		GetRHISRVFromTexture(SceneTargets.TextureGBufferA.get()),
		GetRHISRVFromTexture(SceneTargets.TextureDepthStencil.get()),
		GetRHISRVFromTexture(SceneTargets.PagetableInfos.get()),
		GetRHISRVFromTexture(SceneTargets.PhysicalShadowDepthTexture.get()));

	
	RHICmdList.RHIDispatchComputeShader(static_cast<uint32>(ceil(RViewInfo.ViewWidth / 16)), static_cast<uint32>(ceil(RViewInfo.ViewHeight / 16)), 1);
	RHICmdList.RHIEventEnd();
}