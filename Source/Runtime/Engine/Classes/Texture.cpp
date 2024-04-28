#include "Texture.h"

//#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#endif // !STB_IMAGE_IMPLEMENTATION

#include "ThirdParty/stb_image.h"

void GTexture::LoadTextureFromImage(const char* Name)
{
	unsigned char* ColorData = stbi_load(Name, &SizeX, &SizeY, &Channel, 0);
	if (Channel != 4) { XASSERT(false); }
	TextureDataArray.insert(TextureDataArray.end(), ColorData, ColorData + SizeX * SizeY * Channel);
	stbi_image_free(ColorData);
}

GTexture::GTexture(uint8* TextureDataArrayIn, int32 SizeXIn, int32 SizeYIn, int32 ChannelIn, bool TextureSRGBIn)
{
	SizeX = SizeXIn;
	SizeY = SizeYIn;
	Channel = ChannelIn;
	TextureSRGB = TextureSRGBIn;
	TextureDataArray.insert(TextureDataArray.end(), TextureDataArrayIn, TextureDataArrayIn + SizeX * SizeY * Channel);
}