#include "Common.hlsl"
#include "FastMath.hlsl"
#include "Math.hlsl"

#define CM_TO_SKY_UNIT 0.00001f

cbuffer SkyAtmosphere
{
    float4 SkyAtmosphere_TransmittanceLutSizeAndInvSize;
    float4 SkyAtmosphere_MultiScatteredLuminanceLutSizeAndInvSize;
    float4 SkyAtmosphere_SkyViewLutSizeAndInvSize;
    float4 SkyAtmosphere_CameraAerialPerspectiveVolumeSizeAndInvSize;

	float4 Atmosphere_RayleighScattering;
    
    float4 Atmosphere_MieScattering;
    float4 Atmosphere_MieAbsorption;
    float4 Atmosphere_MieExtinction;
    float4 Atmosphere_GroundAlbedo;

    float Atmosphere_TopRadiusKm;
    float Atmosphere_BottomRadiusKm;
    float Atmosphere_MieDensityExpScale;
    float Atmosphere_RayleighDensityExpScale;

	float Atmosphere_TransmittanceSampleCount;
	float SkyAtmosphere_MultiScatteringSampleCount;
    float Atmosphere_MiePhaseG;
    float Atmosphere_padding0;

    float4 Atmosphere_Light0Illuminance;

    float Atmosphere_CameraAerialPerspectiveVolumeDepthResolution;
    float Atmosphere_CameraAerialPerspectiveVolumeDepthResolutionInv;
    float Atmosphere_CameraAerialPerspectiveVolumeDepthSliceLengthKm;
    float Atmosphere_padding1;

}



/**
 * Returns near intersection in x, far intersection in y, or both -1 if no intersection.
 * RayDirection does not need to be unit length.
 */
float2 RayIntersectSphere(float3 RayOrigin, float3 RayDirection, float4 Sphere)
{
    float3 LocalPosition = RayOrigin - Sphere.xyz;
    float LocalPositionSqr = dot(LocalPosition, LocalPosition);

    float3 QuadraticCoef;
    QuadraticCoef.x = dot(RayDirection, RayDirection);
    QuadraticCoef.y = 2 * dot(RayDirection, LocalPosition);
    QuadraticCoef.z = LocalPositionSqr - Sphere.w * Sphere.w;

    float Discriminant = QuadraticCoef.y * QuadraticCoef.y - 4 * QuadraticCoef.x * QuadraticCoef.z;

    float2 Intersections = -1;

	// Only continue if the ray intersects the sphere
    [flatten]
    if (Discriminant >= 0)
    {
        float SqrtDiscriminant = sqrt(Discriminant);
        Intersections = (-QuadraticCoef.y + float2(-1, 1) * SqrtDiscriminant) / (2 * QuadraticCoef.x);
    }

    return Intersections;
}

// - RayOrigin: ray origin
// - RayDir: normalized ray direction
// - SphereCenter: sphere center
// - SphereRadius: sphere radius
// - Returns distance from RayOrigin to closest intersecion with sphere,
//   or -1.0 if no intersection.
float RaySphereIntersectNearest(float3 RayOrigin, float3 RayDir, float3 SphereCenter, float SphereRadius)
{
	float2 Sol = RayIntersectSphere(RayOrigin, RayDir, float4(SphereCenter, SphereRadius));
	float Sol0 = Sol.x;
	float Sol1 = Sol.y;
	if (Sol0 < 0.0f && Sol1 < 0.0f)
	{
		return -1.0f;
	}
	if (Sol0 < 0.0f)
	{
		return max(0.0f, Sol1);
	}
	else if (Sol1 < 0.0f)
	{
		return max(0.0f, Sol0);
	}
	return max(0.0f, min(Sol0, Sol1));
}


struct MediumSampleRGB
{
    float3 Scattering;
    float3 Absorption;
    float3 Extinction;

    float3 ScatteringMie;
    float3 AbsorptionMie;
    float3 ExtinctionMie;

    float3 ScatteringRay;
    float3 AbsorptionRay;
    float3 ExtinctionRay;

    float3 Albedo;
};

float3 GetAlbedo(float3 Scattering, float3 Extinction)
{
    return Scattering / max(0.001f, Extinction);
}

MediumSampleRGB SampleMediumRGB(in float3 WorldPos)
{
    const float SampleHeight = max(0.0, (length(WorldPos) - Atmosphere_BottomRadiusKm));
    const float DensityMie = exp(Atmosphere_MieDensityExpScale * SampleHeight);
    const float DensityRay = exp(Atmosphere_RayleighDensityExpScale * SampleHeight);

    MediumSampleRGB s;

    s.ScatteringMie = DensityMie * Atmosphere_MieScattering.rgb;
    s.AbsorptionMie = DensityMie * Atmosphere_MieAbsorption.rgb;
    s.ExtinctionMie = DensityMie * Atmosphere_MieExtinction.rgb; // equals to  ScatteringMie + AbsorptionMie

    s.ScatteringRay = DensityRay * Atmosphere_RayleighScattering.rgb;
    s.AbsorptionRay = 0.0f;
    s.ExtinctionRay = s.ScatteringRay + s.AbsorptionRay;

    s.Scattering = s.ScatteringMie + s.ScatteringRay;
    s.Absorption = s.AbsorptionMie + s.AbsorptionRay;
    s.Extinction = s.ExtinctionMie + s.ExtinctionRay;
    s.Albedo = GetAlbedo(s.Scattering, s.Extinction);

    return s;
}

