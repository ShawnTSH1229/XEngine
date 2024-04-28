#pragma once
#include "Runtime/RHI/RHIResource.h"

struct XPreDepthPassResource
{
	std::shared_ptr<XRHIStructBuffer> DepthCmdBufferCulled;
	std::shared_ptr<XRHIStructBuffer> DepthCmdBufferNoCulling;
	std::shared_ptr<XRHICommandSignature> RHIDepthCommandSignature;
	std::shared_ptr<XRHIShaderResourceView>CmdBufferShaderResourceView;
	std::shared_ptr<XRHIUnorderedAcessView> CmdBufferUnorderedAcessView;
	uint64 DepthCounterOffset;
	uint64 DepthCmdBufferOffset;
};