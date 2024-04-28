#include "D3D12PipelineState.h"
#include "D3D12Context.h"
#include "D3D12Texture.h"
#include "D3D12AbstractDevice.h"
#include "D3D12PlatformRHI.h"
#include "D3D12Viewport.h"

void XD3DDirectContex::Create(XD3D12AbstractDevice* device_in)
{
	AbsDevice = device_in;
	VSGlobalConstantBuffer = std::make_shared<XD3D12GlobalConstantBuffer>();;
	PSGlobalConstantBuffer = std::make_shared<XD3D12GlobalConstantBuffer>();;
	CSGlobalConstantBuffer = std::make_shared<XD3D12GlobalConstantBuffer>();;
	AbsDevice->GetConstantBufferUploadHeapAlloc()->Allocate(MAX_GLOBAL_CONSTANT_BUFFER_SIZE, 256, VSGlobalConstantBuffer->ResourceLocation);
	AbsDevice->GetConstantBufferUploadHeapAlloc()->Allocate(MAX_GLOBAL_CONSTANT_BUFFER_SIZE, 256, PSGlobalConstantBuffer->ResourceLocation);
	AbsDevice->GetConstantBufferUploadHeapAlloc()->Allocate(MAX_GLOBAL_CONSTANT_BUFFER_SIZE, 256, CSGlobalConstantBuffer->ResourceLocation);


	XD3D12Device* PhyDevice = AbsDevice->GetPhysicalDevice();
	cmd_direct_alloc.Create(PhyDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	cmd_direct_alloc.Reset();

	cmd_dirrect_list.CreateDirectCmdList(PhyDevice, &cmd_direct_alloc);
	cmd_dirrect_list.Close();

	PassStateManager.Create(PhyDevice, this);
	//cmd_dirrect_list.Reset(&cmd_direct_alloc);
	//direct_cmd_allc_manager = new XD3D12CommandAllocManger(D3D12_COMMAND_LIST_TYPE_DIRECT);
	//direct_cmd_allc_manager->Create(device_in);
}

void XD3DDirectContex::RHIEndFrame()
{
	XD3D12Viewport* ViewPort = AbsDevice->GetViewPort();
	ViewPort->Present();
}

void XD3DDirectContex::ReseizeViewport(uint32 Width, uint32 Height)
{
	XD3D12Viewport* ViewPort = AbsDevice->GetViewPort();
	ViewPort->Resize(Width, Height);
}

void XD3DDirectContex::Execute()
{
	XD3D12CommandQueue* DirectCmdQueue = AbsDevice->GetCmdQueueByType(D3D12_COMMAND_LIST_TYPE_DIRECT);
	cmd_dirrect_list.Close();
	ID3D12CommandList* CmdLists[] = { GetCmdList()->GetDXCmdList() };
	DirectCmdQueue->GetDXCommandQueue()->ExecuteCommandLists(_countof(CmdLists), CmdLists);
	DirectCmdQueue->CommandQueueWaitFlush();
}

void XD3DDirectContex::OpenCmdList()
{
	cmd_dirrect_list.Reset(&cmd_direct_alloc);
}

void XD3DDirectContex::CloseCmdList()
{
	cmd_dirrect_list.Close();
}

void XD3DDirectContex::RHISetShaderUAV(EShaderType ShaderType, uint32 TextureIndex, XRHIUnorderedAcessView* UAV)
{
	XD3D12UnorderedAcessView* D3DUAVPtr = static_cast<XD3D12UnorderedAcessView*>(UAV);
	switch (ShaderType)
	{
	case EShaderType::SV_Pixel:
		PassStateManager.SetUAV<EShaderType::SV_Pixel>(D3DUAVPtr, TextureIndex);
		break;
	case EShaderType::SV_Compute:
		PassStateManager.SetUAV<EShaderType::SV_Compute>(D3DUAVPtr, TextureIndex);
		break;
	default:
		XASSERT(false);
		break;
	}
	
}

void XD3DDirectContex::RHISetShaderTexture(EShaderType ShaderType, uint32 TextureIndex, XRHITexture* NewTextureRHI)
{
	XD3D12TextureBase* D3DTexturePtr = GetD3D12TextureFromRHITexture(NewTextureRHI);
	XD3D12ShaderResourceView* D3DSRVPtr = D3DTexturePtr->GetShaderResourceView();
	switch (ShaderType)
	{
	case EShaderType::SV_Vertex:
		PassStateManager.SetShaderResourceView<EShaderType::SV_Vertex>(D3DSRVPtr, TextureIndex);
		break;
	case EShaderType::SV_Pixel:
		PassStateManager.SetShaderResourceView<EShaderType::SV_Pixel>(D3DSRVPtr, TextureIndex);
		break;
	case EShaderType::SV_Compute:
		PassStateManager.SetShaderResourceView<EShaderType::SV_Compute>(D3DSRVPtr, TextureIndex);
		break;
	default:
		XASSERT(false);
		break;
	}
}

void XD3DDirectContex::RHISetShaderSRV(EShaderType ShaderType, uint32 SRVIndex, XRHIShaderResourceView* SRV)
{
	XD3D12ShaderResourceView* D3D12SRV = static_cast<XD3D12ShaderResourceView*>(SRV);
	switch (ShaderType)
	{
	case EShaderType::SV_Vertex:
		PassStateManager.SetShaderResourceView<EShaderType::SV_Vertex>(D3D12SRV, SRVIndex);
		break;
	case EShaderType::SV_Pixel:
		PassStateManager.SetShaderResourceView<EShaderType::SV_Pixel>(D3D12SRV, SRVIndex);
		break;
	case EShaderType::SV_Compute:
		PassStateManager.SetShaderResourceView<EShaderType::SV_Compute>(D3D12SRV, SRVIndex);
		break;
	default:
		XASSERT(false);
		break;
	}
}
void XD3DDirectContex::SetShaderValue(EShaderType ShaderType, uint32 BufferIndex, uint32 VariableOffsetInBuffer, uint32 NumBytes, const void* NewValue)
{
	switch (ShaderType)
	{
	case EShaderType::SV_Vertex:
		VSGlobalConstantBuffer->SetSlotIndex(BufferIndex);
		VSGlobalConstantBuffer->UpdateData(NewValue, NumBytes, VariableOffsetInBuffer);
		break;
	case EShaderType::SV_Pixel:
		PSGlobalConstantBuffer->SetSlotIndex(BufferIndex);
		PSGlobalConstantBuffer->UpdateData(NewValue, NumBytes, VariableOffsetInBuffer);
		break;
	case EShaderType::SV_Compute:
		CSGlobalConstantBuffer->SetSlotIndex(BufferIndex);
		CSGlobalConstantBuffer->UpdateData(NewValue, NumBytes, VariableOffsetInBuffer);
		break;
	default:XASSERT(false); break;
	}

}

void XD3DDirectContex::RHISetShaderConstantBuffer(EShaderType ShaderType, uint32 BufferIndex, XRHIConstantBuffer* RHIConstantBuffer)
{
	XD3D12ConstantBuffer* ConstantBuffer = static_cast<XD3D12ConstantBuffer*>(RHIConstantBuffer);
	switch (ShaderType)
	{
	case EShaderType::SV_Vertex:
		PassStateManager.SetCBV<EShaderType::SV_Vertex>(ConstantBuffer, BufferIndex);
		break;
	case EShaderType::SV_Pixel:
		PassStateManager.SetCBV<EShaderType::SV_Pixel>(ConstantBuffer, BufferIndex);
		break;
	case EShaderType::SV_Compute:
		PassStateManager.SetCBV<EShaderType::SV_Compute>(ConstantBuffer, BufferIndex);
		break;
	default:
		XASSERT(false);
		break;
	}
}


void XD3DDirectContex::RHISetComputePipelineState(XRHIComputePSO* ComputeState)
{
	XD3DComputePSO* D3DComputePSO = static_cast<XD3DComputePSO*>(ComputeState);
	PassStateManager.SetComputePipelineState(D3DComputePSO);
	VSGlobalConstantBuffer->ResetState();
	PSGlobalConstantBuffer->ResetState();
	CSGlobalConstantBuffer->ResetState();

	cmd_dirrect_list.CmdListFlushBarrier();
}

void XD3DDirectContex::RHISetGraphicsPipelineState(XRHIGraphicsPSO* GraphicsState)
{
	XD3DGraphicsPSO* D3DGraphicsPSO = static_cast<XD3DGraphicsPSO*>(GraphicsState);
	PassStateManager.SetGraphicsPipelineState(D3DGraphicsPSO);
	VSGlobalConstantBuffer->ResetState();
	PSGlobalConstantBuffer->ResetState();
	CSGlobalConstantBuffer->ResetState();

	cmd_dirrect_list.CmdListFlushBarrier();
}


void XD3DDirectContex::RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
{
	if (VSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Vertex, VSGlobalConstantBuffer->BindSlotIndex, VSGlobalConstantBuffer.get());
		VSGlobalConstantBuffer->ResetState();
	}
	if (PSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Pixel, PSGlobalConstantBuffer->BindSlotIndex, PSGlobalConstantBuffer.get());
		PSGlobalConstantBuffer->ResetState();
	}
	if (CSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Compute, CSGlobalConstantBuffer->BindSlotIndex, CSGlobalConstantBuffer.get());
		CSGlobalConstantBuffer->ResetState();
	}

	PassStateManager.ApplyCurrentStateToPipeline<ED3D12PipelineType::D3D12PT_Compute>();
	cmd_dirrect_list->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);

	PassStateManager.ResetState();
}

