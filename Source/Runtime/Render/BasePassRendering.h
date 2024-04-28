#pragma once
#include "MeshMaterialShader.h"
#include "Runtime/Engine/Classes/Material.h"
#include "Runtime/RenderCore/ShaderParameter.h"

template<bool TempType>
class TBasePassPS : public XMeshMaterialShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new TBasePassPS<TempType>(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) 
	{
		
	}
public:
	TBasePassPS(const XShaderInitlizer& Initializer):
		XMeshMaterialShader(Initializer)
	{

	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		std::shared_ptr<GMaterialInstance> MaterialInstancePtr)
	{
		RHICommandList.SetConstantBuffer(EShaderType::SV_Pixel, 0, MaterialInstancePtr->GetRHIConstantBuffer().get());

		for (auto iter = MaterialInstancePtr->MaterialTextureArray.begin(); iter != MaterialInstancePtr->MaterialTextureArray.end(); iter++)
		{
			RHICommandList.SetShaderTexture(EShaderType::SV_Pixel, iter->ResourceIndex, iter->TexturePtr->GetRHITexture2D().get());
		}
	}
};

template<bool TempType>
class TBasePassVS : public XMeshMaterialShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new TBasePassVS<TempType>(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) 
	{
		OutSettings.IncludePathToCode["Generated/LocalVertexFactory.hlsl"] = "*LocalVertexFactory.hlsl";
	}
public:
	TBasePassVS(const XShaderInitlizer& Initializer) :
		XMeshMaterialShader(Initializer)
	{
		cbPerObject.Bind(Initializer.ShaderParameterMap, "cbPerObject");
		cbPass.Bind(Initializer.ShaderParameterMap, "cbView");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* ViewCB,
		XRHIConstantBuffer* CBPerObjectIn)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, cbPass, ViewCB);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, cbPerObject, CBPerObjectIn);
	}
	CBVParameterType cbPerObject;
	CBVParameterType cbPass;
};