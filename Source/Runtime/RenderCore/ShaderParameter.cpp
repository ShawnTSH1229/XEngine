#include "ShaderParameter.h"

void SetShaderParameters(XRayTracingShaderBindsWriter& RTBindingsWriter, const XShaderParameterMap& ShaderParameterMap, std::vector<XRHIShaderResourceView*>& SRVs, std::vector<XRHIUnorderedAcessView*>& UAVs)
{
	uint32 SRVInex = 0;
	uint32 UAVInex = 0;
	for (auto iter : ShaderParameterMap.MapNameToParameter)
	{
		auto ParaType = iter.second.Parametertype;
		switch (ParaType)
		{
		case EShaderParametertype::SRV:
		{
			RTBindingsWriter.SetSRV(iter.second.BufferIndex, SRVs[SRVInex]);
			SRVInex++;
		};
		break;
		case EShaderParametertype::UAV:
		{
			RTBindingsWriter.SetUAV(iter.second.BufferIndex, UAVs[UAVInex]);
			UAVInex++;
		};
		break;
		default:
			XASSERT(false);
			break;
		}
	}
}