void XD3DDirectContex::SetVertexBuffer(XRHIBuffer* RHIVertexBuffer, uint32 VertexBufferSlot, uint32 OffsetFormVBBegin)
{
	PassStateManager.SetVertexBuffer(RHIVertexBuffer, VertexBufferSlot, OffsetFormVBBegin);
}

void XD3DDirectContex::RHIExecuteIndirect(XRHICommandSignature* RHICmdSig, uint32 CmdCount, XRHIStructBuffer* ArgumentBuffer, uint64 ArgumentBufferOffset, XRHIStructBuffer* CountBuffer, uint64 CountBufferOffset)
{
	if (VSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Vertex, VSGlobalConstantBuffer->BindSlotIndex, VSGlobalConstantBuffer.get());
		VSGlobalConstantBuffer->ResetState();
	}
	if (PSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Pixel, PSGlobalConstantBuffer->BindSlotIndex, PSGlobalConstantBuffer.get());
		PSGlobalConstantBuffer->ResetState();
	}
	if (CSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Compute, CSGlobalConstantBuffer->BindSlotIndex, CSGlobalConstantBuffer.get());
		CSGlobalConstantBuffer->ResetState();
	}
	ID3D12CommandSignature* CmdSig = static_cast<XD3DCommandRootSignature*>(RHICmdSig)->DxCommandSignature.Get();
	ID3D12Resource* ArgRes = static_cast<XD3D12StructBuffer*>(ArgumentBuffer)->ResourcePtr.GetBackResource()->GetResource();
	ID3D12Resource* CounterRes = static_cast<XD3D12StructBuffer*>(CountBuffer)->ResourcePtr.GetBackResource()->GetResource();

	XD3D12PlatformRHI::Base_TransitionResource(cmd_dirrect_list, static_cast<XD3D12StructBuffer*>(ArgumentBuffer)->ResourcePtr.GetBackResource(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
	PassStateManager.ApplyCurrentStateToPipeline<ED3D12PipelineType::D3D12PT_Graphics>();

	
	cmd_dirrect_list.GetDXCmdList()->ExecuteIndirect(CmdSig, CmdCount, ArgRes, ArgumentBufferOffset, CounterRes, CountBufferOffset);
}
void XD3DDirectContex::RHIDrawIndexedPrimitive(
	XRHIBuffer* IndexBuffer,
	uint32 IndexCountPerInstance,
	uint32 InstanceCount,
	uint32 StartIndexLocation,
	uint32 BaseVertexLocation,
	uint32 StartInstanceLocation)
{
	if (VSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Vertex, VSGlobalConstantBuffer->BindSlotIndex, VSGlobalConstantBuffer.get());
		VSGlobalConstantBuffer->ResetState();
	}
	if (PSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Pixel, PSGlobalConstantBuffer->BindSlotIndex, PSGlobalConstantBuffer.get());
		PSGlobalConstantBuffer->ResetState();
	}
	if (CSGlobalConstantBuffer->HasValueBind)
	{
		this->RHISetShaderConstantBuffer(EShaderType::SV_Compute, CSGlobalConstantBuffer->BindSlotIndex, CSGlobalConstantBuffer.get());
		CSGlobalConstantBuffer->ResetState();
	}
	PassStateManager.ApplyCurrentStateToPipeline<ED3D12PipelineType::D3D12PT_Graphics>();

	XD3D12ResourcePtr_CPUGPU* IndexBufferPtr = &static_cast<XD3D12IndexBuffer*>(IndexBuffer)->ResourcePtr;
	const DXGI_FORMAT Format = (IndexBuffer->GetStride() == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);

	D3D12_INDEX_BUFFER_VIEW IndexView;
	IndexView.BufferLocation = IndexBufferPtr->GetGPUVirtualPtr();
	IndexView.Format = Format;
	IndexView.SizeInBytes = IndexBuffer->GetSize();

	cmd_dirrect_list.GetDXCmdList()->IASetIndexBuffer(&IndexView);
	cmd_dirrect_list.GetDXCmdList()->DrawIndexedInstanced(
		IndexCountPerInstance,
		InstanceCount,
		StartIndexLocation,
		BaseVertexLocation,
		StartInstanceLocation);
}


