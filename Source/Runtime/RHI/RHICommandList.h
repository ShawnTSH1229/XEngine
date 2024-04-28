#pragma once
#include "PlatformRHI.h"
#include "RHIContext.h"
#include "RHITransitionInfo.h"
#include <array>
class XRHICommandListBase
{
protected:
	struct FPSOContext
	{
		uint32 CachedNumRTs = 0;
		XRHIDepthStencilView CachedDepthStencilTarget;
		std::array<XRHIRenderTargetView, 8>CachedRenderTargets;
	} PSOContext;

public:
	virtual void Open() = 0;
	virtual void Close() = 0;

	inline void SetContext(IRHIContext* InContext)
	{
		Context = InContext;
		ComputeContext = InContext;
	}

	inline IRHIContext* GetContext()const
	{
		return Context;
	}

	inline void SetComputeContext(IRHIContext* Context)
	{
		ComputeContext = Context;
	}

	inline IRHIContext* GetComputeContext()const
	{
		return ComputeContext;
	}

	void CacheActiveRenderTargets(
		uint32 NewNumRTs,
		const XRHIRenderTargetView* NewRenderTargetsRHI,
		const XRHIDepthStencilView* NewDepthStencilRHI)
	{
		PSOContext.CachedNumRTs = NewNumRTs;
		for (int i = 0; i < NewNumRTs; i++)
		{
			PSOContext.CachedRenderTargets[i] = NewRenderTargetsRHI[i];
		}
		PSOContext.CachedDepthStencilTarget = (NewDepthStencilRHI == nullptr ? XRHIDepthStencilView() : *NewDepthStencilRHI);
	}

	void CacheActiveRenderTargets(const XRHIRenderPassInfo& Info)
	{
		XRHISetRenderTargetsInfo RTInfo;
		Info.ConvertToRenderTargetsInfo(RTInfo);
		CacheActiveRenderTargets(RTInfo.NumColorRenderTargets, RTInfo.ColorRenderTarget, &RTInfo.DepthStencilRenderTarget);
	}

	void ApplyCachedRenderTargets(XGraphicsPSOInitializer& GraphicsPSOInit)
	{
		GraphicsPSOInit.RTNums = PSOContext.CachedNumRTs;
		for (uint32 i = 0; i < PSOContext.CachedNumRTs; i++)
		{
			GraphicsPSOInit.RT_Format[i] = PSOContext.CachedRenderTargets[i].Texture->GetFormat();
		}

		if (PSOContext.CachedDepthStencilTarget.Texture != nullptr)
		{
			GraphicsPSOInit.DS_Format = PSOContext.CachedDepthStencilTarget.Texture->GetFormat();
		}
		else
		{
			GraphicsPSOInit.DS_Format = EPixelFormat::FT_Unknown;
		}
	}

private:
	IRHIContext* Context;
	IRHIContext* ComputeContext;
};

class XRHIComputeCommandList :public XRHICommandListBase
{
public:
	void Open()override
	{
		GetComputeContext()->OpenCmdList();
	}

	void Close()override
	{
		GetComputeContext()->CloseCmdList();
	}

	inline void RHIEventBegin(uint32 Metadata, const void* pData, uint32 Size)
	{
		GetComputeContext()->RHIEventBegin(Metadata, pData, Size);
	}

	inline void RHIEventEnd()
	{
		GetComputeContext()->RHIEventEnd();
	}

	inline void RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
		GetComputeContext()->RHIDispatchComputeShader(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	inline void SetComputePipelineState(class XRHIComputePSO* ComputesPipelineState)
	{
		GetComputeContext()->RHISetComputePipelineState(ComputesPipelineState);
	}

	inline void SetConstantBuffer(EShaderType ShaderType, uint32 BufferIndex, XRHIConstantBuffer* RHICBV)
	{
		GetComputeContext()->RHISetShaderConstantBuffer(ShaderType, BufferIndex, RHICBV);
	}

	inline void SetShaderTexture(EShaderType ShaderType, uint32 TextureIndex, XRHITexture* Texture)
	{
		GetComputeContext()->RHISetShaderTexture(ShaderType, TextureIndex, Texture);
	}

	inline void SetShaderUAV(EShaderType ShaderType, uint32 TextureIndex, XRHIUnorderedAcessView* RHIUAV)
	{
		GetComputeContext()->RHISetShaderUAV(ShaderType, TextureIndex, RHIUAV);
	}

	inline void SetShaderSRV(EShaderType ShaderType, uint32 SRVIndex, XRHIShaderResourceView* SRV)
	{
		GetComputeContext()->RHISetShaderSRV(ShaderType, SRVIndex, SRV);
	}
};

inline std::shared_ptr<XRHIBuffer>RHICreateBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateBuffer(Stride, Size, Usage, ResourceData);

}
class XRHICommandList : public XRHIComputeCommandList
{
public:
	void Open()override
	{
		GetContext()->OpenCmdList();
	}