////////////////////////////////////////////////////////////
// LUT functions
////////////////////////////////////////////////////////////

// Transmittance LUT function parameterisation from Bruneton 2017 https://github.com/ebruneton/precomputed_atmospheric_scattering
// uv in [0,1]
// ViewZenithCosAngle in [-1,1]
// ViewHeight in [bottomRAdius, topRadius]


//NOTE : See https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/functions.glsl.html
//explaned in Part Transmittance -> Part Precomputation
void UvToLutTransmittanceParams(out float ViewHeight, out float ViewZenithCosAngle, in float2 UV)
{
	//UV = FromSubUvsToUnit(UV, SkyAtmosphere.TransmittanceLutSizeAndInvSize); // No real impact so off
	float Xmu = UV.x;
	float Xr = UV.y;

	float H = sqrt(Atmosphere_TopRadiusKm * Atmosphere_TopRadiusKm - Atmosphere_BottomRadiusKm * Atmosphere_BottomRadiusKm);
	float Rho = H * Xr;
	ViewHeight = sqrt(Rho * Rho + Atmosphere_BottomRadiusKm * Atmosphere_BottomRadiusKm);

	float Dmin = Atmosphere_TopRadiusKm - ViewHeight;
	float Dmax = Rho + H;
	float D = Dmin + Xmu * (Dmax - Dmin);
	ViewZenithCosAngle = D == 0.0f ? 1.0f : (H * H - Rho * Rho - D * D) / (2.0f * ViewHeight * D);
	ViewZenithCosAngle = clamp(ViewZenithCosAngle, -1.0f, 1.0f);
}

#define DEFAULT_SAMPLE_OFFSET 0.3f

//WorldDir : should be normalized
//to solve tau(s)= -int_0^s (extinction(p))dt
float3 ComputeOpticalDepth(in float3 WorldPos,in float3 WorldDir , in float SampleCount)
{
    float t = 0.0f, tMax = 0.0f;
	float3 PlanetO = float3(0.0f, 0.0f, 0.0f);
	float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_BottomRadiusKm);
	float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_TopRadiusKm);
	
	if (tBottom < 0.0f)
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


////////////////////////////////////////////////////////////
// Transmittance LUT
////////////////////////////////////////////////////////////

#ifndef THREADGROUP_SIZE
#define THREADGROUP_SIZE 1
#endif

RWTexture2D<float3> TransmittanceLutUAV;

[numthreads(THREADGROUP_SIZE, THREADGROUP_SIZE, 1)]
void RenderTransmittanceLutCS(uint3 ThreadId : SV_DispatchThreadID)
{
    float2 PixPos = float2(ThreadId.xy) + 0.5f;

    // Compute camera position from LUT coords
	float2 UV = (PixPos) * SkyAtmosphere_TransmittanceLutSizeAndInvSize.zw;
    float ViewHeight;
	float ViewZenithCosAngle;
	UvToLutTransmittanceParams(ViewHeight, ViewZenithCosAngle, UV);

    //  A few extra needed constants
	//float3 WorldPos = float3(0.0f, 0.0f, ViewHeight);
    float3 WorldPos = float3(0.0f, ViewHeight, 0);
	//float3 WorldDir = float3(0.0f, sqrt(1.0f - ViewZenithCosAngle * ViewZenithCosAngle), ViewZenithCosAngle);
    float3 WorldDir = float3(0.0f, ViewZenithCosAngle,sqrt(1.0f - ViewZenithCosAngle * ViewZenithCosAngle));

	//tau(s)= -int_0^s (extinction(p))dt
	float3 OpticalDepth = ComputeOpticalDepth(WorldPos,WorldDir,Atmosphere_TransmittanceSampleCount);
	
	//Tr = exp(- tau(s))
	float3 transmittance = exp(-OpticalDepth);

    TransmittanceLutUAV[ThreadId.xy]=transmittance;
}



///////////////////////////////////////////////////////////////////////////////////////

void getTransmittanceLutUvs(
	in float viewHeight, in float viewZenithCosAngle, in float BottomRadius, in float TopRadius,
	out float2 UV)
{
    float H = sqrt(max(0.0f, TopRadius * TopRadius - BottomRadius * BottomRadius));
    float Rho = sqrt(max(0.0f, viewHeight * viewHeight - BottomRadius * BottomRadius));

    float Discriminant = viewHeight * viewHeight * (viewZenithCosAngle * viewZenithCosAngle - 1.0f) + TopRadius * TopRadius;
    float D = max(0.0f, (-viewHeight * viewZenithCosAngle + sqrt(Discriminant))); // Distance to atmosphere boundary

    float Dmin = TopRadius - viewHeight;
    float Dmax = Rho + H;
    float Xmu = (D - Dmin) / (Dmax - Dmin);
    float Xr = Rho / H;

    UV = float2(Xmu, Xr);
}

