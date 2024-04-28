#include "D3D12Rootsignature.h"

static D3D12_SHADER_VISIBILITY GetShaderVisibility(EShaderType shader_visibility);
static D3D12_STATIC_SAMPLER_DESC MakeStaticSampler(D3D12_FILTER Filter, D3D12_TEXTURE_ADDRESS_MODE WrapMode, uint32 Register, uint32 Space);

// Static sampler table must match D3DCommon.ush
static const D3D12_STATIC_SAMPLER_DESC StaticSamplerDescs[] =
{
	MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT,        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  0, 1000),
	MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT,        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 1, 1000),
	MakeStaticSampler(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP,  2, 1000),
	MakeStaticSampler(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 3, 1000),
	MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR,       D3D12_TEXTURE_ADDRESS_MODE_WRAP,  4, 1000),
	MakeStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR,       D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 5, 1000),
};

void XD3D12RootSignature::Create(XD3D12Device* device_in, XPipelineRegisterBoundCount& register_count)
{
	device = device_in;
	memset(ShaderResourceBindSlotIndexArray, 0xFF, sizeof(ShaderResourceBindSlotIndexArray));

	for (uint32 i = 0; i < PIPELINE_MAX_ROOT_PARAM_COUNT; ++i)
	{
		desc_range_array[i].BaseShaderRegister = 0;
		desc_range_array[i].RegisterSpace = 0;
		desc_range_array[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		slot_array[i].DescriptorTable.pDescriptorRanges = &desc_range_array[i];
		slot_array[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		slot_array[i].DescriptorTable.NumDescriptorRanges = 1;
	}

	uint32 root_parameter_count = 0;
	for (int i = 0; i < EShaderType_Underlying(EShaderType::SV_ShaderCount); i++)
	{
		const XShaderRegisterCounts& Shader = register_count.register_count[i];
		D3D12_SHADER_VISIBILITY visibility = GetShaderVisibility(EShaderType(i));

		if (Shader.ShaderResourceCount > 0)
		{
			desc_range_array[root_parameter_count].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			desc_range_array[root_parameter_count].NumDescriptors = Shader.ShaderResourceCount;

			slot_array[root_parameter_count].ShaderVisibility = visibility;
			SetSRVDescTableBindSlot(EShaderType(i), root_parameter_count);
			root_parameter_count++;
		}

		if (Shader.ConstantBufferCount > MAX_ROOT_CONSTANT_NUM)
		{
			desc_range_array[root_parameter_count].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			desc_range_array[root_parameter_count].NumDescriptors = Shader.ConstantBufferCount - MAX_ROOT_CONSTANT_NUM;

			slot_array[root_parameter_count].ShaderVisibility = visibility;
			SetCBVDescTableBindSlot(EShaderType(i), root_parameter_count);
			root_parameter_count++;

			XASSERT(false);
		}

		if (Shader.UnorderedAccessCount > 0)
		{
			desc_range_array[root_parameter_count].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			desc_range_array[root_parameter_count].NumDescriptors = Shader.UnorderedAccessCount;

			slot_array[root_parameter_count].ShaderVisibility = visibility;
			SetUAVDescTableTBindSlot(EShaderType(i), root_parameter_count);
			
			root_parameter_count++;
		}

		if (Shader.SamplerCount > 0)
		{
			desc_range_array[root_parameter_count].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
			desc_range_array[root_parameter_count].NumDescriptors = Shader.SamplerCount;

			slot_array[root_parameter_count].ShaderVisibility = visibility;
			SetSampleDescTableBindSlot(EShaderType(i), root_parameter_count);
			root_parameter_count++;
		}
	}

	for (int i = 0; i < EShaderType_Underlying(EShaderType::SV_ShaderCount); i++)
	{
		const XShaderRegisterCounts& Shader = register_count.register_count[i];

		for (uint32 ShaderRegister = 0; (ShaderRegister < Shader.ConstantBufferCount) && (ShaderRegister < MAX_ROOT_CONSTANT_NUM); ShaderRegister++)
		{
			slot_array[root_parameter_count].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			slot_array[root_parameter_count].Descriptor.ShaderRegister = ShaderRegister;
			slot_array[root_parameter_count].Descriptor.RegisterSpace = 0;
			slot_array[root_parameter_count].ShaderVisibility = GetShaderVisibility(EShaderType(i));
			if (ShaderRegister == 0)
			{
				SetCBVRootDescBindSlot(EShaderType(i), root_parameter_count);
			}
			root_parameter_count++;
		}
	}


	root_signature_info.NumParameters = root_parameter_count;
	root_signature_info.pParameters = slot_array;
	root_signature_info.NumStaticSamplers = 6;
	root_signature_info.pStaticSamplers = StaticSamplerDescs;

	//The value in denying access to shader stages is a minor optimization on some hardware.
	root_signature_info.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	XDxRefCount<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&root_signature_info, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->GetDXDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(root_signature.GetAddressOf())));
}
uint32 XD3D12RootSignature::GetSRVDescTableBindSlot(EShaderType shader_type)const
{
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_SRVs];
		break;
	case EShaderType::SV_Pixel:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_SRVs];
		break;
	case EShaderType::SV_Compute:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_SRVs];
		break;
	default:
		XASSERT(false);
		return 0;
		break;
	}
}

