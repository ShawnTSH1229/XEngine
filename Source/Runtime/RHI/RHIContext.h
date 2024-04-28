#pragma once
#include "Runtime/HAL/PlatformTypes.h"
#include "RHIResource.h"
#include "RHIDefines.h"
#include "RHITransitionInfo.h"
#include <span>

#if RHI_RAYTRACING
struct XRayTracingGeometryBuildParams
{
	std::shared_ptr<XRHIRayTracingGeometry> Geometry;
	EAccelerationStructureBuildMode BuildMode;
};

struct XRayTracingSceneBuildParams
{
	XRHIRayTracingScene* Scene = nullptr;

	//XRHIBuffer* ResultBuffer = nullptr;

	XRHIBuffer* ScratchBuffer = nullptr;
	uint32 ScratchBufferOffset = 0;

	XRHIBuffer* InstanceBuffer = nullptr;
	uint32 InstanceBufferOffset = 0;
};

struct XRayTracingShaderBinds
{
	XRHITexture* Textures[64] = {};
	XRHIShaderResourceView* SRVs[64] = {};
	XRHIConstantBuffer* ConstatntBuffers[16] = {};
	XRHISamplerState* Samplers[16] = {};
	XRHIUnorderedAcessView* UAVs[16] = {};
};
#endif

class IRHIContext
{
public:
	virtual void RHIEndFrame() = 0;
	virtual void ReseizeViewport(uint32 Width, uint32 Height) = 0;
	virtual void Execute() = 0;
	virtual void OpenCmdList() = 0;
	virtual void CloseCmdList() = 0;

	virtual void RHIBeginFrame() = 0;

	//SetShaderParameter
	virtual void RHISetShaderUAV(EShaderType ShaderType, uint32 TextureIndex, XRHIUnorderedAcessView* UAV) = 0;
	virtual void RHISetShaderTexture(EShaderType ShaderType, uint32 TextureIndex, XRHITexture* NewTextureRHI) = 0;
	virtual void RHISetShaderSRV(EShaderType ShaderType, uint32 SRVIndex, XRHIShaderResourceView* SRV) = 0;
	virtual void RHISetShaderConstantBuffer(EShaderType ShaderType, uint32 BufferIndex, XRHIConstantBuffer* RHIConstantBuffer) = 0;
	virtual void SetShaderValue(EShaderType ShaderType, uint32 BufferIndex, uint32 VariableOffsetInBuffer, uint32 NumBytes, const void* NewValue) = 0;

	//DrawCall/DisPatch
	virtual void RHIEventBegin(uint32 Metadata, const void* pData, uint32 Size) = 0;
	virtual void RHIEndRenderPass() = 0;
	virtual void RHIEventEnd() = 0;
	virtual void RHIExecuteIndirect(XRHICommandSignature* RHICmdSig, uint32 CmdCount, XRHIStructBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, XRHIStructBuffer* CountBuffer, uint64 CountBufferOffset) = 0;
	virtual void RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) = 0;
	virtual void RHIDrawIndexedPrimitive(XRHIBuffer* IndexBuffer, uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) = 0;

	//SetPSO
	virtual void RHISetGraphicsPipelineState(XRHIGraphicsPSO* GraphicsState) = 0;
	virtual void RHISetComputePipelineState(XRHIComputePSO* ComputeState) = 0;

	//SetVB IB
	virtual void SetVertexBuffer(XRHIBuffer* RHIVertexBuffer, uint32 VertexBufferSlot, uint32 OffsetFormVBBegin) = 0;

	//Misc
	virtual void RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ) = 0;
	virtual void RHIBeginRenderPass(const XRHIRenderPassInfo& InInfo,const char* InName, uint32 Size) = 0;
	virtual void SetRenderTargetsAndViewPort(uint32 NumRTs, const XRHIRenderTargetView* RTViews,const XRHIDepthStencilView* DSView) = 0;

	//Trasition Temporary
	virtual void Transition(XRHITransitionInfo TransitionInfo) { __debugbreak(); };

#if RHI_RAYTRACING
	virtual void RHIBuildAccelerationStructures(const std::span<const XRayTracingGeometryBuildParams> Params, const XRHIBufferRange& ScratchBufferRange) {};
	virtual void RHIBuildAccelerationStructure(const XRayTracingSceneBuildParams& SceneBuildParams) {};
	virtual void BindAccelerationStructureMemory(XRHIRayTracingScene* Scene, std::shared_ptr<XRHIBuffer> Buffer, uint32 BufferOffset) {};
	virtual void RayTraceDispatch(XRHIRayTracingPSO* Pipeline, XRHIRayTracingShader* RayGenShader, XRHIRayTracingScene* Scene, const XRayTracingShaderBinds& GlobalResourceBindings, uint32 Width, uint32 Height) {};
#endif
};