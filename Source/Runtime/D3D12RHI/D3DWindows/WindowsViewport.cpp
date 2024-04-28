#include "Runtime/D3D12RHI/D3D12Viewport.h"
#include "Runtime/D3D12RHI/D3D12PlatformRHI.h"

void XD3D12Viewport::Create(
    XD3D12AbstractDevice* device_in,
    uint32 size_x_in,
    uint32 size_y_in,
    EPixelFormat EPixFormatIn,
    HWND WindowHandle_in)
{
    AbsDevice = device_in;
    SizeX = size_x_in;
    SizeY = size_y_in;
    Format = EPixFormatIn;

    DXGI_FORMAT DxFormat = (DXGI_FORMAT)GPixelFormats[(int)Format].PlatformFormat;

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = SizeX;
    sd.BufferDesc.Height = SizeY;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DxFormat;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
    sd.BufferCount = BACK_BUFFER_COUNT_DX12;
    sd.OutputWindow = WindowHandle_in;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    AbsDevice->GetPhysicalDevice()->GetAdapter()->GetDXFactory()->CreateSwapChain(
        AbsDevice->GetCmdQueueByType(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetDXCommandQueue(),
        &sd,
        mSwapChain.GetAddressOf());
}