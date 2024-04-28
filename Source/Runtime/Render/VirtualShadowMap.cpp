#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "VirtualShadowMap.h"

struct VSMTileMaskStruct
{
	XMatrix LighgViewProject;

	float ClientWidth;
	float ClientHeight;
	float padding0;
	float padding1;
};

struct TiledInfoStruct
{
	XMatrix ViewProjMatrix;
	uint32 IndexX;
	uint32 IndexY;
	uint32 Padding0;
	uint32 Padding1;
};

struct CbCullingParametersStruct
{
	XMatrix  ShdowViewProject;
	XPlane Planes[(int)ECameraPlane::CP_MAX];
	float commandCount;
};

class XVirtualShadowMapResourece :public XRenderResource
{
public:
	//VSMTileMaskCS
	std::shared_ptr <XRHITexture2D> VirtualSMFlags;
	std::shared_ptr<XRHIConstantBuffer>VSMTileMaskConstantBuffer;


	std::shared_ptr<XRHIStructBuffer>VSMCounterBuffer;
	std::shared_ptr<XRHIUnorderedAcessView>VSMCounterBufferUAV;
	//Shadow Command
	std::shared_ptr<XRHIConstantBuffer>RHICbCullingParameters;

	std::shared_ptr<XRHIStructBuffer>ShadowCmdBufferNoCulling;
	std::shared_ptr<XRHIStructBuffer>ShadowCmdBufferCulled;
	std::shared_ptr<XRHIShaderResourceView>ShadowNoCullCmdBufferSRV;
	std::shared_ptr<XRHIUnorderedAcessView> ShadowCulledCmdBufferUAV;
	uint64 ShadowCmdBufferOffset;
	uint32 ShadowCounterOffset;

	std::shared_ptr<XRHIConstantBuffer> TilesShadowViewProjMatrixCB;

	//Shadow Projection Per Tile
	std::shared_ptr<XRHICommandSignature>RHIShadowCommandSignature;
	std::shared_ptr<XRHITexture2D>PlaceHodeltarget;

	void InitRHI()override
	{
		VSMCounterBuffer = RHIcreateStructBuffer(sizeof(uint32),sizeof(uint32), EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)),nullptr);

		VirtualSMFlags =  RHICreateTexture2D(VirtualTileWidthNum, VirtualTileWidthNum, 1, false, false, EPixelFormat::FT_R32_UINT
			, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);
		VSMTileMaskConstantBuffer = RHICreateConstantBuffer(sizeof(VSMTileMaskStruct));
		
		TilesShadowViewProjMatrixCB = RHICreateConstantBuffer(VirtualTileWidthNum * VirtualTileWidthNum * sizeof(TiledInfoStruct));
		PlaceHodeltarget = RHICreateTexture2D(PhysicalTileSize, PhysicalTileSize, 1, false, false, EPixelFormat::FT_R8G8B8A8_UNORM, ETextureCreateFlags(TexCreate_RenderTargetable), 1, nullptr);

		RHICbCullingParameters = RHICreateConstantBuffer(sizeof(CbCullingParametersStruct));
	}
	void ReleaseRHI()override
	{

	}
};

TGlobalResource<XVirtualShadowMapResourece>VirtualShadowMapResourece;



class VSMTileMaskCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new VSMTileMaskCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	VSMTileMaskCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		TextureSampledInput.Bind(Initializer.ShaderParameterMap, "TextureSampledInput");
		cbView.Bind(Initializer.ShaderParameterMap, "cbView");
		VirtualSMFlags.Bind(Initializer.ShaderParameterMap, "VirtualSMFlags");
		cbShadowViewInfo.Bind(Initializer.ShaderParameterMap, "cbShadowViewInfo");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* IncbView,
		XRHIConstantBuffer* cbShadowViewInfoIn,
		XRHIUnorderedAcessView* VirtualSMFlagsIn,
		XRHITexture* InTextureSampledInput
	)
	{
		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, TextureSampledInput, InTextureSampledInput);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbView, IncbView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbShadowViewInfo, cbShadowViewInfoIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualSMFlags, VirtualSMFlagsIn);
	}

	TextureParameterType TextureSampledInput;
	CBVParameterType cbView;
	CBVParameterType cbShadowViewInfo;
	UAVParameterType VirtualSMFlags;
};

