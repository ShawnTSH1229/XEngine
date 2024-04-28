#include "Runtime/RHI/RHICommandList.h"
#include "D3D12PlatformRHI.h"
#include "D3D12AbstractDevice.h"
#include "D3D12Viewport.h"
#include "Runtime/ApplicationCore/Windows/WindowsApplication.h"

XRHITexture* XD3D12PlatformRHI::RHIGetCurrentBackTexture()
{
    return D3DViewPort->GetCurrentBackTexture();
}

void* XD3D12PlatformRHI::LockVertexBuffer(XRHIBuffer* VertexBuffer, uint32 Offset, uint32 SizeRHI)
{
    XD3D12VertexBuffer* D3DVertexBuffer = static_cast<XD3D12VertexBuffer*>(VertexBuffer);
    return D3DVertexBuffer->ResourcePtr.GetMappedCPUResourcePtr();
}

void* XD3D12PlatformRHI::LockIndexBuffer(XRHIBuffer* IndexBuffer, uint32 Offset, uint32 SizeRHI)
{
    XD3D12IndexBuffer* D3DIndexBuffer = static_cast<XD3D12IndexBuffer*>(IndexBuffer);
    return D3DIndexBuffer->ResourcePtr.GetMappedCPUResourcePtr();
}

void XD3D12PlatformRHI::UnLockIndexBuffer(XRHIBuffer* IndexBuffer)
{
}

void XD3D12PlatformRHI::UnLockVertexBuffer(XRHIBuffer* VertexBuffer)
{
}

XD3D12PlatformRHI::XD3D12PlatformRHI()
{
    GPixelFormats[(int)EPixelFormat::FT_Unknown].PlatformFormat = DXGI_FORMAT_UNKNOWN;
    GPixelFormats[(int)EPixelFormat::FT_R16G16B16A16_FLOAT].PlatformFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    GPixelFormats[(int)EPixelFormat::FT_R8G8B8A8_UNORM].PlatformFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    GPixelFormats[(int)EPixelFormat::FT_R8G8B8A8_UNORM_SRGB].PlatformFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    GPixelFormats[(int)EPixelFormat::FT_R24G8_TYPELESS].PlatformFormat = DXGI_FORMAT_R24G8_TYPELESS;
    GPixelFormats[(int)EPixelFormat::FT_R11G11B10_FLOAT].PlatformFormat = DXGI_FORMAT_R11G11B10_FLOAT;
    GPixelFormats[(int)EPixelFormat::FT_R16_FLOAT].PlatformFormat = DXGI_FORMAT_R16_FLOAT;
    GPixelFormats[(int)EPixelFormat::FT_R32_UINT].PlatformFormat = DXGI_FORMAT_R32_UINT;
    GPixelFormats[(int)EPixelFormat::FT_R32G32B32A32_UINT].PlatformFormat = DXGI_FORMAT_R32G32B32A32_UINT;
    GPixelFormats[(int)EPixelFormat::FT_R32_TYPELESS].PlatformFormat = DXGI_FORMAT_R32_TYPELESS;
}

XD3D12PlatformRHI::~XD3D12PlatformRHI()
{
    D3DViewPort.reset();;
    AbsDevice.reset();;
    PhyDevice.reset();;
    D3DAdapterPtr.reset();
}

void XD3D12PlatformRHI::Init()
{
    XDxRefCount<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    debugController->EnableDebugLayer();

    D3DAdapterPtr = std::make_shared<XD3D12Adapter>();
    D3DAdapterPtr->Create();

    PhyDevice = std::make_shared<XD3D12Device>();
    PhyDevice->Create(D3DAdapterPtr.get());

    AbsDevice = std::make_shared<XD3D12AbstractDevice>(); 
    AbsDevice->Create(PhyDevice.get());

    D3DViewPort = std::make_shared<XD3D12Viewport>();
    AbsDevice->SetViewPort(D3DViewPort.get());
    
    D3DViewPort->Create(AbsDevice.get(), XApplication::Application->ClientWidth, XApplication::Application->ClientHeight, EPixelFormat::FT_R8G8B8A8_UNORM, (HWND)XApplication::Application->GetPlatformHandle());
    XD3DDirectContex* DirectCtx = AbsDevice->GetDirectContex(0);

    GRHICmdList.SetContext(DirectCtx);
}
