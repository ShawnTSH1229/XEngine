
#include "D3D12Common.h"
#include "D3D12PlatformRHI.h"
#include "D3D12AbstractDevice.h"
#include "Runtime/RHI/RHICommandList.h"

XRHIUnorderedAcessView* XD3D12PlatformRHI::GetRHIUAVFromTexture(XRHITexture* RHITexture ,uint32 MipIndex)
{
	return  GetD3D12TextureFromRHITexture(RHITexture)->GeUnorderedAcessView(MipIndex);
}

XRHIShaderResourceView* XD3D12PlatformRHI::GetRHISRVFromTexture(XRHITexture* RHITexture, uint32 MipIndex)
{
	return  GetD3D12TextureFromRHITexture(RHITexture)->GetShaderResourceView(MipIndex);
}

std::shared_ptr<XRHIConstantBuffer> XD3D12PlatformRHI::RHICreateConstantBuffer(uint32 size)
{
	return std::shared_ptr<XRHIConstantBuffer>(AbsDevice->CreateUniformBuffer(size));
}

std::shared_ptr<XRHITexture2D> XD3D12PlatformRHI::RHICreateTexture2D(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray,
	bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data,uint32 Size)
{
	return std::shared_ptr<XRHITexture2D>(AbsDevice->CreateD3D12Texture2D(AbsDevice->GetDirectContex(0)->GetCmdList(), width, height, SizeZ, bTextureArray, bCubeTexture, Format, flag, NumMipsIn, tex_data));
}

std::shared_ptr<XRHITexture3D> XD3D12PlatformRHI::RHICreateTexture3D(uint32 width, uint32 height, uint32 SizeZ,
	EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data)
{
	return std::shared_ptr<XRHITexture3D>(AbsDevice->CreateD3D12Texture3D(AbsDevice->GetDirectContex(0)->GetCmdList(), width, height, SizeZ, Format, flag, NumMipsIn, tex_data));
}

