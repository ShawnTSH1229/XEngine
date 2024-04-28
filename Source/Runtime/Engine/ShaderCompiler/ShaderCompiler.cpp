#include <codecvt>
#include <filesystem>
#include <Windows.h>
#include "dxc/dxcapi.h"
#include "ShaderCompiler.h"
#include "Runtime/RenderCore/ShaderCore.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/Core/Misc/Path.h"
#include "Runtime\RHI\PlatformRHI.h"

#define CACHE_PATH BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/Cache")
static const std::filesystem::path ShaderASMPath(CACHE_PATH);

//static const std::filesystem::path ShaderASMPath("E:\\XEngine\\XEnigine\\Cache");
//https://docs.microsoft.com/en-us/windows/win32/direct3dtools/dx-graphics-tools-fxc-syntax

#define CACHE_SHADER 0
#define USE_DXC 1

#if !USE_DXC
class XD3DInclude : public ID3DInclude
{
public:
	std::map<std::string, std::string>* IncludePathToCode;
	bool HasIncludePath;
	HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override
	{
		std::string FileNameStr = std::string(pFileName);
		auto iter = IncludePathToCode->find(FileNameStr);
		if (iter != IncludePathToCode->end())
		{
			*ppData = iter->second.data();
			*pBytes = iter->second.size();

			if (iter->second.data()[0] == '*')
			{
				FileNameStr = iter->second.substr(1, iter->second.size() - 1);
			}
			else
			{
				HasIncludePath = true;
				return S_OK;
			}
		}

		std::string FilePath = GLOBAL_SHADER_PATH + FileNameStr;
		if (!std::filesystem::exists(FilePath))
			return E_FAIL;

		//open and get file size
		std::ifstream FileSourceCode(FilePath, std::ios::ate);
		UINT FileSize = static_cast<UINT>(FileSourceCode.tellg());
		FileSourceCode.seekg(0, std::ios_base::beg);
		
		//read data
		*pBytes = FileSize;
		ACHAR* data = static_cast<ACHAR*>(std::malloc(*pBytes));
		if (data)
		{
			//error X3000: Illegal character in shader file
			//https: //zhuanlan.zhihu.com/p/27253604
			memset(data, '\n', FileSize);
		}
		FileSourceCode.read(data, FileSize);


	
		HasIncludePath = false;
		*ppData = data;
		return S_OK;
}
	HRESULT Close(LPCVOID pData) override
	{
		if (HasIncludePath == false)
		{
			std::free(const_cast<void*>(pData));
		}
		return S_OK;
	}
};

