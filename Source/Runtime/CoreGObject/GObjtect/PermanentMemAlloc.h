#pragma once
#include "Runtime/HAL/PlatformTypes.h"
class XPermanentMemAlloc
{
public:
	XPermanentMemAlloc() :
		PermanentObjectPoolSize(0),
		PermanentObjectPool(nullptr),
		OriginPtr(nullptr) {}

	void AllocatePermanentMemPool(int32 InPermanentObjectPoolSize);
	void* AllocateMem(int32 Size);

	int32 PermanentObjectPoolSize;
	uint8* PermanentObjectPool;
	uint8* OriginPtr;
};
extern XPermanentMemAlloc GlobalGObjectPermanentMemAlloc;