VSMTileMaskCS::ShaderInfos VSMTileMaskCS::StaticShaderInfos(
	"VSMTileMaskCS", GET_SHADER_PATH("VSMTileMaskCS.hlsl"),
	"VSMTileMaskCS", EShaderType::SV_Compute, VSMTileMaskCS::CustomConstrucFunc,
	VSMTileMaskCS::ModifyShaderCompileSettings);



static void TilesShadowViewProjMatrixBuild(XBoundSphere& SceneBoundingSphere,XMatrix& LightViewMatIn,XVector3 MainLightDir)
{
	MainLightDir.Normalize();
	XVector3 LightPos = SceneBoundingSphere.Center + MainLightDir * SceneBoundingSphere.Radius * 1.0;

	float l = SceneBoundingSphere.Center.x - SceneBoundingSphere.Radius;
	float r = SceneBoundingSphere.Center.x + SceneBoundingSphere.Radius;
	float t = SceneBoundingSphere.Center.y + SceneBoundingSphere.Radius;
	float b = SceneBoundingSphere.Center.y - SceneBoundingSphere.Radius;
	float f = SceneBoundingSphere.Radius * 2;
	float n = 0.1f;

	XVector3 UpTemp(0, 1, 0);
	XVector3 WDir = -MainLightDir; WDir.Normalize();
	XVector3 UDir = UpTemp.Cross(WDir); UDir.Normalize();
	XVector3 VDir = WDir.Cross(UDir); VDir.Normalize();

	float TileSize = SceneBoundingSphere.Radius * 2 / (float)VirtualTileWidthNum;

	for (uint32 IndexX = 0; IndexX < VirtualTileWidthNum; IndexX++)
	{
		for (uint32 IndexY = 0; IndexY < VirtualTileWidthNum; IndexY++)
		{
			XMatrix TempSubLightProj = XMatrix::CreateOrthographicOffCenterLH(
				-TileSize * 0.5, TileSize * 0.5,
				-TileSize * 0.5, TileSize * 0.5,
				f, n);

			float SubL = l + (r - l) * ((IndexX) / float(VirtualTileWidthNum));
			float SubR = SubL + TileSize;
			float SubT = t - (t - b) * ((IndexY) / float(VirtualTileWidthNum));
			float SubB = SubT - TileSize;
			XVector3 SubLightPos = LightPos + UDir * (SubL + SubR) * 0.5 + VDir * (SubB + SubT) * 0.5;
			XMatrix SubLightMatrix = LightViewMatIn;

			XVector3 NegEyePosition = -SubLightPos;
			float NegQU = UDir.Dot(NegEyePosition);
			float NegQV = VDir.Dot(NegEyePosition);
			float NegQW = WDir.Dot(NegEyePosition);

			SubLightMatrix.m[3][0] = NegQU;
			SubLightMatrix.m[3][1] = NegQV;
			SubLightMatrix.m[3][2] = NegQW;

			if (IndexX == 31 && IndexY == 31)
			{
				int a = 1;
			}

			XMatrix ViewProject = SubLightMatrix * TempSubLightProj;

			TiledInfoStruct TempTiledInfo;
			TempTiledInfo.ViewProjMatrix = ViewProject;
			TempTiledInfo.IndexX = IndexX;
			TempTiledInfo.IndexY = IndexY;
			TempTiledInfo.Padding0 = 0;
			TempTiledInfo.Padding1 = 1;
			VirtualShadowMapResourece.TilesShadowViewProjMatrixCB->UpdateData(
				&TempTiledInfo,
				sizeof(TiledInfoStruct),
				(IndexY * VirtualTileWidthNum + IndexX) * sizeof(TiledInfoStruct));
		}
	}
}



