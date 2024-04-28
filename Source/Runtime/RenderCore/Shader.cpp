#include "Shader.h"
#include "Runtime/RHI/RHICommandList.h"

std::shared_ptr<XRHIShader> XShaderMappingToRHIShaders_InlineCode::CreateRHIShaderFromCode(int32 ShaderIndex)
{
	const XShaderMappingToCodes::XShaderEntry& ShaderEntry = Code->ShaderEntries[ShaderIndex];
	
	std::size_t CodeHash = Code->EntryCodeHash[ShaderIndex];

	const EShaderType ShaderType = ShaderEntry.Shadertype;
	std::shared_ptr<XRHIShader> RHIShaderRef;
	XArrayView<uint8> CodeView(ShaderEntry.Code.data(), ShaderEntry.Code.size());
	switch (ShaderType)
	{
	case EShaderType::SV_Vertex:RHIShaderRef = std::static_pointer_cast<XRHIShader>(RHICreateVertexShader(CodeView)); break;
	case EShaderType::SV_Pixel:RHIShaderRef = std::static_pointer_cast<XRHIShader>(RHICreatePixelShader(CodeView)); break;
	case EShaderType::SV_Compute:RHIShaderRef = std::static_pointer_cast<XRHIShader>(RHICreateComputeShader(CodeView)); break;
#if RHI_RAYTRACING
	case EShaderType::SV_RayGen: 
	case EShaderType::SV_RayMiss:
	case EShaderType::SV_HitGroup:RHIShaderRef = std::static_pointer_cast<XRHIShader>(RHICreateRayTracingShader(CodeView, ShaderType));
		break;
#endif
	default:XASSERT(false); break;
	}
	RHIShaderRef->SetHash(CodeHash);
	return RHIShaderRef;
}


XShaderMappingBase::~XShaderMappingBase()
{
	delete XShadersStored;
}


std::size_t XShaderMappingToCodes::AddShaderCompilerOutput(XShaderCompileOutput& OutputInfo)
{
	XShaderEntry ShaderEntry;
	ShaderEntry.Code = OutputInfo.ShaderCode;
	ShaderEntry.Shadertype = OutputInfo.Shadertype;
	ShaderEntries.push_back(std::move(ShaderEntry));
	EntryCodeHash.push_back(OutputInfo.SourceCodeHash);
	return ShaderEntries.size() - 1;
}

XXShader* XShaderMappingToXShaders::FindOrAddXShader(const std::size_t HashedShaderNameIndex, XXShader* Shader, int32 PermutationId)
{
	const std::size_t Index = HashedShaderNameIndex;
	auto iter = MapHashedShaderNameIndexToXShaderIndex.find(Index);
	if (iter != MapHashedShaderNameIndexToXShaderIndex.end())
	{
		return ShaderPtrArray[iter->second].get();
	}
	ShaderPtrArray.push_back(std::shared_ptr<XXShader>(Shader));
	MapHashedShaderNameIndexToXShaderIndex[Index] = ShaderPtrArray.size() - 1;
	return ShaderPtrArray.back().get();
}

XXShader* XShaderMappingToXShaders::GetXShader(const std::size_t HashedEntryIndex, int32 PermutationId) const
{
	auto iter = MapHashedShaderNameIndexToXShaderIndex.find(HashedEntryIndex);
	XASSERT(iter != MapHashedShaderNameIndexToXShaderIndex.end());
	std::size_t ShaderPtrArrayIndex = iter->second;
	return ShaderPtrArray[ShaderPtrArrayIndex].get();
}


XShaderInfo::XShaderInfo(
	EShaderTypeForDynamicCast InCastType,
	const char* InShaderName,
	const wchar_t* InSourceFileName,
	const char* InEntryName,
	EShaderType InShaderType,
	XShaderCustomConstructFunctionPtr InCtorPtr,
	ModifyShaderCompileSettingFunctionPtr InModifyDefinesPtr) :
	CastType(InCastType),
	ShaderName(InShaderName),
	SourceFileName(InSourceFileName),
	EntryName(InEntryName),
	ShaderType(InShaderType),
	CtorPtr(InCtorPtr),
	ModifySettingsPtr(InModifyDefinesPtr)
{
	GetShaderInfo_LinkedList(InCastType).push_back(this);
	HashedFileIndex = std::hash<std::wstring>{}(InSourceFileName);
	HashShaderNameIndex = std::hash<std::string>{}(InShaderName);
}

std::list<XShaderInfo*>& XShaderInfo::GetShaderInfo_LinkedList(EShaderTypeForDynamicCast ShaderInfoType)
{
	static std::list<XShaderInfo*> GloablShaderInfosUsedToCompile_LinkedList[(int)EShaderTypeForDynamicCast::NumShaderInfoType];
	return GloablShaderInfosUsedToCompile_LinkedList[(int)ShaderInfoType];
}