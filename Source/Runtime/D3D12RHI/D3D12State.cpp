#include <string>
#include "d3dx12.h"
#include "D3D12Common.h"
#include "D3D12State.h"
#include "D3D12Shader.h"
#include "D3D12PlatformRHI.h"
#include "D3D12Rootsignature.h"
#include "D3D12PipelineState.h"
#include "D3D12AbstractDevice.h"

static D3D12_COMPARISON_FUNC TranslateCompareFunction(ECompareFunction CompareFunction)
{
	switch (CompareFunction)
	{
	case ECompareFunction::CF_Greater: return D3D12_COMPARISON_FUNC_GREATER;
	case ECompareFunction::CF_GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ECompareFunction::CF_Always:return D3D12_COMPARISON_FUNC_ALWAYS;
	default: XASSERT(false); return D3D12_COMPARISON_FUNC_ALWAYS;
	};
}

static D3D12_BLEND_OP TranslateBlendOp(EBlendOperation BlendOp)
{
	switch (BlendOp)
	{
	case EBlendOperation::BO_Add: return D3D12_BLEND_OP_ADD;
	default: XASSERT(false); return D3D12_BLEND_OP_ADD;
	};
}

static D3D12_BLEND TranslateBlendFactor(EBlendFactor BlendFactor)
{
	switch (BlendFactor)
	{
	case EBlendFactor::BF_One: return D3D12_BLEND_ONE;
	case EBlendFactor::BF_Zero: return D3D12_BLEND_ZERO;
	case EBlendFactor::BF_SourceAlpha: return D3D12_BLEND_SRC_ALPHA;
	case EBlendFactor::BF_InverseSourceAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
	default: XASSERT(false); return D3D12_BLEND_ZERO;
	};
}

uint64 XD3D12PlatformRHI::RHIGetCmdBufferOffset(XRHIStructBuffer* RHIStructBuffer)
{
	return static_cast<XD3D12StructBuffer*>(RHIStructBuffer)->ResourcePtr.GetOffsetByteFromBaseResource();
}

