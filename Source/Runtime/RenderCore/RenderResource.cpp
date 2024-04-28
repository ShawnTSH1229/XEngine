#include "RenderResource.h"
#include <vector>
#include "Runtime/RHI/PlatformRHI.h"

std::vector<XRenderResource*>& XRenderResource::GetResourceArray()
{
	static std::vector<XRenderResource*> RenderResourceArray;
	return RenderResourceArray;
}

void XRenderResource::InitResource()
{
	if (ListIndex == -1)
	{
		if (GIsRHIInitialized == false)
		{
			std::vector<XRenderResource*>& RenderResourceArray = GetResourceArray();
			RenderResourceArray.push_back(this);
		}
		
		if (GIsRHIInitialized)
		{
			InitRHI();
		}

		ListIndex = 0;
	}
}

void BeginInitResource(XRenderResource* Resource)
{
	Resource->InitResource();
}