void LutTransmittanceParamsToUv(in float ViewHeight, in float ViewZenithCosAngle, out float2 UV)
{
    getTransmittanceLutUvs(ViewHeight, ViewZenithCosAngle, Atmosphere_BottomRadiusKm, Atmosphere_TopRadiusKm, UV);
}

//#define PI 3.14159265358979
#define PLANET_RADIUS_OFFSET 0.001f
RWTexture2D<float3> MultiScatteredLuminanceLutUAV;
Texture2D<float3> TransmittanceLutTexture;
//SamplerState gsamLinearClamp : register(s5, space1000);

void ComputeScattedLightLuminanceAndMultiScatAs1in(
    in float3 WorldPos, 
    in float3 WorldDir, 
    in float3 Light0Dir,
    in float3 Light0Illuminance,
    in float SampleCount,
	out float3 OutL,
	out float3 OutMultiScatAs1
)
{
    float3 L = 0, MultiScatAs1 = 0;

    float t = 0.0f, tMax = 0.0f;
    float3 PlanetO = float3(0.0f, 0.0f, 0.0f);
    float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_BottomRadiusKm);
    float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_TopRadiusKm);
	
    if (tBottom < 0.0f)
    {
        if      (tTop < 0.0f)   {   tMax = 0.0f;    return;} // No intersection with planet nor its atmosphere: stop right away  
        else                    {   tMax = tTop;}
    }
    else
    {
        if  (tTop > 0.0f)       {   tMax = min(tTop, tBottom); }
    }

    float dt = tMax / SampleCount;
    float3 Throughput = 1.0f;

    const float uniformPhase = 1.0f / (4.0f * PI);
	for (float SampleI = 0.0f; SampleI < SampleCount; SampleI += 1.0f)
	{
        t = tMax * (SampleI + DEFAULT_SAMPLE_OFFSET) / SampleCount;
        float3 P = WorldPos + t * WorldDir;
        float PHeight = length(P);

		MediumSampleRGB Medium = SampleMediumRGB(P);
        const float3 SampleOpticalDepth = Medium.Extinction * dt * 1.0f;
        const float3 SampleTransmittance = exp(-SampleOpticalDepth);

        // 1 is the integration of luminance over the 4pi of a sphere, and assuming an isotropic phase function of 1.0/(4*PI) 
		MultiScatAs1 += Throughput * Medium.Scattering * 1.0f * dt;

        //Compute TransmittanceToLight0 by look up TransmittanceLutTexture ,which is computed in last step
        float2 UV;
        const float3 UpVector = P / PHeight;
        float Light0ZenithCosAngle = dot(Light0Dir, UpVector);
        LutTransmittanceParamsToUv(PHeight, Light0ZenithCosAngle, UV);
        float3 TransmittanceToLight0 = TransmittanceLutTexture.SampleLevel(gsamLinearClamp, UV, 0).rgb;
		
        //Compute PlanetShadow0
        //if Light0 is intersected with plant0 , then PlanetShadow0 is euqals to 0
        float tPlanet0 = RaySphereIntersectNearest(P, Light0Dir, PlanetO + PLANET_RADIUS_OFFSET * UpVector, Atmosphere_BottomRadiusKm);
        float PlanetShadow0 = tPlanet0 >= 0.0f ? 0.0f : 1.0f;

        //Compute Phase 
        //phase funtion become isotropic when scattering time increaseing , so we times uniformPhase
        float3 PhaseTimesScattering0 = Medium.Scattering * uniformPhase;

        //S euals to in point p , how many light scatters to eye dir(PhaseTimesScattering0)
        //and then times(x) transmittance form p to light pos
        //if light dir is intersected with plant , then planet shaow equals to 0
        
        //NOTE : we ingnore MultiScatteredLuminance
        float3 S = Light0Illuminance * PlanetShadow0 * TransmittanceToLight0 * PhaseTimesScattering0;

        // See slide 28 at http://www.frostbite.com/2015/08/physically-based-unified-volumetric-rendering-in-frostbite/ 
        // L = /int_ Tr(x_eye, x_sample) * S dx
        // S is single scattered light , which is computed in last step
        // but Tr(x_eye, x_sample) is unkown
        // /int_0^tmax (e ^(- extinction * pos) x S) dx = (S - S* e ^(- extinction * pos))/ extinction
        float3 Sint = (S - S * SampleTransmittance) / Medium.Extinction; // integrate along the current step segment 
        
        // accumulate and also take into account the transmittance from previous steps
        L += Throughput * Sint; 
        Throughput *= SampleTransmittance;
    }

	//Ground Scattering
	if (tMax == tBottom)
	{
		float3 P = WorldPos + tBottom * WorldDir;
		float PHeight = length(P);

        float2 UV;
		const float3 UpVector = P / PHeight;
		float Light0ZenithCosAngle = dot(Light0Dir, UpVector);
        LutTransmittanceParamsToUv(PHeight, Light0ZenithCosAngle, UV);
        float3 TransmittanceToLight0 = TransmittanceLutTexture.SampleLevel(gsamLinearClamp, UV, 0).rgb;

		const float NdotL0 = saturate(dot(UpVector, Light0Dir));
		L += Light0Illuminance * TransmittanceToLight0 * Throughput * NdotL0 * Atmosphere_GroundAlbedo.rgb / PI;
	}

	OutL=L;
	OutMultiScatAs1=MultiScatAs1;
}