static void CompileDX12Shader(XShaderCompileInput& Input, XShaderCompileOutput& Output)
{
#if defined(DEBUG) || defined(_DEBUG)  
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	
	XDxRefCount<ID3DBlob> CodeGened;
	XDxRefCount<ID3DBlob> Errors;
	
#if CACHE_SHADER
	const std::filesystem::path  FilePath = (ShaderASMPath / (Input.ShaderName + ".cso"));
	if (std::filesystem::exists(FilePath))
	{
		std::ifstream fin(FilePath, std::ios::binary);
		fin.seekg(0, std::ios_base::end);
		std::ifstream::pos_type size = (int)fin.tellg();
		fin.seekg(0, std::ios_base::beg);

		ThrowIfFailed(D3DCreateBlob(size, CodeGened.GetAddressOf()));

		fin.read((char*)CodeGened->GetBufferPointer(), size);
		fin.close();

		Output.Shadertype = Input.Shadertype;
		Output.ShaderCode.clear();
		Output.ShaderCode.insert(
			Output.ShaderCode.end(),
			static_cast<uint8*>(CodeGened->GetBufferPointer()),
			static_cast<uint8*>(CodeGened->GetBufferPointer()) + CodeGened->GetBufferSize());
		Output.SourceCodeHash = std::hash<std::string>{}(
			std::string((const char*)Output.ShaderCode.data(), Output.ShaderCode.size()));
	}
	else
#endif
	{
		std::string Target;
		switch (Input.Shadertype)
		{
		case EShaderType::SV_Vertex:Target = "vs_5_1"; break;
		case EShaderType::SV_Pixel:Target = "ps_5_1"; break;
		case EShaderType::SV_Compute:Target = "cs_5_1"; break;
		default:XASSERT(false); break;
		}

		std::vector<D3D_SHADER_MACRO>Macro;
		if (Input.CompileSettings.Defines.size() > 0)
		{
			Macro.resize(Input.CompileSettings.Defines.size() + 1);
			Macro[Input.CompileSettings.Defines.size()].Name = NULL;
			Macro[Input.CompileSettings.Defines.size()].Definition = NULL;
			int index = 0;
			for (auto iter = Input.CompileSettings.Defines.begin(); iter != Input.CompileSettings.Defines.end(); iter++, index++)
			{
				Macro[index].Name = iter->first.c_str();
				Macro[index].Definition = iter->second.c_str();
			};
		}

		XDxRefCount<ID3DBlob> BeforeCompressed;

		XD3DInclude Include;
		Include.IncludePathToCode = &Input.CompileSettings.IncludePathToCode;
		HRESULT hr = D3DCompileFromFile(
			Input.SourceFilePath.data(),
			Macro.data(), 
			&Include,
			//D3D_COMPILE_STANDARD_FILE_INCLUDE,
			Input.EntryPointName.data(),
			Target.data(), compileFlags, 0,
			&BeforeCompressed, &Errors);

		if (Errors != nullptr)OutputDebugStringA((char*)Errors->GetBufferPointer()); ThrowIfFailed(hr);
		
		CodeGened = BeforeCompressed;

		Output.Shadertype = Input.Shadertype;
		Output.ShaderCode.clear();
		Output.ShaderCode.insert(
			Output.ShaderCode.end(),
			static_cast<uint8*>(CodeGened->GetBufferPointer()),
			static_cast<uint8*>(CodeGened->GetBufferPointer()) + CodeGened->GetBufferSize());
		Output.SourceCodeHash = std::hash<std::string>{}(
			std::string((const char*)Output.ShaderCode.data(), Output.ShaderCode.size()));
#if CACHE_SHADER
		std::ofstream fCodeOut(FilePath, std::ios::binary);
		fCodeOut.write((const char*)Output.ShaderCode.data(), Output.ShaderCode.size());
#endif
	}


	//Reflect
	{
		ID3D12ShaderReflection* Reflection = NULL;
		D3DReflect(CodeGened->GetBufferPointer(), CodeGened->GetBufferSize(), IID_PPV_ARGS(&Reflection));

		D3D12_SHADER_DESC ShaderDesc;
		Reflection->GetDesc(&ShaderDesc);

		uint8 NumSRVCount = 0;
		uint8 NumCBVCount = 0;
		uint8 NumUAVCount = 0;
		uint8 NumSamplerCount = 0;
		for (uint32 i = 0; i < ShaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC  ResourceDesc;
			Reflection->GetResourceBindingDesc(i, &ResourceDesc);
			D3D_SHADER_INPUT_TYPE ResourceType = ResourceDesc.Type;

			XShaderParameterInfo ParameterInfo;
			ParameterInfo.BufferIndex = ResourceDesc.BindPoint;
			ParameterInfo.ResourceCount = ResourceDesc.BindCount;
			
			if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE || ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED)
			{
				NumSRVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::SRV;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
			}
			else if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			{
				NumCBVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::CBV;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
				if (strncmp(ResourceDesc.Name, "cbView", 64) != 0)
				{
					ID3D12ShaderReflectionConstantBuffer* ConstantBuffer = Reflection->GetConstantBufferByName(ResourceDesc.Name);
					D3D12_SHADER_BUFFER_DESC CBDesc;
					ConstantBuffer->GetDesc(&CBDesc);
					for (uint32 ConstantIndex = 0; ConstantIndex < CBDesc.Variables; ConstantIndex++)
					{
						ID3D12ShaderReflectionVariable* Variable = ConstantBuffer->GetVariableByIndex(ConstantIndex);
						D3D12_SHADER_VARIABLE_DESC VariableDesc;
						Variable->GetDesc(&VariableDesc);
						ParameterInfo.VariableOffsetInBuffer = VariableDesc.StartOffset;
						ParameterInfo.VariableSize = VariableDesc.Size;
						Output.ShaderParameterMap.MapNameToParameter[VariableDesc.Name] = ParameterInfo;
					}
				}
			}
			else if (
				ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED || 
				ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_APPEND_STRUCTURED || 
				ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
			{
				NumUAVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::UAV;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
			}
			else if(ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
			{
				NumSamplerCount++;
				ParameterInfo.Parametertype = EShaderParametertype::Sampler;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
			}
			else
			{
				XASSERT(false);
			}
			
			
		}
		int32 TotalOptionalDataSize = 0;
		XShaderResourceCount ResourceCount = { NumSRVCount ,NumCBVCount ,NumUAVCount };

		int32 ResoucrCountSize = static_cast<int32>(sizeof(XShaderResourceCount));
		Output.ShaderCode.push_back(XShaderResourceCount::Key);
		Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&ResoucrCountSize), (uint8*)(&ResoucrCountSize) + 4);
		Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&ResourceCount), (uint8*)(&ResourceCount) + sizeof(XShaderResourceCount));
		
		TotalOptionalDataSize += sizeof(uint8);//XShaderResourceCount::Key
		TotalOptionalDataSize += sizeof(int32);//ResoucrCountSize
		TotalOptionalDataSize += sizeof(XShaderResourceCount);//XShaderResourceCount
		TotalOptionalDataSize += sizeof(int32);//TotalOptionalDataSize
		Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&TotalOptionalDataSize), (uint8*)(&TotalOptionalDataSize) + 4);
	}
}

