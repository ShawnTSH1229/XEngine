#include "Common.hlsl"
#include "FastMath.hlsl"
#include "Math.hlsl"

#define CM_TO_SKY_UNIT 0.00001f
#define DEFAULT_SAMPLE_OFFSET 0.3f

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
};

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