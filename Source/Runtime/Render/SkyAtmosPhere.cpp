#include "DeferredShadingRenderer.h"
#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "Runtime/RenderCore/CommonRenderRresource.h"

class XSkyAtmosphereResourece :public XRenderResource
{
public:
	std::shared_ptr <XRHIConstantBuffer>RHICbSkyAtmosphere;
	std::shared_ptr <XRHITexture2D> TransmittanceLutUAV;
	std::shared_ptr <XRHITexture2D> MultiScatteredLuminanceLutUAV;
	std::shared_ptr <XRHITexture2D> SkyViewLutUAV;

	void InitRHI()override
	{
		RHICbSkyAtmosphere = RHICreateConstantBuffer(sizeof(XSkyAtmosphereParams));

		TransmittanceLutUAV = RHICreateTexture2D(256, 64, 1, false, false,
			EPixelFormat::FT_R11G11B10_FLOAT
			, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1
			, nullptr);

		MultiScatteredLuminanceLutUAV = RHICreateTexture2D(32, 32, 1, false, false,
			EPixelFormat::FT_R11G11B10_FLOAT
			, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1
			, nullptr);

		SkyViewLutUAV = RHICreateTexture2D(192, 104, 1, false, false,
			EPixelFormat::FT_R11G11B10_FLOAT
			, ETextureCreateFlags(TexCreate_UAV | TexCreate_ShaderResource), 1
			, nullptr);
	}
	void ReleaseRHI()override
	{

	}
};

TGlobalResource<XSkyAtmosphereResourece>SkyAtmosphereResourece;

//XRenderTransmittanceLutCS
class XRenderTransmittanceLutCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XRenderTransmittanceLutCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings)
	{
		OutSettings.SetDefines("THREADGROUP_SIZE", "8");
	}
public:

	XRenderTransmittanceLutCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		TransmittanceLutUAV.Bind(Initializer.ShaderParameterMap, "TransmittanceLutUAV");
		cbSkyAtmosphere.Bind(Initializer.ShaderParameterMap, "SkyAtmosphere");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* InUAV,
		XRHIConstantBuffer* IncbSkyAtmosphere)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, TransmittanceLutUAV, InUAV);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbSkyAtmosphere, IncbSkyAtmosphere);
	}

	CBVParameterType cbSkyAtmosphere;
	UAVParameterType TransmittanceLutUAV;
};

XRenderTransmittanceLutCS::ShaderInfos XRenderTransmittanceLutCS::StaticShaderInfos(
	"RenderTransmittanceLutCS", GET_SHADER_PATH("SkyAtmosphere.hlsl"),
	"RenderTransmittanceLutCS", EShaderType::SV_Compute, XRenderTransmittanceLutCS::CustomConstrucFunc,
	XRenderTransmittanceLutCS::ModifyShaderCompileSettings);

class XRenderMultiScatteredLuminanceLutCS : public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XRenderMultiScatteredLuminanceLutCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings)
	{
		OutSettings.SetDefines("THREADGROUP_SIZE", "8");
	}
public:
	XRenderMultiScatteredLuminanceLutCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		MultiScatteredLuminanceLutUAV.Bind(Initializer.ShaderParameterMap, "MultiScatteredLuminanceLutUAV");
		TransmittanceLutTexture.Bind(Initializer.ShaderParameterMap, "TransmittanceLutTexture");
		cbSkyAtmosphere.Bind(Initializer.ShaderParameterMap, "SkyAtmosphere");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIUnorderedAcessView* InUAV,
		XRHIConstantBuffer* IncbSkyAtmosphere,
		XRHITexture* InTransmittanceLutTexture)
	{
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, MultiScatteredLuminanceLutUAV, InUAV);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbSkyAtmosphere, IncbSkyAtmosphere);
		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, TransmittanceLutTexture, InTransmittanceLutTexture);
	}

	UAVParameterType MultiScatteredLuminanceLutUAV;
	CBVParameterType cbSkyAtmosphere;
	TextureParameterType TransmittanceLutTexture;
};

XRenderMultiScatteredLuminanceLutCS::ShaderInfos XRenderMultiScatteredLuminanceLutCS::StaticShaderInfos(
	"RenderMultiScatteredLuminanceLutCS", GET_SHADER_PATH("SkyAtmosphere.hlsl"),
	"RenderMultiScatteredLuminanceLutCS", EShaderType::SV_Compute, XRenderMultiScatteredLuminanceLutCS::CustomConstrucFunc,
	XRenderMultiScatteredLuminanceLutCS::ModifyShaderCompileSettings);

class XRenderSkyViewLutCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XRenderSkyViewLutCS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings)
	{
		OutSettings.SetDefines("THREADGROUP_SIZE", "8");
	}
public:
	XRenderSkyViewLutCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		cbView.Bind(Initializer.ShaderParameterMap, "cbView");
		cbSkyAtmosphere.Bind(Initializer.ShaderParameterMap, "SkyAtmosphere");

		SkyViewLutUAV.Bind(Initializer.ShaderParameterMap, "SkyViewLutUAV");

		MultiScatteredLuminanceLutTexture.Bind(Initializer.ShaderParameterMap, "MultiScatteredLuminanceLutTexture");
		TransmittanceLutTexture.Bind(Initializer.ShaderParameterMap, "TransmittanceLutTexture");
	}

	void SetParameters(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* IncbView,
		XRHIConstantBuffer* IncbSkyAtmosphere,

		XRHIUnorderedAcessView* InSkyViewLutUAV,
		XRHITexture* InTransmittanceLutTexture,
		XRHITexture* InMultiScatteredLuminanceLutTexture
	)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbView, IncbView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbSkyAtmosphere, IncbSkyAtmosphere);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, SkyViewLutUAV, InSkyViewLutUAV);

		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, TransmittanceLutTexture, InTransmittanceLutTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, MultiScatteredLuminanceLutTexture, InMultiScatteredLuminanceLutTexture);
	}

	CBVParameterType cbView;
	CBVParameterType cbSkyAtmosphere;

	UAVParameterType SkyViewLutUAV;

	TextureParameterType MultiScatteredLuminanceLutTexture;
	TextureParameterType TransmittanceLutTexture;
};

XRenderSkyViewLutCS::ShaderInfos XRenderSkyViewLutCS::StaticShaderInfos(
	"RenderSkyViewLutCS", GET_SHADER_PATH("SkyAtmosphere.hlsl"),
	"RenderSkyViewLutCS", EShaderType::SV_Compute, XRenderSkyViewLutCS::CustomConstrucFunc,
	XRenderSkyViewLutCS::ModifyShaderCompileSettings);