#else

static XDxRefCount<IDxcUtils> pUtils;
static XDxRefCount<IDxcIncludeHandler> pDefaultIncludeHandler;

std::string PathConv(std::string Str) {
	return std::string(&Str[std::string(GLOBAL_SHADER_PATH).size()]);
}

class XDXCInclude : public IDxcIncludeHandler
{
public:
	std::map<std::string, std::string>* IncludePathToCode;
	
	HRESULT STDMETHODCALLTYPE LoadSource(
		_In_z_ LPCWSTR pFilename,                                 // Candidate filename.
		_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
	)override
	{
		XDxRefCount<IDxcBlobEncoding> pEncoding;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::string FileNameStr = PathConv(converter.to_bytes(pFilename));
		FileNameStr = FileNameStr.substr();
		auto iter = IncludePathToCode->find(FileNameStr);
		if (iter != IncludePathToCode->end())
		{
			if (iter->second.data()[0] == '*')
			{
				FileNameStr = iter->second.substr(1, iter->second.size() - 1);
			}
			else
			{
				pUtils->CreateBlob(iter->second.c_str(), iter->second.size(), CP_UTF8, pEncoding.GetAddressOf());
				*ppIncludeSource = pEncoding.Detach();
				return S_OK;
			}
		}

		std::string FilePath = GLOBAL_SHADER_PATH +FileNameStr;
		if (!std::filesystem::exists(FilePath))
			return E_FAIL;

		//open and get file size
		std::ifstream FileSourceCode(FilePath, std::ios::ate);
		UINT FileSize = static_cast<UINT>(FileSourceCode.tellg());
		FileSourceCode.seekg(0, std::ios_base::beg);

		//read data
		ACHAR* data = static_cast<ACHAR*>(std::malloc(FileSize));
		if (data)
		{
			//error X3000: Illegal character in shader file
			//https: //zhuanlan.zhihu.com/p/27253604
			memset(data, '\n', FileSize);
		}
		FileSourceCode.read(data, FileSize);


		pUtils->CreateBlob(data, FileSize, CP_UTF8, pEncoding.GetAddressOf());
		*ppIncludeSource = pEncoding.Detach();
		std::free(data);
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
	{
		return pDefaultIncludeHandler->QueryInterface(riid, ppvObject);
	}

	ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
	ULONG STDMETHODCALLTYPE Release(void) override 
	{ 
		return 0; 
	}
};

#if !USE_DX12
#include <vulkan/vulkan.h>
#include "spirv_reflect.h"
#endif

static void CompileDX12ShaderDXC(XShaderCompileInput& Input, XShaderCompileOutput& Output)
{
	if (!pUtils)
	{
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf())));
		ThrowIfFailed(pUtils->CreateDefaultIncludeHandler(pDefaultIncludeHandler.GetAddressOf()));
	}

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	
	XDxRefCount<IDxcCompiler3> pCompiler;
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));
	
	std::vector<LPCWSTR>Args;

	Args.push_back(Input.SourceFilePath.c_str());

	//EntryPoint
	Args.push_back(TEXT("-E"));
	std::wstring EntryName = converter.from_bytes(Input.EntryPointName);
	Args.push_back(EntryName.c_str());
	
	std::string Target;
	switch (Input.Shadertype)
	{
	case EShaderType::SV_Vertex:Target = "vs_6_1"; break;
	case EShaderType::SV_Pixel:Target = "ps_6_1"; break;
	case EShaderType::SV_Compute:Target = "cs_6_1"; break;
#if RHI_RAYTRACING
	case EShaderType::SV_RayGen:Target = "lib_6_3";;
	case EShaderType::SV_RayMiss:Target = "lib_6_3";;
	case EShaderType::SV_HitGroup:Target = "lib_6_3"; break;
#endif
	default:XASSERT(false); break;
	}

	Args.push_back(TEXT("-T"));
	std::wstring Targetame = converter.from_bytes(Target);
	Args.push_back(Targetame.c_str());

	std::vector<std::wstring> DefineArray;
	//Defines
	if (Input.CompileSettings.Defines.size() > 0)
	{
		for (auto iter = Input.CompileSettings.Defines.begin(); iter != Input.CompileSettings.Defines.end(); iter++)
		{
			Args.push_back(TEXT("-D"));
			DefineArray.push_back(converter.from_bytes(std::string(iter->first + "=" + iter->second)));
			Args.push_back(DefineArray[DefineArray.size() - 1].c_str());
		}
	}

	Args.push_back(TEXT("-Zi"));

