#include "Runtime/Core/Math/Math.h"
#include "ResourcecConverter.h"
#include "ThirdParty/stb_image.h"
#include <d3dcompiler.h>
#include "Runtime/Engine/Material/MaterialShared.h"
#include "Runtime/ResourceManager/ResourceManager.h"
#include "Runtime/Core/Misc/Path.h"
std::shared_ptr<GTexture2D> CreateTextureFromImageFile(const std::string& FilePath, bool bSRGB)
{
	int SizeX, SizeY, Channel;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* ColorData = stbi_load(FilePath.c_str(), &SizeX, &SizeY, &Channel, 0);
	
	if (Channel == 3)
	{
		int TexSize = SizeX * SizeY * 4;
		unsigned char* FourChannelData = new unsigned char[TexSize];
		for (int i = 0, k = 0; i < TexSize; i += 4, k += 3)
		{
			FourChannelData[i + 0] = ColorData[k + 0];
			FourChannelData[i + 1] = ColorData[k + 1];
			FourChannelData[i + 2] = ColorData[k + 2];
			FourChannelData[i + 3] = 0b11111111;
		}

		std::shared_ptr<GTexture2D> Result = std::make_shared<GTexture2D>(FourChannelData, SizeX, SizeY, 4, bSRGB);
		stbi_image_free(ColorData);
		delete FourChannelData;
		return Result;
	}
	else if (Channel == 4)
	{
		std::shared_ptr<GTexture2D> Result = std::make_shared<GTexture2D>(ColorData, SizeX, SizeY, 4, bSRGB);
		stbi_image_free(ColorData);
		return Result;
	}
	XASSERT(false);
}

std::shared_ptr<GMaterial> CreateMaterialFromCode(const std::wstring& CodePathIn)
{
	std::shared_ptr<GMaterial> MaterialResult = std::make_shared<GMaterial>();
	MaterialResult->CodePath = CodePathIn;
	
	std::shared_ptr<RMaterial> RMaterialPtr = std::make_shared<RMaterial>();
	AddRMaterialToResourceManager(RMaterialPtr);

	MaterialResult->RMaterialPtr = RMaterialPtr;
	MaterialResult->RMaterialPtr->SetCodePath(CodePathIn);

	XDxRefCount<ID3DBlob> BeforeCompressed;
	{
#if defined(DEBUG) || defined(_DEBUG)  
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		XDxRefCount<ID3DBlob> Errors;
		std::string Target = "ps_5_1";
		HRESULT hr = D3DCompileFromFile(
			CodePathIn.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			"Empty_PS", Target.c_str(), compileFlags, 0, &BeforeCompressed, &Errors);

		if (Errors != nullptr)OutputDebugStringA((char*)Errors->GetBufferPointer()); ThrowIfFailed(hr);
	}

	{
		ID3D12ShaderReflection* Reflection = NULL;
		D3DReflect(BeforeCompressed->GetBufferPointer(), BeforeCompressed->GetBufferSize(), IID_PPV_ARGS(&Reflection));

		D3D12_SHADER_DESC ShaderDesc;
		Reflection->GetDesc(&ShaderDesc);
		for (uint32 i = 0; i < ShaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC  ResourceDesc;
			Reflection->GetResourceBindingDesc(i, &ResourceDesc);
			D3D_SHADER_INPUT_TYPE ResourceType = ResourceDesc.Type;

			if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE)
			{
				MaterialResult->MaterialTextureArray.push_back(MaterialTexParas{ ResourceDesc.Name ,ResourceDesc.BindPoint,nullptr });
			}
			else if (ResourceType == D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER)
			{
				ID3D12ShaderReflectionConstantBuffer* ConstantBuffer = Reflection->GetConstantBufferByName(ResourceDesc.Name);
				D3D12_SHADER_BUFFER_DESC CBDesc;
				ConstantBuffer->GetDesc(&CBDesc);
				for (uint32 ConstantIndex = 0; ConstantIndex < CBDesc.Variables; ConstantIndex++)
				{
					ID3D12ShaderReflectionVariable* Variable = ConstantBuffer->GetVariableByIndex(ConstantIndex);
					D3D12_SHADER_VARIABLE_DESC VariableDesc;
					Variable->GetDesc(&VariableDesc);
					MaterialResult->MaterialValueArray.push_back(MaterialValueParas{ 
						VariableDesc.Name ,
						{},
						ResourceDesc.BindPoint,
						//3,
						VariableDesc.StartOffset,
						VariableDesc.Size });
				}
			}
		}
	}
	return MaterialResult;
}