//x in [0,1] CosLightZenithAngle
//y in [0 1] ViewHeight
[numthreads(THREADGROUP_SIZE, THREADGROUP_SIZE, 1)]
void RenderMultiScatteredLuminanceLutCS(uint3 ThreadId : SV_DispatchThreadID)
{
	float2 PixPos = float2(ThreadId.xy) + 0.5f;
	float CosLightZenithAngle = (PixPos.x * SkyAtmosphere_MultiScatteredLuminanceLutSizeAndInvSize.z) * 2.0f - 1.0f;
    //float3 LightDir = float3(0.0f, sqrt(saturate(1.0f - CosLightZenithAngle * CosLightZenithAngle)), CosLightZenithAngle);
    float3 LightDir = float3(0.0f, CosLightZenithAngle, sqrt(saturate(1.0f - CosLightZenithAngle * CosLightZenithAngle)));
    
    float ViewHeight = Atmosphere_BottomRadiusKm +
	(PixPos.y * SkyAtmosphere_MultiScatteredLuminanceLutSizeAndInvSize.w) *
	(Atmosphere_TopRadiusKm - Atmosphere_BottomRadiusKm);

    //float3 WorldPos = float3(0.0f, 0.0f, ViewHeight);
    float3 WorldPos = float3(0.0f, ViewHeight, 0);
    //float3 WorldDir = float3(0.0f, 0.0f, 1.0f);
    float3 WorldDir = float3(0.0f, 1.0f, 0.0f);

	const float3 OneIlluminance = float3(1.0f, 1.0f, 1.0f);

    float3 OutL_0, OutMultiScatAs1_0, OutL_1, OutMultiScatAs1_1;
    ComputeScattedLightLuminanceAndMultiScatAs1in(
        WorldPos, WorldDir, LightDir, OneIlluminance,
        SkyAtmosphere_MultiScatteringSampleCount, OutL_0, OutMultiScatAs1_0);
    ComputeScattedLightLuminanceAndMultiScatAs1in(
        WorldPos, -WorldDir, LightDir, OneIlluminance,
        SkyAtmosphere_MultiScatteringSampleCount, OutL_1, OutMultiScatAs1_1);

    const float SphereSolidAngle = 4.0f * PI;
    const float IsotropicPhase = 1.0f / SphereSolidAngle;

    float3 IntegratedIlluminance = (SphereSolidAngle / 2.0f) * (OutL_0 + OutL_1);
    float3 MultiScatAs1 = (1.0f / 2.0f) * (OutMultiScatAs1_0 + OutMultiScatAs1_1);
    float3 InScatteredLuminance = IntegratedIlluminance * IsotropicPhase;
	
	// For a serie, sum_{n=0}^{n=+inf} = 1 + r + r^2 + r^3 + ... + r^n = 1 / (1.0 - r)
    const float3 R = MultiScatAs1;
    const float3 SumOfAllMultiScatteringEventsContribution = 1.0f / (1.0f - R);
    float3 L = InScatteredLuminance * SumOfAllMultiScatteringEventsContribution;

    MultiScatteredLuminanceLutUAV[PixPos] = L;
}



////////////////////////////////////////////////////////////
// Sky View LUT
////////////////////////////////////////////////////////////
RWTexture2D<float3> SkyViewLutUAV;
Texture2D<float3> MultiScatteredLuminanceLutTexture;
// This is the camera position relative to the virtual planet center.
// This is convenient because for all the math in this file using world position relative to the virtual planet center.
float3 GetCameraPlanetPos()
{
    return (View_SkyWorldCameraOrigin - View_SkyPlanetCenterAndViewHeight.xyz) * CM_TO_SKY_UNIT;
}

float2 FromSubUvsToUnit(float2 uv, float4 SizeAndInvSize)
{
    return (uv - 0.5f * SizeAndInvSize.zw) * (SizeAndInvSize.xy / (SizeAndInvSize.xy - 1.0f));
}
float2 FromUnitToSubUvs(float2 uv, float4 SizeAndInvSize)
{
    return (uv + 0.5f * SizeAndInvSize.zw) * (SizeAndInvSize.xy / (SizeAndInvSize.xy + 1.0f));
}

