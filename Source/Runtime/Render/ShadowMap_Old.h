#pragma once
#include "Runtime/RenderCore/RenderResource.h"
#define ShadowMapDepthTextureSize (1024)

struct ShadowViewProjectionCBStruct
{
	XMatrix ShadowViewProjection;
	XMatrix ShadowViewProjectionInv;
};
class XShadowMapResourece_Old :public XRenderResource
{
public:
	std::shared_ptr<XRHIConstantBuffer>ShadowViewProjectionCB;
	ShadowViewProjectionCBStruct ShadowViewProjectionCBStructIns;
	void InitRHI()override
	{

		ShadowViewProjectionCB = RHICreateConstantBuffer(sizeof(ShadowViewProjectionCBStruct));
	}
};
extern TGlobalResource<XShadowMapResourece_Old>ShadowMapResourece_Old;
