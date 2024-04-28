#include "Material.h"

void GMaterialInstance::SetMaterialValueFloat(const std::string& ValueName, float Value)
{
	for (auto iter = MaterialFloatArray.begin(); iter != MaterialFloatArray.end(); iter++)
	{
		if (iter->Name == ValueName)
		{
			bValueChanged = true;
			iter->Value.resize(1);
			iter->Value[0] = Value;
			return;
		}
	}
	XASSERT(false);
}

void GMaterialInstance::SetMaterialValueFloat2(const std::string& ValueName, XVector2 Value)
{
	for (auto iter = MaterialFloatArray.begin(); iter != MaterialFloatArray.end(); iter++)
	{
		if (iter->Name == ValueName)
		{
			bValueChanged = true;
			iter->Value.resize(2);
			iter->Value[0] = Value.x;
			iter->Value[1] = Value.y;
			return;
		}
	}
	XASSERT(false);
}

void GMaterialInstance::SetMaterialValueFloat3(const std::string& ValueName, XVector3 Value)
{
	for (auto iter = MaterialFloatArray.begin(); iter != MaterialFloatArray.end(); iter++)
	{
		if (iter->Name == ValueName)
		{
			bValueChanged = true;
			iter->Value.resize(3);
			iter->Value[0] = Value.x;
			iter->Value[1] = Value.y;
			iter->Value[2] = Value.z;
			return;
		}
	}
	XASSERT(false);
}

void GMaterialInstance::SetMaterialValueFloat4(const std::string& ValueName, XVector4 Value)
{
	for (auto iter = MaterialFloatArray.begin(); iter != MaterialFloatArray.end(); iter++)
	{
		if (iter->Name == ValueName)
		{
			bValueChanged = true;
			iter->Value.resize(4);
			iter->Value[0] = Value.x;
			iter->Value[1] = Value.y;
			iter->Value[2] = Value.z;
			iter->Value[3] = Value.w;
			return;
		}
	}
	XASSERT(false);
}

void GMaterialInstance::SetMaterialTexture2D(const std::string& TexName, std::shared_ptr<GTexture2D> TexPtrIn)
{
	for (auto iter = MaterialTextureArray.begin(); iter != MaterialTextureArray.end(); iter++)
	{
		if (iter->Name == TexName)
		{
			bValueChanged = true;
			iter->TexturePtr = TexPtrIn;
			return;
		}
	}
	XASSERT(false);
}

std::shared_ptr<XRHIConstantBuffer> GMaterialInstance::GetRHIConstantBuffer()
{
	if (MaterialRHIConstantBuffer.get() == nullptr)
	{
		MaterialRHIConstantBuffer = RHICreateConstantBuffer(256);
	}

	if (bValueChanged)
	{
		for (auto iter = MaterialFloatArray.begin(); iter != MaterialFloatArray.end(); iter++)
		{
			if (iter->Value.size() != 0)
			{
				MaterialRHIConstantBuffer->UpdateData(iter->Value.data(), iter->SizeInByte, iter->VariableOffsetInBuffer);
			}
		}
		bValueChanged = false;
	}
	return MaterialRHIConstantBuffer;
}



