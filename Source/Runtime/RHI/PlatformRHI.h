#pragma once
#include <memory>
#include "RHIResource.h"

#define USE_DX12 1

class XRHICommandList;
class XPlatformRHI
{
public:
	virtual ~XPlatformRHI() {}
	virtual void Init() = 0;
	
	//CreateVertexLayout
	virtual std::shared_ptr<XRHIVertexLayout> RHICreateVertexDeclaration(const XRHIVertexLayoutArray& Elements) = 0;
	
	//Create Buffer
	virtual std::shared_ptr<XRHIBuffer>RHICreateBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData) = 0;
	
	[[deprecated]]
	virtual std::shared_ptr<XRHIBuffer>RHICreateVertexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData) = 0;
	
	[[deprecated]]
	virtual std::shared_ptr<XRHIBuffer>RHICreateIndexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData) = 0;
	
	[[deprecated]]
	virtual std::shared_ptr<XRHIStructBuffer>RHICreateStructBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData) = 0;
	
	[[deprecated]]
	virtual std::shared_ptr<XRHIUnorderedAcessView> RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer, uint64 CounterOffsetInBytes) = 0;
	
	[[deprecated]]
	virtual std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer) = 0;
	
	virtual std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIBuffer* StructuredBuffer) = 0;

	virtual void RHIResetStructBufferCounter(XRHIStructBuffer* RHIStructBuffer, uint32 CounterOffset) = 0;
	virtual void RHICopyTextureRegion(XRHITexture* RHITextureDst, XRHITexture* RHITextureSrc, uint32 DstX, uint32 DstY, uint32 DstZ, uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ) = 0;

	//Command Signature
	virtual XRHIUnorderedAcessView* GetRHIUAVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0) = 0;
	virtual XRHIShaderResourceView* GetRHISRVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0) = 0;
	virtual uint64 RHIGetCmdBufferOffset(XRHIStructBuffer* RHIStructBuffer) = 0;
	virtual void* RHIGetCommandDataPtr(std::vector<XRHICommandData>&RHICmdData, uint32& OutCmdDataSize) = 0;
	virtual std::shared_ptr<XRHICommandSignature> RHICreateCommandSignature(XRHIIndirectArg* RHIIndirectArg, uint32 ArgCount, XRHIVertexShader* VertexShader, XRHIPixelShader* PixelShader) = 0;

	//Create State
	virtual std::shared_ptr<XRHISamplerState> RHICreateSamplerState(const XSamplerStateInitializerRHI& Initializer) = 0;
	virtual std::shared_ptr<XRHIRasterizationState> RHICreateRasterizationStateState(const XRasterizationStateInitializerRHI& Initializer) = 0;
	virtual std::shared_ptr<XRHIDepthStencilState> RHICreateDepthStencilState(const XDepthStencilStateInitializerRHI& Initializer) = 0;
	virtual std::shared_ptr<XRHIBlendState> RHICreateBlendState(const XBlendStateInitializerRHI& Initializer) = 0;
	
	//Create Shader
	virtual std::shared_ptr<XRHIVertexShader> RHICreateVertexShader(XArrayView<uint8> Code) = 0;
	virtual std::shared_ptr<XRHIPixelShader> RHICreatePixelShader(XArrayView<uint8> Code) = 0;
	virtual std::shared_ptr<XRHIComputeShader> RHICreateComputeShader(XArrayView<uint8> Code) = 0;
	
	//CreatePSO
	virtual std::shared_ptr<XRHIGraphicsPSO> RHICreateGraphicsPipelineState(const  XGraphicsPSOInitializer& PSOInit) = 0;
	virtual std::shared_ptr<XRHIComputePSO> RHICreateComputePipelineState(const XRHIComputeShader* RHIComputeShader) = 0;
	virtual std::shared_ptr<XRHIRayTracingPSO> RHICreateRayTracingPipelineState(const XRayTracingPipelineStateInitializer& OriginalInitializer) { return nullptr; };

	virtual std::shared_ptr<XRHITexture2D> RHICreateTexture2D(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray, bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size = 0) = 0;
	virtual std::shared_ptr<XRHITexture2D> RHICreateTexture2D2(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray, bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size = 0) = 0;
	
	virtual std::shared_ptr<XRHITexture3D> RHICreateTexture3D(uint32 width, uint32 height, uint32 SizeZ, EPixelFormat Format,ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data) = 0;

	virtual std::shared_ptr<XRHIConstantBuffer> RHICreateConstantBuffer(uint32 size) = 0;

	virtual XRHITexture* RHIGetCurrentBackTexture() = 0;
	
	//Lock/UnLock Vertex Buffer
	virtual void* LockVertexBuffer(XRHIBuffer* VertexBuffer, uint32 Offset, uint32 SizeRHI) = 0;
	virtual void UnLockVertexBuffer(XRHIBuffer* VertexBuffer) = 0;

	virtual void* LockIndexBuffer(XRHIBuffer* IndexBuffer, uint32 Offset, uint32 SizeRHI) = 0;
	virtual void UnLockIndexBuffer(XRHIBuffer* IndexBuffer) = 0;

