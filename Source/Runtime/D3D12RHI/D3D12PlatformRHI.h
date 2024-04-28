#pragma once

#include "Runtime/RHI/PlatformRHI.h"
#include "D3D12View.h"
#include "D3D12CommandList.h"

class XD3D12Viewport;
class XD3D12AbstractDevice;

class XD3D12PlatformRHI :public XPlatformRHI
{
public:
	XD3D12PlatformRHI();
	~XD3D12PlatformRHI();
	void Init() override;

	//CreateVertexLayout
	std::shared_ptr<XRHIVertexLayout> RHICreateVertexDeclaration(const XRHIVertexLayoutArray& Elements) final override;
	
	//Create buffer
	std::shared_ptr<XRHIBuffer>RHICreateBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData) { XASSERT(false)return nullptr; };
	std::shared_ptr<XRHIBuffer>RHICreateVertexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override;
	std::shared_ptr<XRHIBuffer>RHICreateIndexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override;
	std::shared_ptr<XRHIStructBuffer>RHICreateStructBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override;
	std::shared_ptr<XRHIUnorderedAcessView> RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer, uint64 CounterOffsetInBytes) final override;
	std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer) final override;
	std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIBuffer* StructuredBuffer) final override { XASSERT_TEMP(false); return nullptr; };
	void RHIResetStructBufferCounter(XRHIStructBuffer* RHIStructBuffer, uint32 CounterOffset)final override;
	void RHICopyTextureRegion(XRHITexture* RHITextureDst, XRHITexture* RHITextureSrc, uint32 DstX, uint32 DstY, uint32 DstZ, uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ)final override;

	//Command Signature
	XRHIUnorderedAcessView* GetRHIUAVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0)override;
	XRHIShaderResourceView* GetRHISRVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0)override;
	uint64 RHIGetCmdBufferOffset(XRHIStructBuffer* RHIStructBuffer)override;
	void* RHIGetCommandDataPtr(std::vector<XRHICommandData>& RHICmdData, uint32& OutCmdDataSize)override;
	std::shared_ptr<XRHICommandSignature> RHICreateCommandSignature(XRHIIndirectArg* RHIIndirectArg, uint32 ArgCount, XRHIVertexShader* VertexShader, XRHIPixelShader* PixelShader) override;
	
	//Create State
	std::shared_ptr<XRHISamplerState> RHICreateSamplerState(const XSamplerStateInitializerRHI& Initializer)final override { return nullptr; };
	std::shared_ptr<XRHIRasterizationState> RHICreateRasterizationStateState(const XRasterizationStateInitializerRHI& Initializer)final override;
	std::shared_ptr<XRHIDepthStencilState> RHICreateDepthStencilState(const XDepthStencilStateInitializerRHI& Initializer)final override;
	std::shared_ptr<XRHIBlendState> RHICreateBlendState(const XBlendStateInitializerRHI& Initializer)final override;
	
	//Create Shader
	std::shared_ptr<XRHIVertexShader> RHICreateVertexShader(XArrayView<uint8> Code)final override;
	std::shared_ptr<XRHIPixelShader> RHICreatePixelShader(XArrayView<uint8> Code)final override;
	std::shared_ptr<XRHIComputeShader> RHICreateComputeShader(XArrayView<uint8> Code)final override;
	
	//CreatePSO
	std::shared_ptr<XRHIGraphicsPSO> RHICreateGraphicsPipelineState(const XGraphicsPSOInitializer& PSOInit)final override;
	std::shared_ptr<XRHIComputePSO> RHICreateComputePipelineState(const XRHIComputeShader* RHIComputeShader)final override;

	//RHICreateUniformBuffer
	std::shared_ptr<XRHIConstantBuffer> RHICreateConstantBuffer(uint32 size)override;

	std::shared_ptr<XRHITexture2D> RHICreateTexture2D(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray,
		bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size = 0) override;

	std::shared_ptr<XRHITexture2D> RHICreateTexture2D2(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray,
		bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size = 0) override
	{
		XASSERT(false);
		return nullptr;
	}

	std::shared_ptr<XRHITexture3D> RHICreateTexture3D(uint32 width, uint32 height, uint32 SizeZ, EPixelFormat Format,
		ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data)override;

	XRHITexture* RHIGetCurrentBackTexture()override;

	//Lock/UnLock Vertex Buffer
	void* LockVertexBuffer(XRHIBuffer* VertexBuffer, uint32 Offset, uint32 SizeRHI)override;
	void UnLockVertexBuffer(XRHIBuffer* VertexBuffer)override;

	void* LockIndexBuffer(XRHIBuffer* IndexBuffer, uint32 Offset, uint32 SizeRHI)override;
	void UnLockIndexBuffer(XRHIBuffer* IndexBuffer)override;
public:
	std::shared_ptr<XD3D12Device>PhyDevice;
	std::shared_ptr<XD3D12Viewport>D3DViewPort;
	std::shared_ptr<XD3D12AbstractDevice>AbsDevice;
	std::shared_ptr<XD3D12Adapter>D3DAdapterPtr;

	static inline void Base_TransitionResource(
		XD3D12DirectCommandList& direct_cmd_list,
		XD3D12Resource* pResource,
		D3D12_RESOURCE_STATES after)
	{
		ResourceState& resource_state = pResource->GetResourceState();
		D3D12_RESOURCE_STATES before = resource_state.GetResourceState();
		direct_cmd_list.CmdListAddTransition(pResource, before, after);
		resource_state.SetResourceState(after);
	}

	static inline void TransitionResource(
		XD3D12DirectCommandList& hCommandList, 
		XD3D12UnorderedAcessView* pView, 
		D3D12_RESOURCE_STATES after)
	{
		XD3D12Resource* pResource = pView->GetResource();
		Base_TransitionResource(hCommandList, pResource, after);
	}
	
	static inline void TransitionResource(
		XD3D12DirectCommandList& direct_cmd_list,
		XD3D12ShaderResourceView* sr_view,
		D3D12_RESOURCE_STATES after)
	{
		XD3D12Resource* pResource = sr_view->GetResource();
		Base_TransitionResource(direct_cmd_list, pResource, after);
	}

	static inline void TransitionResource(
		XD3D12DirectCommandList& direct_cmd_list,
		XD3D12RenderTargetView* rt_view,
		D3D12_RESOURCE_STATES after)
	{
		XD3D12Resource* pResource = rt_view->GetResource();
		Base_TransitionResource(direct_cmd_list, pResource, after);
	}

	static inline void TransitionResource(
		XD3D12DirectCommandList& direct_cmd_list,
		XD3D12DepthStencilView* ds_view,
		D3D12_RESOURCE_STATES after)
	{
		XD3D12Resource* pResource = ds_view->GetResource();
		Base_TransitionResource(direct_cmd_list, pResource, after);
	}
};

