#include "D3D12DescriptorArray.h"

void XD3D12DescArrayManager::Create(XD3D12Device* device_in, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 num)
{
	device = device_in;
	desc_per_heap = num;

	desc.Type = type;
	desc.NumDescriptors = num;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	element_size = device->GetDXDevice()->GetDescriptorHandleIncrementSize(desc.Type);
}

void XD3D12DescArrayManager::AllocateDesc(uint32& index_of_desc_in_heap, uint32& index_of_heap)
{
	if (free_desc_array_index.size() == 0)
	{
		AllocHeap();
	}
	index_of_heap = *free_desc_array_index.begin();
	index_of_desc_in_heap= *AllDescArrays[index_of_heap].DescIndexFree.begin();
	
	AllDescArrays[index_of_heap].DescIndexFree.erase(index_of_desc_in_heap);
	
	if(AllDescArrays[index_of_heap].DescIndexFree.size()==0)
		free_desc_array_index.erase(index_of_heap);
}

void XD3D12DescArrayManager::AllocHeap()
{
	AllDescArrays.push_back(DescArray());
	size_t index = AllDescArrays.size() - 1;
	free_desc_array_index.insert(static_cast<uint32>(index));

	ThrowIfFailed(device->GetDXDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&AllDescArrays[index].d3d12Heap)));
	AllDescArrays[index].CpuPtrBegin = AllDescArrays[index].d3d12Heap->GetCPUDescriptorHandleForHeapStart();
	AllDescArrays[index].GpuPtrBegin = AllDescArrays[index].d3d12Heap->GetGPUDescriptorHandleForHeapStart();
	for (uint32 i = 0; i < desc_per_heap; i++)
	{
		AllDescArrays[index].DescIndexFree.insert(i);
	}
}

void XD3D12DescArrayManager::FreeDesc(uint32 index_of_desc_in_heap, uint32 index_of_heap)
{
	if (free_desc_array_index.find(index_of_heap) == free_desc_array_index.end())
		free_desc_array_index.insert(index_of_heap);

	AllDescArrays[index_of_heap].DescIndexFree.insert(index_of_desc_in_heap);

}
