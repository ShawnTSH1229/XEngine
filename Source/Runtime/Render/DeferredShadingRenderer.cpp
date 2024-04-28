#pragma once
#include "DeferredShadingRenderer.h"

void XDefaultVertexFactory::InitRHI()
{
	XRHIVertexLayoutArray LayoutArray;
	LayoutArray.push_back(XVertexElement(0, EVertexElementType::VET_Float4, 0, 0));
	LayoutArray.push_back(XVertexElement(1, EVertexElementType::VET_Float3, 0, 0 + sizeof(XVector4)));
	LayoutArray.push_back(XVertexElement(2, EVertexElementType::VET_Float4, 0, 0 + sizeof(XVector4) + sizeof(XVector3)));
	LayoutArray.push_back(XVertexElement(3, EVertexElementType::VET_Float2, 0, 0 + sizeof(XVector4) + sizeof(XVector3) + sizeof(XVector4)));

	InitLayout(LayoutArray, ELayoutType::Layout_Default);
}

void XDefaultVertexFactory::ReleaseRHI()
{
	DefaultLayout.reset();
}

XDeferredShadingRenderer::~XDeferredShadingRenderer()
{
	EditorUI.ImGui_Impl_RHI_Shutdown();
}

void XDeferredShadingRenderer::ViewInfoSetup(uint32 Width, uint32 Height, GCamera& CameraIn)
{
	RViewInfo.ViewConstantBuffer = RHICreateConstantBuffer(sizeof(ViewConstantBufferData));
	RViewInfo.ViewWidth = Width;
	RViewInfo.ViewHeight = Height;
	RViewInfo.ViewMats.Create(CameraIn.GetProjectMatrix(), CameraIn.GetEyePosition(), CameraIn.GetTargetPosition());
}

void XDeferredShadingRenderer::ViewInfoUpdate(GCamera& CameraIn)
{
	RViewInfo.ViewMats.UpdateViewMatrix(CameraIn.GetEyePosition(), CameraIn.GetTargetPosition());
	float l = SceneBoundingSphere.Center.x - SceneBoundingSphere.Radius;
	float r = SceneBoundingSphere.Center.x + SceneBoundingSphere.Radius;
	float t = SceneBoundingSphere.Center.y + SceneBoundingSphere.Radius;
	float b = SceneBoundingSphere.Center.y - SceneBoundingSphere.Radius;
	float f = SceneBoundingSphere.Radius * 2;
	float n = 0.1f;

	ShadowLightDir.Normalize();
	XVector3 LightPos = SceneBoundingSphere.Center + ShadowLightDir * SceneBoundingSphere.Radius * 1.0;
	XVector3 UpTemp(0, 1, 0);

	LightViewMat = XMatrix::CreateMatrixLookAtLH(LightPos, SceneBoundingSphere.Center, UpTemp);
	LightProjMat = XMatrix::CreateOrthographicOffCenterLH(l, r, b, t, f, n);
	LightViewProjMat = LightViewMat * LightProjMat;

	FrameNum++;
	ShadowLightDir.Normalize();

	RViewInfo.ViewCBCPUData.StateFrameIndexMod8 = FrameNum % 8;
	RViewInfo.ViewCBCPUData.ViewSizeAndInvSize = XVector4(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1.0 / RViewInfo.ViewWidth, 1.0 / RViewInfo.ViewHeight);
	RViewInfo.ViewCBCPUData.AtmosphereLightDirection = XVector4(ShadowLightDir.x, ShadowLightDir.y, ShadowLightDir.z, 1.0f);
	RViewInfo.ViewCBCPUData.ViewToClip = RViewInfo.ViewMats.GetProjectionMatrix();
	RViewInfo.ViewCBCPUData.TranslatedViewProjectionMatrix = RViewInfo.ViewMats.GetTranslatedViewProjectionMatrix();
	RViewInfo.ViewCBCPUData.ScreenToWorld = RViewInfo.ViewMats.GetScreenToWorld();
	RViewInfo.ViewCBCPUData.ScreenToTranslatedWorld = RViewInfo.ViewMats.GetScreenToTranslatedWorld();
	RViewInfo.ViewCBCPUData.InvDeviceZToWorldZTransform = CreateInvDeviceZToWorldZTransform(RViewInfo.ViewMats.GetProjectionMatrix());
	RViewInfo.ViewCBCPUData.WorldCameraOrigin = RViewInfo.ViewMats.GetViewOrigin();
	RViewInfo.ViewCBCPUData.ViewProjectionMatrix = RViewInfo.ViewMats.GetViewProjectionMatrix();
	RViewInfo.ViewCBCPUData.ViewProjectionMatrixInverse = RViewInfo.ViewMats.GetViewProjectionMatrixInverse();
	RViewInfo.ViewCBCPUData.BufferSizeAndInvSize = XVector4(RViewInfo.ViewWidth, RViewInfo.ViewHeight, 1.0f / RViewInfo.ViewWidth, 1.0f / RViewInfo.ViewHeight);

	RViewInfo.ViewConstantBuffer.get()->UpdateData(&RViewInfo.ViewCBCPUData, sizeof(ViewConstantBufferData), 0);
}

