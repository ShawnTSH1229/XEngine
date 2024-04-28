#include "MainInit.h"
#include "Runtime/CoreGObject/GObjtect/PermanentMemAlloc.h"
#include "Runtime/ResourceManager/ResourceManager.h"
#include "Runtime/RenderCore/GlobalShader.h"

static std::vector<InitPropertyFunPtr>InitPropertyFunArray;

void MainInit::PushToInitPropertyFunArray(InitPropertyFunPtr FunPtrIn)
{
	InitPropertyFunArray.push_back(FunPtrIn);
}

void MainInit::Init()
{
	//Reflection Init
	GlobalGObjectPermanentMemAlloc.AllocatePermanentMemPool(1 << 20);
	for (auto& t : InitPropertyFunArray)
	{
		(*t)(nullptr);
	}

}

void MainInit::InitAfterRHI()
{
	//Material Init
	BegineCompileRMaterialInResourceManager();

	//Compile Global Shader Map
	CompileGlobalShaderMap();
}

void MainInit::Destroy()
{
	if (GGlobalShaderMapping)
		delete GGlobalShaderMapping;
}
