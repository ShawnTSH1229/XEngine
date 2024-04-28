#include "D3D12PassStateManager.h"
#include "D3D12PlatformRHI.h"
#include "D3D12Context.h"
#include "D3D12PipelineState.h"

template void XD3D12PassStateManager::ApplyCurrentStateToPipeline<ED3D12PipelineType::D3D12PT_Graphics>();
template void XD3D12PassStateManager::ApplyCurrentStateToPipeline<ED3D12PipelineType::D3D12PT_Compute>();


void XD3D12PassStateManager::Create(XD3D12Device* device_in, XD3DDirectContex* direct_ctx_in)
{
	direct_ctx = direct_ctx_in;
	PipeCurrDescArrayManager.Create(device_in, direct_ctx_in);
	bNeedSetHeapDesc = true;
	ResetState();
}

void XD3D12PassStateManager::ResetState()
{
	bNeedSetPSO = false;
	bNeedSetRT = false;
	bNeedSetRootSig = false;
	bNeedClearMRT = false;
	bNeedSetVB = false;

	PipelineState.Graphics.DepthStencil = nullptr;
	PipelineState.Common.RootSignature = nullptr;
	PipelineState.Compute.D3DComputePSO = nullptr;

	PipelineState.Graphics.VBSlotIndexMax = 0;
	PipelineState.Common.SRVManager.UnsetMasks();
	PipelineState.Common.CBVRootDescManager.UnsetMasks();
	PipelineState.Common.UAVManager.UnsetMasks();
}


void XD3D12PassStateManager::SetRenderTarget(uint32 num_rt, XD3D12RenderTargetView** rt_array_ptr, XD3D12DepthStencilView* ds_ptr)
{
	bNeedSetRT = true;
	PipelineState.Graphics.DepthStencil = ds_ptr;

	uint32 active_rt = 0;
	for (uint32 i = 0; i < num_rt; ++i)
	{
		if (rt_array_ptr[i] != nullptr)
		{
			PipelineState.Graphics.RenderTargetArray[i] = rt_array_ptr[i];
			++active_rt;
		}
	}

	PipelineState.Graphics.CurrentNumRendertarget = active_rt;
}