	void Close()override
	{
		GetContext()->CloseCmdList();
	}

	inline void RHIBeginFrame()
	{
		GetContext()->RHIBeginFrame();
	}
	
	inline void RHIEndFrame()
	{
		GetContext()->RHIEndFrame();
	}

	inline void RHIEndRenderPass()
	{
		GetContext()->RHIEndRenderPass();
	}

	inline void RHIBeginRenderPass(const XRHIRenderPassInfo& InInfo, const char* InName, uint32 Size)
	{
		GetContext()->RHIBeginRenderPass(InInfo, InName, Size);
	}

	inline void RHIExecuteIndirect(
		XRHICommandSignature* RHICmdSig,uint32 CmdCount,
		XRHIStructBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset,
		XRHIStructBuffer* CountBuffer, uint64 CountBufferOffset)
	{
		GetContext()->RHIExecuteIndirect(RHICmdSig, CmdCount, ArgumentBuffer, ArgumentBufferOffset, CountBuffer, CountBufferOffset);
	}

	inline void RHIDrawIndexedPrimitive(
		XRHIBuffer* IndexBuffer,
		uint32 IndexCountPerInstance,
		uint32 InstanceCount,
		uint32 StartIndexLocation,
		uint32 BaseVertexLocation,
		uint32 StartInstanceLocation)
	{
		GetContext()->RHIDrawIndexedPrimitive(IndexBuffer, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	inline void SetGraphicsPipelineState(class XRHIGraphicsPSO* GraphicsPipelineState)
	{
		GetContext()->RHISetGraphicsPipelineState(GraphicsPipelineState);
	}

	inline void SetConstantBuffer(EShaderType ShaderType, uint32 BufferIndex, XRHIConstantBuffer* RHICBV)
	{
		GetContext()->RHISetShaderConstantBuffer(ShaderType, BufferIndex, RHICBV);
	}

	inline void SetShaderValue(EShaderType ShaderType, uint32 BufferIndex,uint32 VariableOffsetInBuffer, uint32 NumBytes, const void* NewValue)
	{
		GetContext()->SetShaderValue(ShaderType, BufferIndex, VariableOffsetInBuffer, NumBytes, NewValue);
	}

	inline void SetShaderTexture(EShaderType ShaderType, uint32 TextureIndex, XRHITexture* Texture)
	{
		GetContext()->RHISetShaderTexture(ShaderType, TextureIndex, Texture);
	}

	inline void SetVertexBuffer(XRHIBuffer* RHIVertexBuffer, uint32 VertexBufferSlot, uint32 OffsetFormVBBegin)
	{
		GetContext()->SetVertexBuffer(RHIVertexBuffer, VertexBufferSlot, OffsetFormVBBegin);
	}

	inline void SetShaderSRV(EShaderType ShaderType, uint32 SRVIndex, XRHIShaderResourceView* SRV)
	{
		GetContext()->RHISetShaderSRV(ShaderType, SRVIndex, SRV);
	}

	inline void ReseizeViewport(uint32 Width, uint32 Height)
	{
		GetContext()->ReseizeViewport(Width, Height);
	}

	inline void Execute() 
	{
		GetContext()->Execute();
	}

#if RHI_RAYTRACING
	inline void BuildAccelerationStructure(std::shared_ptr<XRHIRayTracingGeometry> Geometry)
	{
		XRayTracingGeometryBuildParams Params;
		Params.Geometry = Geometry;
		Params.BuildMode = EAccelerationStructureBuildMode::Build;

		std::vector< XRayTracingGeometryBuildParams>ParamVec;
		ParamVec.push_back(Params);
		const std::span<const XRayTracingGeometryBuildParams> ParamSpan = { ParamVec };

		XRHIResourceCreateData ResourceData;

		XRHIBufferRange ScratchBufferRanfge{};
		ScratchBufferRanfge.Buffer = RHICreateBuffer(0, Geometry->GetSizeInfo().BuildScratchSize, EBufferUsage::BUF_AccelerationStructure, ResourceData).get();
		
		GetContext()->RHIBuildAccelerationStructures(ParamSpan, ScratchBufferRanfge);
	}

	inline void BindAccelerationStructureMemory(XRHIRayTracingScene* Scene, std::shared_ptr<XRHIBuffer> Buffer, uint32 BufferOffset)
	{
		GetContext()->BindAccelerationStructureMemory(Scene, Buffer, BufferOffset);
	}

	inline void BuildAccelerationStructure(const XRayTracingSceneBuildParams& SceneBuildParams)
	{
		GetContext()->RHIBuildAccelerationStructure(SceneBuildParams);
	}

	inline void Transition(XRHITransitionInfo TransitionInfo)
	{
		GetContext()->Transition(TransitionInfo);
	}

	inline void RayTraceDispatch(XRHIRayTracingPSO* Pipeline, XRHIRayTracingShader* RayGenShader, XRHIRayTracingScene* Scene, const XRayTracingShaderBinds& GlobalResourceBindings, uint32 Width, uint32 Height)
	{
		GetContext()->RayTraceDispatch(Pipeline, RayGenShader, Scene, GlobalResourceBindings, Width, Height);
	}
#endif
};

extern XRHICommandList GRHICmdList;



inline XRHITexture* RHIGetCurrentBackTexture()
{

	return GPlatformRHI->RHIGetCurrentBackTexture();
}

inline XRHIUnorderedAcessView* GetRHIUAVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0)
{
	return GPlatformRHI->GetRHIUAVFromTexture(RHITexture, MipIndex);
}

inline XRHIShaderResourceView* GetRHISRVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0)
{
	return GPlatformRHI->GetRHISRVFromTexture(RHITexture, MipIndex);
}