#if !USE_DX12
	Args.push_back(TEXT("-spirv"));
	Args.push_back(TEXT("-fspv-reflect"));
	Args.push_back(TEXT("-fspv-target-env=vulkan1.3"));
	//Args.push_back(TEXT("-fspv-extension=SPV_EXT_descriptor_indexing"));
	Args.push_back(TEXT("-fspv-extension=SPV_KHR_ray_query"));
	Args.push_back(TEXT("-fspv-extension=SPV_KHR_ray_tracing"));

	Args.push_back(TEXT("-fspv-extension=SPV_GOOGLE_hlsl_functionality1"));
	Args.push_back(TEXT("-fspv-extension=SPV_GOOGLE_user_type"));
	//Args.push_back(TEXT("-fspv-target-env=vulkan1.1"));
#else
	Args.push_back(TEXT("-Qstrip_reflect"));

	Args.push_back(TEXT("-Fo"));
	Args.push_back(TEXT("dxcshader.bin"));

	Args.push_back(TEXT("-Fd"));
	Args.push_back(TEXT("dxcshader.pbd"));
#endif

	XDxRefCount<IDxcBlobEncoding> pSource = nullptr;
	pUtils->LoadFile(Input.SourceFilePath.data(), nullptr, &pSource);
	
	DxcBuffer Source;
	Source.Ptr = pSource->GetBufferPointer();
	Source.Size = pSource->GetBufferSize();
	Source.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.
	

	XDXCInclude DXCInclude;
	DXCInclude.IncludePathToCode = &Input.CompileSettings.IncludePathToCode;
	XDxRefCount<IDxcResult> pResults;
	ThrowIfFailed(pCompiler->Compile(&Source, Args.data(), Args.size(), &DXCInclude, IID_PPV_ARGS(pResults.GetAddressOf())));

	
	XDxRefCount<IDxcBlobUtf8> pErrors = nullptr;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());
	}
		

	HRESULT hrStatus;
	pResults->GetStatus(&hrStatus);
	ThrowIfFailed(hrStatus);

	XDxRefCount<IDxcBlob> CodeGened = nullptr;
	XDxRefCount<IDxcBlobUtf16> pShaderName = nullptr;
	pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&CodeGened), &pShaderName);
	Output.Shadertype = Input.Shadertype;
	Output.ShaderCode.clear();
	Output.ShaderCode.insert(
		Output.ShaderCode.end(),
		static_cast<uint8*>(CodeGened->GetBufferPointer()),
		static_cast<uint8*>(CodeGened->GetBufferPointer()) + CodeGened->GetBufferSize());
	Output.SourceCodeHash = std::hash<std::string>{}(
		std::string((const char*)Output.ShaderCode.data(), Output.ShaderCode.size()));
