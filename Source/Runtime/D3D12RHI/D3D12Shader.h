#pragma once

#include <d3d12.h>
#include "D3D12Rootsignature.h"
#include "Runtime/RHI/RHIResource.h"
#include "Runtime/RenderCore/ShaderCore.h"


using D3DVertexLayoutArray = std::vector<D3D12_INPUT_ELEMENT_DESC>;
class XD3D12VertexLayout :public XRHIVertexLayout
{
public:
	D3DVertexLayoutArray VertexElements;
	explicit XD3D12VertexLayout(const D3DVertexLayoutArray& InVertexElements) :
		VertexElements(InVertexElements) {}
};

//TODO:XD3D12Shader

class XD3D12VertexShader :public XRHIVertexShader
{
public:
	std::vector<uint8>BinaryCode;
	D3D12_SHADER_BYTECODE D3DByteCode;
	XShaderResourceCount ResourceCount;
};

class XD3D12PixelShader : public XRHIPixelShader
{
public:
	std::vector<uint8>BinaryCode;
	D3D12_SHADER_BYTECODE D3DByteCode;
	XShaderResourceCount ResourceCount;
};

class XD3D12ComputeShader :public XRHIComputeShader
{
public:
	std::vector<uint8>BinaryCode;
	D3D12_SHADER_BYTECODE D3DByteCode;
	XShaderResourceCount ResourceCount;
	const XD3D12RootSignature* RootSignature;
};

class XD3D12BoundShaderState
{
public:
	const XD3D12RootSignature* RootSignature;
	
	XD3D12VertexLayout* VertexLayout;
	XD3D12VertexShader* VertexShader;
	XD3D12PixelShader* PiexlShader;
};