void* XD3D12PlatformRHI::RHIGetCommandDataPtr(std::vector<XRHICommandData>& RHICmdData, uint32& OutCmdDataSize)
{
	OutCmdDataSize = 0;

	uint32 NumVirtualAddres = RHICmdData[0].CBVs.size();
	OutCmdDataSize += NumVirtualAddres * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
	OutCmdDataSize += sizeof(D3D12_VERTEX_BUFFER_VIEW);
	OutCmdDataSize += sizeof(D3D12_INDEX_BUFFER_VIEW);
	OutCmdDataSize += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
	OutCmdDataSize = AlignArbitrary(OutCmdDataSize, sizeof(uint64));

	char* RetPtr = (char*)std::malloc(OutCmdDataSize * RHICmdData.size());
	
	uint32 Offset = 0;
	for (int i = 0; i < RHICmdData.size(); i++)
	{
		for (int j = 0; j < RHICmdData[i].CBVs.size(); j++)
		{
			XD3D12ConstantBuffer* ConstantBuffer = static_cast<XD3D12ConstantBuffer*>(RHICmdData[i].CBVs[j]);
			D3D12_GPU_VIRTUAL_ADDRESS VirtualPtr = ConstantBuffer->ResourceLocation.GetGPUVirtualPtr();
			memcpy(RetPtr + Offset, &VirtualPtr, sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
			Offset += sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		}

		XD3D12ResourcePtr_CPUGPU* VertexBufferPtr = &static_cast<XD3D12VertexBuffer*>(RHICmdData[i].VB)->ResourcePtr;

		D3D12_VERTEX_BUFFER_VIEW VertexView;
		VertexView.BufferLocation = VertexBufferPtr->GetGPUVirtualPtr();
		VertexView.StrideInBytes = RHICmdData[i].VB->GetStride();
		VertexView.SizeInBytes = RHICmdData[i].VB->GetSize();
		
		memcpy(RetPtr + Offset, &VertexView, sizeof(D3D12_VERTEX_BUFFER_VIEW));
		Offset += sizeof(D3D12_VERTEX_BUFFER_VIEW);

		XD3D12ResourcePtr_CPUGPU* IndexBufferPtr = &static_cast<XD3D12IndexBuffer*>(RHICmdData[i].IB)->ResourcePtr;
		const DXGI_FORMAT Format = (RHICmdData[i].IB->GetStride() == sizeof(uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);

		D3D12_INDEX_BUFFER_VIEW IndexView;
		IndexView.BufferLocation = IndexBufferPtr->GetGPUVirtualPtr();
		IndexView.Format = Format;
		IndexView.SizeInBytes = RHICmdData[i].IB->GetSize();

		memcpy(RetPtr + Offset, &IndexView, sizeof(D3D12_INDEX_BUFFER_VIEW));
		Offset += sizeof(D3D12_INDEX_BUFFER_VIEW);

		D3D12_DRAW_INDEXED_ARGUMENTS DrawArguments;
		DrawArguments.IndexCountPerInstance = RHICmdData[i].IndexCountPerInstance;
		DrawArguments.InstanceCount = RHICmdData[i].InstanceCount;
		DrawArguments.StartIndexLocation = RHICmdData[i].StartIndexLocation;
		DrawArguments.BaseVertexLocation = RHICmdData[i].BaseVertexLocation;
		DrawArguments.StartInstanceLocation = RHICmdData[i].StartInstanceLocation;

		memcpy(RetPtr + Offset, &DrawArguments, sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
		Offset += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);

		Offset = AlignArbitrary(Offset, sizeof(uint64));
	}

	return RetPtr;
}

std::shared_ptr<XRHICommandSignature> XD3D12PlatformRHI::RHICreateCommandSignature(XRHIIndirectArg* RHIIndirectArg, uint32 ArgCount, XRHIVertexShader* VertexShader, XRHIPixelShader* PixelShader)
{
	XD3DCommandRootSignature* RetRes = new XD3DCommandRootSignature();

	uint32 ByteStride = 0;
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC>ArgumentDescs;
	for (int i = 0; i < ArgCount; i++)
	{
		D3D12_INDIRECT_ARGUMENT_DESC ArgDesc;
		switch (RHIIndirectArg[i].type)
		{
		case IndirectArgType::Arg_CBV:
			ArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
			ArgDesc.ConstantBufferView.RootParameterIndex = RHIIndirectArg[i].CBV.RootParameterIndex;
			ByteStride += sizeof(uint64);
			break;
		
		case IndirectArgType::Arg_VBV:
			ArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
			ArgDesc.VertexBuffer.Slot = 0;
			ByteStride += (sizeof(uint64) + sizeof(uint32) + sizeof(uint32));
			break;

		case IndirectArgType::Arg_IBV:
			ArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
			ByteStride += (sizeof(uint64) + sizeof(uint32) + sizeof(uint32));
			break;
		case IndirectArgType::Arg_Draw_Indexed:
			ArgDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
			ByteStride += (sizeof(int32) + sizeof(uint32) * 4);
			break;
		default:
			XASSERT(false);
			break;
		}
		ArgumentDescs.push_back(ArgDesc);
	}

	ByteStride = AlignArbitrary(ByteStride, sizeof(uint64));

	D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
	CommandSignatureDesc.pArgumentDescs = ArgumentDescs.data();
	CommandSignatureDesc.NumArgumentDescs = ArgumentDescs.size();
	CommandSignatureDesc.ByteStride = ByteStride;

	XPipelineRegisterBoundCount RegisterBoundCount;
	memset(&RegisterBoundCount, 0, sizeof(XPipelineRegisterBoundCount));

	RegisterBoundCount.register_count[(int)EShaderType::SV_Vertex].UnorderedAccessCount
		= static_cast<XD3D12VertexShader*>(VertexShader)->ResourceCount.NumUAV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Vertex].ShaderResourceCount
		= static_cast<XD3D12VertexShader*>(VertexShader)->ResourceCount.NumSRV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Vertex].ConstantBufferCount
		= static_cast<XD3D12VertexShader*>(VertexShader)->ResourceCount.NumCBV;

	RegisterBoundCount.register_count[(int)EShaderType::SV_Pixel].UnorderedAccessCount
		= static_cast<XD3D12PixelShader*>(PixelShader)->ResourceCount.NumUAV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Pixel].ShaderResourceCount
		= static_cast<XD3D12PixelShader*>(PixelShader)->ResourceCount.NumSRV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Pixel].ConstantBufferCount
		= static_cast<XD3D12PixelShader*>(PixelShader)->ResourceCount.NumCBV;

	std::size_t BoundHash = std::hash<std::string>{}(std::string((char*)&RegisterBoundCount, sizeof(XPipelineRegisterBoundCount)));

	auto RootIter = AbsDevice->GetRootSigMap().find(BoundHash);
	if (RootIter == AbsDevice->GetRootSigMap().end())
	{
		std::shared_ptr<XD3D12RootSignature>RootSigPtr = std::make_shared<XD3D12RootSignature>();
		RootSigPtr->Create(PhyDevice.get(), RegisterBoundCount);
		AbsDevice->GetRootSigMap()[BoundHash] = RootSigPtr;
	}

	ID3D12RootSignature*  RootSigPtr=AbsDevice->GetRootSigMap()[BoundHash]->GetDXRootSignature();
	ThrowIfFailed(PhyDevice->GetDXDevice()->CreateCommandSignature(&CommandSignatureDesc, RootSigPtr, IID_PPV_ARGS(&RetRes->DxCommandSignature)));
	return std::shared_ptr<XRHICommandSignature>(RetRes);
}

