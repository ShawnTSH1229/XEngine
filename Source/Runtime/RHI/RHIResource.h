#pragma once

#include <vector>
#include <functional>
#include "RHI.h"
#include "Runtime/HAL/PlatformTypes.h"
#include "Runtime/Core/PixelFormat.h"

enum class IndirectArgType
{
	Arg_CBV,
	Arg_VBV,
	Arg_IBV,
	Arg_Draw_Indexed,
};

struct XRHIIndirectArg
{
	IndirectArgType type;
	union
	{
		struct
		{
			uint32 Slot;
		}VertexBuffer;

		struct
		{
			uint32 RootParameterIndex;
		}CBV;
	};
};

class XRHICommandSignature
{
public:
};

class XRHIViewport
{
public:
};


class XRHIBuffer
{
public:
	XRHIBuffer(uint32 StrideIn, uint32 SizeIn, EBufferUsage InUsage) :
		Size(SizeIn),
		Stride(StrideIn),
		Usage(InUsage){}
	inline uint32 GetStride()const { return Stride; }
	inline uint32 GetSize()const { return Size; }
	EBufferUsage GetUsage() const { return Usage; }
private:
	uint32 Size;
	uint32 Stride;
	EBufferUsage Usage;
};

class XRHIStructBuffer
{
public:
	XRHIStructBuffer(uint32 StrideIn, uint32 SizeIn)
		:Size(SizeIn),
		Stride(StrideIn) {}
	inline uint32 GetStride()const { return Stride; }
	inline uint32 GetSize()const { return Size; }
private:
	uint32 Size;// Used For Counter Offset
	uint32 Stride;
};

class XRHIShader
{
public:
	explicit XRHIShader(EShaderType ShaderTypeIn) :ShaderType(ShaderTypeIn), CodeHash(0) {}

	inline EShaderType		GetShaderType()				{ return ShaderType; }
	inline std::size_t		GetHash()const				{ return CodeHash; }
	inline void				SetHash(std::size_t Hahs)	{ CodeHash = Hahs; }

private:
	EShaderType ShaderType;
	std::size_t CodeHash;
};

class XRHIGraphicsShader :public XRHIShader
{
public:
	explicit XRHIGraphicsShader(EShaderType ShaderTypeIn) :XRHIShader(ShaderTypeIn) {}
};

class XRHIComputeShader :public XRHIShader
{
public:
	XRHIComputeShader() :XRHIShader(EShaderType::SV_Compute) {}
};

class XRHIRayTracingShader :public XRHIShader
{
public:
	XRHIRayTracingShader(EShaderType ShaderTypeIn) :XRHIShader(ShaderTypeIn) {}
};


//class XRHIRayGenShader : public XRHIRayTracingShader
//{
//public:
//	XRHIRayGenShader() :XRHIRayTracingShader(EShaderType::SV_RayGen) {}
//};
//
//class XRHIRayMissShader : public XRHIRayTracingShader
//{
//public:
//	XRHIRayMissShader() :XRHIRayTracingShader(EShaderType::SV_RayMiss) {}
//};
//
//class XRHIHitGroupShader : public XRHIRayTracingShader
//{
//public:
//	XRHIHitGroupShader() :XRHIRayTracingShader(EShaderType::SV_HitGroup) {}
//};

class XRHITexture
{
public:
	XRHITexture(EPixelFormat FormatIn) :Format(FormatIn) {}
	inline EPixelFormat GetFormat()const { return Format; }
	virtual void* GetTextureBaseRHI()
	{
		return nullptr;
	}
private:
	EPixelFormat Format;
};

class XRHITexture2D :public XRHITexture 
{
public:
	XRHITexture2D(EPixelFormat FormatIn) :XRHITexture(FormatIn) {}
};

class XRHITexture3D :public XRHITexture 
{
public:
	XRHITexture3D(EPixelFormat FormatIn) :XRHITexture(FormatIn) {}
};


class XRHIRenderTargetView 
{
public:
	XRHIRenderTargetView():Texture(nullptr){}
	XRHITexture* Texture;
};
class XRHIDepthStencilView 
{
public:
	XRHIDepthStencilView():Texture(nullptr){}
	XRHITexture* Texture;
};
class XRHIShaderResourceView {};
class XRHIUnorderedAcessView {};

class XRHIConstantBuffer
{
public:
	virtual void UpdateData(const void* data, uint32 size, uint32 offset_byte) = 0;
};

