#include "RayTracingInstanceBufferUtil.h"
#include "Runtime\RHI\RHICommandList.h"
#if RHI_RAYTRACING
XRayTracingSceneWithGeometryInstance CreateRayTracingSceneWithGeometryInstance(std::vector<XRayTracingGeometryInstance>& Instances, uint32 NumShaderSlotsPerGeometrySegment, uint32 NumMissShaderSlots, uint32 NumCallableShaderSlots)
{
	const uint32 NumSceneInstances = Instances.size();

	XRayTracingSceneWithGeometryInstance Output;
	XRayTracingSceneInitializer Initializer;
	//Initializer.NumNativeInstance = ;
	
	for (uint32 Index = 0; Index < NumSceneInstances; Index++)
	{
		const XRayTracingGeometryInstance& InstanceDesc = Instances[Index];
		Initializer.NumTotalSegments += InstanceDesc.GeometryRHI->GetNumSegments();
		Initializer.NumNativeInstance += InstanceDesc.NumTransforms;
		Initializer.NumMissShaderSlots = NumMissShaderSlots;
		Initializer.SegmentPrefixSum.push_back(Initializer.NumTotalSegments);
	}

	Output.Scene = RHICreateRayTracingScene(Initializer);
	return Output;
}
#endif