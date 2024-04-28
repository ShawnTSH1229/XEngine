#include "D3D12Allocation.h"
#include "d3dx12.h"
#include "Runtime/Core/Template/XEngineTemplate.h"
#include <iostream>


void XD3DBuddyAllocator::Create(
	XD3D12Device* device_in, 
	
	XAllocConfig config_in,

	uint32 max_block_size_in,
	uint32 min_block_size_in,
	AllocStrategy strategy_in)
{
	SetParentDevice(device_in);
	config = config_in;

	max_block_size = max_block_size_in;
	min_block_size = min_block_size_in;
	max_order = SizeToOrder(max_block_size);

	offset_from_left.clear();
	offset_from_left.resize(max_order+1);
	offset_from_left[max_order].insert(static_cast<uint32>(0));

	strategy = strategy_in;

	if (strategy == AllocStrategy::PlacedResource)
	{
		D3D12_HEAP_PROPERTIES heap_properties;
		heap_properties.Type = config.D3d12HeapType;;
		heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap_properties.CreationNodeMask = 0;
		heap_properties.VisibleNodeMask = 0;

		D3D12_HEAP_DESC heap_desc;
		heap_desc.SizeInBytes = max_block_size;
		heap_desc.Properties = heap_properties;
		heap_desc.Alignment = MIN_PLACED_BUFFER_SIZE;
		heap_desc.Flags = config.D3d12HeapFlags;

		ThrowIfFailed(GetParentDevice()->GetDXDevice()->CreateHeap(&heap_desc, IID_PPV_ARGS(&m_heap)));
		m_heap->SetName(L"buddy allocator heap");
	}
	else
	{
		CD3DX12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(max_block_size);
		ResDesc.Flags |= config.D3d12ResourceFlags;

		ThrowIfFailed(GetParentDevice()->GetDXDevice()->CreateCommittedResource(
			GetRValuePtr((CD3DX12_HEAP_PROPERTIES(config.D3d12HeapType))),
			D3D12_HEAP_FLAG_NONE,
			&ResDesc,
			config.D3d12ResourceStates,
			nullptr,
			IID_PPV_ARGS(back_resource.GetPtrToResourceAdress())));
		back_resource.Create(back_resource.GetResource(), config.D3d12ResourceStates);
		back_resource.GetResource()->SetName(L"Alloc back_resource");
	}

}

bool XD3DBuddyAllocator::Allocate(uint32 allocate_size_byte_in, uint32 alignment, XD3D12ResourcePtr_CPUGPU& resource_location)
{
	bool can_allocate = false;
	uint32 allocate_size_byte = allocate_size_byte_in;
	{
		if (alignment != 0 && min_block_size % alignment != 0)
		{
			allocate_size_byte = allocate_size_byte_in + alignment;
		}
		uint32 order = SizeToOrder(allocate_size_byte);
		for (; order <= max_order; ++order)
		{
			if (offset_from_left[order].size() != 0)
			{
				can_allocate = true;
				break;
			}
		}

		if (allocate_size_byte > (max_block_size - TotalUsed))
		{
			can_allocate = false;
		}
	}


	if (can_allocate)
	{
		uint32 order = SizeToOrder(allocate_size_byte);
		TotalUsed += min_block_size * (1 << order);
		
		XASSERT(TotalUsed < max_block_size);

		uint32 OffsetRes = Allocate_Impl(order);

		resource_location.SetBuddyAllocator(this);
		BuddyAllocatorData& alloc_data = resource_location.GetBuddyAllocData();
		alloc_data.Offset_MinBlockUnit = OffsetRes;
		alloc_data.order = order;

		uint32 AllocatedResourceOffset = uint32(OffsetRes * min_block_size);
		if (alignment != 0 && AllocatedResourceOffset % alignment != 0)
		{
			AllocatedResourceOffset = AlignArbitrary(AllocatedResourceOffset, alignment);
		}
		
		if (strategy == AllocStrategy::ManualSubAllocation)
		{
			resource_location.SetOffsetByteFromBaseResource(AllocatedResourceOffset);
			resource_location.SetBackResource(&back_resource);
			resource_location.SetGPUVirtualPtr(back_resource.GetGPUVirtaulAddress() + AllocatedResourceOffset);

			if (config.D3d12HeapType == D3D12_HEAP_TYPE_UPLOAD) //if cpu writable
			{
				resource_location.SetMappedCPUResourcePtr((uint8*)back_resource.GetMappedResourceCPUPtr() + AllocatedResourceOffset);
			}
		}
	}

	XASSERT(can_allocate != false);

	return can_allocate;
}

void XD3DBuddyAllocator::Deallocate(XD3D12ResourcePtr_CPUGPU& ResourceLocation)
{

}




uint32 XD3DBuddyAllocator::Allocate_Impl(uint32 order)
{
	uint32 offset_left;
	
	XASSERT(order <= max_order);
	
	if (offset_from_left[order].size() == 0)
	{
		offset_left = Allocate_Impl(order + 1);
		uint32 offset_right = offset_left + uint32(((uint32)1) << order);
		offset_from_left[order].insert(offset_right);
	}
	else
	{
		XASSERT(offset_from_left[order].size() > 0)
		offset_left = *(offset_from_left[order].begin());
		offset_from_left[order].erase(offset_left);
	}
	return offset_left;
}