// SkyViewLut is a new texture used for fast sky rendering.
// It is low resolution of the sky rendering around the camera,
// basically a lat/long parameterisation with more texel close to the horizon for more accuracy during sun set.
void UvToSkyViewLutParams(out float3 ViewDir, in float ViewHeight, in float2 UV)
{
	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
    UV = FromSubUvsToUnit(UV, SkyAtmosphere_SkyViewLutSizeAndInvSize);

	float Vhorizon = sqrt(ViewHeight * ViewHeight - Atmosphere_BottomRadiusKm * Atmosphere_BottomRadiusKm);
	float CosBeta = Vhorizon / ViewHeight;
	float Beta = acosFast4(CosBeta);
	float ZenithHorizonAngle = PI - Beta;

	float ViewZenithAngle;
	if (UV.y < 0.5f)
	{
		float Coord = 2.0f * UV.y;
		Coord = 1.0f - Coord;
		Coord *= Coord;
		Coord = 1.0f - Coord;
		ViewZenithAngle = ZenithHorizonAngle * Coord;
	}
	else
	{
		float Coord = UV.y * 2.0f - 1.0f;
		Coord *= Coord;
		ViewZenithAngle = ZenithHorizonAngle + Beta * Coord;
	}
	float CosViewZenithAngle = cos(ViewZenithAngle);
	float SinViewZenithAngle = sqrt(1.0 - CosViewZenithAngle * CosViewZenithAngle) * (ViewZenithAngle > 0.0f ? 1.0f : -1.0f); // Equivalent to sin(ViewZenithAngle)

	float LongitudeViewCosAngle = UV.x * 2.0f * PI;

	float CosLongitudeViewCosAngle = cos(LongitudeViewCosAngle);
	float SinLongitudeViewCosAngle = sqrt(1.0 - CosLongitudeViewCosAngle * CosLongitudeViewCosAngle) * (LongitudeViewCosAngle <= PI ? 1.0f : -1.0f); // Equivalent to sin(LongitudeViewCosAngle)

    //ZenithHorizonAngle is for pi/2 to -pi/2   ->  theta
    //Spherical Coordinate TO Rectangular Coordinates/ Cartesian Coordinates
   
    //sin(theta)*cos(phi)
    //cos(phi)
    //sin(theta)*sin(phi)
    ViewDir = float3(
		SinViewZenithAngle * CosLongitudeViewCosAngle,
        CosViewZenithAngle,
		SinViewZenithAngle * SinLongitudeViewCosAngle);
}

float RayleighPhase(float CosTheta)
{
    float Factor = 3.0f / (16.0f * PI);
    return Factor * (1.0f + CosTheta * CosTheta);
}

float HgPhase(float G, float CosTheta)
{
    float Numer = 1.0f - G * G;
    float Denom = 1.0f + G * G + 2.0f * G * CosTheta;
    return Numer / (4.0f * PI * Denom * sqrt(Denom));
}

float3 ComputeLForRenderSkyViewLut(
	in float3 WorldPos, in float3 WorldDir,
	in float SampleCount, 
	in float3 Light0Dir, in float3 Light0Illuminance,in float tMaxMax=9000000.0f)
{
	float t = 0.0f, tMax = 0.0f;
    float3 L = 0.0f;
    float3 Throughput = 1.0f;

    float3 PlanetO = float3(0.0f, 0.0f, 0.0f);
    float tBottom = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_BottomRadiusKm);
    float tTop = RaySphereIntersectNearest(WorldPos, WorldDir, PlanetO, Atmosphere_TopRadiusKm);
	
    if (tBottom < 0.0f)
    {
        if      (tTop < 0.0f)   {   tMax = 0.0f;    return 0;     } // No intersection with planet nor its atmosphere: stop right away  
        else                    {   tMax = tTop;}
    }
    else
    {
        if  (tTop > 0.0f)       {   tMax = min(tTop, tBottom);  }
    }
    tMax = min(tMax, tMaxMax);
	

    // Phase functions
    const float3 wi = Light0Dir;
    const float3 wo = WorldDir;
    float cosTheta = dot(wi, wo);
    float MiePhaseValueLight0 = HgPhase(Atmosphere_MiePhaseG, -cosTheta); // negate cosTheta because due to WorldDir being a "in" direction. 
    float RayleighPhaseValueLight0 = RayleighPhase(cosTheta);

    float dt = tMax / SampleCount;
	for (float SampleI = 0.0f; SampleI < SampleCount; SampleI += 1.0f)
	{
        t = tMax * (SampleI + DEFAULT_SAMPLE_OFFSET) / SampleCount;

		float3 P = WorldPos + t * WorldDir;
        float PHeight = length(P);

		MediumSampleRGB Medium = SampleMediumRGB(P);
		const float3 SampleOpticalDepth = Medium.Extinction * dt;
		const float3 SampleTransmittance = exp(-SampleOpticalDepth);

        float2 UV;
        const float3 UpVector = P / PHeight;
        float Light0ZenithCosAngle = dot(Light0Dir, UpVector);
        LutTransmittanceParamsToUv(PHeight, Light0ZenithCosAngle, UV);
        float3 TransmittanceToLight0 = TransmittanceLutTexture.SampleLevel(gsamLinearClamp, UV, 0).rgb;
        float3 PhaseTimesScattering0 = Medium.ScatteringMie * MiePhaseValueLight0 + Medium.ScatteringRay * RayleighPhaseValueLight0;

        float2 MultiSactterUV = saturate(float2(Light0ZenithCosAngle * 0.5f + 0.5f,
                                (length(P) - Atmosphere_BottomRadiusKm) /
                                (Atmosphere_TopRadiusKm - Atmosphere_BottomRadiusKm)));
	    
        float3 MultiScatteredLuminance0 = MultiScatteredLuminanceLutTexture.SampleLevel(gsamLinearClamp, MultiSactterUV, 0).rgb;

		// Planet shadow
        float tPlanet0 = RaySphereIntersectNearest(P, Light0Dir, PlanetO + PLANET_RADIUS_OFFSET * UpVector, Atmosphere_BottomRadiusKm);
        float PlanetShadow0 = tPlanet0 >= 0.0f ? 0.0f : 1.0f;
        float3 S = Light0Illuminance * 
            (PlanetShadow0 * TransmittanceToLight0 * PhaseTimesScattering0 + 
            MultiScatteredLuminance0 * Medium.Scattering);

		//details in ComputeScattedLightLuminanceAndMultiScatAs1in
		float3 Sint = (S - S * SampleTransmittance) / Medium.Extinction;
		L += Throughput * Sint;		
		Throughput *= SampleTransmittance;
	}

    return L;
}