void XDeferredShadingRenderer::VSMUpdate()
{
	VSMTileMaskStruct TileMaskStructIns;
	TileMaskStructIns.LighgViewProject = LightViewProjMat;
	TileMaskStructIns.ClientWidth = RViewInfo.ViewWidth;
	TileMaskStructIns.ClientHeight = RViewInfo.ViewHeight;

	VirtualShadowMapResourece.VSMTileMaskConstantBuffer->UpdateData(&TileMaskStructIns, sizeof(VSMTileMaskStruct), 0);

	CbCullingParametersStruct CbCullingParametersStructIns;
	CbCullingParametersStructIns.commandCount = RenderGeos.size();
	CbCullingParametersStructIns.ShdowViewProject = LightViewProjMat;
	RViewInfo.ViewMats.GetPlanes(CbCullingParametersStructIns.Planes);

	VirtualShadowMapResourece.RHICbCullingParameters->UpdateData(&CbCullingParametersStructIns, sizeof(CbCullingParametersStruct), 0);
}

void XDeferredShadingRenderer::VSMTileMaskPass(XRHICommandList& RHICmdList)
{
	RHICmdList.RHIEventBegin(1, "VSMTileMaskCS", sizeof("VSMTileMaskCS"));
	TShaderReference<VSMTileMaskCS> Shader = GetGlobalShaderMapping()->GetShader<VSMTileMaskCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
	Shader->SetParameters(
		RHICmdList,
		RViewInfo.ViewConstantBuffer.get(),
		VirtualShadowMapResourece.VSMTileMaskConstantBuffer.get(),
		GetRHIUAVFromTexture(VirtualShadowMapResourece.VirtualSMFlags.get()),
		SceneTargets.TextureDepthStencil.get());
	RHICmdList.RHIDispatchComputeShader(static_cast<uint32>(ceil(RViewInfo.ViewWidth / 16)), static_cast<uint32>(ceil(RViewInfo.ViewHeight / 16)), 1);
	RHICmdList.RHIEventEnd();
}



class VSMPageTableGenCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new VSMPageTableGenCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	VSMPageTableGenCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualSMFlags.Bind(Initializer.ShaderParameterMap, "VirtualSMFlags");
		PagetableInfos.Bind(Initializer.ShaderParameterMap, "PagetableInfos");
		VSMCounterBuffer.Bind(Initializer.ShaderParameterMap, "VSMCounterBuffer");
	}

	void SetParameters(
		XRHICommandList& RHICommandList, 
		XRHIShaderResourceView* VirtualSMFlagsIn,
		XRHIUnorderedAcessView* PagetableInfosIn,
		XRHIUnorderedAcessView* VSMCounterBufferIn
	)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualSMFlags, VirtualSMFlagsIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, PagetableInfos, PagetableInfosIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VSMCounterBuffer, VSMCounterBufferIn);
	}
	SRVParameterType VirtualSMFlags;
	UAVParameterType PagetableInfos;
	UAVParameterType VSMCounterBuffer;
};

