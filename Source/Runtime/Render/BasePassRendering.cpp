#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "Runtime/Engine/Material/MaterialShared.h"
#include "MeshPassProcessor.h"

#include "BasePassRendering.h"

template<>
TBasePassVS<false>::ShaderInfos TBasePassVS<false>::StaticShaderInfos(
	"TBasePassVS<false>", GET_SHADER_PATH("BasePassVertexShader.hlsl"),
	"VS", EShaderType::SV_Vertex, TBasePassVS<false>::CustomConstrucFunc,
	TBasePassVS<false>::ModifyShaderCompileSettings);

template<>
TBasePassPS<false>::ShaderInfos TBasePassPS<false>::StaticShaderInfos(
	"TBasePassPS<false>", GET_SHADER_PATH("BasePassPixelShader_1.hlsl"),
	"PS", EShaderType::SV_Pixel, TBasePassPS<false>::CustomConstrucFunc,
	TBasePassPS<false>::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::BasePassRendering(XRHICommandList& RHICmdList)
{
	XRHITexture* RTTextures[4];
	RTTextures[0] = SceneTargets.TextureGBufferA.get();
	RTTextures[1] = SceneTargets.TextureGBufferB.get();
	RTTextures[2] = SceneTargets.TextureGBufferC.get();
	RTTextures[3] = SceneTargets.TextureGBufferD.get();
	XRHIRenderPassInfo RTInfos(4, RTTextures, ERenderTargetLoadAction::EClear, SceneTargets.TextureDepthStencil.get(), EDepthStencilLoadAction::ELoad);
	RHICmdList.RHIBeginRenderPass(RTInfos, "GBufferPass", sizeof("GBufferPass"));
	RHICmdList.CacheActiveRenderTargets(RTInfos);

	for (int i = 0; i < RenderGeos.size(); i++)
	{
		std::shared_ptr<GGeomertry>& GeoInsPtr = RenderGeos[i];
		std::shared_ptr<GMaterialInstance>& MaterialInstancePtr = GeoInsPtr->GetMaterialInstance();

		{
			XGraphicsPSOInitializer_WithoutRT PassState;
			{
				XRHIBoundShaderStateInput_WithoutRT PassShaders;
				{
					XMaterialShaderInfo_Set ShaderInfos;
					XMaterialShader_Set XShaders;

					ShaderInfos.ShaderInfoSet[(int)EShaderType::SV_Compute] = nullptr;
					ShaderInfos.ShaderInfoSet[(int)EShaderType::SV_Vertex] = &TBasePassVS<false>::StaticShaderInfos;
					ShaderInfos.ShaderInfoSet[(int)EShaderType::SV_Pixel] = &TBasePassPS<false>::StaticShaderInfos;

					MaterialInstancePtr->MaterialPtr->RMaterialPtr->GetShaderInfos(ShaderInfos, XShaders);

					TShaderReference<TBasePassVS<false>> BaseVertexShader = TShaderReference<TBasePassVS<false>>(
						static_cast<TBasePassVS<false>*>(XShaders.XShaderSet[(int32)EShaderType::SV_Vertex]), XShaders.ShaderMap);

					TShaderReference<TBasePassPS<false>> BasePixelShader = TShaderReference<TBasePassPS<false>>(
						static_cast<TBasePassPS<false>*>(XShaders.XShaderSet[(int32)EShaderType::SV_Pixel]), XShaders.ShaderMap);

					//SetParameter
					{
						BaseVertexShader->SetParameter(RHICmdList, RViewInfo.ViewConstantBuffer.get(), GeoInsPtr->GetPerObjectVertexCBuffer().get());
						BasePixelShader->SetParameter(RHICmdList, MaterialInstancePtr);
					}

					std::shared_ptr<XRHIVertexLayout> RefVertexLayout = DefaultVertexFactory.GetLayout(ELayoutType::Layout_Default);
					PassShaders.RHIVertexLayout = RefVertexLayout.get();

					PassShaders.MappingRHIVertexShader = BaseVertexShader.GetShaderMappingFileUnit()->GetRefShaderMapStoreRHIShaders();
					PassShaders.IndexRHIVertexShader = BaseVertexShader->GetRHIShaderIndex();

					PassShaders.MappingRHIPixelShader = BasePixelShader.GetShaderMappingFileUnit()->GetRefShaderMapStoreRHIShaders();
					PassShaders.IndexRHIPixelShader = BasePixelShader->GetRHIShaderIndex();
				}
				PassState.BoundShaderState = PassShaders;
				PassState.BlendState = TStaticBlendState<>::GetRHI();
				PassState.RasterState = TStaticRasterizationState<>::GetRHI();
				PassState.DepthStencilState = TStaticDepthStencilState<true, ECompareFunction::CF_GreaterEqual>::GetRHI();
			}

			{
				XGraphicsPSOInitializer PSOInitializer = PassState.TransToGraphicsPSOInitializer();
				RHICmdList.ApplyCachedRenderTargets(PSOInitializer);

				SetGraphicsPipelineStateFromPSOInit(RHICmdList, PSOInitializer);
			}
		}

		RHICmdList.SetVertexBuffer(GeoInsPtr->GetGVertexBuffer()->GetRHIVertexBuffer().get(), 0, 0);
		RHICmdList.RHIDrawIndexedPrimitive(GeoInsPtr->GetGIndexBuffer()->GetRHIIndexBuffer().get(), GeoInsPtr->GetIndexCount(), 1, 0, 0, 0);
	}

	RHICmdList.RHIEndRenderPass();
}