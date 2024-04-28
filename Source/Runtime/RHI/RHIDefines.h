#pragma once

#include "Runtime\HAL\PlatformTypes.h"
#include "Runtime\Core\Template\XEngineTemplate.h"

#define RHI_RAYTRACING 1

enum class EShaderType
{
	SV_Vertex = 0,
	SV_Pixel,
	SV_Compute,

	SV_RayGen,
	SV_RayMiss,
	SV_HitGroup,

	SV_ShaderCount
};

enum ETextureCreateFlags
{
	TexCreate_None = 0,
	TexCreate_RenderTargetable = 1 << 0,
	TexCreate_DepthStencilTargetable = 1 << 1,
	TexCreate_ShaderResource = 1 << 2,
	TexCreate_SRGB = 1 << 3,
	TexCreate_UAV = 1 << 4,
};

enum class EBlendOperation
{
	BO_Add,
};

enum class EBlendFactor
{
	BF_Zero,
	BF_One,
	BF_SourceAlpha,
	BF_InverseSourceAlpha,
};

enum class EResourceLockMode
{
	RLM_ReadOnly,
	RLM_WriteOnly,
	RLM_Num
};

enum class ECompareFunction
{
	CF_Greater,
	CF_Less,
	CF_GreaterEqual,
	CF_Always,
};

enum class EFaceCullMode
{
	FC_Back,
	FC_Front,
	FC_None,
};

enum ESamplerFilter
{
	SF_Point,
	SF_Bilinear,
};

enum ESamplerAddressMode
{
	AM_Wrap,
	AM_Clamp,
};

enum class ERenderTargetLoadAction
{
	ENoAction,
	ELoad,
	EClear,
};

enum class EDepthStencilLoadAction
{
	ENoAction,
	ELoad,
	EClear,
};

enum class ERenderTargetStoreAction
{
	ENoAction,
	EStore,
};

enum class EVertexElementType
{
	VET_None,
	VET_Float1,
	VET_Float2,
	VET_Float3,
	VET_Float4,
	VET_UINT16,
	VET_UINT32,
	VET_Color,
	VET_PackedNormal,	// FPackedNormal
	VET_MAX,
	VET_NumBits = 5,
};

enum class EBufferUsage : uint16
{
	BUF_None					= 0,
	BUF_Static					= 1 << 1,
	
	BUF_UnorderedAccess			= 1 << 2,
	BUF_DrawIndirect			= 1 << 3,
	BUF_ShaderResource			= 1 << 4,
	
	BUF_Dynamic					= 1 << 5,

	BUF_AccelerationStructure	= 1 << 6,
	
	BUF_Vertex					= 1 << 7,
	BUF_Index					= 1 << 8,
	BUF_StructuredBuffer		= 1 << 9,

	BUF_AnyDynamic = BUF_Dynamic,
	
};

ENUM_CLASS_FLAGS(EBufferUsage);

#define MaxSimultaneousRenderTargets 8

struct XRHIDescriptorHandle
{
	XRHIDescriptorHandle() {}
	XRHIDescriptorHandle(uint8 InType, uint32 InIndex)
		:Type(InType),
		Index(InIndex)
	{

	}

	bool IsValid() { return Index != 0xffffffff && Type != 0xff; };

	uint32 Index = 0xffffffff;
	uint8 Type = 0xff;
};