VSMPageTableGenCS::ShaderInfos VSMPageTableGenCS::StaticShaderInfos(
	"VSMPageTableGenCS", GET_SHADER_PATH("VSMPagetableGen.hlsl"),
	"VSMPageTableGenCS", EShaderType::SV_Compute, VSMPageTableGenCS::CustomConstrucFunc,
	VSMPageTableGenCS::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::VSMPageTableGen(XRHICommandList& RHICmdList)
{
	RHICmdList.RHIEventBegin(1, "VSMPageTableGenCS", sizeof("VSMPageTableGenCS"));
	RHIResetStructBufferCounter(VirtualShadowMapResourece.VSMCounterBuffer.get(), 0);
	TShaderReference<VSMPageTableGenCS> Shader = GetGlobalShaderMapping()->GetShader<VSMPageTableGenCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
	Shader->SetParameters(RHICmdList, GetRHISRVFromTexture(VirtualShadowMapResourece.VirtualSMFlags.get()), GetRHIUAVFromTexture(SceneTargets.PagetableInfos.get()), VirtualShadowMapResourece.VSMCounterBufferUAV.get());
	RHICmdList.RHIDispatchComputeShader(1, 1, 1);
	RHICmdList.RHIEventEnd();
}


class ShadowCommandBuildCS : public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new ShadowCommandBuildCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	ShadowCommandBuildCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		outputCommands.Bind(Initializer.ShaderParameterMap, "outputCommands");
		inputCommands.Bind(Initializer.ShaderParameterMap, "inputCommands");
		SceneConstantBufferIN.Bind(Initializer.ShaderParameterMap, "SceneConstantBufferIN");
		cbCullingParameters.Bind(Initializer.ShaderParameterMap, "cbCullingParameters");
		VirtualSMFlags.Bind(Initializer.ShaderParameterMap, "VirtualSMFlags");
	}

	void SetParameters(XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* OutputCommandsUAV,
		XRHIShaderResourceView* InputCommandsSRV,
		XRHIShaderResourceView* SceneConstantBufferINView,
		XRHIConstantBuffer* CullingParametersCBV,
		XRHIShaderResourceView* VirtualSMFlagsSRVIn
	)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, outputCommands, OutputCommandsUAV);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, inputCommands, InputCommandsSRV);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SceneConstantBufferIN, SceneConstantBufferINView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbCullingParameters, CullingParametersCBV);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualSMFlags, VirtualSMFlagsSRVIn);
	}
	UAVParameterType outputCommands;
	SRVParameterType inputCommands;
	SRVParameterType SceneConstantBufferIN;
	SRVParameterType VirtualSMFlags;
	CBVParameterType cbCullingParameters;
};

ShadowCommandBuildCS::ShaderInfos ShadowCommandBuildCS::StaticShaderInfos(
	"ShadowCommandBuildCS", GET_SHADER_PATH("ShadowCommandBuild.hlsl"),
	"ShadowCommandBuildCS", EShaderType::SV_Compute, ShadowCommandBuildCS::CustomConstrucFunc,
	ShadowCommandBuildCS::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::VSMShadowCommandBuild(XRHICommandList& RHICmdList)
{
	TilesShadowViewProjMatrixBuild(SceneBoundingSphere, LightViewMat, ShadowLightDir);

	RHICmdList.RHIEventBegin(1, "ShadowCommandBuildCS", sizeof("ShadowCommandBuildCS"));
	TShaderReference<ShadowCommandBuildCS> Shader = GetGlobalShaderMapping()->GetShader<ShadowCommandBuildCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
	RHIResetStructBufferCounter(VirtualShadowMapResourece.ShadowCmdBufferCulled.get(), VirtualShadowMapResourece.ShadowCounterOffset);
	Shader->SetParameters(RHICmdList, VirtualShadowMapResourece.ShadowCulledCmdBufferUAV.get(), VirtualShadowMapResourece.ShadowNoCullCmdBufferSRV.get(),
		GlobalObjectStructBufferSRV.get(), VirtualShadowMapResourece.RHICbCullingParameters.get(), GetRHISRVFromTexture(VirtualShadowMapResourece.VirtualSMFlags.get()));
	RHICmdList.RHIDispatchComputeShader(static_cast<UINT>(ceil(RenderGeos.size() / float(128))), 1, 1);
	RHICmdList.RHIEventEnd();
}


//VS IS Empty , Since this is Set By IndiretcDraw
class XShadowPerTileProjectionVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XShadowPerTileProjectionVS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XShadowPerTileProjectionVS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer) {}
	void SetParameters() {}
};
XShadowPerTileProjectionVS::ShaderInfos XShadowPerTileProjectionVS::StaticShaderInfos(
	"XShadowPerTileProjectionVS", GET_SHADER_PATH("ShadowPerTileProjectionVS.hlsl"),
	"VS", EShaderType::SV_Vertex, XShadowPerTileProjectionVS::CustomConstrucFunc,
	XShadowPerTileProjectionVS::ModifyShaderCompileSettings);


class XShadowPerTileProjectionPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XShadowPerTileProjectionPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XShadowPerTileProjectionPS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		PhysicalShadowDepthTexture.Bind(Initializer.ShaderParameterMap, "PhysicalShadowDepthTexture");
		PagetableInfos.Bind(Initializer.ShaderParameterMap, "PagetableInfos");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* InUAV,
		XRHIShaderResourceView* InSRV)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Pixel, PhysicalShadowDepthTexture, InUAV);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Pixel, PagetableInfos, InSRV);
	}

	SRVParameterType PagetableInfos;
	UAVParameterType PhysicalShadowDepthTexture;
};

