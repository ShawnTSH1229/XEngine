#pragma once
#include <vulkan\vulkan_core.h>
#include <Runtime\HAL\Mch.h>
#include "Runtime/RHI/RHIContext.h"
#include "VulkanBarriers.h"

class XVulkanCommandBufferManager;
class XVulkanPlatformRHI;
class XVulkanDevice;
class XVulkanQueue;
class XVulkanPendingGfxState;
class XVulkanCommandListContext :public IRHIContext
{
public:
	XVulkanCommandListContext(XVulkanPlatformRHI* InRHI, XVulkanDevice* InDevice, XVulkanQueue* InQueue);
	~XVulkanCommandListContext();

	void ReseizeViewport(uint32 Width, uint32 Height)override { XASSERT(false)};

	void Execute()override;
	void OpenCmdList()override;
	void CloseCmdList()override;

	//SetShaderParameter
	void RHISetShaderUAV(EShaderType ShaderType, uint32 TextureIndex, XRHIUnorderedAcessView* UAV)override { XASSERT(false) };
	void RHISetShaderTexture(EShaderType ShaderType, uint32 TextureIndex, XRHITexture* NewTextureRHI)override;
	void RHISetShaderSRV(EShaderType ShaderType, uint32 SRVIndex, XRHIShaderResourceView* SRV)override { XASSERT(false) };
	void RHISetShaderConstantBuffer(EShaderType ShaderType, uint32 BufferIndex, XRHIConstantBuffer* RHIConstantBuffer)override;
	void SetShaderValue(EShaderType ShaderType, uint32 BufferIndex, uint32 VariableOffsetInBuffer, uint32 NumBytes, const void* NewValue)override { XASSERT(false) };

	//SetPSO
	void RHISetGraphicsPipelineState(XRHIGraphicsPSO* GraphicsState)override;
	void RHISetComputePipelineState(XRHIComputePSO* ComputeState)override { XASSERT(false) };

	void RHIExecuteIndirect(XRHICommandSignature* RHICmdSig, uint32 CmdCount, XRHIStructBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, XRHIStructBuffer* CountBuffer, uint64 CountBufferOffset)override { XASSERT(false) };
	void RHIDrawIndexedPrimitive(XRHIBuffer* IndexBuffer, uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation) final override;
	void RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) final override { XASSERT(false) };

	//SetVB IB RHISetStreamSource
	void SetVertexBuffer(XRHIBuffer* RHIVertexBuffer, uint32 VertexBufferSlot, uint32 OffsetFormVBBegin) final override;

	//Misc
	void RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
	void SetRenderTargetsAndViewPort(uint32 NumRTs, const XRHIRenderTargetView* RTViews, const XRHIDepthStencilView* DSView)override { XASSERT(false) };

	//DrawCall/DisPatch
	void RHIEventBegin(uint32 Metadata, const void* pData, uint32 Size)override { XASSERT(false) };
	void RHIEventEnd()override { XASSERT(false) };
	void RHIEndFrame()override;
	void RHIBeginFrame()override { XASSERT(false) };
	void RHIEndRenderPass()override;
	void RHIBeginRenderPass(const XRHIRenderPassInfo& InInfo, const char* InName, uint32 Size)override;
	void Transition(XRHITransitionInfo TransitionInfo);

#if RHI_RAYTRACING
	void RHIBuildAccelerationStructures(const std::span<const XRayTracingGeometryBuildParams> Params, const XRHIBufferRange& ScratchBufferRange);
	void BindAccelerationStructureMemory(XRHIRayTracingScene* Scene, std::shared_ptr<XRHIBuffer> Buffer, uint32 BufferOffset);
	void RHIBuildAccelerationStructure(const XRayTracingSceneBuildParams& SceneBuildParams);
	void RayTraceDispatch(XRHIRayTracingPSO* Pipeline, XRHIRayTracingShader* RayGenShader, XRHIRayTracingScene* Scene, const XRayTracingShaderBinds& GlobalResourceBindings, uint32 Width, uint32 Height);
#endif

	//Vulkan
	XVulkanQueue* GetQueue() { return Queue; }
	XVulkanRenderPass* PrepareRenderPassForPSOCreation(const XGraphicsPSOInitializer& Initializer);
	XVulkanCommandBufferManager* GetCommandBufferManager() { return CmdBufferManager; }

	inline XVulkanLayoutManager& GetLayoutManager()
	{
		return GlobalLayoutManager;
	}

	XVulkanPendingGfxState* GetPendingGfxState()
	{
		return PendingGfxState;
	}
private:
	

	friend class VkHack;

	XVulkanPlatformRHI* RHI;
	XVulkanDevice* Device;
	XVulkanQueue* Queue;
	XVulkanCommandBufferManager* CmdBufferManager;

	XVulkanPendingGfxState* PendingGfxState;

	static XVulkanLayoutManager GlobalLayoutManager;
};