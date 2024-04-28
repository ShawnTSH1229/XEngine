#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"


#include "VoxelizationScene.h"
#include "MeshMaterialShader.h"
#include "Runtime/Engine/Classes/Material.h"
#include "Runtime/Engine/Material/MaterialShared.h"
#include "MeshPassProcessor.h"



struct VoxelSceneVSBufferStruct
{
	XMatrix OrientToXView;
	XMatrix OrientToYView;
	XMatrix OrientToZView;
	XMatrix ProjMatrix;
};


void XSVOGIResourece::InitRHI()
{
#if USE_SVOGI
	{
		SpaseVoxelOctree = RHICreateTexture2D(OctreeRealBufferSize, OctreeRealBufferSize, 1, false, false, EPixelFormat::FT_R32_UINT
			, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);

		cbSVOBuildBufferLevels.resize(OctreeHeight);
		for (uint32 i = 0; i < OctreeHeight; i++)
		{
			cbSVOBuildBufferLevels[i] = RHICreateConstantBuffer(sizeof(cbSVOBuildBufferStruct));
		}
		NodeCountAndOffsetBuffer = RHICreateTexture2D((OctreeHeight + 1) *2, 1, 1, false, false, EPixelFormat::FT_R32_UINT
			, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);

		SVOCounterBuffer = RHIcreateStructBuffer(sizeof(uint32), sizeof(uint32), EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);
		SVOCounterBufferUnorderedAcessView = RHICreateUnorderedAccessView(SVOCounterBuffer.get(), true, true, 0);
	}
	

	//Voxel Scene
	{
		VoxelSceneRTPlaceHolder = RHICreateTexture2D(VoxelDimension, VoxelDimension, 1, false, false, EPixelFormat::FT_R8G8B8A8_UNORM,
			ETextureCreateFlags::TexCreate_RenderTargetable, 1, nullptr);

		VoxelSceneVSCBuffer = RHICreateConstantBuffer(sizeof(VoxelSceneVSBufferStruct));
		VoxelScenePSCBuffer = RHICreateConstantBuffer(sizeof(VoxelScenePSBufferStruct));

		VoxelSceneVSBufferStruct VoxelSceneVSCBufferIns;
		VoxelSceneVSCBufferIns.OrientToXView = XMatrix::CreateMatrixLookAtLH(
			XVector3(MinBoundX, (MinBoundY + MaxBoundY) * 0.5, (MinBoundZ + MaxBoundZ) * 0.5),
			XVector3(MaxBoundX, (MinBoundY + MaxBoundY) * 0.5, (MinBoundZ + MaxBoundZ) * 0.5),
			XVector3(0, 1, 0));

		VoxelSceneVSCBufferIns.OrientToYView = XMatrix::CreateMatrixLookAtLH(
			XVector3((MinBoundX + MaxBoundX) * 0.5, MinBoundY, (MinBoundZ + MaxBoundZ) * 0.5),
			XVector3((MinBoundX + MaxBoundX) * 0.5, MaxBoundY, (MinBoundZ + MaxBoundZ) * 0.5),
			XVector3(0, 0, 1));

		VoxelSceneVSCBufferIns.OrientToZView = XMatrix::CreateMatrixLookAtLH(
			XVector3((MinBoundX + MaxBoundX) * 0.5, (MinBoundY + MaxBoundY) * 0.5, MinBoundZ),
			XVector3((MinBoundX + MaxBoundX) * 0.5, (MinBoundY + MaxBoundY) * 0.5, MaxBoundZ),
			XVector3(0, 1, 0));

		XVector3 BoundCenter = XVector3((MinBoundX + MaxBoundX) * 0.5, (MinBoundY + MaxBoundY) * 0.5, (MinBoundZ + MaxBoundZ) * 0.5);
		XVector3 BoundExtent = XVector3(MaxBoundX, MaxBoundY, MaxBoundZ) - BoundCenter;


		VoxelSceneVSCBufferIns.ProjMatrix = XMatrix::CreateOrthographicOffCenterLH(
			-BoundExtent.x, BoundExtent.x,
			-BoundExtent.y, BoundExtent.y,
			0, BoundExtent.z * 2.0);

		VoxelScenePSBufferStruct VoxelScenePSCBufferIns;
		VoxelScenePSCBufferIns.MinBound = BoundCenter - BoundExtent;
		VoxelScenePSCBufferIns.MaxBound = BoundCenter + BoundExtent;
		VoxelScenePSCBufferIns.VoxelBufferDimension = VoxelDimension;

		VoxelSceneVSCBuffer->UpdateData(&VoxelSceneVSCBufferIns, sizeof(VoxelSceneVSBufferStruct), 0);
		VoxelScenePSCBuffer->UpdateData(&VoxelScenePSCBufferIns, sizeof(VoxelScenePSBufferStruct), 0);

		uint32 StructBufferSize = VoxelDimension * VoxelDimension * 16 * 10;
		uint32 CounterOffset = AlignArbitrary(StructBufferSize, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);
		
		VoxelArrayRW = RHIcreateStructBuffer(
			32, StructBufferSize + sizeof(uint32),
			EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);

		VoxelArrayUnorderedAcessView = RHICreateUnorderedAccessView(VoxelArrayRW.get(), true, true, CounterOffset);
		VoxelArrayShaderResourceView = RHICreateShaderResourceView(VoxelArrayRW.get());

		NodeCountAndOffsetBuffer = RHICreateTexture2D(2 * (OctreeHeight + 4), 1, 1, false, false, EPixelFormat::FT_R32_UINT, ETextureCreateFlags::TexCreate_UAV, 1, nullptr);
	}


	//SVO Inject Light
	{
		IrradianceBrickBufferRWUAV = RHICreateTexture3D(BrickBufferSize, BrickBufferSize, BrickBufferSize,
			EPixelFormat::FT_R16G16B16A16_FLOAT, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);
		RHIcbMainLight = RHICreateConstantBuffer(sizeof(cbMainLightStruct));

		IrradianceBrickBufferPinPongUAV = RHICreateTexture3D(BrickBufferSize, BrickBufferSize, BrickBufferSize,
			EPixelFormat::FT_R16G16B16A16_FLOAT, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1, nullptr);
	}
#endif //  
}

