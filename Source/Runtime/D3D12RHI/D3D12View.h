#pragma once
#include <memory>
#include "D3D12Resource.h"

class XD3D12View
{
protected:
	XD3D12Resource* pResource;
	D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr;
	bool IsDSV;
public:
	inline bool							IsDsv()			{ return IsDSV; };
	inline D3D12_CPU_DESCRIPTOR_HANDLE	GetCPUPtr()		{ return cpu_ptr; }
	inline D3D12_GPU_DESCRIPTOR_HANDLE	GetGPUPtr()		{ return gpu_ptr; }
	inline XD3D12Resource*				GetResource()	{ return pResource; }
};


class XD3D12RenderTargetView :public XRHIRenderTargetView, public XD3D12View
{
private:
	D3D12_RENDER_TARGET_VIEW_DESC desc;
public:
	inline void Create(
		XD3D12Device* device, 
		XD3D12Resource* resource, 
		const D3D12_RENDER_TARGET_VIEW_DESC desc_in,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr_in,
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr_in
		)
	{
		IsDSV = false;
		pResource = resource;
		desc = desc_in;
		cpu_ptr = cpu_ptr_in;
		gpu_ptr = gpu_ptr_in;
		device->GetDXDevice()->CreateRenderTargetView(resource->GetResource(), &desc, cpu_ptr);
	}

	inline D3D12_RENDER_TARGET_VIEW_DESC GetDesc() { return desc; }
};


class XD3D12DepthStencilView :public XRHIDepthStencilView,public XD3D12View
{
private:
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
public:
	inline void Create(
		XD3D12Device* device,
		XD3D12Resource* resource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC desc_in,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr_in,
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr_in)
	{
		IsDSV = true;
		pResource = resource;
		desc = desc_in;
		cpu_ptr = cpu_ptr_in;
		gpu_ptr = gpu_ptr_in;
		device->GetDXDevice()->CreateDepthStencilView(resource->GetResource(), &desc, cpu_ptr);
	}
};

class XD3D12ShaderResourceView :public XRHIShaderResourceView ,public XD3D12View
{
private:
	D3D12_SHADER_RESOURCE_VIEW_DESC desc;
public:
	inline void Create(
		XD3D12Device* device,
		XD3D12Resource* resource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC desc_in,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr_in,
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr_in)
	{
		IsDSV = false;
		pResource = resource;
		desc = desc_in;
		cpu_ptr = cpu_ptr_in;
		gpu_ptr = gpu_ptr_in;
		device->GetDXDevice()->CreateShaderResourceView(resource->GetResource(), &desc, cpu_ptr);
	}
};

class XD3D12UnorderedAcessView :public XRHIUnorderedAcessView, public XD3D12View
{
private:
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
public:
	inline void Create(
		XD3D12Device* device,
		XD3D12Resource* resource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC desc_in,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr_in,
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr_in)
	{
		IsDSV = false;
		pResource = resource;
		desc = desc_in;
		cpu_ptr = cpu_ptr_in;
		gpu_ptr = gpu_ptr_in;
		device->GetDXDevice()->CreateUnorderedAccessView(resource->GetResource(), nullptr, &desc, cpu_ptr);
	}

	inline void Create(
		XD3D12Device* device,
		XD3D12Resource* resource,
		XD3D12Resource* CounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC desc_in,
		D3D12_CPU_DESCRIPTOR_HANDLE cpu_ptr_in,
		D3D12_GPU_DESCRIPTOR_HANDLE gpu_ptr_in)
	{
		IsDSV = false;
		pResource = resource;
		desc = desc_in;
		cpu_ptr = cpu_ptr_in;
		gpu_ptr = gpu_ptr_in;
		device->GetDXDevice()->CreateUnorderedAccessView(resource->GetResource(), CounterResource->GetResource(), &desc, cpu_ptr);
	}
};