#include "d3dx12.h"
#include "D3D12Context.h"
#include "D3D12OnlineDescArrayManager.h"
#include "D3D12PlatformRHI.h"

#pragma warning( push )
#pragma warning( disable : 6385)
#pragma warning( disable : 6001)

template void XD3D12OnlineDescArrayManager::SetRootDescCBVs<EShaderType::SV_Vertex>(
	const XD3D12RootSignature* root_signature,
	XD3D12CBVRootDescManager* gpu_virtual_ptr_array,
	uint16& slot_mask);

template void XD3D12OnlineDescArrayManager::SetRootDescCBVs<EShaderType::SV_Pixel>(
	const XD3D12RootSignature* root_signature,
	XD3D12CBVRootDescManager* gpu_virtual_ptr_array,
	uint16& slot_mask);

template void XD3D12OnlineDescArrayManager::SetRootDescCBVs<EShaderType::SV_Compute>(
	const XD3D12RootSignature* root_signature,
	XD3D12CBVRootDescManager* gpu_virtual_ptr_array,
	uint16& slot_mask);

template void XD3D12OnlineDescArrayManager::SetDescTableSRVs<EShaderType::SV_Pixel>(
	const XD3D12RootSignature* root_signature,
	XD3D12PassShaderResourceManager* SRVManager,
	uint32& slot_start, uint64& slot_mask);

template void XD3D12OnlineDescArrayManager::SetDescTableSRVs<EShaderType::SV_Compute>(
	const XD3D12RootSignature* root_signature,
	XD3D12PassShaderResourceManager* SRVManager,
	uint32& slot_start, uint64& slot_mask);

template void XD3D12OnlineDescArrayManager::SetDescTableUAVs<EShaderType::SV_Compute>(
	const XD3D12RootSignature* root_signature, 
	XD3D12PassUnorderedAcessManager* UAVManager, 
	uint32& slot_start, uint16& slot_mask);

template void XD3D12OnlineDescArrayManager::SetDescTableUAVs<EShaderType::SV_Pixel>(
	const XD3D12RootSignature* root_signature,
	XD3D12PassUnorderedAcessManager* UAVManager,
	uint32& slot_start, uint16& slot_mask);

template void XD3D12OnlineDescArrayManager::SetDescTableUAVs<EShaderType::SV_Vertex>(
	const XD3D12RootSignature* root_signature,
	XD3D12PassUnorderedAcessManager* UAVManager,
	uint32& slot_start, uint16& slot_mask);
void XD3D12OnlineDescArrayManager::Create(XD3D12Device* device_in, XD3DDirectContex* direct_ctx_in)
{
	device = device_in;
	direct_ctx = direct_ctx_in;
	pipeline_current_desc_array.Create(device_in);
}

