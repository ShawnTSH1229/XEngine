#include "d3dx12.h"
#include "D3D12PlatformRHI.h"
#include "D3D12AbstractDevice.h"
#include "D3D12Buffer.h"
template XD3D12VertexBuffer* XD3D12AbstractDevice::DeviceCreateRHIBuffer<XD3D12VertexBuffer>(XD3D12DirectCommandList* D3D12CmdList, const D3D12_RESOURCE_DESC& InDesc, uint32 Alignment, uint32 Stride, uint32 Size, EBufferUsage InUsage, XRHIResourceCreateData& CreateData);
template XD3D12IndexBuffer* XD3D12AbstractDevice::DeviceCreateRHIBuffer<XD3D12IndexBuffer>(XD3D12DirectCommandList* D3D12CmdList, const D3D12_RESOURCE_DESC& InDesc, uint32 Alignment, uint32 Stride, uint32 Size, EBufferUsage InUsage, XRHIResourceCreateData& CreateData);
template XD3D12StructBuffer* XD3D12AbstractDevice::DeviceCreateRHIBuffer<XD3D12StructBuffer>(XD3D12DirectCommandList* D3D12CmdList, const D3D12_RESOURCE_DESC& InDesc, uint32 Alignment, uint32 Stride, uint32 Size, EBufferUsage InUsage, XRHIResourceCreateData& CreateData);

template<typename BufferType>
BufferType* XD3D12AbstractDevice::DeviceCreateRHIBuffer(
	XD3D12DirectCommandList* D3D12CmdList,
	const D3D12_RESOURCE_DESC& InDesc,
	uint32 Alignment, uint32 Stride, uint32 Size,
	EBufferUsage InUsage, XRHIResourceCreateData& CreateData)
{
	const bool bDynamic = ((uint32)InUsage & (uint32)EBufferUsage::BUF_AnyDynamic) ? true : false;
	//X_Assert(bDynamic == false);//Dont Surpport Dynamic Now
	
	BufferType* BufferRet = new BufferType(Stride, Size);
	if (bDynamic)
	{
		ConstantBufferUploadHeapAlloc.Allocate(Size, Alignment, BufferRet->ResourcePtr);
	}
	else
	{
		VIBufferBufferAllocDefault.Allocate(Size, Alignment, BufferRet->ResourcePtr);
	}

	
	

	if (CreateData.ResourceArray != nullptr && bDynamic == false)
	{
		XD3D12ResourcePtr_CPUGPU UpLoadResourcePtr;
		bool res = UploadHeapAlloc.Allocate(Size, Alignment, UpLoadResourcePtr);
		XASSERT(res == true);

		memcpy(UpLoadResourcePtr.GetMappedCPUResourcePtr(), CreateData.ResourceArray->GetResourceData(), CreateData.ResourceArray->GetResourceDataSize());

		D3D12CmdList->GetDXCmdList()->CopyBufferRegion(
			BufferRet->ResourcePtr.GetBackResource()->GetResource(),
			BufferRet->ResourcePtr.GetOffsetByteFromBaseResource(),

			UpLoadResourcePtr.GetBackResource()->GetResource(),
			UpLoadResourcePtr.GetOffsetByteFromBaseResource(),

			CreateData.ResourceArray->GetResourceDataSize());

		CreateData.ResourceArray->ReleaseData();
	}
	return BufferRet;
}