void XDeferredShadingRenderer::SkyAtmosPhereUpdata(XRHICommandList& RHICmdList)
{
	// All distance here are in kilometer and scattering/absorptions coefficient in 1/kilometers.
	const float EarthBottomRadius = 6360.0f;
	const float EarthTopRadius = 6420.0f;

	//SkyComponent SkyAtmosphereCommonData.cpp
	XVector4 RayleighScattering = XVector4(0.175287, 0.409607, 1, 1);
	float RayleighScatteringScale = 0.0331;

	float RayleighExponentialDistribution = 8.0f;
	float RayleighDensityExpScale = -1.0 / RayleighExponentialDistribution;

	XVector4 MieScattering = XVector4(1, 1, 1, 1);
	XVector4 MieAbsorption = XVector4(1, 1, 1, 1);

	// The altitude in kilometer at which Mie effects are reduced to 40%.
	float MieExponentialDistribution = 1.2;
	float MieScatteringScale = 0.003996;
	float MieAbsorptionScale = 0.000444;

	float MieDensityExpScale = -1.0f / MieExponentialDistribution;
	float TransmittanceSampleCount = 10.0f;
	float MultiScatteringSampleCount = 15.0f;

	const float CmToSkyUnit = 0.00001f;			// Centimeters to Kilometers
	const float SkyUnitToCm = 1.0f / 0.00001f;	// Kilometers to Centimeters


	XVector3 CametaWorldOrigin = RViewInfo.ViewMats.GetViewOrigin();
	XVector3 CamForward = RViewInfo.ViewMats.GetEyeForwarddDir();
	//XVector3 CamForward = XVector3(CametaTargetPos.x - CametaWorldOrigin.x, CametaTargetPos.y - CametaWorldOrigin.y, CametaTargetPos.z - CametaWorldOrigin.z);
	XVector3 PlanetCenterKm = XVector3(0, -EarthBottomRadius, 0);

	const float PlanetRadiusOffset = 0.005f;
	const float Offset = PlanetRadiusOffset * SkyUnitToCm;
	const float BottomRadiusWorld = EarthBottomRadius * SkyUnitToCm;
	
	const XVector3 PlanetCenterWorld = PlanetCenterKm * SkyUnitToCm;
	const XVector3 PlanetCenterToCameraWorld = CametaWorldOrigin - PlanetCenterWorld;

	const float DistanceToPlanetCenterWorld= PlanetCenterToCameraWorld.Length();

	
	// If the camera is below the planet surface, we snap it back onto the surface.
	XVector3 SkyWorldCameraOrigin = DistanceToPlanetCenterWorld < (BottomRadiusWorld + Offset)
		? PlanetCenterWorld + (BottomRadiusWorld + Offset) * (PlanetCenterToCameraWorld / DistanceToPlanetCenterWorld) :
		CametaWorldOrigin;

	float SkyViewHeight=(SkyWorldCameraOrigin - PlanetCenterWorld).Length();
	
	RViewInfo.ViewCBCPUData.SkyWorldCameraOrigin = SkyWorldCameraOrigin;
	RViewInfo.ViewCBCPUData.SkyPlanetCenterAndViewHeight = XVector4(PlanetCenterWorld.x, PlanetCenterWorld.y, PlanetCenterWorld.z, SkyViewHeight);

	XVector3 SkyUp = (SkyWorldCameraOrigin - PlanetCenterWorld) * CmToSkyUnit;
	SkyUp.Normalize();
	
	XVector3 SkyLeft = SkyUp.Cross(CamForward);
	SkyLeft.Normalize();

	float DotMainDir = abs(SkyUp.Dot(CamForward));
	if (DotMainDir > 0.999f)
	{
		XASSERT(false);
		XVector3 UpStore = SkyUp;
		const float Sign = UpStore.z >= 0.0f ? 1.0f : -1.0f;
		const float a = -1.0f / (Sign + UpStore.z);
		const float b = UpStore.x * UpStore.y * a;
		CamForward = XVector3(1 + Sign * a * pow(UpStore.x, 2.0f), Sign * b, -Sign * UpStore.x);
		SkyLeft = XVector3(b, Sign + a * pow(UpStore.y, 2.0f), -UpStore.y);
	}
	else
	{
		CamForward = SkyUp.Cross(SkyLeft);
		CamForward.Normalize();
	}

	XVector3 SkyViewRow0 = SkyLeft;
	XVector3 SkyViewRow1 = SkyUp;
	XVector3 SkyViewRow2 = CamForward;

	XMatrix SkyViewLutReferential(
		SkyViewRow0.x, SkyViewRow0.y, SkyViewRow0.z, 0,
		SkyViewRow1.x, SkyViewRow1.y, SkyViewRow1.z, 0,
		SkyViewRow2.x, SkyViewRow2.y, SkyViewRow2.z, 0,
		0, 0, 0, 0);

	RViewInfo.ViewCBCPUData.SkyViewLutReferential = SkyViewLutReferential;

	cbSkyAtmosphereIns.TransmittanceLutSizeAndInvSize = XVector4(256.0, 64.0, 1.0 / 256.0, 1.0 / 64.0);
	cbSkyAtmosphereIns.MultiScatteredLuminanceLutSizeAndInvSize = XVector4(32.0, 32.0, 1.0 / 32.0, 1.0 / 32.0);
	cbSkyAtmosphereIns.SkyViewLutSizeAndInvSize = XVector4(192.0, 104.0, 1.0 / 192.0, 1.0 / 104.0);
	cbSkyAtmosphereIns.CameraAerialPerspectiveVolumeSizeAndInvSize = XVector4(32.0, 32.0, 1.0 / 32.0, 1.0 / 32.0);

	cbSkyAtmosphereIns.RayleighScattering = XVector4(
		RayleighScattering.x * RayleighScatteringScale,
		RayleighScattering.y * RayleighScatteringScale,
		RayleighScattering.z * RayleighScatteringScale,
		RayleighScattering.w * RayleighScatteringScale);

	cbSkyAtmosphereIns.MieScattering = XVector4(
		MieScattering.x * MieScatteringScale,MieScattering.y * MieScatteringScale,
		MieScattering.z * MieScatteringScale,MieScattering.w * MieScatteringScale);

	cbSkyAtmosphereIns.MieAbsorption = XVector4(
		MieAbsorption.x * MieAbsorptionScale, MieAbsorption.y * MieAbsorptionScale,
		MieAbsorption.z * MieAbsorptionScale, MieAbsorption.w * MieAbsorptionScale);

	cbSkyAtmosphereIns.MieExtinction = XVector4(
		cbSkyAtmosphereIns.MieScattering.x + cbSkyAtmosphereIns.MieAbsorption.x,
		cbSkyAtmosphereIns.MieScattering.y + cbSkyAtmosphereIns.MieAbsorption.y,
		cbSkyAtmosphereIns.MieScattering.z + cbSkyAtmosphereIns.MieAbsorption.z,
		cbSkyAtmosphereIns.MieScattering.w + cbSkyAtmosphereIns.MieAbsorption.w);

	cbSkyAtmosphereIns.BottomRadiusKm = EarthBottomRadius;
	cbSkyAtmosphereIns.TopRadiusKm = EarthTopRadius;
	cbSkyAtmosphereIns.MieDensityExpScale = MieDensityExpScale;
	cbSkyAtmosphereIns.RayleighDensityExpScale = RayleighDensityExpScale;
	cbSkyAtmosphereIns.TransmittanceSampleCount = TransmittanceSampleCount;
	cbSkyAtmosphereIns.MultiScatteringSampleCount = MultiScatteringSampleCount;
	cbSkyAtmosphereIns.GroundAlbedo = XVector4(0.66, 0.66, 0.66, 1.0);
	cbSkyAtmosphereIns.MiePhaseG = 0.8f;

	cbSkyAtmosphereIns.Light0Illuminance = XVector4(MainLightColor.x * LightIntensity, MainLightColor.y * LightIntensity, MainLightColor.z * LightIntensity, 0);

	cbSkyAtmosphereIns.CameraAerialPerspectiveVolumeDepthResolution = 16;
	cbSkyAtmosphereIns.CameraAerialPerspectiveVolumeDepthResolutionInv = 1.0 / 16.0;
	cbSkyAtmosphereIns.CameraAerialPerspectiveVolumeDepthSliceLengthKm = 96 / 16;

	SkyAtmosphereResourece.RHICbSkyAtmosphere->UpdateData(&cbSkyAtmosphereIns, sizeof(XSkyAtmosphereParams), 0);
}