XD3D12Texture2D* XD3D12AbstractDevice::CreateD3D12Texture2D(
	XD3D12DirectCommandList* x_cmd_list,
	uint32 width, uint32 height, uint32 SizeZ,
	bool bTextureArray, bool bCubeTexture,
	EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data)
{
	bool bCreateShaderResource = true;
	bool bCreateRTV = false;
	bool bCreateDSV = false;
	bool bCreateUAV = false;

	if (bTextureArray)
	{
		XASSERT(SizeZ > 1);
	}

	auto cmd_list = x_cmd_list->GetDXCmdList();
	XD3D12Resource* TextureResource = &ResourceManagerTempVec[TempResourceIndex];
	TempResourceIndex++;

	DXGI_FORMAT DX_Format = (DXGI_FORMAT)GPixelFormats[(int)Format].PlatformFormat;

	//const DXGI_FORMAT PlatformTextureResourceFormat = FindTextureResourceDXGIFormat(DX_Format);
	const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(DX_Format);
	const DXGI_FORMAT PlatformDepthStencilFormat = FindDepthStencilDXGIFormat(DX_Format);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // If bCreateTexture2DArray
	textureDesc.Alignment = MIN_PLACED_BUFFER_SIZE;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = SizeZ;
	textureDesc.MipLevels = NumMipsIn;
	//textureDesc.Format = DX_Format;
	textureDesc.Format = DX_Format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	//textureDesc.Layout = ;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (flag & ETextureCreateFlags::TexCreate_RenderTargetable)
	{
		textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		bCreateRTV = true;
	}

	if (flag & ETextureCreateFlags::TexCreate_DepthStencilTargetable)
	{
		textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		bCreateDSV = true;
	}

	if (bCreateDSV && !(flag & TexCreate_ShaderResource))
	{
		textureDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		bCreateShaderResource = false;
	}

	if (flag & TexCreate_UAV)
	{
		textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		bCreateUAV = true;
	}



	const XD3D12ResourceTypeHelper Type(textureDesc, D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_STATES InitialState = Type.GetOptimalInitialState(false);

	const D3D12_RESOURCE_ALLOCATION_INFO Info = PhysicalDevice->GetDXDevice()->GetResourceAllocationInfo(0, 1, &textureDesc);
	XD3D12ResourcePtr_CPUGPU default_location;

	if (bCreateRTV || bCreateDSV)
	{
		D3D12_CLEAR_VALUE clearValue;

		if (bCreateRTV)
		{
			clearValue.Format = PlatformShaderResourceFormat;
			clearValue.Color[0] = 0.0f; clearValue.Color[1] = 0.0f; clearValue.Color[2] = 0.0f; clearValue.Color[3] = 0.0f;
		}
		else if (bCreateDSV)
		{
			clearValue.Format = PlatformDepthStencilFormat;
			clearValue.DepthStencil.Stencil = 0;
			clearValue.DepthStencil.Depth = 0.0f;
		}
		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		PhysicalDevice->GetDXDevice()->CreateCommittedResource(
			&HeapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			InitialState,
			&clearValue,
			IID_PPV_ARGS(TextureResource->GetPtrToResourceAdress()));

		TextureResource->SetResourceState(InitialState);
	}
	else
	{
		bool res = DefaultNonRtDsTextureHeapAlloc.Allocate(
			static_cast<uint32>(Info.SizeInBytes),
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			default_location);
		XASSERT(res == true);

		const D3D12_RESOURCE_STATES ResourceState = (tex_data != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : InitialState);

		uint64 ResoureceAlignOffset = AlignArbitrary(
			DefaultNonRtDsTextureHeapAlloc.GetAllocationOffsetInBytes(default_location.GetBuddyAllocData()),
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);


		ThrowIfFailed(PhysicalDevice->GetDXDevice()->CreatePlacedResource(
			default_location.GetBuddyAllocator()->GetDXHeap(),
			ResoureceAlignOffset,
			&textureDesc,
			ResourceState,
			nullptr, IID_PPV_ARGS(TextureResource->GetPtrToResourceAdress())));
		TextureResource->SetResourceState(ResourceState);
	}

	{
		std::wstring str = L"xx" + std::to_wstring(TempResourceIndex) + L"xx\n";
		TextureResource->GetResource()->SetName(str.c_str());
	}

	XD3D12Texture2D* TextureRet = new XD3D12Texture2D(Format);

	if (bCreateShaderResource)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = PlatformShaderResourceFormat;
		//if (bTextureArray)
		//{
		//	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		//	srvDesc.Texture2DArray.MostDetailedMip = 0;
		//	srvDesc.Texture2DArray.MipLevels = TextureResource->GetResource()->GetDesc().MipLevels;
		//	srvDesc.Texture2DArray.FirstArraySlice = 0;
		//	srvDesc.Texture2DArray.ArraySize = textureDesc.DepthOrArraySize;
		//	srvDesc.Texture2DArray.PlaneSlice = 0;
		//}
		//else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = TextureResource->GetResource()->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}

		uint32 index_of_desc_in_heap;
		uint32 index_of_heap;
		CBV_SRV_UAVDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);

		XD3D12ShaderResourceView ShaderResourceView;
		ShaderResourceView.Create(PhysicalDevice, TextureResource,srvDesc, 
			CBV_SRV_UAVDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap),
			CBV_SRV_UAVDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap)
		);
		TextureRet->SetShaderResourceView(ShaderResourceView);

	}

	if (bCreateUAV)
	{
		for (uint32 i = 0; i < NumMipsIn; i++)
		{

			D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
			UavDesc.Format = PlatformShaderResourceFormat;
			//if (bTextureArray)
			//{
			//	UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			//	UavDesc.Texture2DArray.ArraySize= textureDesc.DepthOrArraySize;
			//}
			//else
			{
				UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				UavDesc.Texture2D.MipSlice = i;
				UavDesc.Texture2D.PlaneSlice = 0;
			}


			uint32 index_of_desc_in_heap;
			uint32 index_of_heap;
			CBV_SRV_UAVDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);

			XD3D12UnorderedAcessView UnorderedAcessView;
			UnorderedAcessView.Create(PhysicalDevice, TextureResource,UavDesc, 
				CBV_SRV_UAVDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap),
				CBV_SRV_UAVDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap)
			);
			TextureRet->SetUnorderedAcessView(UnorderedAcessView);
		}
	}

	if (bCreateRTV)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
		rtDesc.Format = TextureResource->GetResource()->GetDesc().Format;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		rtDesc.Texture2D.PlaneSlice = 0;

		uint32 index_of_desc_in_heap;
		uint32 index_of_heap;
		RenderTargetDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);

		XD3D12RenderTargetView ShaderResourceView;
		ShaderResourceView.Create(PhysicalDevice, TextureResource,rtDesc, 
			RenderTargetDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap),
			RenderTargetDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap)
		);
		TextureRet->SetRenderTargetView(ShaderResourceView);

	}

	if (bCreateDSV)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC ds_desc;
		ds_desc.Format = PlatformDepthStencilFormat;
		ds_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		ds_desc.Flags = D3D12_DSV_FLAG_NONE;
		ds_desc.Texture2D.MipSlice = 0;

		uint32 index_of_desc_in_heap_ds;
		uint32 index_of_heap_ds;
		DepthStencilDescArrayManager.AllocateDesc(index_of_desc_in_heap_ds, index_of_heap_ds);

		XD3D12DepthStencilView DepthStencilView;
		DepthStencilView.Create(PhysicalDevice, TextureResource,ds_desc, 
			DepthStencilDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap_ds, index_of_heap_ds),
			DepthStencilDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap_ds, index_of_heap_ds)
		);
		TextureRet->SetDepthStencilView(DepthStencilView);
	}

	if (tex_data != nullptr)
	{
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(TextureResource->GetResource(), 0, 1);

		XD3D12ResourcePtr_CPUGPU upload_location;
		bool res = UploadHeapAlloc.Allocate(static_cast<uint32>(uploadBufferSize), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, upload_location);
		XASSERT(res == true);

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = tex_data;
		textureData.RowPitch = width * 4;
		textureData.SlicePitch = textureData.RowPitch * height;

		CopyDataFromUploadToDefaulHeap
		(cmd_list, TextureResource->GetResource(),
			UploadHeapAlloc.GetDXResource(),
			UploadHeapAlloc.GetAllocationOffsetInBytes(upload_location.GetBuddyAllocData()),
			0, 1, &textureData);
		cmd_list->ResourceBarrier(1, GetRValuePtr((CD3DX12_RESOURCE_BARRIER::Transition(TextureResource->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))));
		TextureResource->SetResourceState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	return TextureRet;
}

