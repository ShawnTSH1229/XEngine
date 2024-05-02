#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "VirtualShadowMapArray.h"
#include "DeferredShadingRenderer.h"

static int GFrameNum = 0;

__declspec(align(256)) struct SShadowViewInfo
{
	XMatrix LighgViewProject;
	XVector3 WorldCameraPosition;
};

class XVirtualShadowMapResource :public XRenderResource
{
public:
	std::shared_ptr<XRHIConstantBuffer> VSMTileShadowViewConstantBuffer;

	std::shared_ptr <XRHIStructBuffer> VirtualShadowMapTileState;
	std::shared_ptr<XRHIUnorderedAcessView> VirtualShadowMapTileStateUAV;


	void InitRHI()override
	{
		uint32 MipNum = VSM_MIP_NUM;
		uint32 TotalTileNum = 0;
		for (uint32 Index = 0; Index < MipNum; Index++)
		{
			uint32 MipTileWidth = (VSM_TILE_MAX_MIP_NUM_XY << (VSM_MIP_NUM - (Index + 1)));
			TotalTileNum += MipTileWidth * MipTileWidth;
		}

		VSMTileShadowViewConstantBuffer = RHICreateConstantBuffer(sizeof(SShadowViewInfo));

		VirtualShadowMapTileState = RHIcreateStructBuffer(sizeof(uint32), sizeof(uint32) * TotalTileNum, EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);
		VirtualShadowMapTileStateUAV = RHICreateUnorderedAccessView(VirtualShadowMapTileState.get(), false, false, 0);
	}
};

TGlobalResource<XVirtualShadowMapResource> VirtualShadowMapResource;

class XVirtualShadowMapTileMark :public XGloablShader
{
public:
	
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapTileMark(Initializer);
	}

	static ShaderInfos StaticShaderInfos;
	
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XVirtualShadowMapTileMark(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		CBView.Bind(Initializer.ShaderParameterMap, "cbView");
		CBShadowViewInfo.Bind(Initializer.ShaderParameterMap, "CbShadowViewInfo");
		SceneDepthTexture.Bind(Initializer.ShaderParameterMap, "SceneDepthTexture");
		VirtualShadowMapTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileState");
	}

	void SetParameters(XRHICommandList& RHICommandList, XRHIConstantBuffer* InputSceneViewBuffer, XRHIConstantBuffer* InputShadowViewInfoBuffer, XRHIUnorderedAcessView* InputVirtualShadowmapTileState, XRHITexture* InputSceneDepthTexture)
	{
		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthTexture, InputSceneDepthTexture);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBView, InputSceneViewBuffer);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBShadowViewInfo, InputShadowViewInfoBuffer);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, InputVirtualShadowmapTileState);
	}

	CBVParameterType CBView;
	CBVParameterType CBShadowViewInfo;
	TextureParameterType SceneDepthTexture;
	UAVParameterType VirtualShadowMapTileState;
};

XVirtualShadowMapTileMark::ShaderInfos XVirtualShadowMapTileMark::StaticShaderInfos("VSMTileMaskCS", GET_SHADER_PATH("VirtualShadowMapTileMark.hlsl"), "VSMTileMaskCS", EShaderType::SV_Compute, XVirtualShadowMapTileMark::CustomConstrucFunc, XVirtualShadowMapTileMark::ModifyShaderCompileSettings);