std::shared_ptr<GGeomertry> CreateDefualCubeGeo()
{
	float Width = 1.0 * 0.5;
	float Height = 1.0 * 0.5;
	float Depth = 1.0 * 0.5;

	std::vector<XVector4>Positions;
	std::vector<XVector3>TangentX;
	std::vector<XVector4>TangentY;
	std::vector<uint16>Indices;
	std::vector<XVector2>TextureCoords;

	uint16 IndexOffset = 0;

	//top face
	{
		Positions.push_back(XVector4(-Width, Height, Depth, 1.0));
		Positions.push_back(XVector4(Width, Height, Depth, 1.0));
		Positions.push_back(XVector4(-Width, Height, -Depth, 1.0));
		Positions.push_back(XVector4(Width, Height, -Depth, 1.0));

		TextureCoords.push_back(XVector2(0, 1));
		TextureCoords.push_back(XVector2(1, 1));
		TextureCoords.push_back(XVector2(0, 0));
		TextureCoords.push_back(XVector2(1, 0));

		TangentX.push_back(XVector3(1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(1.0, 0.0, 0.0));

		TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));
		TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));
		TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));
		TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));

		Indices.push_back(0 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(2 + IndexOffset);

		Indices.push_back(2 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(3 + IndexOffset);
		IndexOffset += 4;
	}

	//bottom face
	{
		Positions.push_back(XVector4(Width, -Height, -Depth, 1.0));
		Positions.push_back(XVector4(-Width, -Height,-Depth, 1.0));
		Positions.push_back(XVector4(-Width, -Height, Depth, 1.0));
		Positions.push_back(XVector4(Width, -Height, Depth, 1.0));

		TextureCoords.push_back(XVector2(0, 1));
		TextureCoords.push_back(XVector2(1, 1));
		TextureCoords.push_back(XVector2(0, 0));
		TextureCoords.push_back(XVector2(1, 0));

		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));

		TangentY.push_back(XVector4(0.0, -1.0, 0.0, 1.0));
		TangentY.push_back(XVector4(0.0, -1.0, 0.0, 1.0));
		TangentY.push_back(XVector4(0.0, -1.0, 0.0, 1.0));
		TangentY.push_back(XVector4(0.0, -1.0, 0.0, 1.0));

		Indices.push_back(0 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(2 + IndexOffset);

		Indices.push_back(2 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(3 + IndexOffset);
		IndexOffset += 4;
	}

	//right face
	{
		Positions.push_back(XVector4(Width, Height, -Depth, 1.0));
		Positions.push_back(XVector4(Width, Height, Depth, 1.0));
		Positions.push_back(XVector4(Width, -Height,-Depth, 1.0));
		Positions.push_back(XVector4(Width, -Height, Depth, 1.0));

		TextureCoords.push_back(XVector2(0, 1));
		TextureCoords.push_back(XVector2(1, 1));
		TextureCoords.push_back(XVector2(0, 0));
		TextureCoords.push_back(XVector2(1, 0));

		TangentX.push_back(XVector3(0.0, 0.0, 1.0));
		TangentX.push_back(XVector3(0.0, 0.0, 1.0));
		TangentX.push_back(XVector3(0.0, 0.0, 1.0));
		TangentX.push_back(XVector3(0.0, 0.0, 1.0));

		TangentY.push_back(XVector4(1.0, 0.0, 0.0, 1.0));
		TangentY.push_back(XVector4(1.0, 0.0, 0.0, 1.0));
		TangentY.push_back(XVector4(1.0, 0.0, 0.0, 1.0));
		TangentY.push_back(XVector4(1.0, 0.0, 0.0, 1.0));

		Indices.push_back(0 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(2 + IndexOffset);

		Indices.push_back(2 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(3 + IndexOffset);
		IndexOffset += 4;
	}

	//left face
	{
		Positions.push_back(XVector4(-Width, Height, Depth, 1.0));
		Positions.push_back(XVector4(-Width, Height, -Depth, 1.0));
		Positions.push_back(XVector4(-Width, -Height, Depth, 1.0));
		Positions.push_back(XVector4(-Width, -Height, -Depth, 1.0));

		TextureCoords.push_back(XVector2(0, 1));
		TextureCoords.push_back(XVector2(1, 1));
		TextureCoords.push_back(XVector2(0, 0));
		TextureCoords.push_back(XVector2(1, 0));

		TangentX.push_back(XVector3(0.0, 0.0, -1.0));
		TangentX.push_back(XVector3(0.0, 0.0, -1.0));
		TangentX.push_back(XVector3(0.0, 0.0, -1.0));
		TangentX.push_back(XVector3(0.0, 0.0, -1.0));

		TangentY.push_back(XVector4(-1.0, 0.0, 0.0, 1.0));
		TangentY.push_back(XVector4(-1.0, 0.0, 0.0, 1.0));
		TangentY.push_back(XVector4(-1.0, 0.0, 0.0, 1.0));
		TangentY.push_back(XVector4(-1.0, 0.0, 0.0, 1.0));

		Indices.push_back(0 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(2 + IndexOffset);

		Indices.push_back(2 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(3 + IndexOffset);
		IndexOffset += 4;
	}

	//front face
	{
		Positions.push_back(XVector4(-Width, Height, -Depth, 1.0));
		Positions.push_back(XVector4(Width, Height, -Depth, 1.0));
		Positions.push_back(XVector4(-Width, -Height, -Depth, 1.0));
		Positions.push_back(XVector4(Width, -Height, -Depth, 1.0));

		TextureCoords.push_back(XVector2(0, 1));
		TextureCoords.push_back(XVector2(1, 1));
		TextureCoords.push_back(XVector2(0, 0));
		TextureCoords.push_back(XVector2(1, 0));

		TangentX.push_back(XVector3(1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(1.0, 0.0, 0.0));

		TangentY.push_back(XVector4(0.0, 0.0, -1.0, 1.0));
		TangentY.push_back(XVector4(0.0, 0.0, -1.0, 1.0));
		TangentY.push_back(XVector4(0.0, 0.0, -1.0, 1.0));
		TangentY.push_back(XVector4(0.0, 0.0, -1.0, 1.0));

		Indices.push_back(0 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(2 + IndexOffset);

		Indices.push_back(2 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(3 + IndexOffset);
		IndexOffset += 4;
	}

	//back face
	{
		Positions.push_back(XVector4(Width, Height, Depth, 1.0));
		Positions.push_back(XVector4(-Width, Height, Depth, 1.0));
		Positions.push_back(XVector4(-Width, -Height, Depth, 1.0));
		Positions.push_back(XVector4(Width, -Height, Depth, 1.0));

		TextureCoords.push_back(XVector2(0, 1));
		TextureCoords.push_back(XVector2(1, 1));
		TextureCoords.push_back(XVector2(0, 0));
		TextureCoords.push_back(XVector2(1, 0));

		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));
		TangentX.push_back(XVector3(-1.0, 0.0, 0.0));

		TangentY.push_back(XVector4(0.0, 0.0, 1.0, 1.0));
		TangentY.push_back(XVector4(0.0, 0.0, 1.0, 1.0));
		TangentY.push_back(XVector4(0.0, 0.0, 1.0, 1.0));
		TangentY.push_back(XVector4(0.0, 0.0, 1.0, 1.0));

		Indices.push_back(0 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(2 + IndexOffset);

		Indices.push_back(2 + IndexOffset);
		Indices.push_back(1 + IndexOffset);
		Indices.push_back(3 + IndexOffset);
		IndexOffset += 4;
	}

	std::shared_ptr<GDataBuffer> PositionDataBuffer = std::make_shared<GDataBuffer>();
	PositionDataBuffer->SetData((uint8*)Positions.data(), Positions.size(), EVertexElementType::VET_Float4);

	std::shared_ptr<GDataBuffer> TangentXDataBuffer = std::make_shared<GDataBuffer>();
	TangentXDataBuffer->SetData((uint8*)TangentX.data(), TangentX.size(), EVertexElementType::VET_Float3);

	std::shared_ptr<GDataBuffer> TangentYDataBuffer = std::make_shared<GDataBuffer>();
	TangentYDataBuffer->SetData((uint8*)TangentY.data(), TangentY.size(), EVertexElementType::VET_Float4);

	std::shared_ptr<GDataBuffer> TextureCoordsDataBuffer = std::make_shared<GDataBuffer>();
	TextureCoordsDataBuffer->SetData((uint8*)TextureCoords.data(), TextureCoords.size(), EVertexElementType::VET_Float2);

	std::shared_ptr<GDataBuffer> IndexDataBuffer = std::make_shared<GDataBuffer>();
	IndexDataBuffer->SetData((uint8*)Indices.data(), Indices.size(), EVertexElementType::VET_UINT16);

	std::shared_ptr<GVertexBuffer>VertexBuffer = std::make_shared<GVertexBuffer>();
	VertexBuffer->SetData(PositionDataBuffer, EVertexAttributeType::VAT_POSITION);
	VertexBuffer->SetData(TangentXDataBuffer, EVertexAttributeType::VAT_TANGENT);
	VertexBuffer->SetData(TangentYDataBuffer, EVertexAttributeType::VAT_NORMAL);
	VertexBuffer->SetData(TextureCoordsDataBuffer, EVertexAttributeType::VAT_TEXCOORD);

	std::shared_ptr<GIndexBuffer> IndexBuffer = std::make_shared<GIndexBuffer>();
	IndexBuffer->SetData(IndexDataBuffer);

	std::shared_ptr<GMeshData>MeshData = std::make_shared<GMeshData>();
	MeshData->SetVertexBuffer(VertexBuffer);
	MeshData->SetIndexBuffer(IndexBuffer);

	std::shared_ptr<GGeomertry>Geomertry = std::make_shared<GGeomertry>();
	Geomertry->SetMeshData(MeshData);

	Geomertry->SetBoundingBox(XVector3(0, 0, 0), XVector3(Width, Height, Depth));
	return Geomertry;
}

std::shared_ptr<GGeomertry> CreateDefualtSphereGeo(float Radius, uint32 SliceCount, uint32 StackCount)
{
	std::vector<XVector4>Positions;
	std::vector<XVector3>TangentXs;
	std::vector<XVector4>TangentYs;
	std::vector<XVector2>TextureCoords;
	std::vector<uint16>Indices;


	XVector3 BoundingBoxMaxVar((std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)());
	XVector3 BoundingBoxMinVar((std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)());

	XVector4 TopPosition(0.0f, Radius, 0.0f, 1.0f);
	BoundingBoxMinVar.x = (BoundingBoxMinVar.x < TopPosition.x) ? BoundingBoxMinVar.x : TopPosition.x;
	BoundingBoxMinVar.y = (BoundingBoxMinVar.y < TopPosition.y) ? BoundingBoxMinVar.y : TopPosition.y;
	BoundingBoxMinVar.z = (BoundingBoxMinVar.z < TopPosition.z) ? BoundingBoxMinVar.z : TopPosition.z;

	BoundingBoxMaxVar.x = (BoundingBoxMaxVar.x > TopPosition.x) ? BoundingBoxMaxVar.x : TopPosition.x;
	BoundingBoxMaxVar.y = (BoundingBoxMaxVar.y > TopPosition.y) ? BoundingBoxMaxVar.y : TopPosition.y;
	BoundingBoxMaxVar.z = (BoundingBoxMaxVar.z > TopPosition.z) ? BoundingBoxMaxVar.z : TopPosition.z;


	XVector3 TopTangentX(1, 0, 0);
	XVector4 TopTangentY(0, 1, 0, 1);
	XVector2 TopTexCoord(0, 1);

	Positions.push_back(TopPosition);
	TangentXs.push_back(TopTangentX);
	TangentYs.push_back(TopTangentY);
	TextureCoords.push_back(TopTexCoord);



	float PhiStep = X_PI / StackCount;
	float ThetaStep = 2.0f * X_PI / SliceCount;
	for (uint32 i = 1; i <= StackCount - 1; ++i)
	{
		float Phi = i * PhiStep;
		for (uint32 j = 0; j <= SliceCount; ++j)
		{
			float Theta = j * ThetaStep;

			XVector4 Position = XVector4(Radius * sinf(Phi) * cosf(Theta), Radius * cosf(Phi), Radius * sinf(Phi) * sinf(Theta), 1.0);
			Positions.push_back(Position);

			BoundingBoxMinVar.x = (BoundingBoxMinVar.x < Position.x) ? BoundingBoxMinVar.x : Position.x;
			BoundingBoxMinVar.y = (BoundingBoxMinVar.y < Position.y) ? BoundingBoxMinVar.y : Position.y;
			BoundingBoxMinVar.z = (BoundingBoxMinVar.z < Position.z) ? BoundingBoxMinVar.z : Position.z;

			BoundingBoxMaxVar.x = (BoundingBoxMaxVar.x > Position.x) ? BoundingBoxMaxVar.x : Position.x;
			BoundingBoxMaxVar.y = (BoundingBoxMaxVar.y > Position.y) ? BoundingBoxMaxVar.y : Position.y;
			BoundingBoxMaxVar.z = (BoundingBoxMaxVar.z > Position.z) ? BoundingBoxMaxVar.z : Position.z;

			XVector3 TangentX(-Radius * sinf(Phi) * sinf(Theta), 0.0f, Radius * sinf(Phi) * cosf(Theta));
			TangentX.Normalize();
			TangentXs.push_back(TangentX);

			XVector3 Position3(Position.x, Position.y, Position.z);
			Position3.Normalize();
			XVector4 Normal(Position3.x, Position3.y, Position3.z, 1.0f);
			TangentYs.push_back(Normal);

			XVector2 TexCoord(Theta / X_2PI, 1 -  Phi / X_PI);
			TextureCoords.push_back(TexCoord);

		}
	}


	XVector4 BottomPosition(0.0f, -Radius, 0.0f, 1.0f);
	BoundingBoxMinVar.x = (BoundingBoxMinVar.x < BottomPosition.x) ? BoundingBoxMinVar.x : BottomPosition.x;
	BoundingBoxMinVar.y = (BoundingBoxMinVar.y < BottomPosition.y) ? BoundingBoxMinVar.y : BottomPosition.y;
	BoundingBoxMinVar.z = (BoundingBoxMinVar.z < BottomPosition.z) ? BoundingBoxMinVar.z : BottomPosition.z;

	BoundingBoxMaxVar.x = (BoundingBoxMaxVar.x > BottomPosition.x) ? BoundingBoxMaxVar.x : BottomPosition.x;
	BoundingBoxMaxVar.y = (BoundingBoxMaxVar.y > BottomPosition.y) ? BoundingBoxMaxVar.y : BottomPosition.y;
	BoundingBoxMaxVar.z = (BoundingBoxMaxVar.z > BottomPosition.z) ? BoundingBoxMaxVar.z : BottomPosition.z;


	XVector3 BottomTangentX(1, 0, 0);
	XVector4 BottomTangentY(0, -1, 0, 1);
	XVector2 BottomTexCoord(0, 0);

	Positions.push_back(BottomPosition);
	TangentXs.push_back(BottomTangentX);
	TangentYs.push_back(BottomTangentY);
	TextureCoords.push_back(BottomTexCoord);

	for (uint32 i = 1; i <= SliceCount; ++i)
	{
		Indices.push_back(0);
		Indices.push_back(i + 1);
		Indices.push_back(i);
	}

	uint32 baseIndex = 1;
	uint32 ringVertexCount = SliceCount + 1;
	for (uint32 i = 0; i < StackCount - 2; ++i)
	{
		for (uint32 j = 0; j < SliceCount; ++j)
		{
			Indices.push_back(baseIndex + i * ringVertexCount + j);
			Indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			
			Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			Indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	uint32 southPoleIndex = (uint32)Positions.size() - 1;
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32 i = 0; i < SliceCount; ++i)
	{
		Indices.push_back(southPoleIndex);
		Indices.push_back(baseIndex + i);
		Indices.push_back(baseIndex + i + 1);
	}

	std::shared_ptr<GDataBuffer> PositionDataBuffer = std::make_shared<GDataBuffer>();
	PositionDataBuffer->SetData((uint8*)Positions.data(), Positions.size(), EVertexElementType::VET_Float4);

	std::shared_ptr<GDataBuffer> TangentXDataBuffer = std::make_shared<GDataBuffer>();
	TangentXDataBuffer->SetData((uint8*)TangentXs.data(), TangentXs.size(), EVertexElementType::VET_Float3);

	std::shared_ptr<GDataBuffer> TangentYDataBuffer = std::make_shared<GDataBuffer>();
	TangentYDataBuffer->SetData((uint8*)TangentYs.data(), TangentYs.size(), EVertexElementType::VET_Float4);

	std::shared_ptr<GDataBuffer> TextureCoordsDataBuffer = std::make_shared<GDataBuffer>();
	TextureCoordsDataBuffer->SetData((uint8*)TextureCoords.data(), TextureCoords.size(), EVertexElementType::VET_Float2);

	std::shared_ptr<GDataBuffer> IndexDataBuffer = std::make_shared<GDataBuffer>();
	IndexDataBuffer->SetData((uint8*)Indices.data(), Indices.size(), EVertexElementType::VET_UINT16);

	std::shared_ptr<GVertexBuffer>VertexBuffer = std::make_shared<GVertexBuffer>();
	VertexBuffer->SetData(PositionDataBuffer, EVertexAttributeType::VAT_POSITION);
	VertexBuffer->SetData(TangentXDataBuffer, EVertexAttributeType::VAT_TANGENT);
	VertexBuffer->SetData(TangentYDataBuffer, EVertexAttributeType::VAT_NORMAL);
	VertexBuffer->SetData(TextureCoordsDataBuffer, EVertexAttributeType::VAT_TEXCOORD);

	std::shared_ptr<GIndexBuffer> IndexBuffer = std::make_shared<GIndexBuffer>();
	IndexBuffer->SetData(IndexDataBuffer);

	std::shared_ptr<GMeshData>MeshData = std::make_shared<GMeshData>();
	MeshData->SetVertexBuffer(VertexBuffer);
	MeshData->SetIndexBuffer(IndexBuffer);

	std::shared_ptr<GGeomertry>Geomertry = std::make_shared<GGeomertry>();
	Geomertry->SetMeshData(MeshData);

	XVector3 BoundingBoxCenter = (BoundingBoxMaxVar + BoundingBoxMinVar) * 0.5f;
	Geomertry->SetBoundingBox(BoundingBoxCenter, BoundingBoxMaxVar - BoundingBoxCenter);
	return Geomertry;
}

std::shared_ptr<GGeomertry> CreateDefualtQuadGeo()
{
	std::vector<XVector4>Positions;
	std::vector<XVector3>TangentX;
	std::vector<XVector4>TangentY;
	std::vector<XVector2>TextureCoords;
	std::vector<uint16>Indices;

	Positions.push_back(XVector4(8.0, 0.0, 8.0, 1.0));
	Positions.push_back(XVector4(8.0, 0.0, -8.0, 1.0));
	Positions.push_back(XVector4(-8.0, 0.0, 8.0, 1.0));
	Positions.push_back(XVector4(-8.0, 0.0, -8.0, 1.0));

	TangentX.push_back(XVector3(1.0, 0.0, 0.0));
	TangentX.push_back(XVector3(1.0, 0.0, 0.0));
	TangentX.push_back(XVector3(1.0, 0.0, 0.0));
	TangentX.push_back(XVector3(1.0, 0.0, 0.0));

	TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));
	TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));
	TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));
	TangentY.push_back(XVector4(0.0, 1.0, 0.0, 1.0));

	TextureCoords.push_back(XVector2(1.0, 1.0));
	TextureCoords.push_back(XVector2(1.0, 0.0));
	TextureCoords.push_back(XVector2(0.0, 1.0));
	TextureCoords.push_back(XVector2(0.0, 0.0));

	Indices.push_back(0);
	Indices.push_back(1);
	Indices.push_back(2);

	Indices.push_back(2);
	Indices.push_back(1);
	Indices.push_back(3);

	
	std::shared_ptr<GDataBuffer> PositionDataBuffer = std::make_shared<GDataBuffer>();
	PositionDataBuffer->SetData((uint8*)Positions.data(), Positions.size(), EVertexElementType::VET_Float4);

	std::shared_ptr<GDataBuffer> TangentXDataBuffer = std::make_shared<GDataBuffer>();
	TangentXDataBuffer->SetData((uint8*)TangentX.data(), TangentX.size(), EVertexElementType::VET_Float3);

	std::shared_ptr<GDataBuffer> TangentYDataBuffer = std::make_shared<GDataBuffer>();
	TangentYDataBuffer->SetData((uint8*)TangentY.data(), TangentY.size(), EVertexElementType::VET_Float4);

	std::shared_ptr<GDataBuffer> TextureCoordsDataBuffer = std::make_shared<GDataBuffer>();
	TextureCoordsDataBuffer->SetData((uint8*)TextureCoords.data(), TextureCoords.size(), EVertexElementType::VET_Float2);

	std::shared_ptr<GDataBuffer> IndexDataBuffer = std::make_shared<GDataBuffer>();
	IndexDataBuffer->SetData((uint8*)Indices.data(), Indices.size(), EVertexElementType::VET_UINT16);

	std::shared_ptr<GVertexBuffer>VertexBuffer = std::make_shared<GVertexBuffer>();
	VertexBuffer->SetData(PositionDataBuffer,EVertexAttributeType::VAT_POSITION);
	VertexBuffer->SetData(TangentXDataBuffer,EVertexAttributeType::VAT_TANGENT);
	VertexBuffer->SetData(TangentYDataBuffer,EVertexAttributeType::VAT_NORMAL);
	VertexBuffer->SetData(TextureCoordsDataBuffer,EVertexAttributeType::VAT_TEXCOORD);

	std::shared_ptr<GIndexBuffer> IndexBuffer = std::make_shared<GIndexBuffer>();
	IndexBuffer->SetData(IndexDataBuffer);

	std::shared_ptr<GMeshData>MeshData = std::make_shared<GMeshData>();
	MeshData->SetVertexBuffer(VertexBuffer);
	MeshData->SetIndexBuffer(IndexBuffer);

	std::shared_ptr<GGeomertry>Geomertry = std::make_shared<GGeomertry>();
	Geomertry->SetMeshData(MeshData);

	Geomertry->SetBoundingBox(XVector3(0, 0, 0), XVector3(8, 0.1, 8));
	return Geomertry;
}

std::shared_ptr<GMaterialInstance> CreateDefautMaterialInstance()
{
	
	std::shared_ptr<GMaterial> MaterialPtr = CreateMaterialFromCode(XPath::ProjectMaterialSavedDir() + L"/DefaultMaterials.hlsl");
	std::shared_ptr<GMaterialInstance> MaterialIns = std::make_shared<GMaterialInstance>(MaterialPtr);
	MaterialIns->SetMaterialValueFloat3("ConstantColor", XVector3(1.0, 1.0, 1.0));
	MaterialIns->SetMaterialValueFloat("ConstantRoughness", 0.67f);
	MaterialIns->SetMaterialValueFloat("ConstantMetatllic", 0.3f);


	//std::shared_ptr<GTexture2D> TexBaseColor =
	//	CreateTextureFromImageFile(XPath::ProjectResourceSavedDirString() + "/TextureNoAsset/T_CheckBoard_D.png", true);
	//std::shared_ptr<GTexture2D> TexNormal =
	//	CreateTextureFromImageFile(XPath::ProjectResourceSavedDirString() + "/TextureNoAsset/T_CheckBoard_N.png", false);

	std::shared_ptr<GTexture2D> TexBaseColor =
		CreateTextureFromImageFile(XPath::ProjectResourceSavedDirString() + "/TextureNoAsset/T_CheckBoard_D.png", true);
	std::shared_ptr<GTexture2D> TexNormal =
		CreateTextureFromImageFile(XPath::ProjectResourceSavedDirString() + "/TextureNoAsset/T_CheckBoard_N.png", false);

	TexBaseColor->CreateRHITexture();
	TexNormal->CreateRHITexture();

	MaterialIns->SetMaterialTexture2D("BaseColorMap", TexBaseColor);
	MaterialIns->SetMaterialTexture2D("NormalMap", TexNormal);
	return MaterialIns;
}

std::shared_ptr<GMaterialInstance> GetDefaultmaterialIns()
{
	static std::shared_ptr<GMaterialInstance> MaterialIns = CreateDefautMaterialInstance();
	return MaterialIns;
} 

std::shared_ptr<GGeomertry> TempCreateCubeGeoWithMat()
{
	std::shared_ptr<GGeomertry> Result = CreateDefualCubeGeo();
	std::shared_ptr<GMaterialInstance> MaterialIns = GetDefaultmaterialIns();
	Result->SetMaterialPtr(MaterialIns);
	return Result;
}

std::shared_ptr<GGeomertry> TempCreateSphereGeoWithMat()
{
	std::shared_ptr<GGeomertry> Result = CreateDefualtSphereGeo(0.5, 36, 36);
	std::shared_ptr<GMaterialInstance> MaterialIns = GetDefaultmaterialIns();
	Result->SetMaterialPtr(MaterialIns);
	return Result;
}

std::shared_ptr<GGeomertry> TempCreateQuadGeoWithMat()
{
	std::shared_ptr<GGeomertry> Result = CreateDefualtQuadGeo();
	std::shared_ptr<GMaterialInstance> MaterialIns = GetDefaultmaterialIns();
	Result->SetMaterialPtr(MaterialIns);
	return Result;
}