static void GetPSODescAndHash(
	const XGraphicsPSOInitializer& PSOInit, 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& PSODesc,
	std::size_t& OutHash)
{
	ZeroMemory(&PSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	const XRHIBoundShaderStateInput& BoundShaderState = PSOInit.BoundShaderState;

	//Layout
	XD3D12VertexLayout* D3D12Layout = static_cast<XD3D12VertexLayout*>(BoundShaderState.RHIVertexLayout);
	PSODesc.InputLayout = { D3D12Layout->VertexElements.data(),(UINT)D3D12Layout->VertexElements.size() };

	//Shsader
	PSODesc.VS = static_cast<XD3D12VertexShader*>(BoundShaderState.RHIVertexShader)->D3DByteCode;
	PSODesc.PS = static_cast<XD3D12PixelShader*>(BoundShaderState.RHIPixelShader)->D3DByteCode;

	//State
	PSODesc.BlendState = static_cast<XD3D12BlendState*>(PSOInit.BlendState)->Desc;
	PSODesc.DepthStencilState = static_cast<XD3D12DepthStencilState*>(PSOInit.DepthStencilState)->Desc;

	//RTV DSV
	PSODesc.NumRenderTargets = PSOInit.RTNums;
	for (int i = 0; i < PSOInit.RTNums; i++)
	{
		PSODesc.RTVFormats[i] = (DXGI_FORMAT)GPixelFormats[(int)PSOInit.RT_Format[i]].PlatformFormat;
	}
	
	PSODesc.DSVFormat = FindDepthStencilDXGIFormat((DXGI_FORMAT)GPixelFormats[(int)PSOInit.DS_Format].PlatformFormat);

	//default
	PSODesc.SampleMask = UINT_MAX;
	PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	XASSERT((PSOInit.RasterState != nullptr));
	PSODesc.RasterizerState = static_cast<XD3D12RasterizationState*>(PSOInit.RasterState)->Desc;
	PSODesc.SampleDesc.Count = 1;
	PSODesc.SampleDesc.Quality = 0;

	//GetHash
	OutHash = 42;
	std::size_t StateHash = std::hash<std::string>{}(std::string(
		(char*)(&PSODesc.BlendState),
		(sizeof(D3D12_BLEND_DESC) + sizeof(UINT) + sizeof(D3D12_RASTERIZER_DESC) + sizeof(D3D12_DEPTH_STENCIL_DESC))
	));
	std::size_t RT_DS_HAHS = std::hash<std::string>{}(std::string(
		(char*)(&PSODesc.NumRenderTargets),
		(sizeof(UINT) + sizeof(DXGI_FORMAT) * 9 + sizeof(DXGI_SAMPLE_DESC) + sizeof(UINT))
	));
	THashCombine(OutHash, StateHash);
	THashCombine(OutHash, RT_DS_HAHS);
	THashCombine(OutHash, PSOInit.BoundShaderState.RHIVertexShader->GetHash());
	THashCombine(OutHash, PSOInit.BoundShaderState.RHIPixelShader->GetHash());
}

static void GetBoundCountAndHash(
	const XGraphicsPSOInitializer& PSOInit,
	XPipelineRegisterBoundCount& RegisterBoundCount,
	std::size_t& OutRootSigHash)
{
	const XRHIBoundShaderStateInput& BoundShaderState = PSOInit.BoundShaderState;

	//RootSiganture
	memset(&RegisterBoundCount, 0, sizeof(XPipelineRegisterBoundCount));

	RegisterBoundCount.register_count[(int)EShaderType::SV_Vertex].UnorderedAccessCount
		= static_cast<XD3D12VertexShader*>(BoundShaderState.RHIVertexShader)->ResourceCount.NumUAV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Vertex].ShaderResourceCount
		= static_cast<XD3D12VertexShader*>(BoundShaderState.RHIVertexShader)->ResourceCount.NumSRV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Vertex].ConstantBufferCount
		= static_cast<XD3D12VertexShader*>(BoundShaderState.RHIVertexShader)->ResourceCount.NumCBV;

	RegisterBoundCount.register_count[(int)EShaderType::SV_Pixel].UnorderedAccessCount
		= static_cast<XD3D12PixelShader*>(BoundShaderState.RHIPixelShader)->ResourceCount.NumUAV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Pixel].ShaderResourceCount
		= static_cast<XD3D12PixelShader*>(BoundShaderState.RHIPixelShader)->ResourceCount.NumSRV;
	RegisterBoundCount.register_count[(int)EShaderType::SV_Pixel].ConstantBufferCount
		= static_cast<XD3D12PixelShader*>(BoundShaderState.RHIPixelShader)->ResourceCount.NumCBV;

	OutRootSigHash = std::hash<std::string>{}
	(std::string((char*)&RegisterBoundCount, sizeof(XPipelineRegisterBoundCount)));
	
}
static std::unordered_map<std::size_t, std::shared_ptr<XD3D12PSOStoreID3DPSO>>HashToID3D12;