XD3D12StructBuffer* XD3D12AbstractDevice::DeviceCreateStructBuffer(XD3D12DirectCommandList* D3D12CmdList, const D3D12_RESOURCE_DESC& InDesc, uint32 Alignment, uint32 Stride, uint32 Size, EBufferUsage InUsage, XRHIResourceCreateData& CreateData)
{
	XD3D12StructBuffer* BufferRet = new XD3D12StructBuffer(Stride, Size);

	XD3D12Resource& BackResource = ResourceManagerTempVec[TempResourceIndex]; TempResourceIndex++;
	ThrowIfFailed(PhysicalDevice->GetDXDevice()->CreateCommittedResource(
		GetRValuePtr((CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT))),
		D3D12_HEAP_FLAG_NONE,
		&InDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(BackResource.GetPtrToResourceAdress())));
	BackResource.Create(BackResource.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST);
	BackResource.GetResource()->SetName(L"Struct Buffer");

	BufferRet->ResourcePtr.SetOffsetByteFromBaseResource(0);
	BufferRet->ResourcePtr.SetBackResource(&BackResource);
	BufferRet->ResourcePtr.SetGPUVirtualPtr(BackResource.GetGPUVirtaulAddress() + 0);

	if (CreateData.ResourceArray != nullptr)
	{
		XD3D12ResourcePtr_CPUGPU UpLoadResourcePtr;
		bool res = UploadHeapAlloc.Allocate(Size, Alignment, UpLoadResourcePtr);
		XASSERT(res == true);

		memcpy(UpLoadResourcePtr.GetMappedCPUResourcePtr(), CreateData.ResourceArray->GetResourceData(), CreateData.ResourceArray->GetResourceDataSize());

		D3D12CmdList->GetDXCmdList()->CopyBufferRegion(
			BufferRet->ResourcePtr.GetBackResource()->GetResource(),
			BufferRet->ResourcePtr.GetOffsetByteFromBaseResource(),

			UpLoadResourcePtr.GetBackResource()->GetResource(),
			UpLoadResourcePtr.GetOffsetByteFromBaseResource(),

			CreateData.ResourceArray->GetResourceDataSize());

		CreateData.ResourceArray->ReleaseData();
	}

	return BufferRet;
}

//void XD3D12AbstractDevice::DeviceClearBufferRegion(XD3D12DirectCommandList* D3D12CmdList, XRHIUnorderedAcessView* UAV, uint32 NumBytes)
//{
//	//XD3D12UnorderedAcessView* UnorderView = static_cast<XD3D12UnorderedAcessView*>(UAV);
//	//XD3D12Resource* D3D12Resource = UnorderView->GetResource();
//	//ID3D12Resource* DestResource = D3D12Resource->GetResource();
//	//D3D12_RESOURCE_STATES ResourceStateBefore = D3D12Resource->GetResourceState().GetResourceState();
//	//
//	//D3D12CmdList->GetDXCmdList()->CopyTextureRegion();
//	//D3D12CmdList->GetDXCmdList()->CopyBufferRegion(DestResource, 0, ID3D12ZeroStructBuffer.Get(), 0, NumBytes);
//	//D3D12CmdList->CmdListAddTransition(D3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, ResourceStateBefore);
//}

void XD3D12AbstractDevice::DeviceCopyTextureRegion(XD3D12DirectCommandList* D3D12CmdList,
	XRHITexture* RHITextureDst, 
	XRHITexture* RHITextureSrc, 
	uint32 DstX, uint32 DstY, uint32 DstZ, 
	uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ)
{
	
	D3D12_RESOURCE_STATES ResourceState;
	D3D12_TEXTURE_COPY_LOCATION Dst;
	XD3D12Resource* DstResource;
	{
		XD3D12TextureBase* D3DTexture2D = (XD3D12TextureBase*)RHITextureDst->GetTextureBaseRHI();;
		XD3D12ShaderResourceView* SRV = D3DTexture2D->GetShaderResourceView(0);

		DstResource = SRV->GetResource();
		Dst.pResource = DstResource->GetResource();
		Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		ResourceState = SRV->GetResource()->GetResourceState().GetResourceState();
	}


	D3D12_TEXTURE_COPY_LOCATION Src;
	{
		XD3D12TextureBase* D3DTexture2D = (XD3D12TextureBase*)RHITextureSrc->GetTextureBaseRHI();;
		XD3D12ShaderResourceView* SRV = D3DTexture2D->GetShaderResourceView(0);

		Src.pResource = SRV->GetResource()->GetResource();
		Src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	}

	Dst.SubresourceIndex = 0;
	Src.SubresourceIndex = 0;

	CD3DX12_BOX SourceBoxD3D(0, 0, 0, 0 + OffsetX, 0 + OffsetY, 0 + OffsetZ);
	XD3D12PlatformRHI::Base_TransitionResource(*D3D12CmdList, DstResource, D3D12_RESOURCE_STATE_COPY_DEST);
	D3D12CmdList->CmdListFlushBarrier();
	D3D12CmdList->GetDXCmdList()->CopyTextureRegion(&Dst,DstX,DstY,DstZ,&Src,&SourceBoxD3D);
	
	//Temp!!!!!!!!!!!
	//XD3D12PlatformRHI::Base_TransitionResource(*D3D12CmdList, DstResource, D3D12_RESOURCE_STATE_GENERIC_READ);
	//D3D12CmdList->CmdListFlushBarrier();
}
void XD3D12AbstractDevice::DeviceResetStructBufferCounter(XD3D12DirectCommandList* D3D12CmdList, XRHIStructBuffer* RHIStructBuffer, uint32 CounterOffset)
{
	const XD3D12ResourcePtr_CPUGPU& DestResourcePtr = static_cast<XD3D12StructBuffer*>(RHIStructBuffer)->ResourcePtr;
	XD3D12Resource* D3D12Resource = DestResourcePtr.GetBackResource();
	ID3D12Resource* DestResource = D3D12Resource->GetResource();
	uint64 DestOffset = DestResourcePtr.GetOffsetByteFromBaseResource();
	
	//D3D12_RESOURCE_STATES ResourceStateBefore = D3D12Resource->GetResourceState().GetResourceState();
	D3D12CmdList->GetDXCmdList()->CopyBufferRegion(DestResource, DestOffset + CounterOffset, ID3D12ZeroStructBuffer.Get(), 0, sizeof(UINT));
	D3D12Resource->GetResourceState().SetResourceState(D3D12_RESOURCE_STATE_COPY_DEST);
	//D3D12CmdList->CmdListAddTransition(D3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, ResourceStateBefore);
}