#if RHI_RAYTRACING
	virtual std::shared_ptr<XRHIRayTracingGeometry> RHICreateRayTracingGeometry(const XRayTracingGeometryInitializer& Initializer) { return nullptr; };
	virtual XRayTracingAccelerationStructSize RHICalcRayTracingGeometrySize(const XRayTracingGeometryInitializer& Initializer) { return XRayTracingAccelerationStructSize(); }
	virtual XRayTracingAccelerationStructSize RHICalcRayTracingSceneSize(uint32 MaxInstances, ERayTracingAccelerationStructureFlags Flags){ return XRayTracingAccelerationStructSize(); }
	virtual std::shared_ptr<XRHIRayTracingScene> RHICreateRayTracingScene(XRayTracingSceneInitializer Initializer) { return nullptr; }
	virtual std::shared_ptr<XRHIRayTracingShader> RHICreateRayTracingShader(XArrayView<uint8> Code, EShaderType ShaderType) { return nullptr; }
#endif

};


extern XPlatformRHI* GPlatformRHI;
extern bool GIsRHIInitialized;

inline std::shared_ptr<XRHIVertexLayout> RHICreateVertexLayout(const XRHIVertexLayoutArray& Elements)
{
	return GPlatformRHI->RHICreateVertexDeclaration(Elements);
}

inline std::shared_ptr<XRHIRasterizationState> RHICreateRasterizationStateState(const XRasterizationStateInitializerRHI& Initializer)
{
	return GPlatformRHI->RHICreateRasterizationStateState(Initializer);
}

inline std::shared_ptr<XRHIDepthStencilState> RHICreateDepthStencilState(const XDepthStencilStateInitializerRHI& Initializer)
{
	return GPlatformRHI->RHICreateDepthStencilState(Initializer);
}

inline std::shared_ptr<XRHIBlendState> RHICreateBlendState(const XBlendStateInitializerRHI& Initializer)
{
	return GPlatformRHI->RHICreateBlendState(Initializer);
}

inline std::shared_ptr<XRHIGraphicsPSO> RHICreateGraphicsPipelineState(const XGraphicsPSOInitializer& PSOInit)
{
	return GPlatformRHI->RHICreateGraphicsPipelineState(PSOInit);
}

#if RHI_RAYTRACING
inline std::shared_ptr<XRHIRayTracingPSO> RHICreateRayTracingPipelineState(const XRayTracingPipelineStateInitializer& OriginalInitializer)
{
	return GPlatformRHI->RHICreateRayTracingPipelineState(OriginalInitializer);
}
#endif

inline std::shared_ptr<XRHIComputePSO> RHICreateComputePipelineState(const XRHIComputeShader* RHIComputeShader)
{
	return GPlatformRHI->RHICreateComputePipelineState(RHIComputeShader);
}

#if RHI_RAYTRACING
inline std::shared_ptr<XRHIRayTracingGeometry> RHICreateRayTracingGeometry(const XRayTracingGeometryInitializer& Initializer)
{
	return GPlatformRHI->RHICreateRayTracingGeometry(Initializer);
}

inline std::shared_ptr<XRHIRayTracingScene> RHICreateRayTracingScene(const XRayTracingSceneInitializer& Initializer)
{
	return GPlatformRHI->RHICreateRayTracingScene(Initializer);
}

inline XRayTracingAccelerationStructSize RHICalcRayTracingGeometrySize(const XRayTracingGeometryInitializer& Initializer)
{
	return GPlatformRHI->RHICalcRayTracingGeometrySize(Initializer);
}

inline XRayTracingAccelerationStructSize RHICalcRayTracingSceneSize(uint32 MaxInstances, ERayTracingAccelerationStructureFlags Flags)
{
	return GPlatformRHI->RHICalcRayTracingSceneSize(MaxInstances, Flags);
}
#endif