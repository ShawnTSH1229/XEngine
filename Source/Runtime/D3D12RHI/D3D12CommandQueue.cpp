#include "D3D12CommandQueue.h"

void XD3D12Fence::Create(XD3D12Device* device)
{
	ThrowIfFailed(device->GetDXDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12_fence)));
}

void XD3D12Fence::SignalGPU(XD3D12CommandQueue* cmd_queue)
{
	curr_cpu_fence++;
	ThrowIfFailed(cmd_queue->GetDXCommandQueue()->Signal(d3d12_fence.Get(), curr_cpu_fence));
}

void XD3D12Fence::SignalGPU(XD3D12CommandQueue* cmd_queue, uint64 fence)
{
	curr_cpu_fence = fence;
	ThrowIfFailed(cmd_queue->GetDXCommandQueue()->Signal(d3d12_fence.Get(), curr_cpu_fence));
}
void XD3D12Fence::WaitCPU()
{
	curr_gpu_fence = d3d12_fence->GetCompletedValue();
	if (curr_gpu_fence < curr_cpu_fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(d3d12_fence->SetEventOnCompletion(curr_cpu_fence, eventHandle));
		if (eventHandle)
		{
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}
void XD3D12Fence::WaitCPU(uint64 cpu_fence)
{
	curr_gpu_fence = d3d12_fence->GetCompletedValue();
	if (curr_gpu_fence < cpu_fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(d3d12_fence->SetEventOnCompletion(cpu_fence, eventHandle));
		if (eventHandle)
		{
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}
}

void XD3D12Fence::WaitGPU(XD3D12CommandQueue* cmd_queue)
{
	cmd_queue->GetDXCommandQueue()->Wait(d3d12_fence.Get(), curr_cpu_fence);
}


void XD3D12CommandQueue::Create(XD3D12Device* device)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = d3d12_queue_type;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(device->GetDXDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12_cmd_queue)));
	d3d12_fence.Create(device);
}

void XD3D12CommandQueue::CommandQueueWaitFlush()
{
	d3d12_fence.SignalGPU(this);
	d3d12_fence.WaitCPU();
}

void XD3D12CommandQueue::SignalGPU()
{
	d3d12_fence.SignalGPU(this);
}

void XD3D12CommandQueue::WaitGPU()
{
	d3d12_fence.WaitGPU(this);
}

void XD3D12CommandQueue::ExecuteCommandListInteral(std::vector<XD3D12DirectCommandList>& Lists)
{

	for (uint32 i = 0; i < Lists.size(); i++)
	{
		if (Lists[i].GetPendingResourceBarrier().size() > 0)
		{
			XASSERT(false);
		}
	}
	std::vector<ID3D12CommandList*>execute_cmd_queue;
	for (uint32 i = 0; i < Lists.size(); i++)
	{
		execute_cmd_queue.push_back(Lists[i].GetDXCmdList());
	}
	
	d3d12_cmd_queue->ExecuteCommandLists(static_cast<UINT>(Lists.size()), execute_cmd_queue.data());
}


