#pragma once

#if !USE_DX12

#include "Runtime\RenderCore\GlobalShader.h"
#include "Runtime\RHI\RHIStaticStates.h"
#include "Runtime\RHI\PipelineStateCache.h"
#include "Runtime\RenderCore\ShaderParameter.h"

struct XBasicRayTracingRay
{
    float Origin[3];
    uint32 Mask;
    float Direction[3];
    float TFar;
};

class XBuildInRGS :public XGloablShader
{
public:
    static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
    {
        return new XBuildInRGS(Initializer);
    }

    static ShaderInfos StaticShaderInfos;
    static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

    XBuildInRGS(const XShaderInitlizer& Initializer)
        :XGloablShader(Initializer)
    {
        TLAS.Bind(Initializer.ShaderParameterMap, "TLAS");
        Rays.Bind(Initializer.ShaderParameterMap, "Rays");
        OcclusionOutput.Bind(Initializer.ShaderParameterMap, "OcclusionOutput");
    }

    void SetParameter(
        XRHICommandList& RHICommandList,
        XRHITexture* InTexture)
    {
    }


    SRVParameterType TLAS;
    SRVParameterType Rays;
    UAVParameterType OcclusionOutput;
};

class XBuildInCHS :public XGloablShader
{
public:
    static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
    {
        return new XBuildInCHS(Initializer);
    }

    static ShaderInfos StaticShaderInfos;
    static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

    XBuildInCHS(const XShaderInitlizer& Initializer)
        :XGloablShader(Initializer)
    {

    }

    void SetParameter(
        XRHICommandList& RHICommandList,
        XRHITexture* InTexture)
    {
    }
};

class XBuildInMSS :public XGloablShader
{
public:
    static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
    {
        return new XBuildInMSS(Initializer);
    }

    static ShaderInfos StaticShaderInfos;
    static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

    XBuildInMSS(const XShaderInitlizer& Initializer)
        :XGloablShader(Initializer)
    {

    }

    void SetParameter(
        XRHICommandList& RHICommandList,
        XRHITexture* InTexture)
    {
    }
};

void DispatchBasicOcclusionRays(XRHICommandList& RHICmdList, XRHIRayTracingScene* Scene, XRHIShaderResourceView* SceneView, XRHIShaderResourceView* RayBufferView, XRHIUnorderedAcessView* ResultView, uint32 NumRays);

#endif