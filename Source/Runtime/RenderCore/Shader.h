#pragma once
#include "Runtime/HAL/PlatformTypes.h"
#include "Runtime/HAL/Mch.h"

#include <fstream>


#include "Runtime/D3D12RHI/d3dx12.h"
#include <d3dcompiler.h>
#include "Runtime/RHI/RHIResource.h"


/////////////////////
#include "Runtime/RenderCore/RenderResource.h"
#include <list>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Runtime/Engine/ShaderCompiler/ShaderCompiler.h"
/////////////////////


class XXShader;
class XShaderInfo;

#pragma region ShaderMap



class XShaderMappingToRHIShaders //: public XRenderResource
{
public:
	XShaderMappingToRHIShaders(std::size_t NumShaders)
	{
		RHIShaders.resize(NumShaders);
	}
	inline XRHIShader* GetRHIShader(int32 ShaderIndex)
	{
		if (RHIShaders[ShaderIndex].get() == nullptr)
		{
			RHIShaders[ShaderIndex] = CreateRHIShaderFromCode(ShaderIndex);
		}
		return RHIShaders[ShaderIndex].get();
	}
protected:
	virtual std::shared_ptr<XRHIShader> CreateRHIShaderFromCode(int32 ShaderIndex) = 0;
	std::vector<std::shared_ptr<XRHIShader>>RHIShaders;
};

class XShaderMappingToCodes
{
public:
	struct XShaderEntry
	{
		std::vector<uint8>Code;
		EShaderType Shadertype;
	};

	std::size_t AddShaderCompilerOutput(XShaderCompileOutput& OutputInfo);

	std::vector<XShaderEntry>ShaderEntries;
	std::vector<std::size_t>EntryCodeHash;
};

class XShaderMappingToRHIShaders_InlineCode :public XShaderMappingToRHIShaders
{
public:
	XShaderMappingToRHIShaders_InlineCode(XShaderMappingToCodes* InCode)
		: XShaderMappingToRHIShaders(InCode->ShaderEntries.size()),
		Code(InCode) {}

	std::shared_ptr<XRHIShader> CreateRHIShaderFromCode(int32 ShaderIndex)override;
	XShaderMappingToCodes* Code;//Stored In ShaderMapBase
};

class XShaderMappingToXShaders
{
public:
	XXShader* FindOrAddXShader(const std::size_t HashedShaderNameIndex, XXShader* Shader, int32 PermutationId = 0);
	XXShader* GetXShader(const std::size_t HashedShaderNameIndex, int32 PermutationId = 0)const;
private:
	std::vector<std::shared_ptr<XXShader>>ShaderPtrArray;
	std::unordered_map<std::size_t, std::size_t>MapHashedShaderNameIndexToXShaderIndex;
};

class XShaderMappingBase
{
public:
	~XShaderMappingBase();

	inline void AssignShaderMappingXShader(XShaderMappingToXShaders* InXShadersStored)
	{
		XShadersStored = InXShadersStored;
	}

	inline void InitRHIShaders_InlineCode()
	{
		RHIShadersStored.reset();
		if (CodesStored.get() != nullptr)
		{
			XShaderMappingToRHIShaders_InlineCode* RHIShader = new XShaderMappingToRHIShaders_InlineCode(CodesStored.get());
			RHIShadersStored = std::shared_ptr<XShaderMappingToRHIShaders>
				(static_cast<XShaderMappingToRHIShaders*>(RHIShader));
		}
	}
	
	inline std::shared_ptr<XShaderMappingToRHIShaders>GetRefShaderMapStoreRHIShaders() const
	{
		return RHIShadersStored;
	}

	inline XShaderMappingToRHIShaders* GetShaderMapStoreRHIShaders() const
	{ 
		return RHIShadersStored.get();
	}

	inline XShaderMappingToXShaders* GetShaderMapStoreXShaders()const 
	{ 
		return XShadersStored;
	}

	inline XShaderMappingToCodes* GetShaderMapStoreCodes()
	{ 
		if (CodesStored.get() == nullptr)
		{
			CodesStored = std::make_shared<XShaderMappingToCodes>();
		}
		return CodesStored.get();
	}

private:
	std::shared_ptr<XShaderMappingToRHIShaders>RHIShadersStored;
	std::shared_ptr<XShaderMappingToCodes>CodesStored;
	XShaderMappingToXShaders* XShadersStored;//Stored In XGlobalShaderMapInProjectUnit
};

template<typename ShadersInfotype>
class TShaderMapping: public XShaderMappingBase
{
public:
};


struct XShaderInitlizer
{
	XShaderInitlizer(XShaderInfo* InShadersInfoPtr, XShaderCompileOutput& Output,std::size_t InRHIShaderIndex) :
		ShadersInfoPtr(InShadersInfoPtr),
		ShaderParameterMap(Output.ShaderParameterMap),
		CodeHash(Output.SourceCodeHash),
		RHIShaderIndex(InRHIShaderIndex) {}