template<EShaderType shader_type>
void XD3D12OnlineDescArrayManager::SetDescTableUAVs(const XD3D12RootSignature* root_signature, XD3D12PassUnorderedAcessManager* UAVManager, uint32& slot_start, uint16& slot_mask)
{
	//X_Assert(shader_type == EShaderType::SV_Compute);

	uint32 first_slot_index = slot_start; unsigned long slotnum;
	if (_BitScanReverse(&slotnum, slot_mask) == 0) { slotnum = -1; }; slotnum++; slot_start += slotnum;

	D3D12_CPU_DESCRIPTOR_HANDLE UavDescriptors[16];
	XD3D12DirectCommandList* cmdList = direct_ctx->GetCmdList();
	const D3D12_CPU_DESCRIPTOR_HANDLE DestCPUPtr = pipeline_current_desc_array.GetCPUDescPtrByIndex(first_slot_index);

	for (uint32 i = 0; i < slotnum; i++)
	{
		UavDescriptors[i] = UAVManager->Views[EShaderType_Underlying(shader_type)][i]->GetCPUPtr();

		XD3D12PlatformRHI::TransitionResource((*cmdList), UAVManager->Views[EShaderType_Underlying(shader_type)][i], (D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	uint32 NumCopy = static_cast<uint32>(slotnum);
	device->GetDXDevice()->CopyDescriptors(1, &DestCPUPtr, &NumCopy, NumCopy, UavDescriptors, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	XD3D12PassUnorderedAcessManager::SlotsBitsUnSet(slot_mask, slotnum);

	uint32 slot_index = root_signature->GetUADescTableBindSlot(shader_type);
	D3D12_GPU_DESCRIPTOR_HANDLE GPUDescPtr = pipeline_current_desc_array.GetGPUDescPtrByIndex(first_slot_index);

	if (shader_type == EShaderType::SV_Compute)
	{
		(*cmdList)->SetComputeRootDescriptorTable(slot_index, GPUDescPtr);
	}
	else
	{
		(*cmdList)->SetGraphicsRootDescriptorTable(slot_index, GPUDescPtr);
	}
}

template<EShaderType shader_type>
void XD3D12OnlineDescArrayManager::SetDescTableSRVs(
	const XD3D12RootSignature* root_signature,
	XD3D12PassShaderResourceManager* SRVManager,
	uint32& slot_start, uint64& slot_mask)
{
	uint32 first_slot_index = slot_start; unsigned long slotnum;
	if (_BitScanReverse64(&slotnum, slot_mask) == 0) { slotnum = -1; }; slotnum++; slot_start += slotnum;

	D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptors[64];
	XD3D12DirectCommandList* cmdList = direct_ctx->GetCmdList();
	const D3D12_CPU_DESCRIPTOR_HANDLE DestCPUPtr = pipeline_current_desc_array.GetCPUDescPtrByIndex(first_slot_index);

	for (uint32 i = 0; i < slotnum; i++)
	{
		SrcDescriptors[i] = SRVManager->Views[EShaderType_Underlying(shader_type)][i]->GetCPUPtr();

		XD3D12PlatformRHI::TransitionResource((*cmdList), SRVManager->Views[EShaderType_Underlying(shader_type)][i],
			(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	}

	uint32 NumCopy = static_cast<uint32>(slotnum);
	device->GetDXDevice()->CopyDescriptors(1, &DestCPUPtr, &NumCopy, NumCopy, SrcDescriptors, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	XD3D12PassShaderResourceManager::SlotsBitsUnSet(slot_mask, slotnum);

	D3D12_GPU_DESCRIPTOR_HANDLE GPUDescPtr = pipeline_current_desc_array.GetGPUDescPtrByIndex(first_slot_index);
	uint32 slot_index = root_signature->GetSRVDescTableBindSlot(shader_type);

	if (shader_type == EShaderType::SV_Compute)
	{
		(*cmdList)->SetComputeRootDescriptorTable(slot_index, GPUDescPtr);
	}
	else
	{
		(*cmdList)->SetGraphicsRootDescriptorTable(slot_index, GPUDescPtr);
	}
}



template<EShaderType shader_type>
void XD3D12OnlineDescArrayManager::SetRootDescCBVs(
	const XD3D12RootSignature* root_signature,
	XD3D12CBVRootDescManager* gpu_virtual_ptr_array,
	uint16& slot_mask)
{
	unsigned long slotnum; if (_BitScanReverse(&slotnum, slot_mask) == 0) { slotnum = -1; }; slotnum++;
	
	uint32 base_index = root_signature->GetCBVRootDescBindSlot(shader_type);

	for (uint32 slot_index = 0; slot_index < slotnum; ++slot_index)
	{
		if (XD3D12CBVRootDescManager::IsSlotNeedSet(slot_mask, slot_index))
		{
			D3D12_GPU_VIRTUAL_ADDRESS gpu_virtual_ptr = gpu_virtual_ptr_array->CurrentGPUVirtualAddress[EShaderType_Underlying(shader_type)][slot_index];

			if (shader_type == EShaderType::SV_Compute)
			{
				(*direct_ctx->GetCmdList())->SetComputeRootConstantBufferView(base_index + slot_index, gpu_virtual_ptr);
			}
			else
			{
				(*direct_ctx->GetCmdList())->SetGraphicsRootConstantBufferView(base_index + slot_index, gpu_virtual_ptr);
			}
			
			XD3D12CBVRootDescManager::SlotBitUnSet(slot_mask, slot_index);
		}
	}
}

#pragma warning( pop )






