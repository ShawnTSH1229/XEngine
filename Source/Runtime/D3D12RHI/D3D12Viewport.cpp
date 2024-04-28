#include "D3D12Viewport.h"
#include "Runtime/D3D12RHI/D3D12PlatformRHI.h"

void XD3D12Viewport::Resize(
    uint32 size_x_in, 
    uint32 size_y_in)
{
    SizeX = size_x_in;
    SizeY = size_y_in;

    DXGI_FORMAT DxFormat = (DXGI_FORMAT)GPixelFormats[(int)Format].PlatformFormat;

    XD3D12Device* PhysicalDevice = AbsDevice->GetPhysicalDevice();
    XD3D12CommandQueue* DirectCmdQueue = AbsDevice->GetCmdQueueByType(D3D12_COMMAND_LIST_TYPE_DIRECT);
    XD3DDirectContex* DirectxCtx = AbsDevice->GetDirectContex(0);


    DirectCmdQueue->CommandQueueWaitFlush();
    DirectxCtx->OpenCmdList();

    for (int i = 0; i < BACK_BUFFER_COUNT_DX12; ++i)
    {
        if(BackBufferResources[i].GetResource())
            BackBufferResources[i].GetResource()->Release();
    }

    ThrowIfFailed(mSwapChain->ResizeBuffers(BACK_BUFFER_COUNT_DX12, SizeX, SizeY,
        DxFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
    
    CurrentBackBuffer = 0;

    D3D12_RENDER_TARGET_VIEW_DESC RtDesc;
    RtDesc.Format = DxFormat;
    RtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    RtDesc.Texture2D.MipSlice = 0;
    RtDesc.Texture2D.PlaneSlice = 0;
    
    for (UINT i = 0; i < BACK_BUFFER_COUNT_DX12; i++)
    {
        uint32 IndexOfDescInHeapRt;
        uint32 IndexOfHeapRt;
        AbsDevice->GetRenderTargetDescArrayManager()->AllocateDesc(IndexOfDescInHeapRt, IndexOfHeapRt);

        ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(BackBufferResources[i].GetPtrToResourceAdress())));
        BackBufferResources[i].SetResourceState(D3D12_RESOURCE_STATE_COMMON);
        BackBufferResources[i].GetResource()->SetName(L"BackBuffer");
        BackRtViews[i].Create(PhysicalDevice,&BackBufferResources[i], RtDesc,
            AbsDevice->GetRenderTargetDescArrayManager()->ComputeCpuPtr(IndexOfDescInHeapRt, IndexOfHeapRt),
            AbsDevice->GetRenderTargetDescArrayManager()->compute_gpu_ptr(IndexOfDescInHeapRt, IndexOfHeapRt)
        );

        std::shared_ptr<XD3D12Texture2D>BackBufferTexture = std::make_shared<XD3D12Texture2D>(Format);
        BackBufferTexture->SetRenderTargetView(BackRtViews[i]);
        BackBufferTextures.push_back(BackBufferTexture);
    }

    DirectxCtx->CloseCmdList();

    ID3D12CommandList* cmdsLists[] = { DirectxCtx->GetCmdList()->GetDXCmdList()};
    DirectCmdQueue->GetDXCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    DirectCmdQueue->CommandQueueWaitFlush();
}

void XD3D12Viewport::Present()
{
    XD3DDirectContex* DirectCtx = AbsDevice->GetDirectContex(0);
    XD3D12CommandQueue* DirectCmdQueue = AbsDevice->GetCmdQueueByType(D3D12_COMMAND_LIST_TYPE_DIRECT);

    XD3D12PlatformRHI::TransitionResource(*DirectCtx->GetCmdList(), this->GetCurrentBackTexture()->GetRenderTargetView(), D3D12_RESOURCE_STATE_PRESENT);
    DirectCtx->GetCmdList()->CmdListFlushBarrier();
    DirectCtx->CloseCmdList();

    ID3D12CommandList* cmdsLists[] = { DirectCtx->GetCmdList()->GetDXCmdList() };
    DirectCmdQueue->GetDXCommandQueue()->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    DirectCmdQueue->CommandQueueWaitFlush();

    ThrowIfFailed(mSwapChain->Present(0, 0));
    CurrentBackBuffer = (CurrentBackBuffer + 1) % BACK_BUFFER_COUNT_DX12;
}
