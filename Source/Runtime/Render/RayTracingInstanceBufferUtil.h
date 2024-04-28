#pragma once
#include "Runtime\RHI\RHIResource.h"
#if RHI_RAYTRACING
struct XRayTracingSceneWithGeometryInstance
{
	std::shared_ptr<XRHIRayTracingScene> Scene;
	
};

XRayTracingSceneWithGeometryInstance CreateRayTracingSceneWithGeometryInstance(
	std::vector<XRayTracingGeometryInstance>& Instances,
	uint32 NumShaderSlotsPerGeometrySegment,
	uint32 NumMissShaderSlots,
	uint32 NumCallableShaderSlots = 0
);
#endif