void XD3D12PlatformRHI::RHIResetStructBufferCounter(XRHIStructBuffer* RHIStructBuffer, uint32 CounterOffset)
{
	AbsDevice->DeviceResetStructBufferCounter(AbsDevice->GetDirectContex(0)->GetCmdList(), RHIStructBuffer, CounterOffset);
}

void XD3D12PlatformRHI::RHICopyTextureRegion(XRHITexture* RHITextureDst,XRHITexture* RHITextureSrc, uint32 DstX, uint32 DstY, uint32 DstZ, uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ)
{
	AbsDevice->DeviceCopyTextureRegion(AbsDevice->GetDirectContex(0)->GetCmdList(), RHITextureDst, RHITextureSrc, DstX, DstY, DstZ, OffsetX, OffsetY, OffsetZ);
}
//void XD3D12PlatformRHI::RHIClearTextureRegion(XRHIUnorderedAcessView* UAV, uint32 NumBytes)
//{
//	AbsDevice->DeviceClearBufferRegion(AbsDevice->GetDirectContex(0)->GetCmdList(), UAV, NumBytes);
//}

XD3D12ShaderResourceView* XD3D12AbstractDevice::RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer)
{
	XD3D12StructBuffer* D3D12StructuredBuffer = static_cast<XD3D12StructBuffer*>(StructuredBuffer);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = StructuredBuffer->GetSize() / StructuredBuffer->GetStride();
	srvDesc.Buffer.StructureByteStride = StructuredBuffer->GetStride();
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	XD3D12Resource* BackResource = static_cast<XD3D12StructBuffer*>(StructuredBuffer)->ResourcePtr.GetBackResource();

	uint32 index_of_desc_in_heap;
	uint32 index_of_heap;
	CBV_SRV_UAVDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);
	D3D12_CPU_DESCRIPTOR_HANDLE CPU_PTR = CBV_SRV_UAVDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap);
	D3D12_GPU_DESCRIPTOR_HANDLE GPU_PTR = CBV_SRV_UAVDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap);

	XD3D12ShaderResourceView* ShaderResourceView = new  XD3D12ShaderResourceView();
	ShaderResourceView->Create(PhysicalDevice, BackResource, srvDesc, CPU_PTR, GPU_PTR);
	return ShaderResourceView;
}

