#pragma once
#include "Runtime/RenderCore/Shader.h"
#include "Runtime/Engine/Classes/EngineTypes.h"
#include "Runtime/Render/MaterialShader.h"

class RMaterial;

enum class EMaterialDomain
{
	Surface,
	PostProcess,
};

struct XMaterialShaderParameters_ForIndex
{
	EMaterialDomain MaterialDomain;
	XMaterialShadingModelField ShadingModels;
};

class XMaterialShaderMappingToXShaders :public XShaderMappingToXShaders
{
public:
};

class XMaterialShaderMapping_MatUnit :public TShaderMapping<XMaterialShaderMappingToXShaders>
{
public:
	void Compile(const XShaderCompileSetting& ShaderCompileSetting);
};

class RMaterial
{
public:
	void BeginCompileShaderMap();
	void GetShaderInfos(const XMaterialShaderInfo_Set& ShaderInfos, XMaterialShader_Set& ShaderOut);
	inline void SetCodePath(const std::wstring& CodePathIn)
	{
		CodePath = CodePathIn;
	}
private:
	std::shared_ptr<XMaterialShaderMapping_MatUnit>RThreadShaderMap;
	std::wstring CodePath;
};

