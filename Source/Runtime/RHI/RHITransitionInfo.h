#pragma once
#include "RHIResource.h"


enum class ERHIAccess : uint32
{
	Unknown = 0,
	BVHRead = 1 << 14,
	BVHWrite = 1 << 15,
};

struct XRHITransitionInfo
{
	union
	{
		XRHIRayTracingAccelerationStruct* BVH;
	};

	enum class EType :uint16
	{
		Unknown,
		BVH
	};
	EType Type = EType::Unknown;
	ERHIAccess AccessBefore = ERHIAccess::Unknown;
	ERHIAccess AccessAfter = ERHIAccess::Unknown;

	XRHITransitionInfo(
		XRHIRayTracingAccelerationStruct* InBVH,
		EType InType,
		ERHIAccess InAccessBefore,
		ERHIAccess InAccessAfter)
		:BVH(InBVH),
		Type(InType),
		AccessBefore(InAccessBefore),
		AccessAfter(InAccessAfter)
	{}
};