#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

struct CbCullingParametersStruct
{
	XMatrix  ShdowViewProject;
	XPlane Planes[(int)ECameraPlane::CP_MAX];
	float commandCount;
};

class XPreDepthCullResourece :public XRenderResource
{
public:
	std::shared_ptr<XRHIConstantBuffer>RHICbCullingParameters;
	void InitRHI()override
	{
		RHICbCullingParameters = RHICreateConstantBuffer(sizeof(CbCullingParametersStruct));
	}
	void ReleaseRHI()override
	{

	}
};

TGlobalResource<XPreDepthCullResourece>PreDepthCullResourece;

class DepthGPUCullingCS : public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new DepthGPUCullingCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	DepthGPUCullingCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		outputCommands.Bind(Initializer.ShaderParameterMap, "outputCommands");
		inputCommands.Bind(Initializer.ShaderParameterMap, "inputCommands");
		SceneConstantBufferIN.Bind(Initializer.ShaderParameterMap, "SceneConstantBufferIN");
		cbCullingParameters.Bind(Initializer.ShaderParameterMap, "cbCullingParameters");
	}

	void SetParameters(XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* OutputCommandsUAV,
		XRHIShaderResourceView* InputCommandsSRV,
		XRHIShaderResourceView* SceneConstantBufferINView,
		XRHIConstantBuffer* CullingParametersCBV)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, outputCommands, OutputCommandsUAV);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, inputCommands, InputCommandsSRV);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SceneConstantBufferIN, SceneConstantBufferINView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbCullingParameters, CullingParametersCBV);
	}

	UAVParameterType outputCommands;
	SRVParameterType inputCommands;
	SRVParameterType SceneConstantBufferIN;
	CBVParameterType cbCullingParameters;
};
DepthGPUCullingCS::ShaderInfos DepthGPUCullingCS::StaticShaderInfos(
	"DepthGPUCullingCS", GET_SHADER_PATH("GPUCulling.hlsl"),
	"CSMain", EShaderType::SV_Compute, DepthGPUCullingCS::CustomConstrucFunc,
	DepthGPUCullingCS::ModifyShaderCompileSettings);

class XPreDepthPassVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XPreDepthPassVS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XPreDepthPassVS(const XShaderInitlizer& Initializer)
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

class XPreDepthPassPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XPreDepthPassPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XPreDepthPassPS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer) {}

	void SetParameter(XRHICommandList& RHICommandList) {}
};

XPreDepthPassVS::ShaderInfos XPreDepthPassVS::StaticShaderInfos(
	"XPreDepthPassVS", GET_SHADER_PATH("DepthOnlyVertexShader.hlsl"),
	"VS", EShaderType::SV_Vertex, XPreDepthPassVS::CustomConstrucFunc,
	XPreDepthPassVS::ModifyShaderCompileSettings);

