#include "D3D12Resource.h"

void XD3D12Resource::Create(ID3D12Resource* resource_in, D3D12_RESOURCE_STATES state)
{
	m_resourceState.SetResourceState(state);
	d3d12_resource = resource_in;
	GPUVirtualPtr = d3d12_resource->GetGPUVirtualAddress();
}


