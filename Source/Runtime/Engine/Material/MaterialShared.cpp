#include <memory>
#include "MaterialShared.h"
#include "MaterialShaderMapSet.h"
#include <locale>
#include <codecvt>
//Temp
#include <filesystem>
#include <fstream>
//

void RMaterial::BeginCompileShaderMap()
{
	std::shared_ptr<XMaterialShaderMapping_MatUnit> NewShaderMap = std::make_shared<XMaterialShaderMapping_MatUnit>();
	std::shared_ptr<XShaderCompileSetting> MaterialCompileSetting = std::make_shared<XShaderCompileSetting>();

	NewShaderMap->AssignShaderMappingXShader(new XShaderMappingToXShaders());

	//Read Material Shader Code
	std::string MaterialShaderCode;
	std::ifstream FileCode(std::filesystem::path(CodePath), std::ios::ate);
	std::ifstream::pos_type FileSize = FileCode.tellg();
	FileCode.seekg(0, std::ios_base::beg);
	MaterialShaderCode.resize(FileSize, '\n');
	FileCode.read(MaterialShaderCode.data(), FileSize);
	FileCode.close();



	MaterialCompileSetting->IncludePathToCode["Generated/Material.hlsl"] = MaterialShaderCode;
	NewShaderMap->Compile(*MaterialCompileSetting);

	//Set Shader Mapping
	RThreadShaderMap = NewShaderMap;
}

void RMaterial::GetShaderInfos(const XMaterialShaderInfo_Set& ShaderInfos, XMaterialShader_Set& ShaderOut)
{
	const XMaterialShaderMapping_MatUnit* ShaderMap = RThreadShaderMap.get();
	ShaderOut.ShaderMap = ShaderMap;

	const XShaderMappingToXShaders* MappingXShaders = ShaderMap->GetShaderMapStoreXShaders();//TODO Mesh Shader
	for (int32 index = 0; index < (int32)EShaderType::SV_ShaderCount; ++index)
	{
		const XShaderInfo* ShaderInfo = ShaderInfos.ShaderInfoSet[index];
		if (ShaderInfo != nullptr)
		{
			ShaderOut.XShaderSet[index] = MappingXShaders->GetXShader(ShaderInfo->GetHashedShaderNameIndex());
		}
	}
}