XD3D12UnorderedAcessView* XD3D12AbstractDevice::RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer,uint64 CounterOffsetInBytes)
{
	XD3D12StructBuffer* D3D12StructBufferStructuredBuffer =static_cast<XD3D12StructBuffer*>(StructuredBuffer);
	XD3D12ResourcePtr_CPUGPU& Ptr = D3D12StructBufferStructuredBuffer->ResourcePtr;
	
	XD3D12Resource* D3D12Resource = Ptr.GetBackResource();
	
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = Ptr.GetOffsetByteFromBaseResource() / StructuredBuffer->GetStride();
	uavDesc.Buffer.NumElements = StructuredBuffer->GetSize()/ StructuredBuffer->GetStride();
	uavDesc.Buffer.StructureByteStride = StructuredBuffer->GetStride();
	uavDesc.Buffer.CounterOffsetInBytes = CounterOffsetInBytes;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	uint32 index_of_desc_in_heap;
	uint32 index_of_heap;
	CBV_SRV_UAVDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);
	D3D12_CPU_DESCRIPTOR_HANDLE CPU_PTR = CBV_SRV_UAVDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap);
	D3D12_GPU_DESCRIPTOR_HANDLE GPU_PTR = CBV_SRV_UAVDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap);
	XD3D12UnorderedAcessView* UnorderedAcessView = new  XD3D12UnorderedAcessView();
	UnorderedAcessView->Create(PhysicalDevice, D3D12Resource, D3D12Resource, uavDesc, CPU_PTR, GPU_PTR);
	
	XASSERT(bUseUAVCounter == true);
	XASSERT(bAppendBuffer == true);
	return UnorderedAcessView;
}
std::shared_ptr<XRHIUnorderedAcessView> XD3D12PlatformRHI::RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer, uint64 CounterOffsetInBytes)
{
	XD3D12UnorderedAcessView* UnorderedAcessView = AbsDevice->RHICreateUnorderedAccessView(StructuredBuffer, bUseUAVCounter, bAppendBuffer, CounterOffsetInBytes);
	return std::shared_ptr<XRHIUnorderedAcessView>(UnorderedAcessView);
}

std::shared_ptr<XRHIShaderResourceView> XD3D12PlatformRHI::RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer)
{
	XD3D12ShaderResourceView* ShaderResourceView=AbsDevice->RHICreateShaderResourceView(StructuredBuffer);
	return std::shared_ptr<XRHIShaderResourceView>(ShaderResourceView);
}


std::shared_ptr<XRHIBuffer> XD3D12PlatformRHI::RHICreateVertexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	const D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(Size);
	const uint32 Alignment = 4;
	XD3D12VertexBuffer* VertexBuffer = AbsDevice->DeviceCreateRHIBuffer<XD3D12VertexBuffer>(AbsDevice->GetDirectContex(0)->GetCmdList(), Desc, Alignment, Stride, Size, Usage, ResourceData);
	return std::shared_ptr<XRHIBuffer>(VertexBuffer);
}

std::shared_ptr<XRHIBuffer> XD3D12PlatformRHI::RHICreateIndexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	const D3D12_RESOURCE_DESC Desc = CD3DX12_RESOURCE_DESC::Buffer(Size);
	const uint32 Alignment = 4;
	XD3D12IndexBuffer* IndexBuffer = AbsDevice->DeviceCreateRHIBuffer<XD3D12IndexBuffer>(AbsDevice->GetDirectContex(0)->GetCmdList(), Desc, Alignment, Stride, Size, Usage, ResourceData);
	return std::shared_ptr<XRHIBuffer>(IndexBuffer);
}

std::shared_ptr<XRHIStructBuffer> XD3D12PlatformRHI::RHICreateStructBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)
{
	D3D12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(Size);
	if ((int)Usage & ((int)EBufferUsage::BUF_UnorderedAccess))
	{
		BufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	const uint32 Alignment = 
		(((int)Usage & ((int)EBufferUsage::BUF_DrawIndirect)) == 0) ? 
		Stride
		: 4;
	XD3D12StructBuffer* IndexBuffer = AbsDevice->DeviceCreateStructBuffer(AbsDevice->GetDirectContex(0)->GetCmdList(), BufferDesc, Alignment, Stride, Size, Usage, ResourceData);
	return std::shared_ptr<XRHIStructBuffer>(IndexBuffer);
}



