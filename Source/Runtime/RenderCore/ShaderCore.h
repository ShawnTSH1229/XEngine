#pragma once
#include "Runtime/HAL/PlatformTypes.h"
#include "Runtime/Core/Template/XEngineTemplate.h"
#include "Runtime/RHI/RHIDefines.h"
#include "Runtime/RHI/RHIResource.h"
#include <map>
#include <string>

//#define GLOBAL_SHADER_PATH "E:/XEngine/XEnigine/Source/Shaders/"
//#define MATERIAL_SHADER_PATH "E:/XEngine/XEnigine/MaterialShaders"

class XShaderCodeReader
{
	XArrayView<uint8> ShaderCode;
public:
	XShaderCodeReader(XArrayView<uint8> InShaderCode) :ShaderCode(InShaderCode) {}

	size_t GetActualShaderCodeSize() const
	{
		return ShaderCode.size() - GetOptionalDataSize();
	}

	const uint8* FindOptionalData(uint8 InKey, uint8 ValueSize) const
	{
		const uint8* End = ShaderCode.data() + ShaderCode.size();

		int32 LocalOptionalDataSize = GetOptionalDataSize();

		const uint8* Start = End - LocalOptionalDataSize;
		End = End - sizeof(LocalOptionalDataSize);
		const uint8* Current = Start;

		while (Current < End)
		{
			char Key = *Current; ++Current;
			uint32 Size = *((const int32*)Current);
			Current += sizeof(Size);

			if (Key == InKey && Size == ValueSize)
			{
				return Current;
			}

			Current += Size;
		}

		return 0;
	}

	int32 GetOptionalDataSize()const
	{
		const uint8* End = ShaderCode.data() + ShaderCode.size();
		int32 OptionalDataSize = ((const int32*)End)[-1];
		return OptionalDataSize;
	}
};

class XShaderCompileSetting
{
public:
	std::map<std::string, std::string>IncludePathToCode;
	std::map<std::string, std::string>Defines;
	inline void SetDefines(const char* Name,const char* Value)
	{
		Defines[Name] = Value;
	}
};

struct XShaderResourceCount
{
	static const uint8 Key = 'p';

	uint8 NumSRV;
	uint8 NumCBV;
	uint8 NumUAV;
	uint8 NumSampler;
};

enum class EShaderParametertype
{
	None=0,
	SRV,
	UAV,
	CBV,
	Sampler,
};
struct XShaderParameterInfo
{
	uint32 BufferIndex;
	uint32 VariableOffsetInBuffer;
	uint32 VariableSize;
	uint32 ResourceCount;
	EShaderParametertype Parametertype;
};

class XShaderParameterMap
{
public:
	std::map<std::string, XShaderParameterInfo>MapNameToParameter;
};