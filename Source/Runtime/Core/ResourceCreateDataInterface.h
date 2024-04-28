#pragma once
#include <type_traits>
#include <vector>
#include "Runtime/HAL/PlatformTypes.h"
class FResourceArrayInterface
{
public:
	virtual ~FResourceArrayInterface() {}
	virtual const void* GetResourceData() const = 0;
	virtual uint32 GetResourceDataSize() const = 0;
	virtual void ReleaseData() = 0;
};


class FResourceVectorUint8 :public FResourceArrayInterface
{
private:
	uint32 Size;
public:
	void* Data;
	void SetResourceDataSize(uint32 SizeIn)
	{
		Size = SizeIn;
	}

	const void* GetResourceData() const override
	{
		return Data;
	}

	uint32 GetResourceDataSize() const override
	{
		return Size;
	}

	void ReleaseData()override
	{
		std::free(Data);
	}
};

template<typename T>
class TResourceVector :public FResourceArrayInterface
{
private:
	std::vector<T>ResourceVec;
public:
	void PushBack(T&& Value)
	{
		ResourceVec.push_back(std::forward<T>(Value));
	}

	const void* GetResourceData() const override
	{
		return ResourceVec.data();
	}

	uint32 GetResourceDataSize() const override
	{
		return ResourceVec.size() * sizeof(T);
	}

	void ReleaseData()override
	{
		ResourceVec.clear();
	}
};