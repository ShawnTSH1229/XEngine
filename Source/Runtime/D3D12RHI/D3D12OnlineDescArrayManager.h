#pragma once
#include "D3D12Rootsignature.h"
#include "D3D12OnlineDescArray.h"

#include "D3D12View.h"

class XD3DDirectContex;

template<typename MaskType>
struct SlotManager
{
	static inline void SetSlotNeedSetBit(MaskType& SlotMask, uint32 SlotIndex)
	{
		SlotMask |= ((MaskType)1 << SlotIndex);
	}
	static inline bool IsSlotNeedSet(const MaskType& SlotMask, uint32 SlotIndex)
	{
		return (SlotMask & ((MaskType)1 << SlotIndex)) != 0;
	}

	static inline void SlotsBitsUnSet(MaskType& SlotMask, uint32 NumSlots)
	{
		SlotMask &= ~(((MaskType)1 << NumSlots) - 1);
	}

	static inline void SlotBitUnSet(MaskType& SlotMask, uint32 SlotIndex)
	{
		SlotMask &= ~((MaskType)1 << SlotIndex);
	}

	inline void UnSetAllBit()
	{
		memset(Mask, 0, sizeof(Mask));
	}

	MaskType Mask[EShaderType_Underlying(EShaderType::SV_ShaderCount)];
};

struct XD3D12CBVRootDescManager : public SlotManager<uint16>
{
	inline void UnsetMasks()
	{
		UnSetAllBit();
		memset(CurrentGPUVirtualAddress, 0, sizeof(CurrentGPUVirtualAddress));
	}
	D3D12_GPU_VIRTUAL_ADDRESS CurrentGPUVirtualAddress[EShaderType_Underlying(EShaderType::SV_ShaderCount)][MAX_ROOT_CONSTANT_NUM];
};


struct XD3D12PassShaderResourceManager : public SlotManager<uint64>
{
	inline void UnsetMasks()
	{
		UnSetAllBit();
		memset(Views, 0, sizeof(Views));
	}
	XD3D12ShaderResourceView* Views[EShaderType_Underlying(EShaderType::SV_ShaderCount)][MAX_SHADER_RESOURCE_NUM];
};

struct XD3D12PassUnorderedAcessManager : public SlotManager<uint16>
{
	inline void UnsetMasks()
	{
		UnSetAllBit();
		memset(Views, 0, sizeof(Views));
	}
	XD3D12UnorderedAcessView* Views[EShaderType_Underlying(EShaderType::SV_ShaderCount)][MAX_UAV_NUM];
};

class XD3D12OnlineDescArrayManager
{
private:
	XD3D12Device*				device;
	XD3DDirectContex*				direct_ctx;
	XD3D12OnlineDescArray	pipeline_current_desc_array;
public:
	XD3D12OnlineDescArrayManager() :device(nullptr), direct_ctx(nullptr) {};
	void Create(XD3D12Device* device_in,XD3DDirectContex* direct_ctx_in);
	inline XD3D12OnlineDescArray* GetCurrentDescArray() { return &pipeline_current_desc_array; }

	template<EShaderType shader_type>
	void SetDescTableUAVs(
		const XD3D12RootSignature* root_signature,
		XD3D12PassUnorderedAcessManager* UAVManager,
		uint32& slot_start, uint16& slot_mask);

	template<EShaderType shader_type>
	void SetDescTableSRVs(
		const XD3D12RootSignature* root_signature, 
		XD3D12PassShaderResourceManager* SRVManager,
		uint32& slot_start, uint64& slot_mask);

	template<EShaderType shader_type>
	void SetRootDescCBVs(
		const XD3D12RootSignature* root_signature,
		XD3D12CBVRootDescManager* gpu_virtual_ptr_array,
		uint16& slot_mask);
};