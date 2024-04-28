#pragma once
#include "Runtime/HAL/Mch.h"
#include "RenderResource.h"
#include "ShaderCore.h"

enum class ELayoutType
{
	Layout_Default,
	Layout_PositionOnly,
	Layout_PositionAndNormal,
};

class XVertexFactory :public XRenderResource
{

public:
	void InitLayout(const XRHIVertexLayoutArray& LayoutArray, ELayoutType LayoutType);
	inline std::shared_ptr<XRHIVertexLayout>& GetLayout(ELayoutType LayoutType = ELayoutType::Layout_Default)
	{
		switch (LayoutType)
		{
		case ELayoutType::Layout_Default:
			return DefaultLayout;

		default:
			XASSERT(false);
			break;
		}
	}
protected:
	std::shared_ptr<XRHIVertexLayout> DefaultLayout;;
};

class XVertexFactoryShaderInfo
{
public:
	inline const char* GetShaderFileName()
	{
		return ShaderFileName;
	}

	inline void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings)
	{
		OutSettings.IncludePathToCode["Generated/VertexFactory.hlsl"] = "#inlcude \"" + std::string(GetShaderFileName())+"\"";
	}

	static std::vector<XVertexFactoryShaderInfo*>& GetVertexFactoryShaderInfo_Array();

private:
	const char* ShaderFileName;
};