template<ED3D12PipelineType PipelineType>
void XD3D12PassStateManager::ApplyCurrentStateToPipeline()
{
	XD3D12DirectCommandList* direct_cmd_list = direct_ctx->GetCmdList();
	ID3D12GraphicsCommandList* dx_cmd_list = direct_cmd_list->GetDXCmdList();

	if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
	{
		if (bNeedSetRT)
		{
			bNeedSetRT = false;
			D3D12_CPU_DESCRIPTOR_HANDLE RTVDescriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
			for (uint32 i = 0; i < PipelineState.Graphics.CurrentNumRendertarget; ++i)
			{
				if (PipelineState.Graphics.RenderTargetArray[i] != NULL)
				{
					XD3D12PlatformRHI::TransitionResource(*direct_cmd_list, PipelineState.Graphics.RenderTargetArray[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
					RTVDescriptors[i] = PipelineState.Graphics.RenderTargetArray[i]->GetCPUPtr();
				}
			}

			if (PipelineState.Graphics.DepthStencil != nullptr)
			{
				XD3D12PlatformRHI::TransitionResource(*direct_cmd_list, PipelineState.Graphics.DepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				direct_ctx->GetCmdList()->CmdListFlushBarrier();

				direct_cmd_list->GetDXCmdList()->OMSetRenderTargets(
					PipelineState.Graphics.CurrentNumRendertarget, RTVDescriptors, true,
					GetRValuePtr(PipelineState.Graphics.DepthStencil->GetCPUPtr()));
			}
			else
			{
				direct_ctx->GetCmdList()->CmdListFlushBarrier();
				direct_cmd_list->GetDXCmdList()->OMSetRenderTargets(PipelineState.Graphics.CurrentNumRendertarget, RTVDescriptors, true, nullptr);
			}
		}
	}


	if (bNeedSetPSO)
	{
		if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
		{
			direct_cmd_list->GetDXCmdList()->SetPipelineState(PipelineState.Common.ID3DPSO);
		}
		else
		{
			direct_cmd_list->GetDXCmdList()->SetPipelineState(PipelineState.Common.ID3DPSO);//TODO
		}

		bNeedSetPSO = false;
	}

	if (bNeedSetRootSig)
	{
		if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
		{
			direct_cmd_list->GetDXCmdList()->SetGraphicsRootSignature(PipelineState.Common.RootSignature->GetDXRootSignature());
		}
		else
		{
			direct_cmd_list->GetDXCmdList()->SetComputeRootSignature(PipelineState.Common.RootSignature->GetDXRootSignature());
		}
		
		bNeedSetRootSig = false;
	}

	if (bNeedSetHeapDesc)
	{
		ID3D12DescriptorHeap* descriptorHeaps[] = { PipeCurrDescArrayManager.GetCurrentDescArray()->GetDescHeapPtr() };
		dx_cmd_list->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		bNeedSetHeapDesc = false;
	}

	if (bNeedSetVB)
	{
		bNeedSetVB = false;
		dx_cmd_list->IASetVertexBuffers(0, PipelineState.Graphics.VBSlotIndexMax+1, PipelineState.Graphics.CurrentVertexBufferViews);
	}

	uint32 NumViews = 0;
	for (uint32 i = 0; i < EShaderType_Underlying(EShaderType::SV_ShaderCount); i++)
	{
		NumViews += PipelineState.Common.NumSRVs[i];
		NumViews += PipelineState.Common.NumUAVs[i];
	}

	uint32 DescArraySlotStart = PipeCurrDescArrayManager.GetCurrentDescArray()->GetCurrentFrameSlotStart(NumViews);

	if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
	{
		dx_cmd_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
	{
		if (PipelineState.Common.SRVManager.Mask[EShaderType_Underlying(EShaderType::SV_Pixel)])
		{
			PipeCurrDescArrayManager.SetDescTableSRVs<EShaderType::SV_Pixel>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.SRVManager,
				DescArraySlotStart,
				PipelineState.Common.SRVManager.Mask[EShaderType_Underlying(EShaderType::SV_Pixel)]);
		}

	}
	else
	{
		if (PipelineState.Common.SRVManager.Mask[EShaderType_Underlying(EShaderType::SV_Compute)])
		{
			PipeCurrDescArrayManager.SetDescTableSRVs<EShaderType::SV_Compute>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.SRVManager,
				DescArraySlotStart,
				PipelineState.Common.SRVManager.Mask[EShaderType_Underlying(EShaderType::SV_Compute)]);
		}
	}

	if (PipelineType == ED3D12PipelineType::D3D12PT_Compute)
	{
		if (PipelineState.Common.UAVManager.Mask[EShaderType_Underlying(EShaderType::SV_Compute)])
		{
			PipeCurrDescArrayManager.SetDescTableUAVs<EShaderType::SV_Compute>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.UAVManager,
				DescArraySlotStart,
				PipelineState.Common.UAVManager.Mask[EShaderType_Underlying(EShaderType::SV_Compute)]);
		}
	}
	else if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
	{
		if (PipelineState.Common.UAVManager.Mask[EShaderType_Underlying(EShaderType::SV_Pixel)])
		{
			PipeCurrDescArrayManager.SetDescTableUAVs<EShaderType::SV_Pixel>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.UAVManager,
				DescArraySlotStart,
				PipelineState.Common.UAVManager.Mask[EShaderType_Underlying(EShaderType::SV_Pixel)]);
		}
		else if (PipelineState.Common.UAVManager.Mask[EShaderType_Underlying(EShaderType::SV_Vertex)])
		{
			PipeCurrDescArrayManager.SetDescTableUAVs<EShaderType::SV_Vertex>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.UAVManager,
				DescArraySlotStart,
				PipelineState.Common.UAVManager.Mask[EShaderType_Underlying(EShaderType::SV_Vertex)]);
		}
	}

	if (PipelineType == ED3D12PipelineType::D3D12PT_Graphics)
	{
		if (PipelineState.Common.CBVRootDescManager.Mask[EShaderType_Underlying(EShaderType::SV_Vertex)])
		{
			PipeCurrDescArrayManager.SetRootDescCBVs<EShaderType::SV_Vertex>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.CBVRootDescManager,
				PipelineState.Common.CBVRootDescManager.Mask[EShaderType_Underlying(EShaderType::SV_Vertex)]);
		}

		if (PipelineState.Common.CBVRootDescManager.Mask[EShaderType_Underlying(EShaderType::SV_Pixel)])
		{
			PipeCurrDescArrayManager.SetRootDescCBVs<EShaderType::SV_Pixel>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.CBVRootDescManager,
				PipelineState.Common.CBVRootDescManager.Mask[EShaderType_Underlying(EShaderType::SV_Pixel)]);
		}

	}
	else
	{
		if (PipelineState.Common.CBVRootDescManager.Mask[EShaderType_Underlying(EShaderType::SV_Compute)])
		{
			PipeCurrDescArrayManager.SetRootDescCBVs<EShaderType::SV_Compute>(
				PipelineState.Common.RootSignature,
				&PipelineState.Common.CBVRootDescManager,
				PipelineState.Common.CBVRootDescManager.Mask[EShaderType_Underlying(EShaderType::SV_Compute)]);
		}

	}

	direct_cmd_list->CmdListFlushBarrier();
}
