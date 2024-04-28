#pragma once
#include <memory>
#include "Runtime/RHI/RHIResource.h"
#include "Runtime/RenderCore/Shader.h"

struct XRHIBoundShaderStateInput_WithoutRT
{
	XRHIVertexLayout* RHIVertexLayout;

	std::shared_ptr<XShaderMappingToRHIShaders>MappingRHIVertexShader;
	std::shared_ptr<XShaderMappingToRHIShaders>MappingRHIPixelShader;
	int32 IndexRHIVertexShader;
	int32 IndexRHIPixelShader;

	inline XRHIBoundShaderStateInput TransToRHIBoundShaderState()
	{
		return XRHIBoundShaderStateInput(RHIVertexLayout,
			MappingRHIVertexShader.get() != nullptr ? static_cast<XRHIVertexShader*>(MappingRHIVertexShader->GetRHIShader(IndexRHIVertexShader)) : nullptr,
			MappingRHIPixelShader.get() != nullptr ? static_cast<XRHIPixelShader*>(MappingRHIPixelShader->GetRHIShader(IndexRHIPixelShader)) : nullptr);
	}
};

class XGraphicsPSOInitializer_WithoutRT
{
public:
	XRHIBoundShaderStateInput_WithoutRT BoundShaderState;
	XRHIBlendState* BlendState;
	XRHIDepthStencilState* DepthStencilState;
	XRHIRasterizationState* RasterState;

	inline XGraphicsPSOInitializer TransToGraphicsPSOInitializer()
	{
		XGraphicsPSOInitializer PSOInitializer;
		PSOInitializer.BoundShaderState = BoundShaderState.TransToRHIBoundShaderState();
		PSOInitializer.BlendState = BlendState;
		PSOInitializer.DepthStencilState = DepthStencilState;
		PSOInitializer.RasterState = RasterState;

		PSOInitializer.RTNums = 0;
		PSOInitializer.RT_Format.fill(EPixelFormat::FT_Unknown);
		PSOInitializer.DS_Format = EPixelFormat::FT_Unknown;

		return PSOInitializer;
	}
};