[numthreads(THREADGROUP_SIZE, THREADGROUP_SIZE, 1)]
void RenderSkyViewLutCS(uint3 ThreadId : SV_DispatchThreadID)
{
    float2 PixPos = float2(ThreadId.xy) + 0.5f;
    float2 UV = PixPos * SkyAtmosphere_SkyViewLutSizeAndInvSize.zw;

    float3 WorldPos = GetCameraPlanetPos();
    float ViewHeight = length(WorldPos);
    WorldPos = float3(0.0, ViewHeight, 0); // transform to local referential

    float3 WorldDir;
    UvToSkyViewLutParams(WorldDir, ViewHeight, UV); // UV to lat/long	

	// For the sky view lut to work, and not be distorted, we need to transform the view and light directions 
	// into a referential with UP being perpendicular to the ground. And with origin at the planet center.
    float3x3 LocalReferencial = (float3x3)cbView_SkyViewLutReferential;
    float3 AtmosphereLightDirection0 = View_AtmosphereLightDirection.xyz;
    //AtmosphereLightDirection0 = mul(LocalReferencial, AtmosphereLightDirection0);
    AtmosphereLightDirection0 = mul_x(AtmosphereLightDirection0,LocalReferencial);

	//if ((Ray is not intersecting the atmosphere)) {}

    
    float3 Light0Illuminance = Atmosphere_Light0Illuminance.xyz;
    float3 OutL = ComputeLForRenderSkyViewLut(
	    WorldPos, WorldDir, 16,
        AtmosphereLightDirection0, Light0Illuminance);
    SkyViewLutUAV[int2(PixPos)] = OutL;

}















float3 GetScreenWorldDir(in float4 SvPosition)
{
    float3 NDCPos = float3((SvPosition.xy * View_ViewSizeAndInvSize.zw - 0.5f) * float2(2, -2), SvPosition.z);
    float2 ScreenPosition = float4(NDCPos.xyz, 1) * SvPosition.w;
    const float Depth = 10000.0f;
    float4 WorldPos = mul_x(float4(ScreenPosition * Depth, Depth, 1),cbView_ScreenToWorld);
    return normalize(WorldPos.xyz - View_WorldCameraOrigin);
}

RWTexture3D<float4> CameraAerialPerspectiveVolumeUAV;

