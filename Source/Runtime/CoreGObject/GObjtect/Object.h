#pragma once
#include "Reflection.h"
#include "Runtime/Core/Serialization/Archive.h"
class GObject
{

//#define Begin
public:
	static void InitialReflectionInfo(XReflectionInfo* InfoPtr);
	static XReflectionInfo StaticReflectionInfo;
//#define End

	virtual void ArchiveImpl(XArchiveBase& Archvice)
	{

	}
};