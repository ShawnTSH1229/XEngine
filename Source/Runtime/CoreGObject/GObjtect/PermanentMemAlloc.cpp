#include "Runtime/Core/Template/AlignmentTemplate.h"
#include "Runtime/HAL/Mch.h"
#include "PermanentMemAlloc.h"
#include <memory>
XPermanentMemAlloc GlobalGObjectPermanentMemAlloc;

void XPermanentMemAlloc::AllocatePermanentMemPool(int32 InPermanentObjectPoolSize)
{
	PermanentObjectPoolSize = InPermanentObjectPoolSize;

	CRT_HEAP_TRACK_OFF_BEGIN;
	PermanentObjectPool = (uint8*)std::malloc(InPermanentObjectPoolSize);
	CRT_HEAP_TRACK_OFF_END;

	OriginPtr = PermanentObjectPool;
}

void* XPermanentMemAlloc::AllocateMem(int32 Size)
{
	PermanentObjectPool = Align(PermanentObjectPool, 16) + Size;
	void* Result = (void*)PermanentObjectPool;

	XASSERT(PermanentObjectPool < OriginPtr + PermanentObjectPoolSize);
	return Result;
}