#include "D3D12Shader.h"
#include "D3D12PlatformRHI.h"
#include "D3D12AbstractDevice.h"

std::shared_ptr<XRHIComputeShader> XD3D12PlatformRHI::RHICreateComputeShader(XArrayView<uint8> Code)
{
	XD3D12ComputeShader* Shader = new XD3D12ComputeShader;
	XShaderCodeReader ShaderCodeReader(Code);

	const XShaderResourceCount* ResourceCount = (const XShaderResourceCount*)(ShaderCodeReader.FindOptionalData(
		XShaderResourceCount::Key, sizeof(XShaderResourceCount)));
	if (ResourceCount == 0) { XASSERT(false); return nullptr; }

	const uint8* CodaData = (const uint8*)Code.data();
	Shader->BinaryCode.insert(Shader->BinaryCode.end(), &CodaData[0], &CodaData[Code.size()]);
	Shader->ResourceCount = *ResourceCount;

	SIZE_T CodeSize = ShaderCodeReader.GetActualShaderCodeSize();
	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = Shader->BinaryCode.data();
	ShaderBytecode.BytecodeLength = CodeSize;
	Shader->D3DByteCode = ShaderBytecode;

	XPipelineRegisterBoundCount RegisterBoundCount;
	memset(&RegisterBoundCount, 0, sizeof(XPipelineRegisterBoundCount));
	RegisterBoundCount.register_count[(int)EShaderType::SV_Compute].ConstantBufferCount = ResourceCount->NumCBV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Compute].ShaderResourceCount = ResourceCount->NumSRV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Compute].UnorderedAccessCount = ResourceCount->NumUAV;

	std::size_t RootHash = std::hash<std::string>{}
	(std::string((char*)&RegisterBoundCount, sizeof(XPipelineRegisterBoundCount)));

	auto& RootMap = AbsDevice->GetRootSigMap();
	auto iter = RootMap.find(RootHash);
	if (iter == RootMap.end())
	{
		std::shared_ptr<XD3D12RootSignature>RootSigPtr = std::make_shared<XD3D12RootSignature>();
		RootSigPtr->Create(PhyDevice.get(), RegisterBoundCount);
		RootMap[RootHash] = RootSigPtr;
	}
	Shader->RootSignature = RootMap[RootHash].get();
	return std::shared_ptr<XD3D12ComputeShader>(Shader);
}


std::shared_ptr<XRHIVertexShader> XD3D12PlatformRHI::RHICreateVertexShader(XArrayView<uint8> Code)
{
	XD3D12VertexShader* VertexShader = new XD3D12VertexShader;
	XShaderCodeReader ShaderCodeReader(Code);

	const XShaderResourceCount* ResourceCount = (const XShaderResourceCount*)(ShaderCodeReader.FindOptionalData(
		XShaderResourceCount::Key, sizeof(XShaderResourceCount)));
	if (ResourceCount == 0) { XASSERT(false); return nullptr; }

	const uint8* CodaData = (const uint8*)Code.data();
	VertexShader->BinaryCode.insert(VertexShader->BinaryCode.end(), &CodaData[0], &CodaData[Code.size()]);
	VertexShader->ResourceCount = *ResourceCount;

	SIZE_T CodeSize = ShaderCodeReader.GetActualShaderCodeSize();
	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = VertexShader->BinaryCode.data();
	ShaderBytecode.BytecodeLength = CodeSize;
	VertexShader->D3DByteCode = ShaderBytecode;
	return std::shared_ptr<XRHIVertexShader>(VertexShader);
}

std::shared_ptr<XRHIPixelShader> XD3D12PlatformRHI::RHICreatePixelShader(XArrayView<uint8> Code)
{
	XD3D12PixelShader* PixelShader = new XD3D12PixelShader;
	XShaderCodeReader ShaderCodeReader(Code);

	const XShaderResourceCount* ResourceCount = (const XShaderResourceCount*)(ShaderCodeReader.FindOptionalData(
		XShaderResourceCount::Key, sizeof(XShaderResourceCount)));
	if (ResourceCount == 0) { return nullptr; }

	const uint8* CodaData = (const uint8*)Code.data();
	PixelShader->BinaryCode.insert(PixelShader->BinaryCode.end(), &CodaData[0], &CodaData[Code.size()]);
	PixelShader->ResourceCount = *ResourceCount;

	SIZE_T CodeSize = ShaderCodeReader.GetActualShaderCodeSize();
	D3D12_SHADER_BYTECODE ShaderBytecode;
	ShaderBytecode.pShaderBytecode = PixelShader->BinaryCode.data();
	ShaderBytecode.BytecodeLength = CodeSize;
	PixelShader->D3DByteCode = ShaderBytecode;
	return std::shared_ptr<XRHIPixelShader>(PixelShader);
}