#pragma once
#include <memory>
#include "Runtime/HAL/PlatformTypes.h"
#include "Runtime/Core/Spatial/Spatial.h"
#include "Runtime/RHI/RHIResource.h"

class GMaterialInstance;

class GDataBuffer :public GObject
{
	friend class GVertexBuffer;
	friend class GIndexBuffer;
public:
	
	static uint32 DataTypeByteSize[(int)EVertexElementType::VET_MAX];
	~GDataBuffer();
	void SetData(uint8* DataStoreIn, uint64 DataNumIn, EVertexElementType DataTypeIn);
	inline uint32 GetDataTypeSize()const
	{
		return DataTypeByteSize[(int)DataType];
	}
	inline const uint8* GetData()const
	{
		return DataStore.data();
	}
	inline uint64 GetDataNum()
	{
		return DataNum;
	}
protected:
	EVertexElementType  DataType;
	std::vector<uint8>DataStore;
	uint64 DataNum;
	uint64 DataByteSize;
};

enum class EVertexAttributeType
{
	VAT_POSITION,
	VAT_TANGENT,
	VAT_NORMAL,
	VAT_TEXCOORD,
	VAT_MAX_NUM,
};


class GVertexBuffer : public GObject
{
public:
	void SetData(std::shared_ptr<GDataBuffer>DataBufferIn, EVertexAttributeType EVAIn);
	void CreateRHIBufferChecked();
	inline std::shared_ptr<XRHIVertexLayout> GetRHIVertexLayout()const
	{
		return RHIVertexLayout;
	}
	inline std::shared_ptr<XRHIBuffer> GetRHIVertexBuffer()const
	{
		return RHIVertexBuffer;
	}
private:
	std::shared_ptr<GDataBuffer> DataBufferPtrArray[(int)EVertexAttributeType::VAT_MAX_NUM];
	std::shared_ptr<XRHIVertexLayout> RHIVertexLayout;
	std::shared_ptr<XRHIBuffer> RHIVertexBuffer;
};

class GIndexBuffer : public GObject
{
	friend class GGeomertry;
public:
	void CreateRHIBufferChecked();
	inline void SetData(std::shared_ptr<GDataBuffer>DataBufferIn)
	{
		IndexBufferPtr = DataBufferIn;
	}
	inline std::shared_ptr<XRHIBuffer> GetRHIIndexBuffer()const
	{
		return RHIIndexBuffer;
	}
private:
	std::shared_ptr<GDataBuffer> IndexBufferPtr;
	std::shared_ptr<XRHIBuffer> RHIIndexBuffer;
};

class GMeshData :public GObject
{
public:
	friend class GGeomertry;
	inline void SetVertexBuffer(std::shared_ptr<GVertexBuffer>VertexBufferPtrIn) { VertexBufferPtr = VertexBufferPtrIn; }
	inline void SetIndexBuffer(std::shared_ptr<GIndexBuffer>IndexBufferPtrIn) { IndexBufferPtr = IndexBufferPtrIn; }
private:
	std::shared_ptr<GVertexBuffer>VertexBufferPtr;
	std::shared_ptr<GIndexBuffer>IndexBufferPtr;
};

struct VertexCBufferStruct
{
	XMatrix WorldMatrix;
	XVector3 BoundBoxMax; float padding0;
	XVector3 BoundBoxMin; float padding1;
};

class GGeomertry :public GSpatial
{
public:
	inline uint64 GetIndexCount()const { return MeshDataPtr->IndexBufferPtr->IndexBufferPtr->GetDataNum(); }
	inline std::shared_ptr<GVertexBuffer> GetGVertexBuffer()const { return MeshDataPtr->VertexBufferPtr; }
	inline std::shared_ptr<XRHIVertexLayout> GetRHIVertexLayout()const { return MeshDataPtr->VertexBufferPtr->GetRHIVertexLayout(); }
	inline std::shared_ptr<XRHIBuffer> GetRHIVertexBuffer()const { return MeshDataPtr->VertexBufferPtr->GetRHIVertexBuffer(); }

	inline std::shared_ptr<GIndexBuffer> GetGIndexBuffer()const { return MeshDataPtr->IndexBufferPtr; }
	inline std::shared_ptr<XRHIBuffer> GetRHIIndexBuffer()const { return MeshDataPtr->IndexBufferPtr->GetRHIIndexBuffer(); }
	inline std::shared_ptr<GMaterialInstance>& GetMaterialInstance() { return MaterialInstancePtr; }

	inline void SetMeshData(std::shared_ptr<GMeshData>MeshDataPtrIn) { MeshDataPtr = MeshDataPtrIn; }
	inline void SetMaterialPtr(std::shared_ptr<GMaterialInstance>MaterialInstancePtrIn) { MaterialInstancePtr = MaterialInstancePtrIn; }


	std::shared_ptr<XRHIConstantBuffer> GetPerObjectVertexCBuffer();
	std::shared_ptr<GGeomertry> CreateGeoInstanceNoMatAndTrans();
	std::shared_ptr<GGeomertry> CreateGeoInstancewithMat();
private:
	std::shared_ptr<GMeshData>MeshDataPtr;
	std::shared_ptr<GMaterialInstance>MaterialInstancePtr;
	std::shared_ptr<XRHIConstantBuffer>PerObjectVertexCBuffer;
};
