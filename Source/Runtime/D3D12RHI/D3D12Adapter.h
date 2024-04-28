#pragma once
#include "Runtime/HAL/Mch.h"
#include "Runtime/HAL/PlatformTypes.h"
class XD3D12Adapter
{
public:
	void Create();
	
	inline IDXGIAdapter* GetDXAdapter() { return DxgiAdapter.Get(); }
	inline IDXGIFactory4* GetDXFactory() { return DxgiFactory.Get(); }

private:
	XDxRefCount<IDXGIFactory4> DxgiFactory;
	XDxRefCount<IDXGIAdapter> DxgiAdapter;
};