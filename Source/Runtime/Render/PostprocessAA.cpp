#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/CommonRenderRresource.h"
#include "Runtime/RenderCore/Shader.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

struct XPostprocessAAParams
{
    XVector2 Input_ExtentInverse;
};

class XPostprocessAAResourece :public XRenderResource
{
public:
    std::shared_ptr<XRHIConstantBuffer>RHICbPostprocessAA;

    void InitRHI()override
    {
        RHICbPostprocessAA = RHICreateConstantBuffer(sizeof(XSkyAtmosphereParams));
    }
};

class XFXAAPassPS :public XGloablShader
{
public:
    static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
    {
        return new XFXAAPassPS(Initializer);
    }

    static ShaderInfos StaticShaderInfos;

    static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
    XFXAAPassPS(const XShaderInitlizer& Initializer)
        :XGloablShader(Initializer)
    {
        FullScreenMap.Bind(Initializer.ShaderParameterMap, "FullScreenMap");
        cbPostprocessAA.Bind(Initializer.ShaderParameterMap, "cbPostprocessFXAA");
    }

    void SetParameter(
        XRHICommandList& RHICommandList,
        XRHITexture* InTexture,
        XRHIConstantBuffer* IncbPostprocessAA)
    {
        SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, FullScreenMap, InTexture);
        SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, cbPostprocessAA, IncbPostprocessAA);
    }
    TextureParameterType    FullScreenMap;
    CBVParameterType cbPostprocessAA;
};

TGlobalResource<XPostprocessAAResourece>PostprocessAAResourece;

XFXAAPassPS::ShaderInfos XFXAAPassPS::StaticShaderInfos(
    "XFXAAPassPS", GET_SHADER_PATH("FXAAShader.hlsl"),
    "FXAA_PS", EShaderType::SV_Pixel, XFXAAPassPS::CustomConstrucFunc,
    XFXAAPassPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::PostprocessAA(XRHICommandList& RHICmdList, XRHITexture* TextureSceneColorSrc,
                                             XRHITexture* TextureSceneColorDest)
{
    XPostprocessAAParams PostprocessAAParams;
    PostprocessAAParams.Input_ExtentInverse = XVector2(1.0f/RViewInfo.ViewWidth,1.0f/RViewInfo.ViewHeight);
    PostprocessAAResourece.RHICbPostprocessAA->UpdateData(&PostprocessAAParams, sizeof(XPostprocessAAParams), 0);
    
    XRHIRenderPassInfo RPInfos(1, &TextureSceneColorDest, ERenderTargetLoadAction::ELoad, nullptr, EDepthStencilLoadAction::ENoAction);
    RHICmdList.RHIBeginRenderPass(RPInfos, "PostProcess_AA", sizeof("PostProcess_AA"));
    RHICmdList.CacheActiveRenderTargets(RPInfos);

    XGraphicsPSOInitializer GraphicsPSOInit;
    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
    GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
    GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

    TShaderReference<RFullScreenQuadVS> VertexShader = GetGlobalShaderMapping()->GetShader<RFullScreenQuadVS>();
    TShaderReference<XFXAAPassPS> PixelShader = GetGlobalShaderMapping()->GetShader<XFXAAPassPS>();
    GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
    GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GFullScreenLayout.RHIVertexLayout.get();

    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
    SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);
    PixelShader->SetParameter(RHICmdList, TextureSceneColorSrc,PostprocessAAResourece.RHICbPostprocessAA.get());

    RHICmdList.SetVertexBuffer(GFullScreenVertexRHI.RHIVertexBuffer.get(), 0, 0);
    RHICmdList.RHIDrawIndexedPrimitive(GFullScreenIndexRHI.RHIIndexBuffer.get(), 6, 1, 0, 0, 0);
    RHICmdList.RHIEndRenderPass();
}