#define USE_PIPELINE_LIBRARY 0

std::shared_ptr<XRHIComputePSO> XD3D12PlatformRHI::RHICreateComputePipelineState(const XRHIComputeShader* RHIComputeShader)
{
	std::size_t PSOHash = RHIComputeShader->GetHash();
	auto iter = HashToID3D12.find(PSOHash);

	const XD3D12ComputeShader* D3DCSShader = static_cast<const XD3D12ComputeShader*>(RHIComputeShader);
	if (iter == HashToID3D12.end())
	{
		XD3D12PSOStoreID3DPSO* IPSO = new XD3D12PSOStoreID3DPSO();

		D3D12_COMPUTE_PIPELINE_STATE_DESC PSODesc;
		ZeroMemory(&PSODesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));
		PSODesc.pRootSignature = D3DCSShader->RootSignature->GetDXRootSignature();
		PSODesc.CS = D3DCSShader->D3DByteCode;
		PSODesc.NodeMask = 0;
		PSODesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;


		const std::wstring PSOCacheName = std::to_wstring(PSOHash);
#if USE_PIPELINE_LIBRARY
		bool loaded = PhyDevice->GetD3D12PipelineLibrary()->LoadPSOFromLibrary(PSOCacheName.data(), &PSODesc, IPSO->GetID3DPSO_Address());
		if (!loaded)
		{
#endif
			ThrowIfFailed(PhyDevice->GetDXDevice()->CreateComputePipelineState(&PSODesc, IID_PPV_ARGS(IPSO->GetID3DPSO_Address())));
#if USE_PIPELINE_LIBRARY
			PhyDevice->GetD3D12PipelineLibrary()->StorePSOToLibrary(PSOCacheName.data(), IPSO->GetID3DPSO());
		}
#endif
		HashToID3D12[PSOHash] = std::shared_ptr<XD3D12PSOStoreID3DPSO>(IPSO);
	}
	return std::make_shared<XD3DComputePSO>(D3DCSShader, HashToID3D12[PSOHash].get());

}
std::shared_ptr<XRHIGraphicsPSO> XD3D12PlatformRHI::RHICreateGraphicsPipelineState(const XGraphicsPSOInitializer& PSOInit)
{

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc;
	std::size_t PSOHash;
	GetPSODescAndHash(PSOInit, PSODesc, PSOHash);
	
	XPipelineRegisterBoundCount RegisterBoundCount;
	std::size_t BoundHash;
	GetBoundCountAndHash(PSOInit, RegisterBoundCount, BoundHash);

	
	auto RootIter = AbsDevice->GetRootSigMap().find(BoundHash);
	if (RootIter == AbsDevice->GetRootSigMap().end())
	{
		std::shared_ptr<XD3D12RootSignature>RootSigPtr = std::make_shared<XD3D12RootSignature>();
		RootSigPtr->Create(PhyDevice.get(), RegisterBoundCount);
		AbsDevice->GetRootSigMap()[BoundHash] = RootSigPtr;
	}
	PSODesc.pRootSignature = AbsDevice->GetRootSigMap()[BoundHash]->GetDXRootSignature();
	

	THashCombine(PSOHash, BoundHash);

	auto PsoIter = HashToID3D12.find(PSOHash);
	
	if (PsoIter == HashToID3D12.end())
	{
		const std::wstring PSOCacheName = std::to_wstring(PSOHash);
		XD3D12PSOStoreID3DPSO* IPSO = new XD3D12PSOStoreID3DPSO();

#if USE_PIPELINE_LIBRARY
		bool loaded = PhyDevice->GetD3D12PipelineLibrary()->LoadPSOFromLibrary(PSOCacheName.data(), &PSODesc, IPSO->GetID3DPSO_Address());		
		if (!loaded)
		{
#endif
			ThrowIfFailed(PhyDevice->GetDXDevice()->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(IPSO->GetID3DPSO_Address())));
#if USE_PIPELINE_LIBRARY
			PhyDevice->GetD3D12PipelineLibrary()->StorePSOToLibrary(PSOCacheName.data(), IPSO->GetID3DPSO());
		}
#endif
		HashToID3D12[PSOHash] = std::shared_ptr<XD3D12PSOStoreID3DPSO>(IPSO);
	}
	return std::make_shared<XD3DGraphicsPSO>(PSOInit, AbsDevice->GetRootSigMap()[BoundHash].get(), HashToID3D12[PSOHash].get());
}



