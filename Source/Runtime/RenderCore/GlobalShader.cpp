#include "GlobalShader.h"

XGlobalShaderMapping_ProjectUnit* GGlobalShaderMapping = nullptr;
XGlobalShaderMapping_ProjectUnit* GetGlobalShaderMapping()
{
	return GGlobalShaderMapping;
}

XGlobalShaderMapping_ProjectUnit::~XGlobalShaderMapping_ProjectUnit()
{
	for (auto iter = MapFromHashedFileIndexToPtr.begin(); iter != MapFromHashedFileIndexToPtr.end(); iter++)
	{
		delete iter->second;
	}
	MapFromHashedFileIndexToPtr.clear();
}

XGlobalShaderMapping_FileUnit* XGlobalShaderMapping_ProjectUnit::FindOrAddShaderMapping_FileUnit(const XShaderInfo* ShaderInfoToCompile)
{
	std::size_t HashedFileIndex = ShaderInfoToCompile->GetHashedFileIndex();
	auto iter = MapFromHashedFileIndexToPtr.find(HashedFileIndex);
	XGlobalShaderMapping_FileUnit* ShaderMapInFileUnit;
	if (iter == MapFromHashedFileIndexToPtr.end())
	{
		ShaderMapInFileUnit = new XGlobalShaderMapping_FileUnit(HashedFileIndex);
		MapFromHashedFileIndexToPtr[HashedFileIndex] = ShaderMapInFileUnit;
	}
	else
	{
		ShaderMapInFileUnit = iter->second;
	}
	return ShaderMapInFileUnit;
}

TShaderReference<XXShader> XGlobalShaderMapping_ProjectUnit::GetShader(XShaderInfo* ShaderInfo, int32 PermutationId) const
{
	auto iter = MapFromHashedFileIndexToPtr.find(ShaderInfo->GetHashedFileIndex());
	if (iter != MapFromHashedFileIndexToPtr.end())
	{
		XGlobalShaderMapping_FileUnit* MapInFileUnit = iter->second;
		return TShaderReference<XXShader>(
			MapInFileUnit->GetShaderMapStoreXShaders()->GetXShader(ShaderInfo->GetHashedShaderNameIndex(), 0),
			MapInFileUnit);
	}
	return TShaderReference<XXShader>();
}