uint32 XD3D12RootSignature::GetUADescTableBindSlot(EShaderType shader_type) const
{
	switch (shader_type)
	{
	case EShaderType::SV_Compute:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_UAVs];
		break;
	case EShaderType::SV_Pixel:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_UAVs];
		break;
	default:
		XASSERT(false);
		return 0;
		break;
	}
}

uint32 XD3D12RootSignature::GetCBVDescTableBindSlot(EShaderType shader_type)const
{
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_CBVs];
		break;
	case EShaderType::SV_Pixel:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_CBVs];
		break;
	case EShaderType::SV_Compute:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_CBVs];
		break;
	default:
		XASSERT(false);
		return 0;
		break;
	}
}

uint32 XD3D12RootSignature::GetSampleDescTableBindSlot(EShaderType shader_type)const
{
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_Samplers];
		break;
	case EShaderType::SV_Pixel:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_Samplers];
		break;
	case EShaderType::SV_Compute:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_Samplers];
		break;
	default:
		XASSERT(false);
		return 0;
		break;
	}
}

uint32 XD3D12RootSignature::GetCBVRootDescBindSlot(EShaderType shader_type)const
{
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_RootCBVs];
		break;
	case EShaderType::SV_Pixel:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_RootCBVs];
		break;
	case EShaderType::SV_Compute:
		return ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_RootCBVs];
		break;
	default:
		XASSERT(false);
		return 0;
		break;
	}
}

void XD3D12RootSignature::SetSRVDescTableBindSlot(EShaderType shader_type, uint8 RootParameterIndex)
{
	uint8* pBindSlot = nullptr;
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_SRVs];
		break;
	case EShaderType::SV_Pixel:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_SRVs];
		break;
	case EShaderType::SV_Compute:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_SRVs];
		break;
	default:
		XASSERT(false);
		break;
	}
	*pBindSlot = RootParameterIndex;
}

void XD3D12RootSignature::SetCBVDescTableBindSlot(EShaderType shader_type, uint8 RootParameterIndex)
{
	uint8* pBindSlot = nullptr;
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_CBVs];
		break;
	case EShaderType::SV_Pixel:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_CBVs];
		break;
	case EShaderType::SV_Compute:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_CBVs];
		break;
	default:
		XASSERT(false);
		break;
	}
	*pBindSlot = RootParameterIndex;
}

void XD3D12RootSignature::SetSampleDescTableBindSlot(EShaderType shader_type, uint8 RootParameterIndex)
{
	uint8* pBindSlot = nullptr;
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_Samplers];
		break;
	case EShaderType::SV_Pixel:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_Samplers];
		break;
	case EShaderType::SV_Compute:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_Samplers];
		break;
	default:
		XASSERT(false);
		break;
	}
	*pBindSlot = RootParameterIndex;
}

void XD3D12RootSignature::SetUAVDescTableTBindSlot(EShaderType shader_type, uint8 RootParameterIndex)
{
	uint8* pBindSlot = nullptr;
	switch (shader_type)
	{
	case EShaderType::SV_Compute:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_UAVs];
		break;
	case EShaderType::SV_Pixel:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_UAVs];
		break;
	default:
		XASSERT(false);
		break;
	}
	*pBindSlot = RootParameterIndex;
}

void XD3D12RootSignature::SetCBVRootDescBindSlot(EShaderType shader_type, uint8 RootParameterIndex)
{
	uint8* pBindSlot = nullptr;
	switch (shader_type)
	{
	case EShaderType::SV_Vertex:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::VS_RootCBVs];
		break;
	case EShaderType::SV_Pixel:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::PS_RootCBVs];
		break;
	case EShaderType::SV_Compute:
		pBindSlot = &ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::ALL_RootCBVs];
		break;
	default:
		XASSERT(false);
		break;
	}
	*pBindSlot = RootParameterIndex;
}

static D3D12_SHADER_VISIBILITY GetShaderVisibility(EShaderType shader_visibility)
{
	D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
	switch (shader_visibility)
	{
	case EShaderType::SV_Vertex:
		visibility = D3D12_SHADER_VISIBILITY_VERTEX;
		break;
	case EShaderType::SV_Pixel:
		visibility = D3D12_SHADER_VISIBILITY_PIXEL;
		break;
	case EShaderType::SV_Compute:
		visibility = D3D12_SHADER_VISIBILITY_ALL;
		break;
	default:
		XASSERT(false);
		break;
	}
	return visibility;
}

static D3D12_STATIC_SAMPLER_DESC MakeStaticSampler(D3D12_FILTER Filter, D3D12_TEXTURE_ADDRESS_MODE WrapMode, uint32 Register, uint32 Space)
{
	D3D12_STATIC_SAMPLER_DESC Result = {};

	Result.Filter = Filter;
	Result.AddressU = WrapMode;
	Result.AddressV = WrapMode;
	Result.AddressW = WrapMode;
	Result.MipLODBias = 0.0f;
	Result.MaxAnisotropy = 1;
	Result.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	Result.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	Result.MinLOD = 0.0f;
	Result.MaxLOD = D3D12_FLOAT32_MAX;
	Result.ShaderRegister = Register;
	Result.RegisterSpace = Space;
	Result.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	return Result;
}