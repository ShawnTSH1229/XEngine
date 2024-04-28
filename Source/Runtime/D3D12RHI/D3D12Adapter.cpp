#include "D3D12Adapter.h"

void XD3D12Adapter::Create()
{
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&DxgiFactory)));
	
	//0x10DE : Nvidia | 0x8086 : Intel | 0x1002 : AMD
	int PreferredVendor = 0x10DE;
	
	for (UINT AdapterIndex = 0; DxgiFactory->EnumAdapters(AdapterIndex, &DxgiAdapter) != DXGI_ERROR_NOT_FOUND; ++AdapterIndex)
	{
		if (DxgiAdapter)
		{
			DXGI_ADAPTER_DESC AdapterDesc;
			ThrowIfFailed(DxgiAdapter->GetDesc(&AdapterDesc));
			if (PreferredVendor == AdapterDesc.VendorId) { break; }
		}
	}
	
	XASSERT(((&DxgiAdapter) != nullptr));
}

