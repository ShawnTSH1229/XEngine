#pragma once
#include "ShaderCore.h"
#include "Runtime/HAL/Mch.h"
#include "Runtime\RHI\RHICommandList.h"

class XShaderVariableParameter
{
public:
	XShaderVariableParameter():BufferIndex(0), VariableOffsetInBuffer(0),NumBytes(0) {}
	inline void Bind(const XShaderParameterMap& ParameterMap, const char* ParameterName)
	{
		auto iter = ParameterMap.MapNameToParameter.find(ParameterName);
		if (iter != ParameterMap.MapNameToParameter.end())
		{
			BufferIndex = iter->second.BufferIndex;
			VariableOffsetInBuffer = iter->second.VariableOffsetInBuffer;
			NumBytes = iter->second.VariableSize;
		}
	}

	inline uint16 GetBufferIndex()const { return BufferIndex; }
	inline uint16 GetVariableOffsetInBuffer()const { return VariableOffsetInBuffer; }
	inline uint16 GetNumBytes()const { return NumBytes; }
private:
	uint16 BufferIndex;
	uint16 VariableOffsetInBuffer;
	uint16 NumBytes;
};

enum class EParameterType
{
	PT_Texture,
	PT_SRV,
	PT_UAV,
	PT_CBV
};

class XSRVCBVUAVParameter
{
public:
	XSRVCBVUAVParameter(EParameterType InType) :ResourceIndex(0), ResourceNum(0) , Type(InType){}
	inline void Bind(const XShaderParameterMap& ParameterMap, const char* ParameterName)
	{
		auto iter = ParameterMap.MapNameToParameter.find(ParameterName);
		if (iter != ParameterMap.MapNameToParameter.end())
		{
			ResourceIndex = iter->second.BufferIndex;
			ResourceNum = iter->second.ResourceCount;
			return;
		}
		XASSERT_TEMP(false);
	}
	uint16 GetResourceIndex()const { return ResourceIndex; }
	uint16 GetResourceNum()const { return ResourceNum; };
	EParameterType Type;
private:
	uint16 ResourceIndex;
	uint16 ResourceNum;
};

//using TextureParameterType = XSRVCBVUAVParameter;
//using SRVParameterType = XSRVCBVUAVParameter;
//using UAVParameterType = XSRVCBVUAVParameter;
//using CBVParameterType = XSRVCBVUAVParameter;

class TextureParameterType : public XSRVCBVUAVParameter
{
public:
	TextureParameterType()
		:XSRVCBVUAVParameter(EParameterType::PT_Texture)
	{

	}

};
class SRVParameterType : public XSRVCBVUAVParameter
{
public:
	SRVParameterType()
		:XSRVCBVUAVParameter(EParameterType::PT_SRV)
	{

	}

};
class UAVParameterType : public XSRVCBVUAVParameter
{
public:
	UAVParameterType()
		:XSRVCBVUAVParameter(EParameterType::PT_UAV)
	{

	}

};
class CBVParameterType : public XSRVCBVUAVParameter
{
public:
	CBVParameterType()
		:XSRVCBVUAVParameter(EParameterType::PT_CBV)
	{

	}

};


template<typename TRHICmdList>
inline void SetShaderConstantBufferParameter(
	TRHICmdList& RHICmdList,
	EShaderType ShaderType,
	const CBVParameterType& ConstantBufferParameter,
	XRHIConstantBuffer* RHIConstantBuffer,
	uint32 ElementIndex = 0
)
{
	XASSERT_TEMP(ConstantBufferParameter.GetResourceNum() > 0);
	if (ConstantBufferParameter.GetResourceNum() > 0)
	{
		RHICmdList.SetConstantBuffer(ShaderType, ConstantBufferParameter.GetResourceIndex() + ElementIndex, RHIConstantBuffer);
	}
	else
	{
		XLOG("ConstantBufferParameter.GetResourceNum() > 0");
	}
}

template<typename TRHICmdList>
inline void SetShaderSRVParameter(
	TRHICmdList& RHICmdList,
	EShaderType ShaderType,
	const SRVParameterType& SRVParameter,
	XRHIShaderResourceView* RHISRV,
	uint32 ElementIndex = 0
)
{
	XASSERT_TEMP(SRVParameter.GetResourceNum() > 0);
	if (SRVParameter.GetResourceNum() > 0)
	{
		RHICmdList.SetShaderSRV(ShaderType, SRVParameter.GetResourceIndex() + ElementIndex, RHISRV);
	}
	else
	{
		XLOG("ERROR:SRVParameter.GetResourceNum() > 0");
	}
}

template<typename TRHICmdList, class ParameterType>
inline void SetShaderValue(
	TRHICmdList& RHICmdList,
	EShaderType ShaderType,
	const XShaderVariableParameter& ShaderVariableParameter,
	ParameterType& Value,
	uint32 ElementIndex = 0
)
{
	XASSERT_TEMP(sizeof(ParameterType) == ShaderVariableParameter.GetNumBytes());
	if (sizeof(ParameterType) == ShaderVariableParameter.GetNumBytes())
	{
		RHICmdList.SetShaderValue(
			ShaderType,
			ShaderVariableParameter.GetBufferIndex(),
			ShaderVariableParameter.GetVariableOffsetInBuffer(),
			ShaderVariableParameter.GetNumBytes(),
			&Value);
	}
	else
	{
		XLOG("ERROR:sizeof(ParameterType) == ShaderVariableParameter.GetNumBytes()");
	}
}
template<typename TRHICmdList>
inline void SetTextureParameter(
	TRHICmdList& RHICmdList,
	EShaderType ShaderType,
	const TextureParameterType& TextureParameter,
	XRHITexture* TextureRHI,
	uint32 ElementIndex = 0
)
{
	XASSERT_TEMP(TextureParameter.GetResourceNum() > 0);
	if (TextureParameter.GetResourceNum() > 0)
	{
		RHICmdList.SetShaderTexture(ShaderType, TextureParameter.GetResourceIndex() + ElementIndex, TextureRHI);
	}
	else
	{
		XLOG("ERROR:TextureParameter.GetResourceNum() > 0 == false");
	}
}

template<typename TRHICmdList>
inline void SetShaderUAVParameter(
	TRHICmdList& RHICmdList,
	EShaderType ShaderType,
	const UAVParameterType& UAVParameter,
	XRHIUnorderedAcessView* InUAV,
	uint32 ElementIndex = 0
)
{
	XASSERT_TEMP(UAVParameter.GetResourceNum() > 0);
	if (UAVParameter.GetResourceNum() > 0)
	{
		RHICmdList.SetShaderUAV(ShaderType, UAVParameter.GetResourceIndex() + ElementIndex, InUAV);
	}
	else
	{
		XLOG("ERROR:UAVParameter.GetResourceNum() > 0 == false");
	}
}


#if RHI_RAYTRACING
struct XRayTracingShaderBindsWriter : XRayTracingShaderBinds
{
	void SetSRV(uint32 BindIndex,XRHIShaderResourceView* Value)
	{
		SRVs[BindIndex] = Value;
	}

	void SetUAV(uint32 BindIndex, XRHIUnorderedAcessView* Value)
	{
		UAVs[BindIndex] = Value;
	}
};

void SetShaderParameters(XRayTracingShaderBindsWriter& RTBindingsWriter, const XShaderParameterMap& ShaderParameterMap, std::vector<XRHIShaderResourceView*>& SRVs, std::vector<XRHIUnorderedAcessView*>& UAVs);
#endif