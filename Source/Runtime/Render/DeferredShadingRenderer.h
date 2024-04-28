#pragma once
#include "Runtime/RHI/RHICommandList.h"
#include "Runtime/Engine/SceneView.h"
#include "Runtime/RenderCore/VertexFactory.h"
#include "Runtime/Render/SceneRendering.h"
#include "PreDepthPassGPUCulling.h"
#include "SceneRenderTagrget.h"
#include "SkyAtmosPhere.h"
#include "Runtime/Core/Misc/Path.h"
//!!!!!!!!!
#include "Editor/EditorUI.h"

//Temps
#include "Runtime/Core/Mesh/GeomertyData.h"
#include "Runtime/Core/ComponentNode/Camera.h"
//

class XDefaultVertexFactory :public XVertexFactory
{
public:
	void InitRHI()override;
	void ReleaseRHI()override;
};

#define USE_SVOGI 0

class XDeferredShadingRenderer
{
public:
	~XDeferredShadingRenderer();
	void SceneTagetGen();

	void VoxelizationPass(XRHICommandList& RHICmdList);
	void SpaseVoxelOctreeBuild(XRHICommandList& RHICmdList);

	//Width Height CamIns
	void ViewInfoSetup(uint32 Width,uint32 Height,GCamera& CameraIn);
	void ViewInfoUpdate(GCamera& CameraIn);

	void GlobalPerObjectBufferSetup();

	void Setup(
		std::vector<std::shared_ptr<GGeomertry>>& RenderGeosIn,
		XBoundSphere SceneBoundingpSphere,
		XVector3 ShadowLightDirIn,
		XVector3 MainLightColorIn,
		float LightIntensityIn,
		XRHICommandList& RHICmdList);

	void Rendering(XRHICommandList& RHICmdList);
	
	//Pre Depth Pass GPU Culling
	void PreDepthPassGPUCullingSetup();

	void PreDepthPassGPUCulling(XRHICommandList& RHICmdList);
	void PreDepthPassRendering(XRHICommandList& RHICmdList);

	//Virtual Shadow Map Generate
	void VSMSetup();
	void VSMUpdate();

	void VSMTileMaskPass(XRHICommandList& RHICmdList);
	void VSMPageTableGen(XRHICommandList& RHICmdList);
	void VSMShadowCommandBuild(XRHICommandList& RHICmdList);
	void VirtualShadowMapGen(XRHICommandList& RHICmdList);

	//HZBPass
	void HZBPass(XRHICommandList& RHICmdList);

	//SkyAtmosPhere Rendering
	void SkyAtmosPhereUpdata(XRHICommandList& RHICmdList);
	void SkyAtmosPhereRendering(XRHICommandList& RHICmdList);

	//BasePass Rendering
	void BasePassRendering(XRHICommandList& RHICmdList);

	//Shadow Mask Generate
	void ShadowMaskGenerate(XRHICommandList& RHICmdList);
	void VSMTileMaskClear(XRHICommandList& RHICmdList);

	//LightPass
	void LightPass(XRHICommandList& RHICmdList);

	//SkyAtmoSphereCombine
	void SkyAtmoSphereCombine(XRHICommandList& RHICmdList);

	//
	void ConeTracingPass(XRHICommandList& RHICmdList, XRHITexture* TextureSceneColorDest);

	//PostProcessToneMapping
	void PostProcessToneMapping(XRHICommandList& RHICmdList, XRHITexture* TextureSceneColorSrc, XRHITexture* TextureSceneColorDest);

	//PostprocessAA
	void PostprocessAA(XRHICommandList& RHICmdList, XRHITexture* TextureSceneColorSrc, XRHITexture* TextureSceneColorDest);
	
	//
	void TempUIRenderer(XRHICommandList& RHICmdList, XRHITexture* DestTex);

	//
	void PresentPass(XRHICommandList& RHICmdList,XRHITexture* TexSrc);

	//
	void ShadowMapGenPass(XRHICommandList& RHICmdList);

	void SVOInjectLightPass(XRHICommandList& RHICmdList);
private:
	XPreDepthPassResource PreDepthPassResource;
	XSceneRenderTarget SceneTargets;
	XSkyAtmosphereParams cbSkyAtmosphereIns;
	XEditorUI EditorUI;

	std::shared_ptr<XRHIStructBuffer>GlobalObjectStructBuffer;
	std::shared_ptr<XRHIShaderResourceView>GlobalObjectStructBufferSRV;
public:
	XMatrix LightViewMat;
	XMatrix LightProjMat;
	XMatrix LightViewProjMat;

	uint64 FrameNum;

	XBoundSphere SceneBoundingSphere;
	XVector3 ShadowLightDir;

	XVector3 MainLightColor;
	float LightIntensity;

	std::vector<std::shared_ptr<GGeomertry>>RenderGeos;
	
	XDefaultVertexFactory DefaultVertexFactory;
	XSceneView RViewInfo;
};