void XD3DDirectContex::RHISetRenderTargets(uint32 num_rt, XRHIRenderTargetView** rt_array_ptr, XRHIDepthStencilView* ds_ptr)
{
	DSVPtr = static_cast<XD3D12DepthStencilView*>(ds_ptr);
	CurrentNumRT = num_rt;

	for (uint32 i = 0; i < num_rt; i++)
	{
		RTPtrArrayPtr[i] = static_cast<XD3D12RenderTargetView*>(rt_array_ptr[i]);
	}

	PassStateManager.SetRenderTarget(num_rt, RTPtrArrayPtr, DSVPtr);
}


void XD3DDirectContex::RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
{
	Viewport = { MinX, MinY, (MaxX - MinX), (MaxY - MinY), MinZ, MaxZ };
	ScissorRect = { static_cast<long>(MinX), static_cast<long>(MinY), static_cast<long>(MaxX), static_cast<long>(MaxY) };
	cmd_dirrect_list->RSSetViewports(1, &Viewport);
	cmd_dirrect_list->RSSetScissorRects(1, &ScissorRect);
}

void XD3DDirectContex::RHIClearMRT(bool ClearRT, bool ClearDS, float* ColorArray, float DepthValue, uint8 StencilValue)
{
	uint32 numRT = CurrentNumRT;

	if (ClearRT)
	{
		for (uint32 i = 0; i < numRT; i++) {
			XD3D12PlatformRHI::TransitionResource(
				cmd_dirrect_list,
				RTPtrArrayPtr[i],
				D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
	}
	
	if (ClearDS)
	{
		XD3D12PlatformRHI::TransitionResource(
			cmd_dirrect_list,
			DSVPtr,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	cmd_dirrect_list.CmdListFlushBarrier();

	if (ClearRT)
	{
		for (uint32 i = 0; i < numRT; i++)
		{
			cmd_dirrect_list->ClearRenderTargetView(
				RTPtrArrayPtr[i]->GetCPUPtr(),
				ColorArray, 0, nullptr);
		}
	}
	
	if (ClearDS)
	{
		cmd_dirrect_list->ClearDepthStencilView(
			DSVPtr->GetCPUPtr(),
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			DepthValue,
			StencilValue,
			0,
			nullptr);
	}
}

void XD3DDirectContex::SetRenderTargetsAndViewPort(uint32 NumRTs, const XRHIRenderTargetView* RTViews, const XRHIDepthStencilView* DSView)
{
	if (DSView->Texture != nullptr)
	{
		XD3D12TextureBase* DSTex = GetD3D12TextureFromRHITexture(DSView->Texture);
		DSVPtr = DSTex->GeDepthStencilView();
	}
	else
	{
		DSVPtr = nullptr;
	}
	
	CurrentNumRT = NumRTs;

	for (uint32 i = 0; i < CurrentNumRT; i++)
	{
		XD3D12TextureBase* RTTex = GetD3D12TextureFromRHITexture(RTViews[i].Texture);
		RTPtrArrayPtr[i] = RTTex->GetRenderTargetView();
	}

	PassStateManager.SetRenderTarget(CurrentNumRT, RTPtrArrayPtr, DSVPtr);

	if (CurrentNumRT > 0 && RTPtrArrayPtr[0] != nullptr)
	{
		if (RTPtrArrayPtr[0]->GetDesc().ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D)
		{
			RHISetViewport(0.0f, 0.0f, 0.0f,
				RTPtrArrayPtr[0]->GetResource()->GetResource()->GetDesc().Width,
				RTPtrArrayPtr[0]->GetResource()->GetResource()->GetDesc().Height,
				1.0f);
		}
		else
		{
			XASSERT(false);
		}
	}
	else if (DSVPtr != nullptr)
	{
		RHISetViewport(0.0f, 0.0f, 0.0f,
			DSVPtr->GetResource()->GetResource()->GetDesc().Width,
			DSVPtr->GetResource()->GetResource()->GetDesc().Height,
			1.0f);
	}
	else
	{
		XASSERT(false);
	}
}

static float ClearColorBlack[4] = { 0,0,0,0 };
void XD3DDirectContex::SetRenderTargetsAndClear(const XRHISetRenderTargetsInfo& RTInfos)
{
	this->SetRenderTargetsAndViewPort(RTInfos.NumColorRenderTargets, RTInfos.ColorRenderTarget, &RTInfos.DepthStencilRenderTarget);
	this->RHIClearMRT(RTInfos.bClearColor, RTInfos.bClearDepth/*RTInfos.bClearStencil*/, ClearColorBlack, 0.0f, 0);
}


void XD3DDirectContex::ResetCmdAlloc()
{
	cmd_direct_alloc.Reset();
}
