
#include "RayTracingBasicShaders.h"

#if !USE_DX12
#include "Runtime/Core/MainInit.h"
#include "Runtime/Engine/ResourcecConverter.h"
#include "Runtime/Core/ComponentNode/GameTimer.h"

#include "Runtime/ApplicationCore/Windows/WindowsApplication.h"
#include "Runtime/ApplicationCore/GlfwApp/GlfwApplication.h"

#include <fstream>
#include "Runtime\VulkanRHI\VulkanResource.h"
#include <Runtime\VulkanRHI\VulaknHack.h>
#include "Runtime\VulkanRHI\VulkanResource.h"


#include <Runtime\RenderCore\ShaderParameter.h>

#include "Runtime/Render/DeferredShadingRenderer.h"
#include "Runtime\Render\RayTracingInstanceBufferUtil.h"

XBuildInRGS::ShaderInfos XBuildInRGS::StaticShaderInfos(
    "XBuildInRGS", GET_SHADER_PATH("VulkanShaderTest/RayTracingTestShader.hlsl"),
    "RayGenMain", EShaderType::SV_RayGen, XBuildInRGS::CustomConstrucFunc,
    XBuildInRGS::ModifyShaderCompileSettings);

XBuildInCHS::ShaderInfos XBuildInCHS::StaticShaderInfos(
    "XBuildInCHS", GET_SHADER_PATH("VulkanShaderTest/RayTracingTestShader.hlsl"),
    "ClostHitMain", EShaderType::SV_HitGroup, XBuildInCHS::CustomConstrucFunc,
    XBuildInCHS::ModifyShaderCompileSettings);

XBuildInMSS::ShaderInfos XBuildInMSS::StaticShaderInfos(
    "XBuildInMSS", GET_SHADER_PATH("VulkanShaderTest/RayTracingTestShader.hlsl"),
    "MissMain", EShaderType::SV_RayMiss, XBuildInMSS::CustomConstrucFunc,
    XBuildInMSS::ModifyShaderCompileSettings);

struct XBasicRayTracingPipeline
{
    std::shared_ptr< XRHIRayTracingPSO> PipelineState = nullptr;
    TShaderReference<XBuildInRGS>OcclusionRGS;
};

XBasicRayTracingPipeline GetBasicRayTracingPipeline(XRHICommandList& RHICmdList)
{
    auto* ShaderMap = GetGlobalShaderMapping();

    XRayTracingPipelineStateInitializer PipelineInitializer;

    TShaderReference<XBuildInRGS> OcclusionRGS = ShaderMap->GetShader<XBuildInRGS>();
    TShaderReference<XBuildInCHS> CloseHitShader = ShaderMap->GetShader<XBuildInCHS>();
    TShaderReference<XBuildInMSS> MissShader = ShaderMap->GetShader<XBuildInMSS>();

    std::vector<std::shared_ptr<XRHIRayTracingShader>>  RayGenShaderTable;
    RayGenShaderTable.push_back(std::shared_ptr<XRHIRayTracingShader>(OcclusionRGS.GetRayTracingShader()));
    PipelineInitializer.SetRayGenShaderTable(RayGenShaderTable);

    std::vector<std::shared_ptr<XRHIRayTracingShader>>  ClostHitShaderTable;
    ClostHitShaderTable.push_back(std::shared_ptr<XRHIRayTracingShader>(CloseHitShader.GetRayTracingShader()));
    PipelineInitializer.SetHitGroupShaderTable(ClostHitShaderTable);

    std::vector<std::shared_ptr<XRHIRayTracingShader>>  RayMissShaderTable;
    RayMissShaderTable.push_back(std::shared_ptr<XRHIRayTracingShader>(MissShader.GetRayTracingShader()));
    PipelineInitializer.SetMissShaderTable(RayMissShaderTable);

    XBasicRayTracingPipeline Res;
    Res.PipelineState = PipelineStateCache::GetAndOrCreateRayTracingPipelineState(RHICmdList, PipelineInitializer);
    Res.OcclusionRGS = OcclusionRGS;
    return Res;
}

void DispatchBasicOcclusionRays(XRHICommandList& RHICmdList, XRHIRayTracingScene* Scene, XRHIShaderResourceView* SceneView, XRHIShaderResourceView* RayBufferView, XRHIUnorderedAcessView* ResultView, uint32 NumRays)
{
    auto RayTracingPipeline = GetBasicRayTracingPipeline(RHICmdList);

    XRayTracingShaderBindsWriter GlobalResources;

    std::vector<XRHIShaderResourceView*> SRVs;
    SRVs.push_back(SceneView);
    SRVs.push_back(RayBufferView);

    std::vector<XRHIUnorderedAcessView*> UAVs;
    UAVs.push_back(ResultView);

    SetShaderParameters(GlobalResources, RayTracingPipeline.OcclusionRGS->ShaderParameterMap, SRVs, UAVs);
    RHICmdList.RayTraceDispatch(RayTracingPipeline.PipelineState.get(), RayTracingPipeline.OcclusionRGS.GetRayTracingShader(), Scene, GlobalResources, NumRays, 1);
}
#endif