//Compute Form StartDepth To Voxel In ThreadId.xyz
//And RenderSkyViewLutCS Is From Camera Pos To TopRadius
[numthreads(THREADGROUP_SIZE, THREADGROUP_SIZE, THREADGROUP_SIZE)]
void RenderCameraAerialPerspectiveVolumeCS(uint3 ThreadId : SV_DispatchThreadID)
{
    float2 PixPos = float2(ThreadId.xy) + 0.5f;
    float2 UV = PixPos * SkyAtmosphere_CameraAerialPerspectiveVolumeSizeAndInvSize.zw;
   
    float4 SVPos = float4(UV * View_ViewSizeAndInvSize.xy, 0.0f, 1.0f);
    float3 WorldDir = GetScreenWorldDir(SVPos);
    float3 CamPos = GetCameraPlanetPos();

   // CameraAerialPerspectiveVolumeDepthResolution = 16
    float Slice = ((float(ThreadId.z) + 0.5f) * Atmosphere_CameraAerialPerspectiveVolumeDepthResolutionInv); // +0.5 to always have a distance to integrate over
    Slice *= Slice; // squared distribution
    Slice *= Atmosphere_CameraAerialPerspectiveVolumeDepthResolution;
   
    float AerialPerspectiveStartDepthKm = 0.1;
    float3 RayStartWorldPos = CamPos + AerialPerspectiveStartDepthKm * WorldDir; // Offset according to start depth
   
   //CameraAerialPerspectiveVolumeDepthSliceLengthKm = 96Km / 16 = 6Km
    float tMax = Slice * Atmosphere_CameraAerialPerspectiveVolumeDepthSliceLengthKm;
    float3 VoxelWorldPos = RayStartWorldPos + tMax * WorldDir;
    float VoxelHeight = length(VoxelWorldPos);
   
    const float UnderGround = VoxelHeight < Atmosphere_BottomRadiusKm;
   
    float3 CameraToVoxel = VoxelWorldPos - CamPos;
    float CameraToVoxelLen = length(CameraToVoxel);
    float3 CameraToVoxelDir = CameraToVoxel / CameraToVoxelLen;
    float PlanetNearT = RaySphereIntersectNearest(CamPos, CameraToVoxelDir, float3(0, 0, 0), Atmosphere_BottomRadiusKm);
    bool BelowHorizon = PlanetNearT > 0.0f && CameraToVoxelLen > PlanetNearT;
   
    if (BelowHorizon || UnderGround)
    {
        CamPos += normalize(CamPos) * 0.02f;
        VoxelWorldPos = CamPos + PlanetNearT * CameraToVoxelDir;
        tMax = length(VoxelWorldPos - RayStartWorldPos);
    }
   
    float SampleCount = max(1.0f, (float(ThreadId.z) + 1.0f));
    float3 AtmosphereLightDirection0 = View_AtmosphereLightDirection.xyz;
    float3 Light0Illuminance = Atmosphere_Light0Illuminance.xyz;

    float3 OutL = ComputeLForRenderSkyViewLut(
	    RayStartWorldPos, WorldDir, SampleCount,
       AtmosphereLightDirection0, Light0Illuminance, tMax);

    CameraAerialPerspectiveVolumeUAV[ThreadId.xyz] = float4(OutL, 1.0f);
}

float4 PrepareOutput(float3 PreExposedLuminance, float3 Transmittance = float3(1.0f, 1.0f, 1.0f))
{
	// Sky materials can result in high luminance values, e.g. the sun disk. 
	// We also half that range to also make sure we have room for other additive elements such as bloom, clouds or particle visual effects.
    float3 SafePreExposedLuminance = min(PreExposedLuminance, Max10BitsFloat.xxx * 0.5f);

    const float GreyScaleTransmittance = dot(Transmittance, float3(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f));
    return float4(SafePreExposedLuminance, GreyScaleTransmittance);
}

void SkyViewLutParamsToUv(
	in bool IntersectGround, in float ViewZenithCosAngle, in float3 ViewDir, 
    in float ViewHeight, in float BottomRadius, in float4 SkyViewLutSizeAndInvSize,
	out float2 UV)
{
    float Vhorizon = sqrt(ViewHeight * ViewHeight - BottomRadius * BottomRadius);
    float CosBeta = Vhorizon / ViewHeight; // GroundToHorizonCos
    float Beta = acosFast4(CosBeta);
    float ZenithHorizonAngle = PI - Beta;
    float ViewZenithAngle = acosFast4(ViewZenithCosAngle);

    if (!IntersectGround)
    {
        float Coord = ViewZenithAngle / ZenithHorizonAngle;
        Coord = 1.0f - Coord;
        Coord = sqrt(Coord);
        Coord = 1.0f - Coord;
        UV.y = Coord * 0.5f;
    }
    else
    {
        float Coord = (ViewZenithAngle - ZenithHorizonAngle) / Beta;
        Coord = sqrt(Coord);
        UV.y = Coord * 0.5f + 0.5f;
    }

	{
        UV.x = (atan2Fast(-ViewDir.z, -ViewDir.x) + PI) / (2.0f * PI);
    }

	// Constrain uvs to valid sub texel range (avoid zenith derivative issue making LUT usage visible)
    UV = FromUnitToSubUvs(UV, SkyViewLutSizeAndInvSize);
}

//struct VertexIn
//{
//    float2 PosIn : POSITION;
//    float2 TexC : TEXCOORD;
//};
//
//float4 VS(VertexIn vin) : SV_POSITION
//{
//    float4 PosH = float4(vin.PosIn, 0.0f, 1.0f);
//    return PosH;
//}



Texture2D SkyViewLutTexture;
Texture2D SceneTexturesStruct_SceneDepthTexture;
Texture2D TransmittanceLutTexture_Combine;
//Texture3D AerialPerspectiveVolumeTexture_Combine;