#if USE_DX12
	//Reflect
	{
		XDxRefCount< ID3D12ShaderReflection > Reflection;

		XDxRefCount<IDxcBlob> pReflectionData;
		pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflectionData), nullptr);
		if (pReflectionData != nullptr)
		{
			DxcBuffer ReflectionData;
			ReflectionData.Encoding = DXC_CP_ACP;
			ReflectionData.Ptr = pReflectionData->GetBufferPointer();
			ReflectionData.Size = pReflectionData->GetBufferSize();
			pUtils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&Reflection));
		} 

		D3D12_SHADER_DESC ShaderDesc;
		Reflection->GetDesc(&ShaderDesc);

		uint8 NumSRVCount = 0;
		uint8 NumCBVCount = 0;
		uint8 NumUAVCount = 0;
		uint8 NumSamplerCount = 0;
		for (uint32 i = 0; i < ShaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC  ResourceDesc;
			Reflection->GetResourceBindingDesc(i, &ResourceDesc);
			D3D_SHADER_INPUT_TYPE ResourceType = ResourceDesc.Type;

			XShaderParameterInfo ParameterInfo;
			ParameterInfo.BufferIndex = ResourceDesc.BindPoint;
			ParameterInfo.ResourceCount = ResourceDesc.BindCount;

			if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE || ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED)
			{
				NumSRVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::SRV;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
			}
			else if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			{
				NumCBVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::CBV;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
				if (strncmp(ResourceDesc.Name, "cbView", 64) != 0)
				{
					ID3D12ShaderReflectionConstantBuffer* ConstantBuffer = Reflection->GetConstantBufferByName(ResourceDesc.Name);
					D3D12_SHADER_BUFFER_DESC CBDesc;
					ConstantBuffer->GetDesc(&CBDesc);
					for (uint32 ConstantIndex = 0; ConstantIndex < CBDesc.Variables; ConstantIndex++)
					{
						ID3D12ShaderReflectionVariable* Variable = ConstantBuffer->GetVariableByIndex(ConstantIndex);
						D3D12_SHADER_VARIABLE_DESC VariableDesc;
						Variable->GetDesc(&VariableDesc);
						ParameterInfo.VariableOffsetInBuffer = VariableDesc.StartOffset;
						ParameterInfo.VariableSize = VariableDesc.Size;
						Output.ShaderParameterMap.MapNameToParameter[VariableDesc.Name] = ParameterInfo;
					}
				}
			}
			else if (
				ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWTYPED ||
				ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_APPEND_STRUCTURED ||
				ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
			{
				NumUAVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::UAV;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
			}
			else if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER)
			{
				NumSamplerCount++;
				ParameterInfo.Parametertype = EShaderParametertype::Sampler;
				Output.ShaderParameterMap.MapNameToParameter[ResourceDesc.Name] = ParameterInfo;
			}
			else
			{
				XASSERT(false);
			}


		}
		int32 TotalOptionalDataSize = 0;
		XShaderResourceCount ResourceCount = { NumSRVCount ,NumCBVCount ,NumUAVCount ,NumSamplerCount};

		int32 ResoucrCountSize = static_cast<int32>(sizeof(XShaderResourceCount));
		Output.ShaderCode.push_back(XShaderResourceCount::Key);
		Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&ResoucrCountSize), (uint8*)(&ResoucrCountSize) + 4);
		Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&ResourceCount), (uint8*)(&ResourceCount) + sizeof(XShaderResourceCount));

		TotalOptionalDataSize += sizeof(uint8);//XShaderResourceCount::Key
		TotalOptionalDataSize += sizeof(int32);//ResoucrCountSize
		TotalOptionalDataSize += sizeof(XShaderResourceCount);//XShaderResourceCount
		TotalOptionalDataSize += sizeof(int32);//TotalOptionalDataSize
		Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&TotalOptionalDataSize), (uint8*)(&TotalOptionalDataSize) + 4);
	}
