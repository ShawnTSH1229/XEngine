#pragma once
#include "D3D12PhysicDevice.h"
#include "Runtime/RHI/RHIResource.h"



class XD3D12ResourceTypeHelper
{
public:
	const uint32 bSRV : 1;
	const uint32 bDSV : 1;
	const uint32 bRTV : 1;
	const uint32 bUAV : 1;
	const uint32 bWritable : 1;
	const uint32 bSRVOnly : 1;
	const uint32 bBuffer : 1;
	const uint32 bReadBackResource : 1;

	XD3D12ResourceTypeHelper(D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType) :
		bSRV((Desc.Flags& D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0),
		bDSV((Desc.Flags& D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0),
		bRTV((Desc.Flags& D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0),
		bUAV((Desc.Flags& D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0),
		bWritable(bDSV || bRTV || bUAV),
		bSRVOnly(bSRV && !bWritable),
		bBuffer(Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER),
		bReadBackResource(HeapType == D3D12_HEAP_TYPE_READBACK)
	{}

	inline const D3D12_RESOURCE_STATES GetOptimalInitialState(bool bAccurateWriteableStates) const
	{
		if (bSRVOnly)
		{
			return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}
		else if (bBuffer && !bUAV)
		{
			return (bReadBackResource) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else if (bWritable)
		{
			if (bAccurateWriteableStates)
			{
				if (bDSV)
				{
					return D3D12_RESOURCE_STATE_DEPTH_WRITE;
				}
				else if (bRTV)
				{
					return D3D12_RESOURCE_STATE_RENDER_TARGET;
				}
				else if (bUAV)
				{
					return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				}
			}
			else
			{
				// This things require tracking anyway
				return D3D12_RESOURCE_STATE_COMMON;
			}
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}


};



class ResourceState
{
private:
	D3D12_RESOURCE_STATES m_ResourceState;
public:
	inline D3D12_RESOURCE_STATES GetResourceState() { return m_ResourceState; }
	inline void SetResourceState(const D3D12_RESOURCE_STATES state) { m_ResourceState = state; }
};

class XD3D12Resource
{
public:
	void Create(ID3D12Resource* resource_in, D3D12_RESOURCE_STATES state);

	inline void							SetResourceState(D3D12_RESOURCE_STATES state) { m_resourceState.SetResourceState(state); }
	inline ID3D12Resource*				GetResource() { return d3d12_resource.Get(); }
	inline ID3D12Resource**				GetPtrToResourceAdress() { return &d3d12_resource; }
	inline ResourceState&				GetResourceState() { return m_resourceState; }
	inline D3D12_GPU_VIRTUAL_ADDRESS	GetGPUVirtaulAddress() { return GPUVirtualPtr; }

	inline void* GetMappedResourceCPUPtr() 
	{ 
		d3d12_resource->Map(0, nullptr, &mapped_resource_cpu_ptr);
		return mapped_resource_cpu_ptr;
	}
private:
	void*							mapped_resource_cpu_ptr;
	ResourceState					m_resourceState;
	D3D12_GPU_VIRTUAL_ADDRESS		GPUVirtualPtr;
	XDxRefCount<ID3D12Resource>		d3d12_resource;
};

class XD3DBuddyAllocator;
struct BuddyAllocatorData
{
	uint32 Offset_MinBlockUnit;//unit in minblock size
	uint32 order;
};

class XD3D12ResourcePtr_CPUGPU
{
private:
	BuddyAllocatorData buddy_alloc_data;
	XD3DBuddyAllocator* buddy_alloc;
	
	/// for mannual suballocation
	void* mapped_resource_cpu_ptr;
	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualPtr;
	XD3D12Resource* CommitBackResource;
	uint64 OffsetBytesFromBaseResource;//unit in bytes
	//End
	
public:
	XD3D12ResourcePtr_CPUGPU() :
		buddy_alloc(nullptr),
		mapped_resource_cpu_ptr(nullptr),
		CommitBackResource(nullptr),
		OffsetBytesFromBaseResource(0) {}

	/// for mannual suballocation
	inline void SetOffsetByteFromBaseResource(uint64 Value) { OffsetBytesFromBaseResource = Value; }
	inline void SetBackResource(XD3D12Resource* ResourceIn) { CommitBackResource = ResourceIn; }
	inline void SetMappedCPUResourcePtr(void* address_in) { mapped_resource_cpu_ptr = address_in; }
	inline void SetGPUVirtualPtr(D3D12_GPU_VIRTUAL_ADDRESS address) { GPUVirtualPtr = address; }
	
	inline uint64 GetOffsetByteFromBaseResource()const { return  OffsetBytesFromBaseResource; }
	inline XD3D12Resource* GetBackResource()const { return CommitBackResource; }
	inline void* GetMappedCPUResourcePtr() { return mapped_resource_cpu_ptr; }
	inline D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualPtr() { return GPUVirtualPtr; };
	//End

	inline void SetBuddyAllocator(XD3DBuddyAllocator* alloc_in) { buddy_alloc = alloc_in; };
	inline XD3DBuddyAllocator* GetBuddyAllocator() { return buddy_alloc; };
	inline BuddyAllocatorData& GetBuddyAllocData() { return buddy_alloc_data; };
};


class XD3DCommandRootSignature :public XRHICommandSignature
{
public:
	XDxRefCount<ID3D12CommandSignature>DxCommandSignature;
};

//No Multi Inherit !!
class XD3D12VertexBuffer :public XRHIBuffer
{
public:
	//StrideIn is Unused
	XD3D12VertexBuffer(uint32 StrideIn, uint32 SizeIn) :
		XRHIBuffer(StrideIn,SizeIn,EBufferUsage::BUF_Vertex) {}

	XD3D12ResourcePtr_CPUGPU ResourcePtr;
};

class XD3D12IndexBuffer :public XRHIBuffer
{
public:
	XD3D12IndexBuffer(uint32 StrideIn, uint32 SizeIn) :
		XRHIBuffer(StrideIn, SizeIn, EBufferUsage::BUF_Index) {}

	XD3D12ResourcePtr_CPUGPU ResourcePtr;
};



#define MAX_GLOBAL_CONSTANT_BUFFER_SIZE		4096


class XD3D12ConstantBuffer :public XRHIConstantBuffer
{
public:
	XD3D12ResourcePtr_CPUGPU ResourceLocation;
	
	virtual void UpdateData(const void* data, uint32 size,uint32 offset_byte)override
	{
		void* data_ptr = ResourceLocation.GetMappedCPUResourcePtr();
		data_ptr = static_cast<uint8*>(data_ptr) + offset_byte;
		memcpy(data_ptr, data, size);
	}
};

class XD3D12GlobalConstantBuffer :public XD3D12ConstantBuffer
{
public:
	uint32 BindSlotIndex;
	bool HasValueBind;
public:
	XD3D12GlobalConstantBuffer() :BindSlotIndex(0), HasValueBind(false) {}

	inline void ResetState()
	{
		HasValueBind = false;
	}

	inline void SetSlotIndex(uint32 BufferIndex)
	{
		if (HasValueBind && (BufferIndex != BindSlotIndex))
		{
			XASSERT(false);
		}

		if (HasValueBind == false)
		{
			BindSlotIndex = BufferIndex;
			HasValueBind = true;
		}
	}

	inline void UpdateData(const void* data, uint32 size, uint32 offset_byte)override
	{
		XD3D12ConstantBuffer::UpdateData(data, size, offset_byte);
	}
};

