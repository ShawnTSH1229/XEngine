#pragma once
#include "Shader.h"


class XGloablShaderMappingToXShaders :public XShaderMappingToXShaders
{
	friend class XGlobalShaderMapping_FileUnit;
public:
	XGloablShaderMappingToXShaders(std::size_t InHashedSourceFileIndex) :
		HashedSourceFileIndex(InHashedSourceFileIndex) {};
private:
	std::size_t HashedSourceFileIndex;
};

class XGlobalShaderMapping_FileUnit :public TShaderMapping<XGloablShaderMappingToXShaders>
{
public:
	XGlobalShaderMapping_FileUnit(const std::size_t& InHashedSourceFileIndex)
	{
		AssignShaderMappingXShader(new XGloablShaderMappingToXShaders(InHashedSourceFileIndex));
	}
};

class XGlobalShaderMapping_ProjectUnit 
{
public:
	~XGlobalShaderMapping_ProjectUnit();
	XGlobalShaderMapping_FileUnit* FindOrAddShaderMapping_FileUnit(const XShaderInfo* ShaderInfoToCompile);
	
	TShaderReference<XXShader> GetShader(XShaderInfo* ShaderInfo, int32 PermutationId = 0) const;
	
	template<typename XXShaderClass>
	TShaderReference<XXShaderClass> GetShader(int32 PermutationId = 0)const
	{
		TShaderReference<XXShader> Shader = GetShader(&XXShaderClass::StaticShaderInfos, PermutationId);
		return TShaderReference<XXShaderClass>::Cast(Shader);
	}
	
	inline std::unordered_map<std::size_t, XGlobalShaderMapping_FileUnit*>& GetGlobalShaderMap_HashMap()
	{
		return MapFromHashedFileIndexToPtr;
	}
private:
	std::unordered_map<std::size_t, XGlobalShaderMapping_FileUnit*>MapFromHashedFileIndexToPtr;
};

extern XGlobalShaderMapping_ProjectUnit* GetGlobalShaderMapping();
extern class XGlobalShaderMapping_ProjectUnit* GGlobalShaderMapping;


class XGloablShaderInfos : public XShaderInfo
{
public:
	XGloablShaderInfos(
		const char* InShaderName,
		const wchar_t* InSourceFileName,
		const char* InEntryName,
		EShaderType ShaderType,
		XShaderCustomConstructFunctionPtr InCtorPtr,
		ModifyShaderCompileSettingFunctionPtr InModifySettingsPtr) :
		XShaderInfo(
			EShaderTypeForDynamicCast::Global,
			InShaderName,
			InSourceFileName,
			InEntryName,
			ShaderType,
			InCtorPtr,
			InModifySettingsPtr) {}
};

class XGloablShader : public XXShader
{
public:
	XGloablShader(const XShaderInitlizer& Initializer):XXShader(Initializer) {}
	using ShaderInfos = XGloablShaderInfos;
};