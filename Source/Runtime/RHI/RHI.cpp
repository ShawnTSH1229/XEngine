#include "RHI.h"

FPixelFormatInfo GPixelFormats[(int)EPixelFormat::FT_MAX] =
{
	{L"Unkown",							0},
	{L"FT_R16G16B16A16_FLOAT",			0},
	{L"FT_R8G8B8A8_UNORM",				0},
	{L"FT_R8G8B8A8_UNORM_SRGB",			0},
	{L"FT_R24G8_TYPELESS",				0},
	{L"FT_R11G11B10_FLOAT",				0},
	{L"FT_R16_FLOAT",					0},
	{L"FT_R32G32B32A32_UINT",			0},
};

uint32 GRHIRayTracingScratchBufferAlignment = 0;
uint32 GRHIRayTracingAccelerationStructureAlignment = 0;