	const XShaderParameterMap& ShaderParameterMap;
	XShaderInfo* ShadersInfoPtr;
	std::size_t CodeHash;
	std::size_t RHIShaderIndex;
};

////class FShaderType

class XShaderInfo
{
public:
	enum class EShaderTypeForDynamicCast
	{
		Global,
		Material,
		MeshMaterial,
		NumShaderInfoType,
	};

	typedef void (*ModifyShaderCompileSettingFunctionPtr) (XShaderCompileSetting& CompileSetting);
	typedef XXShader* (*XShaderCustomConstructFunctionPtr)(const XShaderInitlizer& Initializer);

	XShaderInfo(
		EShaderTypeForDynamicCast InCastType,
		const char* InShaderName,
		const wchar_t* InSourceFileName,
		const char* InEntryName,
		EShaderType InShaderType,
		XShaderCustomConstructFunctionPtr InCtorPtr,
		ModifyShaderCompileSettingFunctionPtr InModifySettingsPtr
		);
	static std::list<XShaderInfo*>& GetShaderInfo_LinkedList(EShaderTypeForDynamicCast ShaderInfoType);

	inline std::size_t GetHashedFileIndex()const { return HashedFileIndex; }
	inline std::size_t GetHashedShaderNameIndex()const { return HashShaderNameIndex; };

	inline const wchar_t* GetSourceFileName()const { return SourceFileName; }
	inline const char* GetEntryName()const { return EntryName; }
	inline EShaderType GetShaderType()const { return ShaderType; }
	inline const char* GetShaderName()const { return ShaderName; };
	
public:
	ModifyShaderCompileSettingFunctionPtr ModifySettingsPtr;
	XShaderCustomConstructFunctionPtr CtorPtr;
private:

	std::size_t HashedFileIndex;
	std::size_t HashShaderNameIndex;

	EShaderTypeForDynamicCast CastType;
	const char* ShaderName;
	const wchar_t* SourceFileName;
	const char* EntryName;
	EShaderType ShaderType;
};



template<typename XXShaderClass>
class TShaderReference
{
public:
	TShaderReference() :ShaderPtr(nullptr), ShaderMapFileUnitPtr(nullptr) {}
	TShaderReference(XXShaderClass* InShaderPtr, const XShaderMappingBase* InShaderMapFileUnitPtr)
		:ShaderPtr(InShaderPtr),
		ShaderMapFileUnitPtr(InShaderMapFileUnitPtr) {}

	template<typename OtherXXShaderClass>
	static TShaderReference<XXShaderClass> Cast(const TShaderReference<OtherXXShaderClass>& Rhs)
	{
		return TShaderReference<XXShaderClass>(static_cast<XXShaderClass*>(Rhs.GetShader()), Rhs.GetShaderMappingFileUnit());
	}

	inline XXShaderClass* GetShader()const { return ShaderPtr; }
	inline const XShaderMappingBase* GetShaderMappingFileUnit()const { return ShaderMapFileUnitPtr; }
	
	inline XXShaderClass* operator->() const { return ShaderPtr; }

	inline XRHIShader* GetOrCreateRHIShaderFromMapping(/*EShaderType ShaderType*/ ) const
	{
		return ShaderMapFileUnitPtr->GetShaderMapStoreRHIShaders()->GetRHIShader(ShaderPtr->GetRHIShaderIndex());
	}
	
	inline XRHIVertexShader* GetVertexShader() const
	{
		return static_cast<XRHIVertexShader*>(GetOrCreateRHIShaderFromMapping());
	}

	inline XRHIPixelShader* GetPixelShader() const
	{
		return static_cast<XRHIPixelShader*>(GetOrCreateRHIShaderFromMapping());
	}

	inline XRHIComputeShader* GetComputeShader() const
	{
		return static_cast<XRHIComputeShader*>(GetOrCreateRHIShaderFromMapping());
	}

	inline XRHIRayTracingShader* GetRayTracingShader() const
	{
		return static_cast<XRHIRayTracingShader*>(GetOrCreateRHIShaderFromMapping());
	}

private:
	XXShaderClass* ShaderPtr;
	const XShaderMappingBase* ShaderMapFileUnitPtr;
};


class XXShader
{
public:
	XXShader(const XShaderInitlizer& Initializer)
		:ShadersInfoPtr(Initializer.ShadersInfoPtr),
		CodeHash(Initializer.CodeHash),
		RHIShaderIndex(Initializer.RHIShaderIndex),
		ShaderParameterMap(Initializer.ShaderParameterMap)
	{}

	inline std::size_t GetCodeHash()const { return CodeHash; };
	inline std::size_t GetRHIShaderIndex()const { return RHIShaderIndex; };
	const XShaderParameterMap& ShaderParameterMap;
private:
	XShaderInfo* ShadersInfoPtr;
	std::size_t CodeHash;
	std::size_t RHIShaderIndex;
};