struct XRHICommandData
{
	std::vector<XRHIConstantBuffer*>CBVs;
	XRHIBuffer* VB;
	XRHIBuffer* IB;

	uint32 IndexCountPerInstance;
	uint32 InstanceCount;
	uint32 StartIndexLocation;
	int32  BaseVertexLocation;
	uint32 StartInstanceLocation;
};


class XRHISamplerState {};
class XRHIBlendState {};
class XRHIDepthStencilState {};
class XRHIRasterizationState {};

#define VERTEX_LAYOUT_MAX 16
using XRHIVertexLayoutArray = std::vector<XVertexElement>;
class XRHIVertexLayout {};


class XRHIVertexShader: public XRHIGraphicsShader 
{
public:
	XRHIVertexShader() :XRHIGraphicsShader(EShaderType::SV_Vertex) {}
};
class XRHIPixelShader : public XRHIGraphicsShader 
{
public:
	XRHIPixelShader() :XRHIGraphicsShader(EShaderType::SV_Pixel) {}
};

struct XRHIBoundShaderStateInput
{
	XRHIVertexLayout* RHIVertexLayout;
	XRHIVertexShader* RHIVertexShader;
	XRHIPixelShader* RHIPixelShader;
	XRHIBoundShaderStateInput() :
		RHIVertexLayout(nullptr),
		RHIVertexShader(nullptr),
		RHIPixelShader(nullptr) {}

	XRHIBoundShaderStateInput(
		XRHIVertexLayout* RHIVertexLayoutIn,
		XRHIVertexShader* RHIVertexShaderIn,
		XRHIPixelShader* RHIPixelShaderIn) :
		RHIVertexLayout(RHIVertexLayoutIn),
		RHIVertexShader(RHIVertexShaderIn),
		RHIPixelShader(RHIPixelShaderIn) {}
};

class XGraphicsPSOInitializer
{
public:
	XRHIBoundShaderStateInput BoundShaderState;
	XRHIBlendState* BlendState;
	XRHIRasterizationState* RasterState;
	XRHIDepthStencilState* DepthStencilState;
	uint32 RTNums;
	std::array<EPixelFormat, 8>RT_Format;
	EPixelFormat DS_Format;

	inline std::size_t GetHashIndex()const
	{
		std::size_t seed = 42;
		THashCombine(seed, BoundShaderState.RHIVertexLayout);
		THashCombine(seed, BoundShaderState.RHIVertexShader);
		THashCombine(seed, BoundShaderState.RHIPixelShader);
		THashCombine(seed, BlendState);
		THashCombine(seed, RasterState);
		THashCombine(seed, DepthStencilState);
		THashCombine(seed, RTNums);
		for (int i = 0; i < 8; i++)
		{
			THashCombine(seed, (int)RT_Format[i]);
		}
		THashCombine(seed, (int)DS_Format);
		return seed;
	}
};

class XRHIGraphicsPSO {};
class XRHIComputePSO{};
class XRHIRayTracingPSO {};

class XRHISetRenderTargetsInfo
{
public:
	// Color Render Targets Info
	XRHIRenderTargetView ColorRenderTarget[8];
	int32 NumColorRenderTargets;
	bool bClearColor;

	// Depth/Stencil Render Target Info
	XRHIDepthStencilView DepthStencilRenderTarget;
	bool bClearDepth;
	bool bClearStencil;

	XRHISetRenderTargetsInfo()
		:NumColorRenderTargets(0),
		bClearColor(false),
		bClearDepth(false),
		bClearStencil(false) {}
};

struct XRHIRenderPassInfo
{
	struct XColorTarget
	{
		XRHITexture* RenderTarget;
		ERenderTargetLoadAction LoadAction;
		ERenderTargetStoreAction StoreAction;
	};
	XColorTarget RenderTargets[8];

	struct EDepthStencilTarget
	{
		XRHITexture* DepthStencilTarget;
		EDepthStencilLoadAction LoadAction;
		ERenderTargetStoreAction StoreAction;
	};
	EDepthStencilTarget DepthStencilRenderTarget;

