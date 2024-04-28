#pragma once
#include "Runtime/CoreGObject/GObjtect/Object.h"
#include "Runtime/RHI/RHIResource.h"
#include "Runtime/RHI/RHICommandList.h"

#include <memory>
#include <vector>





class GTexture :public GObject
{
	//Declare Reflection Begin
public:
	static void InitialReflectionInfo(XReflectionInfo* InfoPtr);
	static XReflectionInfo StaticReflectionInfo;

	using SuperClass = GObject;
	//Declare Reflection Begin

	//Declare Initial Begin
	static bool HasReisterClass;
	static bool PushInitFunToGloablFunArray();
private:
	//Declare Initial End

public:
	GTexture() {}
	GTexture(uint8* TextureDataArrayIn, int32 SizeXIn, int32 SizeYIn, int32 ChannelIn, bool TextureSRGBIn);

	virtual void ArchiveImpl(XArchiveBase& Archvice)
	{
		SuperClass::ArchiveImpl(Archvice);
		Archvice.ArchiveFun(SizeX);
		Archvice.ArchiveFun(SizeY);
		Archvice.ArchiveFun(Channel);
		Archvice.ArchiveFun(TextureDataArray);
	}

	virtual void CreateRHITexture()
	{
	
	}

	//deprecated
	void LoadTextureFromImage(const char* Name);

	

	int32 SizeX;
	int32 SizeY;
	int32 Channel;

	bool TextureSRGB;
protected:
	
	std::vector<uint8>TextureDataArray;//Temp Data Array , NO MipMap Now
};

class GTexture2D :public GTexture
{
public:
	GTexture2D() {}
	GTexture2D(uint8* TextureDataArrayIn, int32 SizeXIn, int32 SizeYIn, int32 ChannelIn, bool TextureSRGBIn)
		:GTexture(TextureDataArrayIn, SizeXIn, SizeYIn, ChannelIn, TextureSRGBIn) {}
	virtual void CreateRHITexture()override
	{
		if (TextureSRGB)
		{
			RHITexturePrivate = RHICreateTexture2D(SizeX, SizeY, 1, false, false, EPixelFormat::FT_R8G8B8A8_UNORM_SRGB
				, ETextureCreateFlags(TexCreate_SRGB), 1, TextureDataArray.data());
		}
		else
		{
			RHITexturePrivate = RHICreateTexture2D(SizeX, SizeY, 1, false, false, EPixelFormat::FT_R8G8B8A8_UNORM
				, ETextureCreateFlags(TexCreate_None), 1, TextureDataArray.data());
		}
	}

	inline std::shared_ptr<XRHITexture2D> GetRHITexture2D()
	{
		if (RHITexturePrivate.get() == nullptr)
		{
			CreateRHITexture();
		}
		return RHITexturePrivate;
	}
private:
	std::shared_ptr<XRHITexture2D>RHITexturePrivate;
};