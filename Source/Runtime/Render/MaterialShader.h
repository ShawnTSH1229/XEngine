#pragma once
#include "Runtime/RenderCore/Shader.h"

class XMaterialShaderInfo :public XShaderInfo
{
public:
	XMaterialShaderInfo(
		const char* InShaderName,
		const wchar_t* InSourceFileName,
		const char* InEntryName,
		EShaderType InShaderType,
		XShaderCustomConstructFunctionPtr InCtorPtr,
		ModifyShaderCompileSettingFunctionPtr InModifySettingsPtr) :
		XShaderInfo(EShaderTypeForDynamicCast::Material,
			InShaderName, InSourceFileName, InEntryName, InShaderType, InCtorPtr, InModifySettingsPtr) {}
};

class XMaterialShader :public XXShader
{
public:
	XMaterialShader(const XShaderInitlizer& Initializer)
		:XXShader(Initializer) {}

	using ShaderInfos = XMaterialShaderInfo;
};


struct XMaterialShaderInfo_Set
{
	XShaderInfo* ShaderInfoSet[(int)EShaderType::SV_ShaderCount];
};

struct XMaterialShader_Set
{
	const XShaderMappingBase* ShaderMap;
	XXShader* XShaderSet[(int)EShaderType::SV_ShaderCount];
};