void XDeferredShadingRenderer::SkyAtmosPhereRendering(XRHICommandList& RHICmdList)
{
	{
		RHICmdList.RHIEventBegin(1, "RenderTransmittanceLutCS", sizeof("RenderTransmittanceLutCS"));
		TShaderReference<XRenderTransmittanceLutCS> Shader = GetGlobalShaderMapping()->GetShader<XRenderTransmittanceLutCS>();
		XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
		Shader->SetParameters(RHICmdList, GetRHIUAVFromTexture(SkyAtmosphereResourece.TransmittanceLutUAV.get()), SkyAtmosphereResourece.RHICbSkyAtmosphere.get());
		RHICmdList.RHIDispatchComputeShader(256 / 8, 64 / 8, 1);
		RHICmdList.RHIEventEnd();
	}

	{
		RHICmdList.RHIEventBegin(1, "XRenderMultiScatteredLuminanceLutCS", sizeof("XRenderMultiScatteredLuminanceLutCS"));
		TShaderReference<XRenderMultiScatteredLuminanceLutCS> Shader = GetGlobalShaderMapping()->GetShader<XRenderMultiScatteredLuminanceLutCS>();
		XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
		Shader->SetParameters(RHICmdList, GetRHIUAVFromTexture(SkyAtmosphereResourece.MultiScatteredLuminanceLutUAV.get()), SkyAtmosphereResourece.RHICbSkyAtmosphere.get(), SkyAtmosphereResourece.TransmittanceLutUAV.get());
		RHICmdList.RHIDispatchComputeShader(32 / 8, 32 / 8, 1);
		RHICmdList.RHIEventEnd();
	}

	//RenderSkyViewLutCS
	{
		RHICmdList.RHIEventBegin(1, "XRenderSkyViewLutCS", sizeof("XRenderSkyViewLutCS"));
		TShaderReference<XRenderSkyViewLutCS> Shader = GetGlobalShaderMapping()->GetShader<XRenderSkyViewLutCS>();
		XRHIComputeShader* ComputeShader = Shader.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
		Shader->SetParameters(RHICmdList, RViewInfo.ViewConstantBuffer.get(), SkyAtmosphereResourece.RHICbSkyAtmosphere.get(),
			GetRHIUAVFromTexture(SkyAtmosphereResourece.SkyViewLutUAV.get()), SkyAtmosphereResourece.TransmittanceLutUAV.get(), SkyAtmosphereResourece.MultiScatteredLuminanceLutUAV.get());
		RHICmdList.RHIDispatchComputeShader(192 / 8, 104 / 8, 1);
		RHICmdList.RHIEventEnd();
	}
}


class XRenderSkyAtmosphereRayMarchingPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XRenderSkyAtmosphereRayMarchingPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XRenderSkyAtmosphereRayMarchingPS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		cbView.Bind(Initializer.ShaderParameterMap, "cbView");
		SkyAtmosphere.Bind(Initializer.ShaderParameterMap, "SkyAtmosphere");

		SkyViewLutTexture.Bind(Initializer.ShaderParameterMap, "SkyViewLutTexture");
		SceneDepthTexture.Bind(Initializer.ShaderParameterMap, "SceneTexturesStruct_SceneDepthTexture");
		TransmittanceLutTexture_Combine.Bind(Initializer.ShaderParameterMap, "TransmittanceLutTexture_Combine");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHIConstantBuffer* IncbView,
		XRHIConstantBuffer* InSkyAtmosphere,
		XRHITexture* InSkyViewLutTexture,
		XRHITexture* InSceneDepthTexture,
		XRHITexture* InTransmittanceLutTexture_Combine
	)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, cbView, IncbView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Pixel, SkyAtmosphere, InSkyAtmosphere);

		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, SkyViewLutTexture, InSkyViewLutTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, SceneDepthTexture, InSceneDepthTexture);
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, TransmittanceLutTexture_Combine, InTransmittanceLutTexture_Combine);
	}

	CBVParameterType cbView;
	CBVParameterType SkyAtmosphere;

	TextureParameterType SkyViewLutTexture;
	TextureParameterType SceneDepthTexture;
	TextureParameterType TransmittanceLutTexture_Combine;
};
XRenderSkyAtmosphereRayMarchingPS::ShaderInfos XRenderSkyAtmosphereRayMarchingPS::StaticShaderInfos(
	"XRenderSkyAtmosphereRayMarchingPS", GET_SHADER_PATH("SkyAtmosphere.hlsl"),
	"RenderSkyAtmosphereRayMarchingPS", EShaderType::SV_Pixel, XRenderSkyAtmosphereRayMarchingPS::CustomConstrucFunc,
	XRenderSkyAtmosphereRayMarchingPS::ModifyShaderCompileSettings);

void XDeferredShadingRenderer::SkyAtmoSphereCombine(XRHICommandList& RHICmdList)
{
	XRHITexture* TextureSceneColor = SceneTargets.TextureSceneColorDeffered.get();
	XRHIRenderPassInfo RPInfos(1, &TextureSceneColor, ERenderTargetLoadAction::ELoad, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList.RHIBeginRenderPass(RPInfos, "SkyAtmosphere Combine Pass", sizeof("SkyAtmosphere Combine Pass"));
	RHICmdList.CacheActiveRenderTargets(RPInfos);

	XGraphicsPSOInitializer GraphicsPSOInit;
	GraphicsPSOInit.BlendState = TStaticBlendState<true,
		EBlendOperation::BO_Add, EBlendFactor::BF_One, EBlendFactor::BF_One,
		EBlendOperation::BO_Add, EBlendFactor::BF_One, EBlendFactor::BF_One>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
	GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

	TShaderReference<RFullScreenQuadVS> VertexShader = GetGlobalShaderMapping()->GetShader<RFullScreenQuadVS>();
	TShaderReference<XRenderSkyAtmosphereRayMarchingPS> PixelShader = GetGlobalShaderMapping()->GetShader<XRenderSkyAtmosphereRayMarchingPS>();
	GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
	GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GFullScreenLayout.RHIVertexLayout.get();

	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);
	PixelShader->SetParameter(RHICmdList, RViewInfo.ViewConstantBuffer.get(), SkyAtmosphereResourece.RHICbSkyAtmosphere.get(),
		SkyAtmosphereResourece.SkyViewLutUAV.get(), SceneTargets.TextureDepthStencil.get(), SkyAtmosphereResourece.TransmittanceLutUAV.get());

	RHICmdList.SetVertexBuffer(GFullScreenVertexRHI.RHIVertexBuffer.get(), 0, 0);
	RHICmdList.RHIDrawIndexedPrimitive(GFullScreenIndexRHI.RHIIndexBuffer.get(), 6, 1, 0, 0, 0);
	RHICmdList.RHIEndRenderPass();
}