	void ConvertToRenderTargetsInfo(XRHISetRenderTargetsInfo& OutRTInfo) const
	{
		for (int i = 0; i < 8; i++)
		{
			if (!RenderTargets[i].RenderTarget)
			{
				break;
			}
			OutRTInfo.ColorRenderTarget[i].Texture = RenderTargets[i].RenderTarget;
			OutRTInfo.bClearColor |= (RenderTargets[i].LoadAction == ERenderTargetLoadAction::EClear ? true : false);
			OutRTInfo.NumColorRenderTargets++;
		}

		OutRTInfo.DepthStencilRenderTarget.Texture = DepthStencilRenderTarget.DepthStencilTarget;
		OutRTInfo.bClearDepth = (DepthStencilRenderTarget.LoadAction == EDepthStencilLoadAction::EClear ? true : false);
	}
	explicit XRHIRenderPassInfo(
		int NumColorRTs,
		XRHITexture* ColorRTs[],
		ERenderTargetLoadAction ColorLoadAction,
		XRHITexture* DepthRT,
		EDepthStencilLoadAction DSLoadAction,

		ERenderTargetStoreAction ColorStoreAction = ERenderTargetStoreAction::EStore,//TempHack
		ERenderTargetStoreAction DSStoreAction = ERenderTargetStoreAction::EStore
	)
	{
		for (int i = 0; i < NumColorRTs; i++)
		{
			RenderTargets[i].RenderTarget = ColorRTs[i];
			RenderTargets[i].LoadAction = ColorLoadAction;
			RenderTargets[i].StoreAction = ColorStoreAction;
		}
		memset(&RenderTargets[NumColorRTs], 0, sizeof(XColorTarget) * (8 - NumColorRTs));

		if (DepthRT != nullptr)
		{
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.LoadAction = DSLoadAction;
			DepthStencilRenderTarget.StoreAction = DSStoreAction;
		}
		else
		{
			DepthStencilRenderTarget.DepthStencilTarget = nullptr;
			DepthStencilRenderTarget.LoadAction = EDepthStencilLoadAction::ENoAction;
			DepthStencilRenderTarget.StoreAction = ERenderTargetStoreAction::ENoAction;
		}
	}
};

enum class ERayTracingAccelerationStructureFlags : uint32
{
	None = 0,
	AllowUpdate = 1 << 0,
	AllowCompaction = 1 << 1,
	PreferTrace = 1 << 2,
	PreferBuild = 1 << 3,
};

enum class EAccelerationStructureBuildMode
{
	Build,
	Update
};

//
// Ray tracing resources
//

enum class ERayTracingInstanceFlags : uint8
{
	None = 0,
	TriangleCullDisable = 1 << 1, // No back face culling. Triangle is visible from both sides.
	TriangleCullReverse = 1 << 2, // Makes triangle front-facing if its vertices are counterclockwise from ray origin.
	ForceOpaque = 1 << 3, // Disable any-hit shader invocation for this instance.
	ForceNonOpaque = 1 << 4, // Force any-hit shader invocation even if geometries inside the instance were marked opaque.
};
ENUM_CLASS_FLAGS(ERayTracingInstanceFlags);

struct XRayTracingAccelerationStructSize
{
	uint64 ResultSize = 0;
	uint64 BuildScratchSize = 0;
	uint64 UpdateScratchSize = 0;
};

class XRHIRayTracingAccelerationStruct
{
public:
	friend class IRHIContext;

	XRayTracingAccelerationStructSize GetSizeInfo()const
	{
		return SizeInfo;
	}
protected:
	XRayTracingAccelerationStructSize SizeInfo = {};
};

//Build BLAS ----------------------------------------------------------------------------------------------------------

struct XRayTracingGeometrySegment
{
public:
	std::shared_ptr<XRHIBuffer> VertexBuffer = nullptr;
	EVertexElementType VertexElementType = EVertexElementType::VET_Float3;

	// Offset in bytes from the base address of the vertex buffer.
	uint32 VertexBufferOffset = 0;
	uint32 MaxVertices = 0;
	uint32 FirstPrimitive = 0;
	uint32 NumPrimitives = 0;

	// Number of bytes between elements of the vertex buffer (sizeof VET_Float3 by default).
	// Must be equal or greater than the size of the position vector.
	uint32 VertexBufferStride = 12;

	// Indicates whether any-hit shader could be invoked when hitting this geometry segment.
	// Setting this to `false` turns off any-hit shaders, making the section "opaque" and improving ray tracing performance.
	bool bForceOpaque = true;
	bool bEnabled = true;
};

struct XRayTracingGeometryInitializer
{
public:
	std::shared_ptr<XRHIBuffer>IndexBuffer;
	uint32 IndexBufferOffset = 0;
	std::vector<XRayTracingGeometrySegment>Segments;

	
	bool bPreferBuild = false;
	bool bAllowUpdate = false;
	bool bAllowCompaction = true;
};