float3 GetLightDiskLuminance(float3 WorldPos, float3 WorldDir)
{
    float3 LightDiskLuminance = 0;
    float t = RaySphereIntersectNearest(WorldPos, WorldDir, float3(0.0f, 0.0f, 0.0f), Atmosphere_BottomRadiusKm);
    if (t < 0.0f)
    {
        const float CosHalfApex = 0.9999f;
        const float ViewDotLight = dot(WorldDir, View_AtmosphereLightDirection.xyz);
        if (ViewDotLight > CosHalfApex)
        {
            const float PHeight = length(WorldPos);
            const float3 UpVector = WorldPos / PHeight;
            const float LightZenithCosAngle = dot(WorldDir, UpVector);
            float2 TransmittanceLutUv;
            getTransmittanceLutUvs(PHeight, LightZenithCosAngle, Atmosphere_BottomRadiusKm, Atmosphere_TopRadiusKm, TransmittanceLutUv);
            LightDiskLuminance = TransmittanceLutTexture_Combine.Sample(gsamLinearClamp, TransmittanceLutUv, 0.0f).rgb;
            LightDiskLuminance *= Atmosphere_Light0Illuminance.xyz;
        }
        const float3 MaxLightLuminance = 6400.0f;
        LightDiskLuminance = min(LightDiskLuminance, MaxLightLuminance);
       
        const float HalfCosHalfApex = CosHalfApex + (1.0f - CosHalfApex) * 0.75; // Start fading when at 25% distance from light disk center (in cosine space)

		// Apply smooth fading at edge. This is currently an eye balled fade out that works well in many cases.
        const float Weight = 1.0 - saturate((HalfCosHalfApex - ViewDotLight) / (HalfCosHalfApex - CosHalfApex));
        float LightFade = Weight * Weight;
        LightFade *= LightFade;
        LightDiskLuminance = LightDiskLuminance * LightFade;
    }

    

    return LightDiskLuminance;
}

void RenderSkyAtmosphereRayMarchingPS(
	in float4 SVPos : SV_POSITION,
	out float4 OutLuminance : SV_Target0)
{
    OutLuminance = 0;
    float2 PixPos = SVPos.xy;
    float2 UvBuffer = PixPos * View_BufferSizeAndInvSize.zw;
    float DeviceZ = SceneTexturesStruct_SceneDepthTexture.Sample(gsamPointWarp, UvBuffer).r;
    //float NDCZ = ConvertFromDeviceZ_To_NDCZBeforeDivdeW(DeviceZ);

    float3 WorldPos = GetCameraPlanetPos();
    float3 WorldDir = GetScreenWorldDir(SVPos);
    float ViewHeight = length(WorldPos);

    float3 PreExposedL = 0.0f;
    //GetLightDiskLuminance
    PreExposedL += GetLightDiskLuminance(WorldPos, WorldDir);

    
    if (ViewHeight < (Atmosphere_TopRadiusKm + 0.01) && DeviceZ < 0.0001f)
    {
        float2 UV;
        float3x3 LocalReferencial = (float3x3) cbView_SkyViewLutReferential;
        float3 WorldPosLocal = float3(0.0, ViewHeight, 0.0);
        float3 UpVectorLocal = float3(0.0, 1.0, 0.0);
        float3 WorldDirLocal = mul_x(WorldDir,LocalReferencial);
        float ViewZenithCosAngle = dot(WorldDirLocal, UpVectorLocal);

        bool IntersectGround = RaySphereIntersectNearest(WorldPosLocal, WorldDirLocal, float3(0, 0, 0), Atmosphere_BottomRadiusKm) >= 0.0f;
        SkyViewLutParamsToUv(IntersectGround, ViewZenithCosAngle, WorldDirLocal, ViewHeight, Atmosphere_BottomRadiusKm, SkyAtmosphere_SkyViewLutSizeAndInvSize, UV);
        PreExposedL += SkyViewLutTexture.SampleLevel(gsamLinearClamp, UV, 0).rgb;
        OutLuminance = PrepareOutput(PreExposedL);
        return;
    }

    //{
    //    //float tDepth = max(0.0f, WorldZ - AerialPerspectiveVolumeStartDepth);
    //    float tDepth = max(0.0f, WorldZ - 100); //For test
    //
    //    float LinearSlice = tDepth / Atmosphere_CameraAerialPerspectiveVolumeDepthSliceLengthKm; //TODO Inv
    //    float LinearW = LinearSlice * Atmosphere_CameraAerialPerspectiveVolumeDepthResolutionInv; // Depth slice coordinate in [0,1]
    //    float NonLinW = sqrt(LinearW);
    //    float NonLinSlice = NonLinW * Atmosphere_CameraAerialPerspectiveVolumeDepthResolution;
    //
    //    const float HalfSliceDepth = 0.70710678118654752440084436210485f; // sqrt(0.5f)
    //    float Weight = 1.0f;
    //    if (NonLinSlice < HalfSliceDepth)
    //    {
	//	// We multiply by weight to fade to 0 at depth 0. It works for luminance and opacity.
    //        Weight = saturate(NonLinSlice * NonLinSlice * 2.0f); // Square to have a linear falloff from the change of distribution above
    //    }
    //    float4 AP = AerialPerspectiveVolumeTexture_Combine.Sample(gsamLinearClamp, float3(UvBuffer, NonLinW), 0.0f);
	//    // Lerp to no contribution near the camera (careful as AP contains transmittance)
    //    AP.rgb *= Weight;
    //    AP.a = 1.0 - (Weight * (1.0f - AP.a));
    //
    //    PreExposedL += AP.rgb;
    //    OutLuminance = PrepareOutput(PreExposedL, float3(AP.a, AP.a, AP.a));
    //    return;
    //}

}