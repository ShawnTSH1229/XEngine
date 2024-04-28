#pragma once
#include "Texture.h"
#include "Runtime/Core/Math/Math.h"


struct MaterialValueParas
{
	std::string Name;
	std::vector<float>Value;
	unsigned int BufferIndex;
	unsigned int VariableOffsetInBuffer;
	unsigned int SizeInByte;
};

struct MaterialTexParas
{
	std::string Name;
	unsigned int ResourceIndex;
	std::shared_ptr<GTexture2D>TexturePtr;
};

class RMaterial;
class GMaterial :public GObject
{
public:
	std::vector<MaterialValueParas>MaterialValueArray;
	std::vector<MaterialTexParas>MaterialTextureArray;
	std::shared_ptr<RMaterial> RMaterialPtr;
	std::wstring CodePath;
};

//attach to mesh
class GMaterialInstance :public GObject
{
public:
	GMaterialInstance(std::shared_ptr<GMaterial>MaterialPtrIn)
	{
		bValueChanged = true;
		MaterialPtr = MaterialPtrIn;
		MaterialFloatArray = MaterialPtrIn->MaterialValueArray;
		MaterialTextureArray = MaterialPtrIn->MaterialTextureArray;
	}

	std::shared_ptr<XRHIConstantBuffer> GetRHIConstantBuffer();
	inline std::shared_ptr<GTexture2D>& TempGetGTexPtrByIndex(uint32 Index)
	{
		for (auto iter = MaterialTextureArray.begin(); iter != MaterialTextureArray.end(); iter++)
		{
			if (iter->ResourceIndex == Index)
			{
				return iter->TexturePtr;
			}
		}
	}

	void SetMaterialValueFloat(const std::string& ValueName, float Value);
	void SetMaterialValueFloat2(const std::string& ValueName, XVector2 Value);
	void SetMaterialValueFloat3(const std::string& ValueName, XVector3 Value);
	void SetMaterialValueFloat4(const std::string& ValueName, XVector4 Value);
	void SetMaterialTexture2D(const std::string& TexName, std::shared_ptr<GTexture2D> TexPtrIn);

	std::vector<MaterialValueParas>MaterialFloatArray;
	std::vector<MaterialTexParas>MaterialTextureArray;
	std::shared_ptr<GMaterial>MaterialPtr;
private:
	bool bValueChanged;
	std::shared_ptr<XRHIConstantBuffer> MaterialRHIConstantBuffer;
};