#pragma once
#include "D3D12Resource.h"
class XD3D12StructBuffer : public XRHIStructBuffer
{
public:
	XD3D12StructBuffer(uint32 StrideIn, uint32 SizeIn) :
		XRHIStructBuffer(StrideIn, SizeIn) {}
	XD3D12ResourcePtr_CPUGPU ResourcePtr;
};