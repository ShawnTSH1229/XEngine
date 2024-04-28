#include "Reflection.h"

std::vector<XReflectionInfo*>& GetReflectionInfoArray()
{
	static std::vector<XReflectionInfo*> ReflectionInfoArray;
	return ReflectionInfoArray;
}


XReflectionInfo::XReflectionInfo(const std::string& ReflectInfoNameIn, XReflectionInfo* Super)
{
	ReflectInfoName = ReflectInfoNameIn;
	SurperPtr = Super;
	GetReflectionInfoArray().push_back(this);
}