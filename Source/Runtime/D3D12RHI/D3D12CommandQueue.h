#pragma once

#include "D3D12PhysicDevice.h"
#include "D3D12CommandList.h"

class XD3D12CommandQueue;
class XD3D12Fence
{
public:
	XD3D12Fence() :curr_gpu_fence(0), curr_cpu_fence(0)/*, eventHandle(nullptr)*/ {}
	void Create(XD3D12Device* device);
	void SignalGPU(XD3D12CommandQueue* cmd_queue);
	void SignalGPU(XD3D12CommandQueue* cmd_queue, uint64 fence);
	void WaitCPU();
	void WaitCPU(uint64 fence);
	void WaitGPU(XD3D12CommandQueue* cmd_queue);

	inline uint64 CurrentCPUFence() { return curr_cpu_fence; }
	inline uint64 CurrentGPUFence() { return curr_gpu_fence; }
private:
	XDxRefCount<ID3D12Fence>d3d12_fence;
	uint64 curr_gpu_fence;
	uint64 curr_cpu_fence;
};


class XD3D12CommandQueue
{
public:
	XD3D12CommandQueue(D3D12_COMMAND_LIST_TYPE queue_type_in): d3d12_queue_type(queue_type_in) {};
	void Create(XD3D12Device* device);
	void CommandQueueWaitFlush();
	
	void SignalGPU();
	void WaitGPU();

	inline void Signal() { d3d12_fence.SignalGPU(this); }
	inline void Signal(uint64 fence) { d3d12_fence.SignalGPU(this, fence); }
	inline void Wait() { d3d12_fence.WaitCPU(); }

	inline XD3D12Fence* GetFence() { return &d3d12_fence; }
	inline ID3D12CommandQueue* GetDXCommandQueue() { return d3d12_cmd_queue.Get(); }
private:
	void ExecuteCommandListInteral(std::vector<XD3D12DirectCommandList>& Lists);

	D3D12_COMMAND_LIST_TYPE d3d12_queue_type;
	XD3D12Fence d3d12_fence;
	XDxRefCount<ID3D12CommandQueue>d3d12_cmd_queue;
};