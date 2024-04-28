#include "Runtime/CoreGObject/GObjtect/Object.h"

XReflectionInfo GObject::StaticReflectionInfo("GObject", nullptr);

void GObject::InitialReflectionInfo(XReflectionInfo* InfoPtr)
{
	if (InfoPtr == nullptr)
	{
		InfoPtr = &GObject::StaticReflectionInfo;
	}
}