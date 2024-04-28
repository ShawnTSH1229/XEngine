#pragma once
#include <type_traits>
#include "Runtime/HAL/PlatformTypes.h"
#include <vector>
class XArchiveBase
{
private:
	bool bIsLoad : 1;
	bool bIsSave : 1;
public:
	XArchiveBase()
		:bIsLoad(false),
		bIsSave(false) { }

	virtual ~XArchiveBase() {}

	inline void SetIsLoad(bool Value)
	{
		bIsLoad = Value;
	}

	inline void SetIsSave(bool Value)
	{
		bIsSave = Value;
	}

	virtual void Close()
	{

	}

	template<typename T>
	requires std::is_arithmetic_v<T>
	XArchiveBase& ArchiveFun(T& ValueIn)
	{
		this->Archive_Impl(&ValueIn, sizeof(ValueIn));
		return *this;
	}

	template<typename T>
	requires std::is_arithmetic_v<T>
	XArchiveBase& ArchiveFun(std::vector<T>& ValueIn)
	{
		if (bIsLoad)
		{
			int64 ArrayByteSize;
			this->Archive_Impl(&ArrayByteSize, sizeof(ArrayByteSize));
			ValueIn.resize(ArrayByteSize / sizeof(T));
			this->Archive_Impl(ValueIn.data(), ArrayByteSize);
		}
		else if (bIsSave)
		{
			int64 ArrayByteSize = ValueIn.size() * sizeof(T);
			this->Archive_Impl(&ArrayByteSize, sizeof(ArrayByteSize));
			this->Archive_Impl(ValueIn.data(), ArrayByteSize);
		}
		return *this;
	}
private:
	virtual void Archive_Impl(void* V, int64 ByteSize)
	{

	}
};





