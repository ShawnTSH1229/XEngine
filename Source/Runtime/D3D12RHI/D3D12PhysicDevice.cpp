#include "D3D12PhysicDevice.h"
#include "D3D12CommandQueue.h"

void XD3D12Device::Create(XD3D12Adapter* InAdapter)
{
	Adapter = InAdapter;
	ThrowIfFailed(D3D12CreateDevice(InAdapter->GetDXAdapter(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device)));
	d3d12_device->QueryInterface(ID3D12Device1Ptr.GetAddressOf());

	D3D12PipelineLibrary.DeserializingPSOLibrary(this);
}


