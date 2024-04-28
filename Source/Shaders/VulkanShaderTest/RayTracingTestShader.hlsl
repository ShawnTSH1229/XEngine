#define RAY_TRACING_MASK_OPAQUE						0x01    // Opaque and alpha tested meshes and particles (e.g. used by reflection, shadow, AO and GI tracing passes)

#define RAY_TRACING_SHADER_SLOT_MATERIAL	0
#define RAY_TRACING_SHADER_SLOT_SHADOW		1
#define RAY_TRACING_NUM_SHADER_SLOTS		2

struct FBasicRayData
{
	float3 Origin;
	uint Mask;
	float3 Direction;
	float TFar;
};

struct FMinimalPayload
{
	float HitT; // Distance from ray origin to the intersection point in the ray direction. Negative on miss.

	bool IsMiss() { return HitT < 0; }
	bool IsHit() { return !IsMiss(); }

	void SetMiss() { HitT = -1; }
};

struct FIntersectionPayload : FMinimalPayload
{
	uint   PrimitiveIndex; // Index of the primitive within the geometry inside the bottom-level acceleration structure instance. Undefined on miss.
	uint   InstanceIndex;  // Index of the current instance in the top-level structure. Undefined on miss.
	float2 Barycentrics;   // Primitive barycentric coordinates of the intersection point. Undefined on miss.
};

struct FDefaultPayload : FIntersectionPayload
{
	uint   InstanceID; // Value of FRayTracingGeometryInstance::UserData. Undefined on miss.
};

struct FRayTracingIntersectionAttributes
{
	uint2 PackedData;

	float2 GetBarycentrics()
	{
		return asfloat(PackedData);
	}

	void SetBarycentrics(float2 Barycentrics)
	{
		PackedData = asuint(Barycentrics);
	}
};

RaytracingAccelerationStructure               TLAS;
StructuredBuffer<FBasicRayData>               Rays;
RWStructuredBuffer<uint>                      OcclusionOutput;

[shader("raygeneration")] 
void RayGenMain()
{
	const uint RayIndex = DispatchRaysIndex().x;
	FBasicRayData InputRay = Rays[RayIndex];

	RayDesc Ray;
	Ray.Origin = InputRay.Origin;
	Ray.Direction = InputRay.Direction;
	Ray.TMin = 0.0f;
	Ray.TMax = InputRay.TFar;

	uint RayFlags = 0
	              | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH // terminate immediately upon detecting primitive intersection
	              | RAY_FLAG_FORCE_OPAQUE                    // don't run anyhit shader
	              | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;        // don't run closesthit shader
	const uint InstanceInclusionMask = RAY_TRACING_MASK_OPAQUE;

	FDefaultPayload Payload = (FDefaultPayload)0;

	TraceRay(
		TLAS,   // AccelerationStructure
		RayFlags,
		InstanceInclusionMask,
		RAY_TRACING_SHADER_SLOT_MATERIAL, // RayContributionToHitGroupIndex
		RAY_TRACING_NUM_SHADER_SLOTS,     // MultiplierForGeometryContributionToShaderIndex
		0,      // MissShaderIndex
		Ray,    // RayDesc
		Payload // Payload
	);

	// Workaround for DXC bug creating validation error
	//OcclusionOutput[RayIndex] = Payload.IsHit() ? ~0 : 0;
	OcclusionOutput[RayIndex] = ((FMinimalPayload)Payload).IsHit() ? ~0 : 0;
}

[shader("closesthit")] 
void ClostHitMain(
	inout FDefaultPayload Payload,
	in FRayTracingIntersectionAttributes Attributes)
{
	Payload.HitT = RayTCurrent();
	Payload.Barycentrics = Attributes.GetBarycentrics();
	Payload.InstanceIndex = InstanceIndex();
	Payload.PrimitiveIndex = PrimitiveIndex();
}

[shader("miss")] 
void MissMain(inout FDefaultPayload Payload)
{
	Payload.SetMiss();
}