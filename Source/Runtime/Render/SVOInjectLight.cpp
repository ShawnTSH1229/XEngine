#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "VoxelizationScene.h"
#include "ShadowMap_Old.h"

#if USE_SVOGI
class ShadowMapInjectLightCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new ShadowMapInjectLightCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	ShadowMapInjectLightCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		cbShadow.Bind(Initializer.ShaderParameterMap, "cbShadow");
		cbVoxelization.Bind(Initializer.ShaderParameterMap, "cbVoxelization");
		cbMainLight.Bind(Initializer.ShaderParameterMap, "cbMainLight");

		VoxelArrayR.Bind(Initializer.ShaderParameterMap, "VoxelArrayR");
		SceneDepthInput.Bind(Initializer.ShaderParameterMap, "SceneDepthInput");

		IrradianceBrickBufferRW.Bind(Initializer.ShaderParameterMap, "IrradianceBrickBufferRW");
		SpaseVoxelOctreeRW.Bind(Initializer.ShaderParameterMap, "SpaseVoxelOctreeRW");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* cbShadowIn,
		XRHIConstantBuffer* cbVoxelizationIn,
		XRHIConstantBuffer* cbMainLightIn,

		XRHIShaderResourceView* VoxelArrayRIn,
		XRHIShaderResourceView* SceneDepthInputIn,

		XRHIUnorderedAcessView* IrradianceBrickBufferRWIn,
		XRHIUnorderedAcessView* SpaseVoxelOctreeRWIn
	)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbShadow, cbShadowIn);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbVoxelization, cbVoxelizationIn);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbMainLight, cbMainLightIn);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VoxelArrayR, VoxelArrayRIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthInput, SceneDepthInputIn);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, IrradianceBrickBufferRW, IrradianceBrickBufferRWIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, SpaseVoxelOctreeRW, SpaseVoxelOctreeRWIn);
	}

	CBVParameterType cbShadow;
	CBVParameterType cbVoxelization;
	CBVParameterType cbMainLight;

	SRVParameterType VoxelArrayR;
	SRVParameterType SceneDepthInput;

	UAVParameterType IrradianceBrickBufferRW;
	UAVParameterType SpaseVoxelOctreeRW;
};
ShadowMapInjectLightCS::ShaderInfos ShadowMapInjectLightCS::StaticShaderInfos(
	"ShadowMapInjectLightCS", GET_SHADER_PATH("SVOInjectLight.hlsl"),
	"ShadowMapInjectLightCS", EShaderType::SV_Compute, ShadowMapInjectLightCS::CustomConstrucFunc,
	ShadowMapInjectLightCS::ModifyShaderCompileSettings);

class ResetOctreeFlagsCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new ResetOctreeFlagsCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	ResetOctreeFlagsCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		SpaseVoxelOctreeRW.Bind(Initializer.ShaderParameterMap, "SpaseVoxelOctreeRW");
		NodeCountAndOffsetBuffer.Bind(Initializer.ShaderParameterMap, "NodeCountAndOffsetBuffer");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* SpaseVoxelOctreeRWIn,
		XRHIUnorderedAcessView* NodeCountAndOffsetBufferUAVIn)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, SpaseVoxelOctreeRW, SpaseVoxelOctreeRWIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, NodeCountAndOffsetBuffer, NodeCountAndOffsetBufferUAVIn);
	}
	UAVParameterType NodeCountAndOffsetBuffer;
	UAVParameterType SpaseVoxelOctreeRW;
};




class AverageLitNodeValuesCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new AverageLitNodeValuesCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	AverageLitNodeValuesCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		IrradianceBrickBufferROnly.Bind(Initializer.ShaderParameterMap, "IrradianceBrickBufferROnly");
		IrradianceBrickBufferWOnly.Bind(Initializer.ShaderParameterMap, "IrradianceBrickBufferWOnly");
		NodeCountAndOffsetBufferR.Bind(Initializer.ShaderParameterMap, "NodeCountAndOffsetBufferR");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIShaderResourceView* IrradianceBrickBufferROnlyIn,
		XRHIShaderResourceView* NodeCountAndOffsetBufferRIn,
		XRHIUnorderedAcessView* IrradianceBrickBufferWOnlyIn
	)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, IrradianceBrickBufferWOnly, IrradianceBrickBufferWOnlyIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, NodeCountAndOffsetBufferR, NodeCountAndOffsetBufferRIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, IrradianceBrickBufferROnly, IrradianceBrickBufferROnlyIn);
	}
	SRVParameterType NodeCountAndOffsetBufferR;
	UAVParameterType IrradianceBrickBufferROnly;
	UAVParameterType IrradianceBrickBufferWOnly;
};
AverageLitNodeValuesCS::ShaderInfos AverageLitNodeValuesCS::StaticShaderInfos(
	"AverageLitNodeValuesCS", GET_SHADER_PATH("SVOInjectLight.hlsl"),
	"AverageLitNodeValuesCS", EShaderType::SV_Compute, AverageLitNodeValuesCS::CustomConstrucFunc,
	AverageLitNodeValuesCS::ModifyShaderCompileSettings);

class GatherValuesFromLowLevelCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new GatherValuesFromLowLevelCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	GatherValuesFromLowLevelCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		SpaseVoxelOctreeR.Bind(Initializer.ShaderParameterMap, "SpaseVoxelOctreeR");
		IrradianceBrickBufferROnly.Bind(Initializer.ShaderParameterMap, "IrradianceBrickBufferROnly");
		NodeCountAndOffsetBufferR.Bind(Initializer.ShaderParameterMap, "NodeCountAndOffsetBufferR");
		IrradianceBrickBufferWOnly.Bind(Initializer.ShaderParameterMap, "IrradianceBrickBufferWOnly");
		cbSVOBuildBuffer.Bind(Initializer.ShaderParameterMap, "cbSVOBuildBuffer");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIShaderResourceView* SpaseVoxelOctreeRIn,
		XRHIShaderResourceView* IrradianceBrickBufferROnlyIn,
		XRHIShaderResourceView* NodeCountAndOffsetBufferRIn,
		XRHIUnorderedAcessView* IrradianceBrickBufferWOnlyIn,
		XRHIConstantBuffer* cbSVOBuildBufferIn
	)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SpaseVoxelOctreeR, SpaseVoxelOctreeRIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, IrradianceBrickBufferROnly, IrradianceBrickBufferROnlyIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, NodeCountAndOffsetBufferR, NodeCountAndOffsetBufferRIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, IrradianceBrickBufferWOnly, IrradianceBrickBufferWOnlyIn);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbSVOBuildBuffer, cbSVOBuildBufferIn);
	}
	SRVParameterType SpaseVoxelOctreeR;
	SRVParameterType IrradianceBrickBufferROnly;
	SRVParameterType NodeCountAndOffsetBufferR;
	UAVParameterType IrradianceBrickBufferWOnly;
	CBVParameterType cbSVOBuildBuffer;
};
GatherValuesFromLowLevelCS::ShaderInfos GatherValuesFromLowLevelCS::StaticShaderInfos(
	"GatherValuesFromLowLevelCS", GET_SHADER_PATH("SVOInjectLight.hlsl"),
	"GatherValuesFromLowLevelCS", EShaderType::SV_Compute, GatherValuesFromLowLevelCS::CustomConstrucFunc,
	GatherValuesFromLowLevelCS::ModifyShaderCompileSettings);