void XSVOGIResourece::ReleaseRHI()
{
}


TGlobalResource<XSVOGIResourece>SVOGIResourece;

class XVoxelSceneVS : public XMeshMaterialShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVoxelSceneVS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) 
	{
	}
public:
	XVoxelSceneVS(const XShaderInitlizer& Initializer) :
		XMeshMaterialShader(Initializer)
	{
		OrientMatrix.Bind(Initializer.ShaderParameterMap, "OrientMatrix");
		CBPerObject.Bind(Initializer.ShaderParameterMap, "cbPerObject");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* OrientMatrixIn,
		XRHIConstantBuffer* CBPerObjectIn)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, OrientMatrix, OrientMatrixIn);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, CBPerObject, CBPerObjectIn);
		
	}
	CBVParameterType OrientMatrix;
	CBVParameterType CBPerObject;
};

XVoxelSceneVS::ShaderInfos XVoxelSceneVS::StaticShaderInfos(
	"XVoxelSceneVS", GET_SHADER_PATH("VoxelizationVS.hlsl"),
	"VS", EShaderType::SV_Vertex, XVoxelSceneVS::CustomConstrucFunc,
	XVoxelSceneVS::ModifyShaderCompileSettings);

class XVoxelScenePS : public XMeshMaterialShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVoxelScenePS(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XVoxelScenePS(const XShaderInitlizer& Initializer) :
		XMeshMaterialShader(Initializer) 
	{
		VoxelArrayUAV.Bind(Initializer.ShaderParameterMap, "VoxelArrayRW");
		ConstantArrayCBV.Bind(Initializer.ShaderParameterMap, "cbVoxelization");
		NodeCountAndOffsetBufferUAV.Bind(Initializer.ShaderParameterMap, "NodeCountAndOffsetBuffer");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		std::shared_ptr<GMaterialInstance> MaterialInstancePtr,
		XRHIUnorderedAcessView* VoxelArrayUAVIn,
		XRHIUnorderedAcessView* NodeCountAndOffsetBufferUAVIn,
		XRHIConstantBuffer* PSVoxelBufferIn
		)
	{
		RHICommandList.SetConstantBuffer(EShaderType::SV_Pixel, 0, MaterialInstancePtr->GetRHIConstantBuffer().get());

		for (auto iter = MaterialInstancePtr->MaterialTextureArray.begin(); iter != MaterialInstancePtr->MaterialTextureArray.end(); iter++)
		{
			RHICommandList.SetShaderTexture(EShaderType::SV_Pixel, iter->ResourceIndex, iter->TexturePtr->GetRHITexture2D().get());
		}

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Pixel, NodeCountAndOffsetBufferUAV, NodeCountAndOffsetBufferUAVIn);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Pixel, VoxelArrayUAV, VoxelArrayUAVIn);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, ConstantArrayCBV, PSVoxelBufferIn);
	}

	UAVParameterType NodeCountAndOffsetBufferUAV;
	UAVParameterType VoxelArrayUAV;
	CBVParameterType ConstantArrayCBV;

};

