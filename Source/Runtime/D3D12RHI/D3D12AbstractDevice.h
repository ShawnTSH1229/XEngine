#pragma once
#include "D3D12CommandQueue.h"
#include "D3D12Context.h"
#include "D3D12DescriptorArray.h"
#include "D3D12Allocation.h"
#include "D3D12Texture.h"
#include "D3D12Buffer.h"
#include <memory>

class XD3D12Viewport;
class XD3D12AbstractDevice
{
public:

	XD3D12AbstractDevice():
		TempResourceIndex(0),
		PhysicalDevice(nullptr),
		DirectxCmdQueue(nullptr),
		ComputeCmdQueue(nullptr)
		{};

	~XD3D12AbstractDevice();
	void Create(XD3D12Device* PhysicalDeviceIn);

	inline XD3D12Device*		GetPhysicalDevice()						{ return PhysicalDevice; }

	//index 0 is default Contex
	inline XD3DDirectContex*		GetDirectContex(uint32 index_thread)	{ return &DirectCtxs[index_thread]; }
	inline XD3DBuddyAllocator*		GetConstantBufferUploadHeapAlloc()		{ return &ConstantBufferUploadHeapAlloc; }
	
	inline XD3D12DescArrayManager*	GetRenderTargetDescArrayManager()		{ return &RenderTargetDescArrayManager; }
	inline XD3D12DescArrayManager*	GetDepthStencilDescArrayManager()		{ return &DepthStencilDescArrayManager; }
	inline XD3D12DescArrayManager*	GetShaderResourceDescArrayManager()		{ return &CBV_SRV_UAVDescArrayManager; }

	inline std::unordered_map<std::size_t, std::shared_ptr<XD3D12RootSignature>>& GetRootSigMap() { return RootSignatureMap; }
	inline std::unordered_map<std::size_t, std::shared_ptr<XD3DCommandRootSignature>>& 
		GetCmdRootSigMap() { return CmdRootSignatureMap; }

	XD3D12CommandQueue* GetCmdQueueByType(D3D12_COMMAND_LIST_TYPE cmd_type);

	std::shared_ptr<XD3D12ConstantBuffer>CreateUniformBuffer(uint32 size);
	XD3D12ConstantBuffer* CreateConstantBuffer(uint32 size);

	XD3D12Texture2D* CreateD3D12Texture2D(XD3D12DirectCommandList* x_cmd_list, uint32 width, uint32 height, uint32 SizeZ,bool bTextureArray, bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data);
	XD3D12Texture3D* CreateD3D12Texture3D(XD3D12DirectCommandList* x_cmd_list, uint32 width, uint32 height, uint32 SizeZ,EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data);

	template<typename BufferType>
	BufferType* DeviceCreateRHIBuffer(XD3D12DirectCommandList* D3D12CmdList, const D3D12_RESOURCE_DESC& InDesc, uint32 Alignment,
		uint32 Stride, uint32 Size, EBufferUsage InUsage, XRHIResourceCreateData& CreateData);

	XD3D12StructBuffer* DeviceCreateStructBuffer(XD3D12DirectCommandList* D3D12CmdList, const D3D12_RESOURCE_DESC& InDesc, uint32 Alignment,uint32 Stride, uint32 Size, EBufferUsage InUsage, XRHIResourceCreateData& CreateData);

	void DeviceResetStructBufferCounter(XD3D12DirectCommandList* D3D12CmdList, XRHIStructBuffer* RHIStructBuffer, uint32 CounterOffset);
	void DeviceCopyTextureRegion(XD3D12DirectCommandList* D3D12CmdList,
		XRHITexture* RHITextureDst,
		XRHITexture* RHITextureSrc,
		uint32 DstX, uint32 DstY, uint32 DstZ,
		uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ);
	XD3D12ShaderResourceView* RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer);
	XD3D12UnorderedAcessView* RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer, uint64 CounterOffsetInBytes);

	inline XD3D12Viewport* GetViewPort() { return Viewport; }
	inline void SetViewPort(XD3D12Viewport* VPIn) { Viewport = VPIn; }
private:

	uint64 TempResourceIndex;
	std::vector<XD3D12Resource>ResourceManagerTempVec;


	std::unordered_map<std::size_t, std::shared_ptr<XD3D12RootSignature>>RootSignatureMap;
	std::unordered_map<std::size_t, std::shared_ptr<XD3DCommandRootSignature>>CmdRootSignatureMap;

	XD3D12Viewport* Viewport;
	XD3D12Device* PhysicalDevice;

	XDxRefCount<ID3D12Resource> ID3D12ZeroStructBuffer;
	XD3D12CommandQueue* DirectxCmdQueue;;
	XD3D12CommandQueue* ComputeCmdQueue;


	XD3DBuddyAllocator VIBufferBufferAllocDefault;//manual+default +allow unrodered acess
	//XD3DBuddyAllocator VIBufferBufferAllocUpload;//manual+upload ->uisng UploadHeapAlloc

	XD3DBuddyAllocator DefaultNonRtDsTextureHeapAlloc;	//placed+default
	XD3DBuddyAllocator UploadHeapAlloc;					//manual+upload
	XD3DBuddyAllocator ConstantBufferUploadHeapAlloc;	//manual+upload

	std::vector<XD3DDirectContex>DirectCtxs;

	XD3D12DescArrayManager RenderTargetDescArrayManager;
	XD3D12DescArrayManager DepthStencilDescArrayManager;
	XD3D12DescArrayManager CBV_SRV_UAVDescArrayManager;
	//XD3D12DescArrayManager ConstantBufferDescArrayManager;
};