XShadowPerTileProjectionPS::ShaderInfos XShadowPerTileProjectionPS::StaticShaderInfos(
	"XShadowPerTileProjectionPS", GET_SHADER_PATH("ShadowPerTileProjectionPS.hlsl"),
	"PS", EShaderType::SV_Pixel, XShadowPerTileProjectionPS::CustomConstrucFunc,
	XShadowPerTileProjectionPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::VirtualShadowMapGen(XRHICommandList& RHICmdList)
{
	XRHITexture* PlaceHodeltargetTex = VirtualShadowMapResourece.PlaceHodeltarget.get();
	XRHIRenderPassInfo RPInfos(1, &PlaceHodeltargetTex, ERenderTargetLoadAction::EClear, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RPInfos, "VirtualShadowMapGenPass", sizeof("VirtualShadowMapGenPass"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<XShadowPerTileProjectionVS> VertexShader = GetGlobalShaderMapping()->GetShader<XShadowPerTileProjectionVS>();
	TShaderReference<XShadowPerTileProjectionPS> PixelShader = GetGlobalShaderMapping()->GetShader<XShadowPerTileProjectionPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	std::shared_ptr<XRHIVertexLayout> RefVertexLayout = DefaultVertexFactory.GetLayout(ELayoutType::Layout_Default);
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = RefVertexLayout.get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);
	PixelShader->SetParameters(RHICmdList, GetRHIUAVFromTexture(SceneTargets.PhysicalShadowDepthTexture.get()), GetRHISRVFromTexture(SceneTargets.PagetableInfos.get()));
	RHICmdList.RHIExecuteIndirect(VirtualShadowMapResourece.RHIShadowCommandSignature.get(), RenderGeos.size() * 12,
		VirtualShadowMapResourece.ShadowCmdBufferCulled.get(), VirtualShadowMapResourece.ShadowCmdBufferOffset, 
		VirtualShadowMapResourece.ShadowCmdBufferCulled.get(), VirtualShadowMapResourece.ShadowCounterOffset);
	RHICmdList.RHIEndRenderPass();
}

class VSMTileMaskClearCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new VSMTileMaskClearCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	VSMTileMaskClearCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualSMFlags.Bind(Initializer.ShaderParameterMap, "VirtualSMFlags");
		PhysicalShadowDepthTexture.Bind(Initializer.ShaderParameterMap, "PhysicalShadowDepthTexture");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* VirtualSMFlagsIn,
		XRHIUnorderedAcessView* PhysicalShadowDepthTextureIn)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualSMFlags, VirtualSMFlagsIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, PhysicalShadowDepthTexture, PhysicalShadowDepthTextureIn);
	}
	UAVParameterType VirtualSMFlags;
	UAVParameterType PhysicalShadowDepthTexture;
};
VSMTileMaskClearCS::ShaderInfos VSMTileMaskClearCS::StaticShaderInfos(
	"VSMTileMaskClearCS", GET_SHADER_PATH("VSMTileMaskClearCS.hlsl"),
	"VSMTileMaskClearCS", EShaderType::SV_Compute, VSMTileMaskClearCS::CustomConstrucFunc,
	VSMTileMaskClearCS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::VSMTileMaskClear(XRHICommandList& RHICmdList)
{
	RHICmdList.RHIEventBegin(1, "VSMTileMaskClearCS", sizeof("VSMTileMaskClearCS"));
	TShaderReference<VSMTileMaskClearCS> Shader = GetGlobalShaderMapping()->GetShader<VSMTileMaskClearCS>();
	XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
	SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
	Shader->SetParameters(RHICmdList, 
		GetRHIUAVFromTexture(VirtualShadowMapResourece.VirtualSMFlags.get()), 
		GetRHIUAVFromTexture(SceneTargets.PhysicalShadowDepthTexture.get()));
	RHICmdList.RHIDispatchComputeShader(VirtualTileWidthNum / 16, VirtualTileWidthNum / 16, 1);
	RHICmdList.RHIEventEnd();
}


void XDeferredShadingRenderer::VSMSetup()
{
	std::vector<XRHIIndirectArg>IndirectShadowArgs;
	IndirectShadowArgs.resize(6);
	IndirectShadowArgs[0].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[0].CBV.RootParameterIndex = 2;

	IndirectShadowArgs[1].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[1].CBV.RootParameterIndex = 3;

	IndirectShadowArgs[2].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[2].CBV.RootParameterIndex = 4;

	IndirectShadowArgs[3].type = IndirectArgType::Arg_VBV;
	IndirectShadowArgs[4].type = IndirectArgType::Arg_IBV;
	IndirectShadowArgs[5].type = IndirectArgType::Arg_Draw_Indexed;

	TShaderReference<XShadowPerTileProjectionVS> VertexShader = GetGlobalShaderMapping()->GetShader<XShadowPerTileProjectionVS>();
	TShaderReference<XShadowPerTileProjectionPS> PixelShader = GetGlobalShaderMapping()->GetShader<XShadowPerTileProjectionPS>();
	VirtualShadowMapResourece.RHIShadowCommandSignature = RHICreateCommandSignature(IndirectShadowArgs.data(), IndirectShadowArgs.size(), VertexShader.GetVertexShader(), PixelShader.GetPixelShader());


	std::vector<XRHICommandData> RHICmdData;
	RHICmdData.resize(RenderGeos.size());
	for (int i = 0; i < RenderGeos.size(); i++)
	{
		auto& it = RenderGeos[i];
		RHICmdData[i].CBVs.push_back(it->GetPerObjectVertexCBuffer().get());
		RHICmdData[i].CBVs.push_back(VirtualShadowMapResourece.TilesShadowViewProjMatrixCB.get());
		RHICmdData[i].CBVs.push_back(VirtualShadowMapResourece.TilesShadowViewProjMatrixCB.get());

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
	void* DataPtrret = RHIGetCommandDataPtr(RHICmdData, OutCmdDataSize);

	uint32 ShadowPassIndirectBufferDataSize = OutCmdDataSize * (RHICmdData.size() * 12);
	FResourceVectorUint8 ShadowIndirectBufferData;
	ShadowIndirectBufferData.Data = DataPtrret;
	ShadowIndirectBufferData.SetResourceDataSize(ShadowPassIndirectBufferDataSize);
	XRHIResourceCreateData IndirectBufferResourceData(&ShadowIndirectBufferData);
	VirtualShadowMapResourece.ShadowCmdBufferNoCulling = RHIcreateStructBuffer(OutCmdDataSize, ShadowPassIndirectBufferDataSize,
		EBufferUsage(int(EBufferUsage::BUF_DrawIndirect) | (int)EBufferUsage::BUF_ShaderResource), IndirectBufferResourceData);

	VirtualShadowMapResourece.ShadowCmdBufferOffset = RHIGetCmdBufferOffset(VirtualShadowMapResourece.ShadowCmdBufferNoCulling.get());
	VirtualShadowMapResourece.ShadowCounterOffset = AlignArbitrary(ShadowPassIndirectBufferDataSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);

	VirtualShadowMapResourece.ShadowCmdBufferCulled = RHIcreateStructBuffer(ShadowPassIndirectBufferDataSize, VirtualShadowMapResourece.ShadowCounterOffset + sizeof(UINT),
		EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);

	VirtualShadowMapResourece.ShadowNoCullCmdBufferSRV = RHICreateShaderResourceView(VirtualShadowMapResourece.ShadowCmdBufferNoCulling.get());
	VirtualShadowMapResourece.ShadowCulledCmdBufferUAV = RHICreateUnorderedAccessView(VirtualShadowMapResourece.ShadowCmdBufferCulled.get(), true, true, VirtualShadowMapResourece.ShadowCounterOffset);
	VirtualShadowMapResourece.VSMCounterBufferUAV = RHICreateUnorderedAccessView(VirtualShadowMapResourece.VSMCounterBuffer.get(), true, true, 0);
}