#endif
void XDeferredShadingRenderer::SVOInjectLightPass(XRHICommandList& RHICmdList)
{
#if USE_SVOGI
	cbMainLightStruct cbMainLightins;
	cbMainLightins.LightColor = MainLightColor;
	cbMainLightins.LightIntensity = LightIntensity;
	cbMainLightins.LightDir = ShadowLightDir;
	SVOGIResourece.RHIcbMainLight->UpdateData(&cbMainLightins, sizeof(cbMainLightStruct), 0);

	{
		RHICmdList.RHIEventBegin(1, "ShadowMapInjectLightCS", sizeof("ShadowMapInjectLightCS"));
		TShaderReference<ShadowMapInjectLightCS> Shader = GetGlobalShaderMapping()->GetShader<ShadowMapInjectLightCS>();
		XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
		Shader->SetParameters(
			RHICmdList,
			ShadowMapResourece_Old.ShadowViewProjectionCB.get(),
			SVOGIResourece.VoxelScenePSCBuffer.get(),
			SVOGIResourece.RHIcbMainLight.get(),
			SVOGIResourece.VoxelArrayShaderResourceView.get(),
			GetRHISRVFromTexture(SceneTargets.TempShadowDepthTexture.get()),
			GetRHIUAVFromTexture(SVOGIResourece.IrradianceBrickBufferRWUAV.get()),
			GetRHIUAVFromTexture(SVOGIResourece.SpaseVoxelOctree.get())
		);
		RHICmdList.RHIDispatchComputeShader(ShadowMapDepthTextureSize / 16, ShadowMapDepthTextureSize /16, 1);
		RHICmdList.RHIEventEnd();
	}

	
	SVOGIResourece.BrickBufferPingPong[0] = SVOGIResourece.IrradianceBrickBufferRWUAV.get();
	SVOGIResourece.BrickBufferPingPong[1] = SVOGIResourece.IrradianceBrickBufferPinPongUAV.get();
	SVOGIResourece.PingPongIndex = 0;
	//()%2 for R
	//( + 1)%2 for W
	
	{
		RHICmdList.RHIEventBegin(1, "AverageLitNodeValuesCS", sizeof("AverageLitNodeValuesCS"));
		TShaderReference<AverageLitNodeValuesCS> Shader = GetGlobalShaderMapping()->GetShader<AverageLitNodeValuesCS>();
		XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
		Shader->SetParameters(
			RHICmdList,
			//GetRHISRVFromTexture(SVOGIResourece.IrradianceBrickBufferRWUAV.get()),
			GetRHISRVFromTexture(SVOGIResourece.BrickBufferPingPong[SVOGIResourece.PingPongIndex % 2]),
			GetRHISRVFromTexture(SVOGIResourece.NodeCountAndOffsetBuffer.get()),
			//GetRHIUAVFromTexture(SVOGIResourece.IrradianceBrickBufferPinPongUAV.get())
			GetRHIUAVFromTexture(SVOGIResourece.BrickBufferPingPong[(SVOGIResourece.PingPongIndex + 1) % 2])
			//,GetRHIUAVFromTexture(SVOGIResourece.NodeCountAndOffsetBuffer.get())
		);
		RHICmdList.RHIDispatchComputeShader(VoxelDimension * 256 / 128, 1, 1);
		RHICmdList.RHIEventEnd();
		SVOGIResourece.PingPongIndex++;
	}

	//8 leaf
	//7 AverageLitNodeValuesCS
	//start at 6

	uint32 DispatchSize = VoxelDimension * 256 / 128;
	for (int32 i = OctreeHeight - 2 - 1; i >= 0; --i)
	{
		DispatchSize /= 2;
		RHICmdList.RHIEventBegin(1, "GatherValuesFromLowLevelCS", sizeof("GatherValuesFromLowLevelCS"));
		TShaderReference<GatherValuesFromLowLevelCS> GatherValuesFromLowLevelCSShader = GetGlobalShaderMapping()->GetShader<GatherValuesFromLowLevelCS>();
		XRHIComputeShader* FilterComputeShader = GatherValuesFromLowLevelCSShader.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, FilterComputeShader);
		GatherValuesFromLowLevelCSShader->SetParameters(
			RHICmdList,
			GetRHISRVFromTexture(SVOGIResourece.SpaseVoxelOctree.get()),
			GetRHISRVFromTexture(SVOGIResourece.BrickBufferPingPong[SVOGIResourece.PingPongIndex % 2]),
			GetRHISRVFromTexture(SVOGIResourece.NodeCountAndOffsetBuffer.get()),
			GetRHIUAVFromTexture(SVOGIResourece.BrickBufferPingPong[(SVOGIResourece.PingPongIndex + 1) % 2]),
			SVOGIResourece.cbSVOBuildBufferLevels[i].get()
		);
		RHICmdList.RHIDispatchComputeShader(DispatchSize, 1, 1);
		RHICmdList.RHIEventEnd();
		SVOGIResourece.PingPongIndex++;
	}
#endif
}