class XVirtualShadowMapVisualize :public XGloablShader
{
public:

	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapVisualize(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XVirtualShadowMapVisualize(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		CBView.Bind(Initializer.ShaderParameterMap, "cbView");
		CBShadowViewInfo.Bind(Initializer.ShaderParameterMap, "CbShadowViewInfo");
		SceneDepthTexture.Bind(Initializer.ShaderParameterMap, "SceneDepthTexture");
		OutputVisualizeTexture.Bind(Initializer.ShaderParameterMap, "OutputVisualizeTexture");
	}

	void SetParameters(XRHICommandList& RHICommandList, XRHIConstantBuffer* InputSceneViewBuffer, XRHIConstantBuffer* InputShadowViewInfoBuffer, XRHIUnorderedAcessView* InputVisualizeTexture, XRHITexture* InputSceneDepthTexture)
	{
		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthTexture, InputSceneDepthTexture);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBView, InputSceneViewBuffer);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBShadowViewInfo, InputShadowViewInfoBuffer);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, OutputVisualizeTexture, InputVisualizeTexture);
	}

	CBVParameterType CBView;
	CBVParameterType CBShadowViewInfo;
	TextureParameterType SceneDepthTexture;
	UAVParameterType OutputVisualizeTexture;
};
XVirtualShadowMapVisualize::ShaderInfos XVirtualShadowMapVisualize::StaticShaderInfos("VSMVisualizeCS", GET_SHADER_PATH("VirtualShadowMapVisualize.hlsl"), "VSMVisualizeCS", EShaderType::SV_Compute, XVirtualShadowMapVisualize::CustomConstrucFunc, XVirtualShadowMapVisualize::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::VirutalShadowMapSetup()
{

}

void XDeferredShadingRenderer::VirtualShadowMapUpdate()
{
	GFrameNum++;
	float ZPos = sin((GFrameNum % 512) / 512.0 * 3.1415926535 * 2.0) * 60.0 + 60.0;

	std::shared_ptr<GGeomertry> DynamicGeometry = RenderGeos[0];
	DynamicGeometry->SetWorldTranslate(XVector3(1.0, 1.0, ZPos));
}

void XDeferredShadingRenderer::VirtualShadowMapRendering(XRHICommandList& RHICmdList)
{
	SShadowViewInfo VirtualShadowMapInfo;
	VirtualShadowMapInfo.LighgViewProject = LightViewProjMat;
	VirtualShadowMapInfo.WorldCameraPosition = RViewInfo.ViewMats.GetViewOrigin();

	VirtualShadowMapResource.VSMTileShadowViewConstantBuffer->UpdateData(&VirtualShadowMapInfo, sizeof(SShadowViewInfo), 0);

	VirtualShadowMapTileMark(RHICmdList);
}

void XDeferredShadingRenderer::VirtualShadowMapTileMark(XRHICommandList& RHICmdList)
{
	//TODO: Add Resource Transation

	RHICmdList.RHIEventBegin(1, "VSMTileMaskCS", sizeof("VSMTileMaskCS"));
	TShaderReference<XVirtualShadowMapTileMark> VSMTileMaskCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapTileMark>();
	SetComputePipelineStateFromCS(RHICmdList, VSMTileMaskCS.GetComputeShader());
	VSMTileMaskCS->SetParameters(RHICmdList, RViewInfo.ViewConstantBuffer.get(), VirtualShadowMapResource.VSMTileShadowViewConstantBuffer.get(), VirtualShadowMapResource.VirtualShadowMapTileStateUAV.get(), SceneTargets.TextureDepthStencil.get());
	RHICmdList.RHIDispatchComputeShader(static_cast<uint32>((RViewInfo.ViewWidth  + 15 )/ 16), static_cast<uint32>((RViewInfo.ViewHeight + 15) / 16), 1);
	RHICmdList.RHIEventEnd();
}

enum class EVSMVisualizeType
{
	EVSMVIS_TILEMASK,
};

void XDeferredShadingRenderer::VirtualShadowMapVisualize(XRHICommandList& RHICmdList)
{
	XRHITexture* TextureSceneColor = SceneTargets.TextureSceneColorDeffered.get();

	RHICmdList.RHIEventBegin(1, "VSMVisualizeCS", sizeof("VSMVisualizeCS"));
	TShaderReference<XVirtualShadowMapVisualize> VSMVisualizeCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapVisualize>();
	SetComputePipelineStateFromCS(RHICmdList, VSMVisualizeCS.GetComputeShader());
	VSMVisualizeCS->SetParameters(RHICmdList, RViewInfo.ViewConstantBuffer.get(), VirtualShadowMapResource.VSMTileShadowViewConstantBuffer.get(), GetRHIUAVFromTexture(TextureSceneColor), SceneTargets.TextureDepthStencil.get());
	RHICmdList.RHIDispatchComputeShader(static_cast<uint32>((RViewInfo.ViewWidth + 15) / 16), static_cast<uint32>((RViewInfo.ViewHeight + 15) / 16), 1);
	RHICmdList.RHIEventEnd();
}