XD3D12Texture3D* XD3D12AbstractDevice::CreateD3D12Texture3D(
	XD3D12DirectCommandList* x_cmd_list,
	uint32 width, uint32 height, uint32 SizeZ,
	EPixelFormat Format,
	ETextureCreateFlags flag,
	uint32 NumMipsIn,
	uint8* tex_data)
{
	bool bCreateShaderResource = true;
	bool bCreateRTV = false;
	bool bCreateUAV = false;

	auto cmd_list = x_cmd_list->GetDXCmdList();

	XD3D12Resource* TextureResource = &ResourceManagerTempVec[TempResourceIndex];
	TempResourceIndex++;

	DXGI_FORMAT DX_Format = (DXGI_FORMAT)GPixelFormats[(int)Format].PlatformFormat;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	textureDesc.Alignment = MIN_PLACED_BUFFER_SIZE;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = SizeZ;
	textureDesc.MipLevels = NumMipsIn;
	textureDesc.Format = DX_Format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (flag & ETextureCreateFlags::TexCreate_RenderTargetable)
	{
		XASSERT(false);
	}

	if (flag & TexCreate_UAV)
	{
		textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		bCreateUAV = true;
	}

	const DXGI_FORMAT PlatformShaderResourceFormat = FindShaderResourceDXGIFormat(DX_Format);
	const DXGI_FORMAT PlatformDepthStencilFormat = FindDepthStencilDXGIFormat(DX_Format);

	const XD3D12ResourceTypeHelper Type(textureDesc, D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_STATES InitialState = Type.GetOptimalInitialState(false);

	const D3D12_RESOURCE_ALLOCATION_INFO Info = PhysicalDevice->GetDXDevice()->GetResourceAllocationInfo(0, 1, &textureDesc);
	XD3D12ResourcePtr_CPUGPU default_location;

	if (bCreateRTV)
	{
		D3D12_CLEAR_VALUE clearValue;
		clearValue.Format = PlatformShaderResourceFormat;
		clearValue.Color[0] = 0.0f; clearValue.Color[1] = 0.0f; clearValue.Color[2] = 0.0f; clearValue.Color[3] = 0.0f;

		const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		PhysicalDevice->GetDXDevice()->CreateCommittedResource(
			&HeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
			InitialState, &clearValue,
			IID_PPV_ARGS(TextureResource->GetPtrToResourceAdress()));

		TextureResource->SetResourceState(InitialState);
	}
	else
	{
		bool res = DefaultNonRtDsTextureHeapAlloc.Allocate(static_cast<uint32>(Info.SizeInBytes),
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, default_location);
		XASSERT(res == true);

		const D3D12_RESOURCE_STATES ResourceState = (tex_data != nullptr ? D3D12_RESOURCE_STATE_COPY_DEST : InitialState);

		uint64 ResoureceAlignOffset = AlignArbitrary(
			DefaultNonRtDsTextureHeapAlloc.GetAllocationOffsetInBytes(default_location.GetBuddyAllocData()),
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

		ThrowIfFailed(PhysicalDevice->GetDXDevice()->CreatePlacedResource(
			default_location.GetBuddyAllocator()->GetDXHeap(), ResoureceAlignOffset,
			&textureDesc, ResourceState, nullptr, IID_PPV_ARGS(TextureResource->GetPtrToResourceAdress())));

		TextureResource->SetResourceState(ResourceState);
	}

	{
		std::wstring str = L"xx" + std::to_wstring(TempResourceIndex) + L"xx\n";
		TextureResource->GetResource()->SetName(str.c_str());
	}

	XD3D12Texture3D* TextureRet = new XD3D12Texture3D(Format);
	if (bCreateShaderResource)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = PlatformShaderResourceFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MipLevels = TextureResource->GetResource()->GetDesc().MipLevels;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.ResourceMinLODClamp = 0;

		uint32 index_of_desc_in_heap;
		uint32 index_of_heap;
		CBV_SRV_UAVDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);

		XD3D12ShaderResourceView ShaderResourceView;
		ShaderResourceView.Create(PhysicalDevice, TextureResource,srvDesc, 
			CBV_SRV_UAVDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap),
			CBV_SRV_UAVDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap)
		);
		TextureRet->SetShaderResourceView(ShaderResourceView);

	}

	if (bCreateUAV)
	{
		XASSERT(NumMipsIn == 1);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
		UavDesc.Format = PlatformShaderResourceFormat;
		UavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		UavDesc.Texture3D.WSize = SizeZ;
		UavDesc.Texture3D.FirstWSlice = 0;
		UavDesc.Texture3D.MipSlice = 0;

		uint32 index_of_desc_in_heap;
		uint32 index_of_heap;
		CBV_SRV_UAVDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);

		XD3D12UnorderedAcessView UnorderedAcessView;
		UnorderedAcessView.Create(PhysicalDevice, TextureResource,UavDesc, 
			CBV_SRV_UAVDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap),
			CBV_SRV_UAVDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap)
		);
		TextureRet->SetUnorderedAcessView(UnorderedAcessView);
	}

	if (bCreateRTV)
	{
		XASSERT(false);
		D3D12_RENDER_TARGET_VIEW_DESC rtDesc = {};
		rtDesc.Format = TextureResource->GetResource()->GetDesc().Format;
		rtDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = 0;
		rtDesc.Texture2D.PlaneSlice = 0;

		uint32 index_of_desc_in_heap;
		uint32 index_of_heap;
		RenderTargetDescArrayManager.AllocateDesc(index_of_desc_in_heap, index_of_heap);

		XD3D12RenderTargetView ShaderResourceView;
		ShaderResourceView.Create(PhysicalDevice, TextureResource, rtDesc, 
			RenderTargetDescArrayManager.ComputeCpuPtr(index_of_desc_in_heap, index_of_heap),
			RenderTargetDescArrayManager.compute_gpu_ptr(index_of_desc_in_heap, index_of_heap)
		);
		TextureRet->SetRenderTargetView(ShaderResourceView);
	}

	if (tex_data != nullptr)
	{
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(TextureResource->GetResource(), 0, 1);

		XD3D12ResourcePtr_CPUGPU upload_location;
		bool res = UploadHeapAlloc.Allocate(static_cast<uint32>(uploadBufferSize), D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, upload_location);
		XASSERT(res == true);

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = tex_data;
		textureData.RowPitch = width * 4;
		textureData.SlicePitch = textureData.RowPitch * height;

		CopyDataFromUploadToDefaulHeap(cmd_list, TextureResource->GetResource(), UploadHeapAlloc.GetDXResource(),
			UploadHeapAlloc.GetAllocationOffsetInBytes(upload_location.GetBuddyAllocData()), 0, 1, &textureData);

		cmd_list->ResourceBarrier(1, GetRValuePtr((CD3DX12_RESOURCE_BARRIER::Transition(TextureResource->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))));
		TextureResource->SetResourceState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	return TextureRet;
}

XD3D12CommandQueue* XD3D12AbstractDevice::GetCmdQueueByType(D3D12_COMMAND_LIST_TYPE cmd_type)
{
	switch (cmd_type)
	{
	case D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT:
		return DirectxCmdQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return ComputeCmdQueue;
		break;
	default:
		XASSERT(false);
		break;
	}
	return DirectxCmdQueue;
}