#include "SkyAtmosphereCommon.hlsl"
RWTexture2D<float3> TransmittanceLut;

//uv.x zenith cos angle: [-1,1] 
//uv.y view height [bottom radius, top radius]
void UvToLutTransmittanceParams(out float ViewHeight, out float ViewZenithCosAngle, in float2 UV)
{
    float Xmu = UV.x;
	float Xr = UV.y;

    // H: Distance to top atmosphere boundary for a horizontal ray at ground level
    float H = sqrt(Atmosphere_TopRadiusKm * Atmosphere_TopRadiusKm - Atmosphere_BottomRadiusKm * Atmosphere_BottomRadiusKm);

    // Rho: Distance to the horizon, from which we can compute ViewHeight
	float Rho = H * Xr;
	ViewHeight = sqrt(Rho * Rho + Atmosphere_BottomRadiusKm * Atmosphere_BottomRadiusKm);

    float Dmin = Atmosphere_TopRadiusKm - ViewHeight;
	float Dmax = Rho + H;
	float D = Dmin + Xmu * (Dmax - Dmin);
	ViewZenithCosAngle = D == 0.0f ? 1.0f : (H * H - Rho * Rho - D * D) / (2.0f * ViewHeight * D);
	ViewZenithCosAngle = clamp(ViewZenithCosAngle, -1.0f, 1.0f);
}

float3 ComputeOpticalDepth(in float3 WorldPos,in float3 WorldDir , in float SampleCount)
{
    float t = 0.0f, tMax = 0.0f;
    float3 PlanetO = float3(0.0f, 0.0f, 0.0f);
    float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_BottomRadiusKm);
    float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_TopRadiusKm);

    if (tBottom < 0.0f) // No intersection with plant
	{
		if (tTop < 0.0f)	{	tMax = 0.0f; return 0;	} // No intersection with planet nor its atmosphere: stop right away  
		else				{	tMax = tTop;			}
	}
	else
	{
		if (tTop > 0.0f)	{ tMax = min(tTop, tBottom); }
	}

    float dt = tMax / SampleCount;
    float3 OpticalDepth = 0.0f;
    for (float SampleI = 0.0f; SampleI < SampleCount; SampleI += 1.0f)
    {
        t = tMax * (SampleI + DEFAULT_SAMPLE_OFFSET) / SampleCount;
		float3 P = WorldPos + t * WorldDir;
		MediumSampleRGB Medium = SampleMediumRGB(P);
        const float3 SampleOpticalDepth = Medium.Extinction * dt;
		OpticalDepth += SampleOpticalDepth;		
    }
    return OpticalDepth;
}

[numthreads(8, 8, 1)]
void TransmittanceLutCS(uint3 ThreadId : SV_DispatchThreadID)
{
    float2 PixPos = float2(ThreadId.xy) + 0.5f;

    float2 UV = (PixPos) * SkyAtmosphere_TransmittanceLutSizeAndInvSize.zw;

    float ViewHeight;
	float ViewZenithCosAngle;
	UvToLutTransmittanceParams(ViewHeight, ViewZenithCosAngle, UV);

    float3 WorldPos = float3(0.0f, ViewHeight, 0);
    float3 WorldDir = float3(0.0f, ViewZenithCosAngle,sqrt(1.0f - ViewZenithCosAngle * ViewZenithCosAngle));

    //tau(s)= -int_0^s (extinction(p))dt
	float3 OpticalDepth = ComputeOpticalDepth(WorldPos,WorldDir,Atmosphere_TransmittanceSampleCount);
    
    //Tr = exp(- tau(s))
	float3 transmittance = exp(-OpticalDepth);

    TransmittanceLut[ThreadId.xy] = transmittance;
}