std::shared_ptr<XRHIRasterizationState> XD3D12PlatformRHI::RHICreateRasterizationStateState(const XRasterizationStateInitializerRHI& Initializer)
{
	XD3D12RasterizationState* RasterizationState = new XD3D12RasterizationState;
	D3D12_RASTERIZER_DESC& RasterStateDesc = RasterizationState->Desc;
	RasterStateDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	
	D3D12_CULL_MODE CullMode;
	switch (Initializer.CullMode)
	{
	case EFaceCullMode::FC_Front:
		CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_FRONT;
		break;
	[[likely]] case EFaceCullMode::FC_Back:
		CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
		break;
	case EFaceCullMode::FC_None:
		CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
		break;
	}
	RasterStateDesc.CullMode = CullMode;
	RasterStateDesc.ConservativeRaster = Initializer.bConservative ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return std::shared_ptr<XRHIRasterizationState>(RasterizationState);
}

std::shared_ptr<XRHIDepthStencilState> XD3D12PlatformRHI::RHICreateDepthStencilState(const XDepthStencilStateInitializerRHI& Initializer)
{
	XD3D12DepthStencilState* DepthStencilState = new XD3D12DepthStencilState;
	D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc = DepthStencilState->Desc;
	memset(&DepthStencilDesc, 0, sizeof(D3D12_DEPTH_STENCIL_DESC));

	DepthStencilDesc.DepthEnable = Initializer.DepthCompFunc != ECompareFunction::CF_Always || Initializer.bEnableDepthWrite;
	DepthStencilDesc.DepthWriteMask = Initializer.bEnableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthStencilDesc.DepthFunc = TranslateCompareFunction(Initializer.DepthCompFunc);
	return std::shared_ptr<XRHIDepthStencilState>(DepthStencilState);
}

std::shared_ptr<XRHIBlendState> XD3D12PlatformRHI::RHICreateBlendState(const XBlendStateInitializerRHI& Initializer)
{
	XD3D12BlendState* BlendState = new XD3D12BlendState;
	D3D12_BLEND_DESC& BlendStateDesc = BlendState->Desc;
	BlendStateDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BlendStateDesc.RenderTarget[0].BlendEnable = Initializer.RenderTargets[0].RTBlendeEnable;

	BlendStateDesc.RenderTarget[0].BlendOp = TranslateBlendOp(Initializer.RenderTargets[0].RTColorBlendOp);
	BlendStateDesc.RenderTarget[0].SrcBlend = TranslateBlendFactor(Initializer.RenderTargets[0].RTColorSrcBlend);
	BlendStateDesc.RenderTarget[0].DestBlend = TranslateBlendFactor(Initializer.RenderTargets[0].RTColorDestBlend);

	BlendStateDesc.RenderTarget[0].BlendOpAlpha = TranslateBlendOp(Initializer.RenderTargets[0].RTAlphaBlendOp);
	BlendStateDesc.RenderTarget[0].SrcBlendAlpha = TranslateBlendFactor(Initializer.RenderTargets[0].RTAlphaSrcBlend);
	BlendStateDesc.RenderTarget[0].DestBlendAlpha = TranslateBlendFactor(Initializer.RenderTargets[0].RTAlphaDestBlend);
	
	return std::shared_ptr<XRHIBlendState>(BlendState);
}