struct GlobalPerObjectConstants
{
	XMatrix World;
	XVector3 BoundingBoxCenter;
	float padding0 = 1.0f;
	XVector3 BoundingBoxExtent;
	float padding1 = 1.0f;
};

void XDeferredShadingRenderer::GlobalPerObjectBufferSetup()
{
	uint32 ObjConstVecSize = sizeof(GlobalPerObjectConstants) * RenderGeos.size();
	GlobalPerObjectConstants* ConstantArray = (GlobalPerObjectConstants*)std::malloc(ObjConstVecSize);

	for (int i = 0; i < RenderGeos.size(); i++)
	{
		auto& RG = RenderGeos[i];

		XBoundingBox BoudingBoxTans = RG->GetBoudingBoxWithTrans();
		ConstantArray[i].BoundingBoxCenter = BoudingBoxTans.Center ;
		ConstantArray[i].BoundingBoxExtent = BoudingBoxTans.Extent;
		ConstantArray[i].World = RG->GetWorldTransform().GetCombineMatrix();
	}

	FResourceVectorUint8 ObjectStructBufferData;
	ObjectStructBufferData.Data = ConstantArray;
	ObjectStructBufferData.SetResourceDataSize(ObjConstVecSize);
	XRHIResourceCreateData ObjectStructBufferResourceData(&ObjectStructBufferData);

	GlobalObjectStructBuffer = RHIcreateStructBuffer(sizeof(VertexCBufferStruct), ObjConstVecSize,
		EBufferUsage((int)EBufferUsage::BUF_ShaderResource), ObjectStructBufferResourceData);

	GlobalObjectStructBufferSRV = RHICreateShaderResourceView(GlobalObjectStructBuffer.get());
}

void XDeferredShadingRenderer::Setup(
	std::vector<std::shared_ptr<GGeomertry>>& RenderGeosIn,
	XBoundSphere SceneBoundingpSphere,
	XVector3 ShadowLightDirIn,
	XVector3 MainLightColorIn,
	float LightIntensityIn,
	XRHICommandList& RHICmdList)
{
	FrameNum = 0;
	SceneBoundingSphere = SceneBoundingpSphere;
	ShadowLightDir = ShadowLightDirIn;
	MainLightColor = MainLightColorIn;
	LightIntensity = LightIntensityIn;
	RenderGeos = RenderGeosIn;

	GlobalPerObjectBufferSetup();
	BeginInitResource(&DefaultVertexFactory);
	SceneTagetGen();
	PreDepthPassGPUCullingSetup();
	VSMSetup();

	

	EditorUI.SetDefaltStyle();
	EditorUI.InitIOInfo(RViewInfo.ViewWidth, RViewInfo.ViewHeight);
	EditorUI.ImGui_Impl_RHI_Init();
}

void XDeferredShadingRenderer::Rendering(XRHICommandList& RHICmdList)
{
	SkyAtmosPhereUpdata(RHICmdList);

	RHICmdList.RHIBeginFrame();

#if USE_SVOGI
	VoxelizationPass(RHICmdList);
	SpaseVoxelOctreeBuild(RHICmdList);
#endif
	{
		PreDepthPassGPUCulling(RHICmdList);
		PreDepthPassRendering(RHICmdList);
		HZBPass(RHICmdList);
	}
	{
		VSMUpdate();
		VSMTileMaskPass(RHICmdList);
		VSMPageTableGen(RHICmdList);
		VSMShadowCommandBuild(RHICmdList);
		VirtualShadowMapGen(RHICmdList);
	}
#if USE_SVOGI
	ShadowMapGenPass(RHICmdList);
	SVOInjectLightPass(RHICmdList);
#endif
	SkyAtmosPhereRendering(RHICmdList);
	BasePassRendering(RHICmdList);
	ShadowMaskGenerate(RHICmdList);
	VSMTileMaskClear(RHICmdList);
	LightPass(RHICmdList);
	SkyAtmoSphereCombine(RHICmdList);
#if USE_SVOGI
	ConeTracingPass(RHICmdList, SceneTargets.TextureSceneColorDeffered.get());
#endif
	//0 -> r , 1 -> w
	XRHITexture* PingPongTex[2] = { SceneTargets.TextureSceneColorDeffered.get() ,SceneTargets.TextureSceneColorDefferedPingPong.get() };
	int32 PingPongindex = 0;
	{
		PostProcessToneMapping(RHICmdList, PingPongTex[PingPongindex % 2], PingPongTex[(PingPongindex + 1) % 2]);
		PingPongindex++;
		PostprocessAA(RHICmdList, PingPongTex[PingPongindex % 2], PingPongTex[(PingPongindex + 1) % 2]);

		TempUIRenderer(RHICmdList, PingPongTex[(PingPongindex + 1) % 2]);
		PresentPass(RHICmdList, PingPongTex[(PingPongindex + 1) % 2]);
	}
	RHICmdList.RHIEndFrame();
}


