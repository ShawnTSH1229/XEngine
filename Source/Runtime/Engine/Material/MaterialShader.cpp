#include "MaterialShared.h"
#include "MaterialShaderMapSet.h"

class XMaterialShaderMappingSetsManager
{
public:
	static XMaterialShaderMappingSetsManager& Get()
	{
		static XMaterialShaderMappingSetsManager Instance;
		return Instance;
	}

	std::vector<std::unique_ptr<XMaterialShaderMapSet>>ShaderMapSets;

	const XMaterialShaderMapSet& GetSet()
	{
		XMaterialShaderMapSet* MapSet;
		
		if (ShaderMapSets.size() == 1)
		{
			MapSet = ShaderMapSets.begin()->get();
		}
		else
		{
			MapSet = new XMaterialShaderMapSet();
			const std::list<XShaderInfo*>& MeshMaterialShaderInfos = XShaderInfo::GetShaderInfo_LinkedList(
				XShaderInfo::EShaderTypeForDynamicCast::MeshMaterial);

			for (auto iter = MeshMaterialShaderInfos.begin(); iter != MeshMaterialShaderInfos.end(); iter++)
			{
				MapSet->ShaderInfos.push_back(*iter);
			}
			ShaderMapSets.push_back(std::unique_ptr<XMaterialShaderMapSet>(MapSet));
		}
		return *MapSet;
	}
};

const XMaterialShaderMapSet& GetMaterialShaderMapSet()
{
	return XMaterialShaderMappingSetsManager::Get().GetSet();
}

void XMaterialShaderMapping_MatUnit::Compile(const XShaderCompileSetting& ShaderCompileSetting)
{

	const XMaterialShaderMapSet& MapSet = GetMaterialShaderMapSet();
	for (auto ShaderInfo : MapSet.ShaderInfos)
	{
		XShaderCompileInput Input;
		Input.SourceFilePath = ShaderInfo->GetSourceFileName();
		Input.EntryPointName = ShaderInfo->GetEntryName();
		Input.Shadertype = ShaderInfo->GetShaderType();
		Input.ShaderName = ShaderInfo->GetShaderName();
		Input.CompileSettings = ShaderCompileSetting;
		ShaderInfo->ModifySettingsPtr(Input.CompileSettings);

		XShaderCompileOutput Output;
		CompileMaterialShader(Input, Output);

		//Step1 Store Compile Shader Code
		std::size_t HashIndex = this->GetShaderMapStoreCodes()->AddShaderCompilerOutput(Output);

		//Step2 Store XShader
		XXShader* Shader = ShaderInfo->CtorPtr(XShaderInitlizer(ShaderInfo, Output, HashIndex));
		this->GetShaderMapStoreXShaders()->FindOrAddXShader(ShaderInfo->GetHashedShaderNameIndex(), Shader);
	}

	this->InitRHIShaders_InlineCode();
}