XVoxelScenePS::ShaderInfos XVoxelScenePS::StaticShaderInfos(
	"XVoxelScenePS", GET_SHADER_PATH("VoxelizationPS.hlsl"),
	"PS", EShaderType::SV_Pixel, XVoxelScenePS::CustomConstrucFunc,
	XVoxelScenePS::ModifyShaderCompileSettings);

bool VoxelSceneInitialize = false;

void XDeferredShadingRenderer::VoxelizationPass(XRHICommandList& RHICmdList)
{
	if (VoxelSceneInitialize == true) [[unlikely]]
	{
		return;
	}
	
	XRHITexture* RTTextures = SVOGIResourece.VoxelSceneRTPlaceHolder.get();
	XRHIRenderPassInfo RTInfos(1, &RTTextures, ERenderTargetLoadAction::ENoAction, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RTInfos, "VoxelScene", sizeof("VoxelScene"));
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
					ShaderInfos.ShaderInfoSet[(int)EShaderType::SV_Vertex] = &XVoxelSceneVS::StaticShaderInfos;
					ShaderInfos.ShaderInfoSet[(int)EShaderType::SV_Pixel] = &XVoxelScenePS::StaticShaderInfos;

					MaterialInstancePtr->MaterialPtr->RMaterialPtr->GetShaderInfos(ShaderInfos, XShaders);

					TShaderReference<XVoxelSceneVS> BaseVertexShader = TShaderReference<XVoxelSceneVS>(
						static_cast<XVoxelSceneVS*>(XShaders.XShaderSet[(int32)EShaderType::SV_Vertex]), XShaders.ShaderMap);

					TShaderReference<XVoxelScenePS> BasePixelShader = TShaderReference<XVoxelScenePS>(
						static_cast<XVoxelScenePS*>(XShaders.XShaderSet[(int32)EShaderType::SV_Pixel]), XShaders.ShaderMap);

					//SetParameter
					{
						BaseVertexShader->SetParameter(RHICmdList, SVOGIResourece.VoxelSceneVSCBuffer.get(), GeoInsPtr->GetPerObjectVertexCBuffer().get());
						BasePixelShader->SetParameter(RHICmdList, MaterialInstancePtr,
							SVOGIResourece.VoxelArrayUnorderedAcessView.get(), 
							GetRHIUAVFromTexture(SVOGIResourece.NodeCountAndOffsetBuffer.get()),
							SVOGIResourece.VoxelScenePSCBuffer.get());
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
				PassState.RasterState = TStaticRasterizationState<true, EFaceCullMode::FC_None>::GetRHI();
				PassState.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
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


