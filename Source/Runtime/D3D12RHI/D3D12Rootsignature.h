#pragma once
#include "Runtime/RHI/RHI.h"
#include "D3D12PhysicDevice.h"

struct XShaderRegisterCounts
{
	uint8 SamplerCount;
	uint8 ConstantBufferCount;
	uint8 ShaderResourceCount;
	uint8 UnorderedAccessCount;
};

enum class ERootParameterKeys
{
	VS_SRVs,
	VS_CBVs,
	VS_RootCBVs,
	VS_Samplers,

	PS_SRVs,
	PS_CBVs,
	PS_RootCBVs,
	PS_Samplers,

	ALL_SRVs,
	ALL_CBVs,
	ALL_RootCBVs,
	ALL_Samplers,

	ALL_UAVs,
	Count,
};

struct XPipelineRegisterBoundCount
{
	XShaderRegisterCounts register_count[EShaderType_Underlying(EShaderType::SV_ShaderCount)];
};


#define MAX_ROOT_CONSTANT_NUM 16
#define MAX_UAV_NUM 16
#define MAX_SHADER_RESOURCE_NUM 64

#define PIPELINE_MAX_ROOT_PARAM_COUNT 32
class XD3D12RootSignature
{
private:
	uint8 ShaderResourceBindSlotIndexArray[(int)ERootParameterKeys::Count];

	D3D12_ROOT_PARAMETER  slot_array[PIPELINE_MAX_ROOT_PARAM_COUNT];
	D3D12_DESCRIPTOR_RANGE  desc_range_array[PIPELINE_MAX_ROOT_PARAM_COUNT];

	D3D12_ROOT_SIGNATURE_DESC root_signature_info;
	XRefCountPtr<ID3D12RootSignature> root_signature;
	XDxRefCount<ID3DBlob> serializedRootSig;

	XD3D12Device* device;//TODO
public:
	XD3D12RootSignature() :device(nullptr), serializedRootSig(nullptr), root_signature(nullptr) {}
	~XD3D12RootSignature() {}
	void Create(XD3D12Device* device_in, XPipelineRegisterBoundCount& register_count);

	uint32 GetSRVDescTableBindSlot(EShaderType shader_type)const;
	uint32 GetUADescTableBindSlot(EShaderType shader_type)const;
	uint32 GetCBVDescTableBindSlot(EShaderType shader_type)const;
	uint32 GetSampleDescTableBindSlot(EShaderType shader_type)const;
	uint32 GetCBVRootDescBindSlot(EShaderType shader_type)const;

	void SetSRVDescTableBindSlot(EShaderType shader_type, uint8 RootParameterIndex);
	void SetCBVDescTableBindSlot(EShaderType shader_type, uint8 RootParameterIndex);
	void SetSampleDescTableBindSlot(EShaderType shader_type, uint8 RootParameterIndex);
	void SetUAVDescTableTBindSlot(EShaderType shader_type, uint8 RootParameterIndex);
	void SetCBVRootDescBindSlot(EShaderType shader_type, uint8 RootParameterIndex);

	inline ID3D12RootSignature* GetDXRootSignature()const { return root_signature.Get(); }
};

