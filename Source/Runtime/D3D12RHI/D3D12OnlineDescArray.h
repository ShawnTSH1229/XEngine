#pragma once
#include "D3D12PhysicDevice.h"
#include <vector>

class XD3D12OnlineDescArray :public XD3D12DeviceChild
{
public:
	void Create(XD3D12Device* device_in);
	
	inline uint32 GetElemSize() { return elemt_size; }
	inline ID3D12DescriptorHeap* GetDescHeapPtr() { return DescArray.Get(); }

	inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescPtrByIndex(uint32 index) 
	{
		D3D12_CPU_DESCRIPTOR_HANDLE ret_ptr = { cpu_ptr_begin.ptr + +static_cast<UINT64>(index) * static_cast<UINT64>(elemt_size) };
		return ret_ptr;
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescPtrByIndex(uint32 index) 
	{
		D3D12_GPU_DESCRIPTOR_HANDLE ret_ptr = { gpu_ptr_begin.ptr + +static_cast<UINT64>(index) * static_cast<UINT64>(elemt_size) };
		return ret_ptr;
	}
	
	inline uint32 GetCurrentFrameSlotStart(uint32 NumSlotUsed)
	{
		uint32 RetValue = CurrentSlotIndex;
		XASSERT(CurrentSlotIndex + NumSlotUsed < TotalNum);
		CurrentSlotIndex += NumSlotUsed;
		return RetValue;
	}

	inline void ResetIndexToZero()
	{
		CurrentSlotIndex = 0;
	}

private:
	uint32 TotalNum;
	uint32 elemt_size;
	uint32 CurrentSlotIndex;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr_begin;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr_begin;
	XDxRefCount<ID3D12DescriptorHeap>DescArray;
};

