#pragma once
#include "RenderResource.h"
#include "Runtime/RHI/RHIResource.h"
#include "Runtime/RHI/PlatformRHI.h"
#include "Runtime/Core/Math/Math.h"
#include "GlobalShader.h"

struct XFullScreenQuadVertex
{
	XVector2 Position;
	XVector2 UV;
	XFullScreenQuadVertex(XVector2 PositionIn, XVector2 UVIn)
		:Position(PositionIn), UV(UVIn) {}
};

class RFullScreenQuadVertexLayout : public XRenderResource
{
public:
	std::shared_ptr<XRHIVertexLayout> RHIVertexLayout;
	virtual void InitRHI()override
	{
		XRHIVertexLayoutArray LayoutArray;
		LayoutArray.push_back(XVertexElement(0, EVertexElementType::VET_Float2, 0, 0));
		LayoutArray.push_back(XVertexElement(1, EVertexElementType::VET_Float2, 0, 0 + sizeof(DirectX::XMFLOAT2)));
		RHIVertexLayout = RHICreateVertexLayout(LayoutArray);
	}

	virtual void ReleaseRHI()override
	{
		RHIVertexLayout.reset();
	}
};

class RFullScreenQuadVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new RFullScreenQuadVS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileDefines(XShaderCompileSetting& OutDefines) {}
public:
	RFullScreenQuadVS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{

	}
};


class XScreenQuadVertexBuffer :public RVertexBuffer
{
public:
	void InitRHI()override;
};


class XScreenQuadIndexBuffer :public RIndexBuffer
{
public:
	void InitRHI()override;
};

extern TGlobalResource<XScreenQuadIndexBuffer> GFullScreenIndexRHI;
extern TGlobalResource<XScreenQuadVertexBuffer> GFullScreenVertexRHI;
extern TGlobalResource<RFullScreenQuadVertexLayout> GFullScreenLayout;
