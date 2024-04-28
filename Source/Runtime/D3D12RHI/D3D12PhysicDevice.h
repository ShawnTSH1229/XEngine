#pragma once
#include "D3D12Adapter.h"
#include "D3D12PipelineLibrary.h"
class XD3D12Device
{
private:
	
	XD3D12Adapter* Adapter;
	XDxRefCount<ID3D12Device> d3d12_device;
	XDxRefCount<ID3D12Device1> ID3D12Device1Ptr;
	XD3D12PipelineLibrary D3D12PipelineLibrary;

public:
	void Create(XD3D12Adapter* InAdapter);

	inline XD3D12Adapter*			GetAdapter()				{ return Adapter; }
	inline ID3D12Device*			GetDXDevice()				{ return d3d12_device.Get(); }
	inline ID3D12Device1*			GetDXDevice1()				{ return ID3D12Device1Ptr.Get(); }
};

class XD3D12DeviceChild
{
public:
	explicit XD3D12DeviceChild() :p_device(nullptr) {};

	inline void					SetParentDevice(XD3D12Device* p_device_in) { p_device = p_device_in; }
	inline XD3D12Device*	GetParentDevice() { XASSERT((p_device != nullptr)); return p_device; };
private:
	XD3D12Device* p_device;
};