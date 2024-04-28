#include "ResourceManager.h"

static std::vector<std::shared_ptr<RMaterial>>& GetMaterialPtrArray()
{
	static std::vector<std::shared_ptr<RMaterial>> MaterialPtrArray;
	return MaterialPtrArray;
}

void AddRMaterialToResourceManager(std::shared_ptr<RMaterial>& MaterialPtr)
{
	GetMaterialPtrArray().push_back(MaterialPtr);
}

void BegineCompileRMaterialInResourceManager()
{
	std::vector<std::shared_ptr<RMaterial>>& GlobalRMaterialPtr = GetMaterialPtrArray();
	for (auto iter = GlobalRMaterialPtr.begin(); iter != GlobalRMaterialPtr.end(); iter++)
	{
		(*iter)->BeginCompileShaderMap();
	}
}
