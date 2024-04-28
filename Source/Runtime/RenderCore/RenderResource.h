#pragma once
#include <vector>
#include <memory>
#include "Runtime/RHI/RHIResource.h"
#include "Runtime/HAL/PlatformTypes.h"
class XRenderResource
{
public:
	XRenderResource() : ListIndex(-1) {}

	template<typename FunctionType>
	static void ForAllResources(const FunctionType& Function)
	{
		const std::vector<XRenderResource*>ResourceArray = GetResourceArray();
		for (int Index = 0; Index < ResourceArray.size(); Index++)
		{
			XRenderResource* Resource = ResourceArray[Index];
			if (Resource)
			{
				Function(Resource);
			}
		}
	}

	static void InitRHIForAllResources()
	{
		ForAllResources([](XRenderResource* Resource) { Resource->InitRHI(); });
	}

	virtual void InitResource();
	virtual void ReleaseResource() {};
	virtual void InitRHI() {}
	virtual void ReleaseRHI() {}
private:
	static std::vector<XRenderResource*>& GetResourceArray();
	int32 ListIndex;
};

extern void BeginInitResource(XRenderResource* Resource);

template<typename ResourceClass>
class TGlobalResource : public ResourceClass
{
public:
	TGlobalResource()
	{
		InitGlobalResource();
	}
private:
	void InitGlobalResource()
	{
		//if (IsInRenderingThread())
		//{
		//	((ResourceType*)this)->InitResource();
		//}
		//else
		{
			BeginInitResource((ResourceClass*)this);
		}
	}
};

class RVertexBuffer :public XRenderResource
{
public:
	std::shared_ptr<XRHIBuffer>RHIVertexBuffer;
};

class RIndexBuffer :public XRenderResource
{
public:
	std::shared_ptr<XRHIBuffer>RHIIndexBuffer;
};