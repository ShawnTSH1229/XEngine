#include "Runtime/Engine/Classes/Texture.h"
#include "Runtime/Core/MainInit.h"

XReflectionInfo GTexture::StaticReflectionInfo("Texture", &GObject::StaticReflectionInfo);

//Begin Add Property
void GTexture::InitialReflectionInfo(XReflectionInfo* InfoPtr)
{
	
	GTexture* NullPtr = nullptr;
	XProperty* PropertyTemp = nullptr;
	if (InfoPtr == nullptr)
	{
		InfoPtr = &GTexture::StaticReflectionInfo;
	}

	GObject::InitialReflectionInfo(InfoPtr);
	

//Add Property
	PropertyTemp = PropertyCreator::GetAutoPropertyCreator(NullPtr->SizeX).
		CreateProperty(InfoPtr,"SizeX", (unsigned int)((char*)&(NullPtr->SizeX) - (char*)NullPtr));
	InfoPtr->AddProperty(PropertyTemp);

	PropertyTemp = PropertyCreator::GetAutoPropertyCreator(NullPtr->SizeY).
		CreateProperty(InfoPtr, "SizeY", (unsigned int)((char*)&(NullPtr->SizeY) - (char*)NullPtr));
	InfoPtr->AddProperty(PropertyTemp);

	PropertyTemp = PropertyCreator::GetAutoPropertyCreator(NullPtr->Channel).
		CreateProperty(InfoPtr, "Channel", (unsigned int)((char*)&(NullPtr->Channel) - (char*)NullPtr));
	InfoPtr->AddProperty(PropertyTemp);

//End Add Property
}


//Impl Initial Begin
bool GTexture::HasReisterClass = false;
static bool TempValue = GTexture::PushInitFunToGloablFunArray();
bool GTexture::PushInitFunToGloablFunArray()
{
	if (!HasReisterClass)
	{
		MainInit::PushToInitPropertyFunArray(&GTexture::InitialReflectionInfo);
		HasReisterClass = true;
	}
	return false;
}



//Impl Initial End