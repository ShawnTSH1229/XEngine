#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

class XHZBPassCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XHZBPassCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XHZBPassCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		DispatchThreadIdToBufferUV.Bind(Initializer.ShaderParameterMap, "DispatchThreadIdToBufferUV");

		TextureSampledInput.Bind(Initializer.ShaderParameterMap, "TextureSampledInput");
		FurthestHZBOutput_0.Bind(Initializer.ShaderParameterMap, "FurthestHZBOutput_0");
		FurthestHZBOutput_1.Bind(Initializer.ShaderParameterMap, "FurthestHZBOutput_1");
		FurthestHZBOutput_2.Bind(Initializer.ShaderParameterMap, "FurthestHZBOutput_2");
		FurthestHZBOutput_3.Bind(Initializer.ShaderParameterMap, "FurthestHZBOutput_3");
		FurthestHZBOutput_4.Bind(Initializer.ShaderParameterMap, "FurthestHZBOutput_4");

		//VirtualSMFlags.Bind(Initializer.ShaderParameterMap, "VirtualSMFlags");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XVector4 InDispatchThreadIdToBufferUV,

		XRHITexture* InTextureSampledInput,
		XRHIUnorderedAcessView* InFurthestHZBOutput_0,
		XRHIUnorderedAcessView* InFurthestHZBOutput_1,
		XRHIUnorderedAcessView* InFurthestHZBOutput_2,
		XRHIUnorderedAcessView* InFurthestHZBOutput_3,
		XRHIUnorderedAcessView* InFurthestHZBOutput_4
	)
	{
		SetShaderValue(RHICommandList, EShaderType::SV_Compute, DispatchThreadIdToBufferUV, InDispatchThreadIdToBufferUV);

		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, TextureSampledInput, InTextureSampledInput);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, FurthestHZBOutput_0, InFurthestHZBOutput_0);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, FurthestHZBOutput_1, InFurthestHZBOutput_1);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, FurthestHZBOutput_2, InFurthestHZBOutput_2);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, FurthestHZBOutput_3, InFurthestHZBOutput_3);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, FurthestHZBOutput_4, InFurthestHZBOutput_4);
	}

	XShaderVariableParameter DispatchThreadIdToBufferUV;

	TextureParameterType TextureSampledInput;
	UAVParameterType FurthestHZBOutput_0;
	UAVParameterType FurthestHZBOutput_1;
	UAVParameterType FurthestHZBOutput_2;
	UAVParameterType FurthestHZBOutput_3;
	UAVParameterType FurthestHZBOutput_4;
};

XHZBPassCS::ShaderInfos XHZBPassCS::StaticShaderInfos(
	"HZBBuildCS", GET_SHADER_PATH("HZB.hlsl"),
	"HZBBuildCS", EShaderType::SV_Compute, XHZBPassCS::CustomConstrucFunc,
	XHZBPassCS::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::HZBPass(XRHICommandList& RHICmdList)
{
	RHICmdList.RHIEventBegin(1, "HZBPass", sizeof("HZBPass"));
	TShaderReference<XHZBPassCS> Shader = GetGlobalShaderMapping()->GetShader<XHZBPassCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
	Shader->SetParameters(RHICmdList, XVector4(1.0 / 512.0, 1.0 / 512.0, 1.0, 1.0), SceneTargets.TextureDepthStencil.get(),
		GetRHIUAVFromTexture(SceneTargets.FurthestHZBOutput.get(), 0), GetRHIUAVFromTexture(SceneTargets.FurthestHZBOutput.get(), 1),
		GetRHIUAVFromTexture(SceneTargets.FurthestHZBOutput.get(), 2), GetRHIUAVFromTexture(SceneTargets.FurthestHZBOutput.get(), 3),
		GetRHIUAVFromTexture(SceneTargets.FurthestHZBOutput.get(), 4));
	RHICmdList.RHIDispatchComputeShader(512 / 16, 512 / 16, 1);
	RHICmdList.RHIEventEnd();
}


