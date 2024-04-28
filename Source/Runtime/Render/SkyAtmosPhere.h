#pragma once
#include "Runtime/Core/Math/Math.h"

struct XSkyAtmosphereParams
{
	XVector4 TransmittanceLutSizeAndInvSize;
	XVector4 MultiScatteredLuminanceLutSizeAndInvSize;
	XVector4 SkyViewLutSizeAndInvSize;
	XVector4 CameraAerialPerspectiveVolumeSizeAndInvSize;

	XVector4 RayleighScattering;
	XVector4 MieScattering;
	XVector4 MieAbsorption;
	XVector4 MieExtinction;

	XVector4 GroundAlbedo;

	float TopRadiusKm;
	float BottomRadiusKm;
	float MieDensityExpScale;
	float RayleighDensityExpScale;

	float TransmittanceSampleCount;
	float MultiScatteringSampleCount;
	float MiePhaseG;
	float padding0 = 0;

	XVector4 Light0Illuminance;

	float CameraAerialPerspectiveVolumeDepthResolution;
	float CameraAerialPerspectiveVolumeDepthResolutionInv;
	float CameraAerialPerspectiveVolumeDepthSliceLengthKm;
	float padding1;
};