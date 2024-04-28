#include "GeomertyData.h"
#include "Runtime/Core/ResourceCreateDataInterface.h"
#include "Runtime/RHI/RHICommandList.h"
#include "Runtime/Engine/Classes/Material.h"

uint32 GDataBuffer::DataTypeByteSize[(int)EVertexElementType::VET_MAX] =
{
	0,

	4,  
	8,
	12,
	16,

	2,
	4,

	16,
	0,
};

static_assert((int)EVertexElementType::VET_MAX == 9,"(int)EVertexElementType::VET_MAX == 9");

GDataBuffer::~GDataBuffer()
{
}

void GDataBuffer::SetData(uint8* DataStoreIn, uint64 DataNumIn, EVertexElementType DataTypeIn)
{
	DataType = DataTypeIn;
	DataNum = DataNumIn;
	DataByteSize = DataTypeByteSize[(int)DataType] * DataNum;

	DataStore.insert(DataStore.end(), DataStoreIn, DataStoreIn + DataByteSize);
}

void GVertexBuffer::SetData(std::shared_ptr<GDataBuffer> DataBufferIn, EVertexAttributeType EVAIn)
{
	DataBufferPtrArray[(int)EVAIn] = DataBufferIn;
}

void GVertexBuffer::CreateRHIBufferChecked()
{
	if (RHIVertexLayout.get() != nullptr)
	{
		return;
	}

	XRHIVertexLayoutArray LayoutArray;
	uint32 SemanticIndex = 0;
	uint32 ByteOffset = 0;
	
	for (int i = 0; i < (int)EVertexAttributeType::VAT_MAX_NUM; i++)
	{
		std::shared_ptr<GDataBuffer> BufferPtr = DataBufferPtrArray[i];
		if (BufferPtr.get() != nullptr)
		{
			LayoutArray.push_back(XVertexElement(SemanticIndex, BufferPtr->DataType, 0, ByteOffset));
			SemanticIndex++;
			ByteOffset += BufferPtr->GetDataTypeSize();
		}
	}
	RHIVertexLayout = RHICreateVertexLayout(LayoutArray);

	uint32 ElementSize = ByteOffset;
	uint64 ArrayElementNum = DataBufferPtrArray[(int)EVertexAttributeType::VAT_POSITION]->DataNum;;
	uint64 ArrayByteSize = ElementSize * ArrayElementNum;
	
	FResourceVectorUint8 ResourceData;
	ResourceData.Data = std::malloc(ArrayByteSize);
	ResourceData.SetResourceDataSize(ArrayByteSize);
	
	for (uint64 i = 0; i < ArrayElementNum; i++)
	{
		uint32 OffsetInVertex = 0;
		for (int j = 0; j < (int)EVertexAttributeType::VAT_MAX_NUM; j++)
		{
			std::shared_ptr<GDataBuffer> BufferPtr = DataBufferPtrArray[j];
			if (BufferPtr.get() != nullptr)
			{
				memcpy((uint8*)ResourceData.Data + i * ElementSize + OffsetInVertex, 
					BufferPtr->GetData() + i * BufferPtr->GetDataTypeSize(), BufferPtr->GetDataTypeSize());
				OffsetInVertex += BufferPtr->GetDataTypeSize();
			}
		}
	}

	XRHIResourceCreateData VertexCreateData(&ResourceData);
	RHIVertexBuffer = RHIcreateVertexBuffer(ElementSize, ArrayByteSize, EBufferUsage::BUF_Static, VertexCreateData);
}

void GIndexBuffer::CreateRHIBufferChecked()
{
	FResourceVectorUint8 ResourceData;
	ResourceData.Data = std::malloc(IndexBufferPtr->DataByteSize);
	ResourceData.SetResourceDataSize(IndexBufferPtr->DataByteSize);
	memcpy(ResourceData.Data, IndexBufferPtr->GetData(), IndexBufferPtr->DataByteSize);
	XRHIResourceCreateData CreateData(&ResourceData);
	RHIIndexBuffer = RHICreateIndexBuffer(IndexBufferPtr->GetDataTypeSize(), IndexBufferPtr->DataByteSize, EBufferUsage::BUF_Static, CreateData);
}


std::shared_ptr<XRHIConstantBuffer> GGeomertry::GetPerObjectVertexCBuffer()
{
	if (PerObjectVertexCBuffer.get() == nullptr)
	{
		PerObjectVertexCBuffer = RHICreateConstantBuffer(sizeof(VertexCBufferStruct));
	}

	VertexCBufferStruct VertexCB;
	VertexCB.WorldMatrix = GetWorldTransform().GetCombineMatrix();
	VertexCB.BoundBoxMax = GetBoudingBoxWithTrans().Center + GetBoudingBoxWithTrans().Extent;
	VertexCB.BoundBoxMin = GetBoudingBoxWithTrans().Center - GetBoudingBoxWithTrans().Extent;

	PerObjectVertexCBuffer->UpdateData(&VertexCB, sizeof(VertexCBufferStruct), 0);

	return PerObjectVertexCBuffer;
}

std::shared_ptr<GGeomertry> GGeomertry::CreateGeoInstanceNoMatAndTrans()
{
	std::shared_ptr<GGeomertry> RetGeo = std::make_shared<GGeomertry>();
	RetGeo->MeshDataPtr = this->MeshDataPtr;
	RetGeo->MaterialInstancePtr = std::make_shared<GMaterialInstance>(this->MaterialInstancePtr->MaterialPtr);
	RetGeo->PerObjectVertexCBuffer = nullptr;
	RetGeo->SetBoundingBox(this->GetBoudingBoxNoTrans());
	return RetGeo;
}

std::shared_ptr<GGeomertry> GGeomertry::CreateGeoInstancewithMat()
{
	std::shared_ptr<GGeomertry> RetGeo = std::make_shared<GGeomertry>();
	RetGeo->MeshDataPtr = this->MeshDataPtr;
	RetGeo->MaterialInstancePtr = std::make_shared<GMaterialInstance>(this->MaterialInstancePtr->MaterialPtr);
	RetGeo->MaterialInstancePtr->MaterialFloatArray = this->MaterialInstancePtr->MaterialFloatArray;
	RetGeo->MaterialInstancePtr->MaterialTextureArray = this->MaterialInstancePtr->MaterialTextureArray;

	RetGeo->PerObjectVertexCBuffer = nullptr;
	RetGeo->SetBoundingBox(this->GetBoudingBoxNoTrans());
	return RetGeo;
}
