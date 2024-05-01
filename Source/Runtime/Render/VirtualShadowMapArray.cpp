#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "VirtualShadowMapArray.h"
#include "DeferredShadingRenderer.h"

static int GFrameNum = 0;

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
		CBView.Bind(Initializer.ShaderParameterMap, "TextureSampledInput");
	}

	void SetParameters(XRHICommandList& RHICommandList, XRHIConstantBuffer* InputSceneViewBuffer, XRHIConstantBuffer* InputShadowViewInfoBuffer, XRHIUnorderedAcessView* InputVirtualShadowmapTileState, XRHITexture* InputSceneDepthTexture)
	{

	}

	CBVParameterType CBView;
	CBVParameterType CBShadowViewInfo;
	TextureParameterType SceneDepthTexture;
	UAVParameterType VirtualShadowMapTileState;
};

XVirtualShadowMapTileMark::ShaderInfos XVirtualShadowMapTileMark::StaticShaderInfos("VSMTileMaskCS", GET_SHADER_PATH("VirtualShadowMapTileMark.hlsl"), "VSMTileMaskCS", EShaderType::SV_Compute, XVirtualShadowMapTileMark::CustomConstrucFunc, XVirtualShadowMapTileMark::ModifyShaderCompileSettings);

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




void XDeferredShadingRenderer::VirtualShadowMapRendering()
{

}

void XDeferredShadingRenderer::VirtualShadowMapVisualize()
{
}