inline uint64 RHIGetCmdBufferOffset(XRHIStructBuffer* RHIStructBuffer)
{
	return GPlatformRHI->RHIGetCmdBufferOffset(RHIStructBuffer);
}

inline void* RHIGetCommandDataPtr(std::vector<XRHICommandData>& RHICmdData,uint32& OutCmdDataSize)
{
	return GPlatformRHI->RHIGetCommandDataPtr(RHICmdData, OutCmdDataSize);
}

inline std::shared_ptr<XRHICommandSignature> RHICreateCommandSignature(
	XRHIIndirectArg* RHIIndirectArg,uint32 ArgCount,XRHIVertexShader* VertexShader,XRHIPixelShader* PixelShader)
{
	return GPlatformRHI->RHICreateCommandSignature(RHIIndirectArg, ArgCount, VertexShader, PixelShader);
}

inline void* LockVertexBuffer(XRHIBuffer* VertexBuffer, uint32 Offset, uint32 SizeRHI)
{
	return GPlatformRHI->LockVertexBuffer(VertexBuffer, Offset, SizeRHI);
}
inline void UnLockVertexBuffer(XRHIBuffer* VertexBuffer)
{
	return GPlatformRHI->UnLockVertexBuffer(VertexBuffer);
}

inline void* LockIndexBuffer(XRHIBuffer* IndexBuffer, uint32 Offset, uint32 SizeRHI)
{
	return GPlatformRHI->LockIndexBuffer(IndexBuffer, Offset, SizeRHI);
}
inline void UnLockIndexBuffer(XRHIBuffer* IndexBuffer)
{
	return GPlatformRHI->UnLockIndexBuffer(IndexBuffer);
}

inline std::shared_ptr<XRHIConstantBuffer> RHICreateConstantBuffer(uint32 size)
{
	return GPlatformRHI->RHICreateConstantBuffer(size);
}