#else
	//\VulkanShaderFormat\Private\VulkanShaderCompiler.cpp
	spv_reflect::ShaderModule SpirvReflection(CodeGened->GetBufferSize(), CodeGened->GetBufferPointer());

	uint32 NumBindings = 0;
	SpvReflectResult SpvResult = SpirvReflection.EnumerateDescriptorBindings(&NumBindings, nullptr);

	uint8 NumSRVCount = 0;
	uint8 NumCBVCount = 0;
	uint8 NumUAVCount = 0;
	uint8 NumSamplerCount = 0;

	if (NumBindings > 0)
	{
		std::vector<SpvReflectDescriptorBinding*> Bindings;
		Bindings.resize(NumBindings);
		SpvResult = SpirvReflection.EnumerateDescriptorBindings(&NumBindings, &Bindings[0]);
		XASSERT(SpvResult == SPV_REFLECT_RESULT_SUCCESS);


		for (SpvReflectDescriptorBinding* BindingEntry : Bindings)
		{
			XShaderParameterInfo ParameterInfo;
			ParameterInfo.BufferIndex = BindingEntry->binding;
			ParameterInfo.ResourceCount = BindingEntry->count;
			if (BindingEntry->resource_type == SPV_REFLECT_RESOURCE_FLAG_CBV)
			{
				NumCBVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::CBV;
				Output.ShaderParameterMap.MapNameToParameter[BindingEntry->name] = ParameterInfo;
				//if (strncmp(ResourceDesc.Name, "cbView", 64) != 0)
			}
			else if(BindingEntry->resource_type == SPV_REFLECT_RESOURCE_FLAG_SRV)
			{
				NumSRVCount++;
				ParameterInfo.Parametertype = EShaderParametertype::SRV;
				Output.ShaderParameterMap.MapNameToParameter[BindingEntry->name] = ParameterInfo;
			}
			else if (BindingEntry->resource_type == SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
			{
				NumSamplerCount++;
				ParameterInfo.Parametertype = EShaderParametertype::Sampler;
				Output.ShaderParameterMap.MapNameToParameter[BindingEntry->name] = ParameterInfo;
			}
			else
			{
				XASSERT_TEMP(false);
			}
		}
	}

	int32 TotalOptionalDataSize = 0;
	XShaderResourceCount ResourceCount = { NumSRVCount ,NumCBVCount ,NumUAVCount ,NumSamplerCount };

	int32 ResoucrCountSize = static_cast<int32>(sizeof(XShaderResourceCount));
	Output.ShaderCode.push_back(XShaderResourceCount::Key);
	Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&ResoucrCountSize), (uint8*)(&ResoucrCountSize) + 4);
	Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&ResourceCount), (uint8*)(&ResourceCount) + sizeof(XShaderResourceCount));

	std::string EntryPointName = SpirvReflection.GetEntryPointName();
	int32 EntryPointSize = EntryPointName.size();
	Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&EntryPointSize), (uint8*)(&EntryPointSize) + 4);
	Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(EntryPointName.c_str()), (uint8*)(EntryPointName.c_str()) + EntryPointName.size());

	TotalOptionalDataSize += sizeof(uint8);//XShaderResourceCount::Key
	TotalOptionalDataSize += sizeof(int32);//ResoucrCountSize
	TotalOptionalDataSize += sizeof(XShaderResourceCount);//XShaderResourceCount

	TotalOptionalDataSize += sizeof(int32);//EntryPointSize
	TotalOptionalDataSize += EntryPointSize;//EntryPointName

	TotalOptionalDataSize += sizeof(int32);//TotalOptionalDataSize
	Output.ShaderCode.insert(Output.ShaderCode.end(), (uint8*)(&TotalOptionalDataSize), (uint8*)(&TotalOptionalDataSize) + 4);