using XRayTracingAccelerationStructureAddress = uint64;

class XRHIRayTracingGeometry : public XRHIRayTracingAccelerationStruct
{
public:
	XRHIRayTracingGeometry(const XRayTracingGeometryInitializer& InInitializer)
		: Initializer(InInitializer)
	{}

	uint32 GetNumSegments()const
	{
		return Initializer.Segments.size();
	}

	virtual XRayTracingAccelerationStructureAddress GetAccelerationStructureAddress() const = 0;

	XRayTracingGeometryInitializer Initializer;
};

//Build TLAS ----------------------------------------------------------------------------------------------------------

struct XRayTracingGeometryInstance
{
	std::shared_ptr<XRHIRayTracingGeometry>GeometryRHI = nullptr;
	uint32 NumTransforms = 0;
	float Trasnforms[3][4];

	// Mask that will be tested against one provided to TraceRay() in shader code.
	// If binary AND of instance mask with ray mask is zero, then the instance is considered not intersected / invisible.
	uint8 RayMask = 0xFF;
};


struct XRayTracingSceneInitializer
{
	// One entry per instance
	std::vector<XRHIRayTracingGeometry*>PerInstanceGeometres;

	uint32 NumNativeInstance = 0;
	uint32 NumTotalSegments = 0;

	// Exclusive prefix sum of instance geometry segments is used to calculate SBT record address from instance and segment indices.
	std::vector<uint32> SegmentPrefixSum;

	// This value controls how many elements will be allocated in the shader binding table per geometry segment.
// Changing this value allows different hit shaders to be used for different effects.
// For example, setting this to 2 allows one hit shader for regular material evaluation and a different one for shadows.
// Desired hit shader can be selected by providing appropriate RayContributionToHitGroupIndex to TraceRay() function.
// Use ShaderSlot argument in SetRayTracingHitGroup() to assign shaders and resources for specific part of the shder binding table record.
	uint32 ShaderSlotsPerGeometrySegment = 1;

	// At least one miss shader must be present in a ray tracing scene.
// Default miss shader is always in slot 0. Default shader must not use local resources.
// Custom miss shaders can be bound to other slots using SetRayTracingMissShader().
	uint32 NumMissShaderSlots = 1;
};

class XRHIRayTracingScene : public XRHIRayTracingAccelerationStruct
{
public:

	virtual const XRayTracingSceneInitializer GetRayTracingSceneInitializer()const = 0;
};

class XRayTracingPipelineSignature
{
public:
	inline std::size_t GetHashIndex()const
	{
		std::size_t seed = 42;
		THashCombine(seed, RayGenHash);
		THashCombine(seed, MissHash);
		THashCombine(seed, HitGroupHash);
		return seed;
	}
protected:
	uint64 RayGenHash = 0;
	uint64 MissHash = 0;
	uint64 HitGroupHash = 0;
};

class XRayTracingPipelineStateInitializer : public XRayTracingPipelineSignature
{
public:

	void SetRayGenShaderTable(const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InRayGenShaders, uint64 Hash = 0)
	{
		RayGenTable = InRayGenShaders;
		RayGenHash = Hash ? Hash : ComputeShaderTableHash(InRayGenShaders);
	}

	void SetMissShaderTable(const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InMissShaders, uint64 Hash = 0)
	{
		MissTable = InMissShaders;
		MissHash = Hash ? Hash : ComputeShaderTableHash(InMissShaders);
	}

	void SetHitGroupShaderTable(const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InHitGroupShaders, uint64 Hash = 0)
	{
		HitGroupTable = InHitGroupShaders;
		HitGroupHash = Hash ? Hash : ComputeShaderTableHash(InHitGroupShaders);
	}

	std::vector<std::shared_ptr<XRHIRayTracingShader>> RayGenTable;
	std::vector<std::shared_ptr<XRHIRayTracingShader>> MissTable;
	std::vector<std::shared_ptr<XRHIRayTracingShader>> HitGroupTable;
protected:

	uint64 ComputeShaderTableHash(const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InRayGenShaders ,uint64 InitialHash = 5699878132332235837ull)
	{
		uint64 CombineHash = InitialHash;
		for (auto RTShader : InRayGenShaders)
		{
			THashCombine(CombineHash, RTShader->GetHash());
		}
		return CombineHash;
	}


};