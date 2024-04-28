#pragma once
#include "D3D12PhysicDevice.h"
#include "D3D12Resource.h"
#include <vector>

class XD3D12CommandAllocator
{
public:
	void Create(XD3D12Device* device, D3D12_COMMAND_LIST_TYPE type);
	void Reset();
	
	ID3D12CommandAllocator* GetDXAlloc() { return DxCmdAlloc.Get(); };
private:
	XDxRefCount<ID3D12CommandAllocator>DxCmdAlloc;
};

struct XD3D12PendingResourceBarrier
{
	XD3D12Resource* Resource;
	D3D12_RESOURCE_STATES State;
};

class XD3D12ResourceBarrierManager
{
public:
	void AddTransition(XD3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After);
	void Flush(ID3D12GraphicsCommandList* pCommandList);
private:
	std::vector<D3D12_RESOURCE_BARRIER> barriers;
};

class XD3D12DirectCommandList
{
public:
	void CreateDirectCmdList(XD3D12Device* device, XD3D12CommandAllocator* cmd_alloc);
	void Reset(XD3D12CommandAllocator* cmd_alloc);
	void Close();

	ID3D12GraphicsCommandList* operator->()
	{
		return d3d12_cmd_list.Get();
	}

	inline void CmdListFlushBarrier() { resource_barrier_manager.Flush(d3d12_cmd_list.Get()); }
	inline std::vector<XD3D12PendingResourceBarrier>& GetPendingResourceBarrier() { return pending_resource_state_array; }
	
	inline void CmdListAddTransition(XD3D12Resource* pResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
	{
		if (Before != After)
		{
			resource_barrier_manager.AddTransition(pResource, Before, After);
		}
	}

	inline void CmdListAddPendingState(XD3D12Resource* pResource, D3D12_RESOURCE_STATES After)
	{
		pending_resource_state_array.push_back({ pResource ,After });
	}
	
	inline void Execute()
	{

	}
private:
public:
	
	inline ID3D12GraphicsCommandList* GetDXCmdList() { return d3d12_cmd_list.Get(); }

private:
	std::vector<XD3D12PendingResourceBarrier>pending_resource_state_array;
	XD3D12ResourceBarrierManager resource_barrier_manager;
	XDxRefCount <ID3D12GraphicsCommandList>d3d12_cmd_list;
};