#endif
}

#endif

void CompileMaterialShader(XShaderCompileInput& Input, XShaderCompileOutput& Output)
{
#if USE_DXC 
	CompileDX12ShaderDXC(Input, Output);
#else
	CompileDX12Shader(Input, Output);
#endif
}

void CompileGlobalShaderMap()
{
	if (GGlobalShaderMapping == nullptr)
	{
		//ShaderMapInFileUnit has three part , first : source code ,second : shaders info,third : RHIShader 

		GGlobalShaderMapping = new XGlobalShaderMapping_ProjectUnit();
		std::list<XShaderInfo*>& ShaderInfosUsedToCompile_LinkedList = XShaderInfo::GetShaderInfo_LinkedList(XShaderInfo::EShaderTypeForDynamicCast::Global);
		for (auto iter = ShaderInfosUsedToCompile_LinkedList.begin(); iter != ShaderInfosUsedToCompile_LinkedList.end(); iter++)
		{
			XShaderCompileInput Input;
			Input.SourceFilePath = (*iter)->GetSourceFileName();
			Input.EntryPointName = (*iter)->GetEntryName();
			Input.Shadertype = (*iter)->GetShaderType();
			Input.ShaderName = (*iter)->GetShaderName();
			(*iter)->ModifySettingsPtr(Input.CompileSettings);

			XShaderCompileOutput Output;
#if USE_DXC 
			CompileDX12ShaderDXC(Input, Output);
#else
			CompileDX12Shader(Input, Output);
#endif
			
			XGlobalShaderMapping_FileUnit* ShaderFileUnit = GGlobalShaderMapping->FindOrAddShaderMapping_FileUnit((*iter));
			
			//first:store source code
			std::size_t HashIndex = ShaderFileUnit->GetShaderMapStoreCodes()->AddShaderCompilerOutput(Output);
			
			//second: xshaders
			XXShader* Shader = (*iter)->CtorPtr(XShaderInitlizer(*iter, Output, HashIndex));
			ShaderFileUnit->GetShaderMapStoreXShaders()->FindOrAddXShader((*iter)->GetHashedShaderNameIndex(), Shader, 0);
		}
		
		//third: RHIShader
		std::unordered_map<std::size_t, XGlobalShaderMapping_FileUnit*>ShaderMapFileUnit = GGlobalShaderMapping->GetGlobalShaderMap_HashMap();
		for (auto iter = ShaderMapFileUnit.begin(); iter != ShaderMapFileUnit.end(); iter++)
		{
			iter->second->InitRHIShaders_InlineCode();
		}
	}
}