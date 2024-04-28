#include "VertexFactory.h"
#include "Runtime/RHI/PipelineStateCache.h"

void XVertexFactory::InitLayout(const XRHIVertexLayoutArray& LayoutArray, ELayoutType LayoutType)
{
	if (LayoutType == ELayoutType::Layout_Default)
	{
		DefaultLayout = PipelineStateCache::GetOrCreateVertexLayout(LayoutArray);
	}
	else
	{
		XASSERT(false);
	}
}

std::vector<XVertexFactoryShaderInfo*>& XVertexFactoryShaderInfo::GetVertexFactoryShaderInfo_Array()
{
	static std::vector<XVertexFactoryShaderInfo*> VertexFactoryShaderInfos;
	return VertexFactoryShaderInfos;
}