inline std::shared_ptr<XRHITexture2D> RHICreateTexture2D(uint32 width, uint32 height, uint32 SizeZ,
	bool bTextureArray, bool bCubeTexture, EPixelFormat Format,
	ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data)
{
	return GPlatformRHI->RHICreateTexture2D(width, height, SizeZ, bTextureArray, bCubeTexture, Format, flag, NumMipsIn, tex_data);
}

inline std::shared_ptr<XRHITexture2D> RHICreateTexture2D2(uint32 width, uint32 height, uint32 SizeZ,
	bool bTextureArray, bool bCubeTexture, EPixelFormat Format,
	ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size)
{
	return GPlatformRHI->RHICreateTexture2D2(width, height, SizeZ, bTextureArray, bCubeTexture, Format, flag, NumMipsIn, tex_data, Size);
}

inline std::shared_ptr<XRHITexture3D> RHICreateTexture3D(uint32 width, uint32 height, uint32 SizeZ, EPixelFormat Format,
	ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data)
{
	return GPlatformRHI->RHICreateTexture3D(width, height, SizeZ, Format, flag, NumMipsIn, tex_data);
}

inline std::shared_ptr<XRHIBuffer>RHIcreateVertexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateVertexBuffer(Stride, Size, Usage, ResourceData);
}

inline std::shared_ptr<XRHIBuffer>RHIcreateVertexBuffer2(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateBuffer(Stride, Size, Usage | EBufferUsage::BUF_Vertex, ResourceData);
}

inline void RHIResetStructBufferCounter(XRHIStructBuffer* RHIStructBuffer,uint32 CounterOffset)
{
	GPlatformRHI->RHIResetStructBufferCounter(RHIStructBuffer, CounterOffset);
}

inline void RHICopyTextureRegion(XRHITexture* RHITextureDst, XRHITexture* RHITextureSrc, uint32 DstX, uint32 DstY, uint32 DstZ, uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ)
{
	GPlatformRHI->RHICopyTextureRegion(RHITextureDst, RHITextureSrc, DstX, DstY, DstZ, OffsetX, OffsetY, OffsetZ);
}

inline std::shared_ptr<XRHIUnorderedAcessView> RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer, uint64 CounterOffsetInBytes)
{
	return GPlatformRHI->RHICreateUnorderedAccessView(StructuredBuffer, bUseUAVCounter, bAppendBuffer, CounterOffsetInBytes);
}

inline std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer)
{
	return GPlatformRHI->RHICreateShaderResourceView(StructuredBuffer);
}

inline std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView2(XRHIBuffer* Buffer)
{
	return GPlatformRHI->RHICreateShaderResourceView(Buffer);
}

inline std::shared_ptr<XRHIStructBuffer>RHIcreateStructBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateStructBuffer(Stride, Size, Usage, ResourceData);
}

inline std::shared_ptr<XRHIBuffer>RHIcreateStructBuffer2(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateBuffer(Stride, Size, Usage | EBufferUsage::BUF_StructuredBuffer, ResourceData);
}

inline std::shared_ptr<XRHIBuffer>RHICreateIndexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateIndexBuffer(Stride, Size, Usage, ResourceData);
}

inline std::shared_ptr<XRHIBuffer>RHICreateIndexBuffer2(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	return GPlatformRHI->RHICreateBuffer(Stride, Size, Usage | EBufferUsage::BUF_Index, ResourceData);
}

inline std::shared_ptr<XRHIVertexShader> RHICreateVertexShader(XArrayView<uint8> Code)
{
	return GPlatformRHI->RHICreateVertexShader(Code);
}

inline std::shared_ptr<XRHIPixelShader> RHICreatePixelShader(XArrayView<uint8> Code)
{
	return GPlatformRHI->RHICreatePixelShader(Code);
}

inline std::shared_ptr<XRHIComputeShader> RHICreateComputeShader(XArrayView<uint8> Code)
{
	return GPlatformRHI->RHICreateComputeShader(Code);
}

#if RHI_RAYTRACING
inline std::shared_ptr<XRHIRayTracingShader> RHICreateRayTracingShader(XArrayView<uint8> Code, EShaderType ShaderType)
{
	return GPlatformRHI->RHICreateRayTracingShader(Code, ShaderType);
}
#endif

