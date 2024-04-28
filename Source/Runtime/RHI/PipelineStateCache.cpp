#include <unordered_map>
#include "Runtime/HAL/Mch.h"
#include "PipelineStateCache.h"

void SetComputePipelineStateFromCS(XRHICommandList& RHICmdList, const XRHIComputeShader* CSShader)
{
	std::shared_ptr <XRHIComputePSO> RHIComputePSO = PipelineStateCache::GetAndOrCreateComputePipelineState(RHICmdList, CSShader);
	XASSERT(RHIComputePSO.get() != nullptr);
	RHICmdList.SetComputePipelineState(RHIComputePSO.get());
}

void SetGraphicsPipelineStateFromPSOInit(XRHICommandList& RHICmdList, const XGraphicsPSOInitializer& Initializer)
{
	std::shared_ptr <XRHIGraphicsPSO> RHIGraphicsPSO = PipelineStateCache::GetAndOrCreateGraphicsPipelineState(RHICmdList, Initializer);
	XASSERT(RHIGraphicsPSO.get() != nullptr);
	RHICmdList.SetGraphicsPipelineState(RHIGraphicsPSO.get());
}

static std::unordered_map<std::size_t, std::shared_ptr<XRHIComputePSO>>GComputePSOMap;
static std::unordered_map<std::size_t, std::shared_ptr<XRHIGraphicsPSO>>GGraphicsPSOMap;
static std::unordered_map<std::size_t, std::shared_ptr<XRHIRayTracingPSO>>GRayTracingPSOMap;
static std::unordered_map<std::size_t, std::shared_ptr<XRHIVertexLayout>>GVertexLayout;

namespace PipelineStateCache
{
	std::shared_ptr<XRHIVertexLayout> GetOrCreateVertexLayout(const XRHIVertexLayoutArray& LayoutArray)
	{
		std::size_t HashIndex = std::hash<std::string>{}(std::string((char*)LayoutArray.data()));
		
		auto iter = GVertexLayout.find(HashIndex);
		if (iter == GVertexLayout.end())
		{
			GVertexLayout[HashIndex] = RHICreateVertexLayout(LayoutArray);
		}

		return GVertexLayout[HashIndex];
	}

	std::shared_ptr <XRHIComputePSO> GetAndOrCreateComputePipelineState(XRHICommandList& RHICmdList, const XRHIComputeShader* ComputeShader)
	{
		std::size_t HashIndex = ComputeShader->GetHash();
		auto iter = GComputePSOMap.find(HashIndex);
		if (iter == GComputePSOMap.end())
		{
			std::shared_ptr <XRHIComputePSO> ComputePSO = RHICreateComputePipelineState(ComputeShader);
			GComputePSOMap[HashIndex] = ComputePSO;
		}
		return GComputePSOMap[HashIndex];
	}

	std::shared_ptr <XRHIGraphicsPSO> GetAndOrCreateGraphicsPipelineState(XRHICommandList& RHICmdList, const XGraphicsPSOInitializer& OriginalInitializer)
	{
		const XGraphicsPSOInitializer* Initializer = &OriginalInitializer;
		std::size_t HashIndex = OriginalInitializer.GetHashIndex();

		auto iter = GGraphicsPSOMap.find(HashIndex);
		if (iter == GGraphicsPSOMap.end())
		{
			std::shared_ptr <XRHIGraphicsPSO> GraphicPSO = RHICreateGraphicsPipelineState(OriginalInitializer);
			GGraphicsPSOMap[HashIndex] = GraphicPSO;
		}
		return GGraphicsPSOMap[HashIndex];
	}

#if RHI_RAYTRACING
	std::shared_ptr<XRHIRayTracingPSO> GetAndOrCreateRayTracingPipelineState(XRHICommandList& RHICmdList, const XRayTracingPipelineStateInitializer& OriginalInitializer)
	{
		const XRayTracingPipelineStateInitializer* Initializer = &OriginalInitializer;
		std::size_t HashIndex = OriginalInitializer.GetHashIndex();

		auto iter = GRayTracingPSOMap.find(HashIndex);
		if (iter == GRayTracingPSOMap.end())
		{
			std::shared_ptr <XRHIRayTracingPSO> GraphicPSO = RHICreateRayTracingPipelineState(OriginalInitializer);
			GRayTracingPSOMap[HashIndex] = GraphicPSO;
		}
		return GRayTracingPSOMap[HashIndex];
	}
#endif
}

