#pragma once
#include "MaterialShader.h"

class XMeshMaterialShaderInfo :public XShaderInfo
{
public:
	XMeshMaterialShaderInfo(
		const char* InShaderName,
		const wchar_t* InSourceFileName,
		const char* InEntryName,
		EShaderType InShaderType,
		XShaderCustomConstructFunctionPtr InCtorPtr,
		ModifyShaderCompileSettingFunctionPtr InModifySettingsPtr) :
		XShaderInfo(EShaderTypeForDynamicCast::MeshMaterial,
			InShaderName, InSourceFileName, InEntryName, InShaderType, InCtorPtr, InModifySettingsPtr) {}
};

class XMeshMaterialShader :public XMaterialShader
{
public:
	XMeshMaterialShader(const XShaderInitlizer& Initializer)
		:XMaterialShader(Initializer) {}
	using ShaderInfos = XMeshMaterialShaderInfo;
};