XPreDepthPassPS::ShaderInfos XPreDepthPassPS::StaticShaderInfos(
	"XPreDepthPassPS", GET_SHADER_PATH("DepthOnlyVertexShader.hlsl"),
	"PS", EShaderType::SV_Pixel, XPreDepthPassPS::CustomConstrucFunc,
	XPreDepthPassPS::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::PreDepthPassGPUCullingSetup()
{
	std::vector<XRHIIndirectArg>IndirectPreDepthArgs;
	IndirectPreDepthArgs.resize(4);
	IndirectPreDepthArgs[0].type = IndirectArgType::Arg_CBV;
	IndirectPreDepthArgs[0].CBV.RootParameterIndex = 0;
	IndirectPreDepthArgs[1].type = IndirectArgType::Arg_VBV;
	IndirectPreDepthArgs[2].type = IndirectArgType::Arg_IBV;
	IndirectPreDepthArgs[3].type = IndirectArgType::Arg_Draw_Indexed;

	TShaderReference<XPreDepthPassVS> VertexShader = GetGlobalShaderMapping()->GetShader<XPreDepthPassVS>();
	TShaderReference<XPreDepthPassPS> PixelShader = GetGlobalShaderMapping()->GetShader<XPreDepthPassPS>();
	PreDepthPassResource.RHIDepthCommandSignature = RHICreateCommandSignature(IndirectPreDepthArgs.data(), 4, VertexShader.GetVertexShader(), PixelShader.GetPixelShader());

	std::vector<XRHICommandData> RHICmdData;
	RHICmdData.resize(RenderGeos.size());
	for (int i = 0; i < RenderGeos.size(); i++)
	{
		auto& it = RenderGeos[i];
		RHICmdData[i].CBVs.push_back(it->GetPerObjectVertexCBuffer().get());
		auto VertexBufferPtr = it->GetRHIVertexBuffer();
		auto IndexBufferPtr = it->GetRHIIndexBuffer();
		RHICmdData[i].VB = VertexBufferPtr.get();
		RHICmdData[i].IB = IndexBufferPtr.get();
		RHICmdData[i].IndexCountPerInstance = it->GetIndexCount();
		RHICmdData[i].InstanceCount = 1;
		RHICmdData[i].StartIndexLocation = 0;
		RHICmdData[i].BaseVertexLocation = 0;
		RHICmdData[i].StartInstanceLocation = 0;
	}

	uint32 OutCmdDataSize;
	void*  DataPtrret = RHIGetCommandDataPtr(RHICmdData, OutCmdDataSize);

	uint32 DepthPassIndirectBufferDataSize = OutCmdDataSize * RHICmdData.size();
	FResourceVectorUint8 DepthIndirectBufferData;
	DepthIndirectBufferData.Data = DataPtrret;
	DepthIndirectBufferData.SetResourceDataSize(DepthPassIndirectBufferDataSize);
	XRHIResourceCreateData IndirectBufferResourceData(&DepthIndirectBufferData);
	PreDepthPassResource.DepthCmdBufferNoCulling = RHIcreateStructBuffer(OutCmdDataSize, DepthPassIndirectBufferDataSize, EBufferUsage(int(EBufferUsage::BUF_DrawIndirect) | (int)EBufferUsage::BUF_ShaderResource), IndirectBufferResourceData);
	
	PreDepthPassResource.DepthCmdBufferOffset = RHIGetCmdBufferOffset(PreDepthPassResource.DepthCmdBufferNoCulling.get());
	PreDepthPassResource.DepthCounterOffset = AlignArbitrary(DepthPassIndirectBufferDataSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	PreDepthPassResource.DepthCmdBufferCulled = RHIcreateStructBuffer(DepthPassIndirectBufferDataSize, PreDepthPassResource.DepthCounterOffset + sizeof(UINT), EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);
	PreDepthPassResource.CmdBufferShaderResourceView = RHICreateShaderResourceView(PreDepthPassResource.DepthCmdBufferNoCulling.get());
	PreDepthPassResource.CmdBufferUnorderedAcessView = RHICreateUnorderedAccessView(PreDepthPassResource.DepthCmdBufferCulled.get(), true, true, PreDepthPassResource.DepthCounterOffset);

	CbCullingParametersStruct CbCullingParametersStructIns;
	CbCullingParametersStructIns.commandCount = RenderGeos.size();
	CbCullingParametersStructIns.ShdowViewProject = LightViewProjMat;
	RViewInfo.ViewMats.GetPlanes(CbCullingParametersStructIns.Planes);

	PreDepthCullResourece.RHICbCullingParameters->UpdateData(&CbCullingParametersStructIns, sizeof(CbCullingParametersStruct), 0);
}


void XDeferredShadingRenderer::PreDepthPassGPUCulling(XRHICommandList& RHICmdList)
{
	RHICmdList.RHIEventBegin(1, "GPUCulling", sizeof("GPUCulling"));
	TShaderReference<DepthGPUCullingCS> Shader = GetGlobalShaderMapping()->GetShader<DepthGPUCullingCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
	RHIResetStructBufferCounter(PreDepthPassResource.DepthCmdBufferCulled.get(), PreDepthPassResource.DepthCounterOffset);
	Shader->SetParameters(RHICmdList, PreDepthPassResource.CmdBufferUnorderedAcessView.get(), PreDepthPassResource.CmdBufferShaderResourceView.get(), GlobalObjectStructBufferSRV.get(), PreDepthCullResourece.RHICbCullingParameters.get());
	RHICmdList.RHIDispatchComputeShader(static_cast<uint32>(ceil(RenderGeos.size() / float(128))), 1, 1);
	RHICmdList.RHIEventEnd();
}

void XDeferredShadingRenderer::PreDepthPassRendering(XRHICommandList& RHICmdList)
{
	XRHIRenderPassInfo RPInfos(0, nullptr, ERenderTargetLoadAction::ENoAction, SceneTargets.TextureDepthStencil.get(), EDepthStencilLoadAction::EClear);
	RHICmdList.RHIBeginRenderPass(RPInfos, "PreDepthPass", sizeof("PreDepthPass"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, ECompareFunction::CF_GreaterEqual>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<XPreDepthPassVS> VertexShader = GetGlobalShaderMapping()->GetShader<XPreDepthPassVS>();
	TShaderReference<XPreDepthPassPS> PixelShader = GetGlobalShaderMapping()->GetShader<XPreDepthPassPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = DefaultVertexFactory.GetLayout(ELayoutType::Layout_Default).get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameter(RHICmdList, RViewInfo.ViewConstantBuffer.get());

	RHICmdList.RHIExecuteIndirect(PreDepthPassResource.RHIDepthCommandSignature.get(), RenderGeos.size(), PreDepthPassResource.DepthCmdBufferCulled.get(), PreDepthPassResource.DepthCmdBufferOffset, PreDepthPassResource.DepthCmdBufferCulled.get(), PreDepthPassResource.DepthCounterOffset);
	RHICmdList.RHIEventEnd();
}
