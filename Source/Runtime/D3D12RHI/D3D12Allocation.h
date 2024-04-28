#pragma once

#include <set>
#include <vector>
#include "D3D12PhysicDevice.h"
#include "D3D12Resource.h"

//64KB
#define MIN_PLACED_BUFFER_SIZE (64*1024)



enum class AllocStrategy
{
	PlacedResource,
	ManualSubAllocation,
};

struct XAllocConfig
{
	//PlacedResource
	D3D12_HEAP_TYPE D3d12HeapType;
	D3D12_HEAP_FLAGS D3d12HeapFlags;

	//ManualSubAllocation
	D3D12_RESOURCE_STATES D3d12ResourceStates;
	D3D12_RESOURCE_FLAGS D3d12ResourceFlags;
};

//#define LOG_USED_BLOCK 0

class XD3DBuddyAllocator: public XD3D12DeviceChild
{
#ifdef LOG_USED_BLOCK
	int heap_index = -1;
	std::vector<int> blocks_free_index;
#endif
private:
	std::vector<std::set<uint32>>offset_from_left;

	uint32 max_block_size;
	uint32 min_block_size;
	uint32 max_order;
	uint32 TotalUsed;

	XAllocConfig config;

	XDxRefCount<ID3D12Heap>m_heap;
	XD3D12Resource back_resource;

	AllocStrategy strategy;
public:
	XD3DBuddyAllocator() : max_block_size(0), min_block_size(0), max_order(-1), TotalUsed(0) {};
	~XD3DBuddyAllocator() {};
	
	void Create(XD3D12Device* device_in, XAllocConfig config_in, uint32 max_block_size_in, uint32 min_block_size_in, AllocStrategy strategy_in);

	bool Allocate(uint32 allocate_size_byte, uint32 alignment, XD3D12ResourcePtr_CPUGPU& resource_location);
	void Deallocate(XD3D12ResourcePtr_CPUGPU& ResourceLocation);

	inline uint64 GetAllocationOffsetInBytes(const BuddyAllocatorData& AllocatorPrivateData) const 
	{
		return uint64(AllocatorPrivateData.Offset_MinBlockUnit * min_block_size);
	}
	inline ID3D12Heap* GetDXHeap() { return m_heap.Get(); };
	inline ID3D12Resource* GetDXResource() { return back_resource.GetResource(); };
private:
	uint32 Allocate_Impl(uint32 order);
	inline uint32 SizeToOrder(uint32 size)
	{
		uint32 min_block_times = (size + (min_block_size - 1)) / min_block_size;
		unsigned long Result;_BitScanReverse(&Result, min_block_times + min_block_times - 1);
		return Result;
	}
	
}; 

//class XD3DMultiBuddyAllocator
//{
//private:
//	std::vector<XD3DBuddyAllocator>alloc;
//public:
//};
//
//class TextureAllocator
//{
//private:
//	XD3DMultiBuddyAllocator multi_buddy_allocator;
//public:
//};

