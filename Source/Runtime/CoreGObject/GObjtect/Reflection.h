#pragma once
#include <string>
#include <functional>
#include <vector>

#include "Runtime/HAL/Mch.h"
#include "PermanentMemAlloc.h"

class XReflectionInfo;
class XProperty
{
public:
	enum
	{
		F_NONE = 0X00,
		F_SAVE_LOAD = 0X01,
	};

	XProperty(XReflectionInfo* ReflectionInfoIn, const std::string& PropertyNameIn, unsigned int PropertyOffsetIn)
		:ReflectionInfo(ReflectionInfoIn),
		PropertyName(PropertyNameIn),
		PropertyOffset(PropertyOffsetIn){}

	XReflectionInfo* ReflectionInfo;
	std::string PropertyName;
	unsigned int PropertyOffset;
	unsigned int PropertyFlag;
};



class XReflectionInfo
{
private:
	std::string ReflectInfoName;
	XReflectionInfo* SurperPtr;
public:
	XReflectionInfo(const std::string& ReflectInfoNameIn, XReflectionInfo* Super);
	
	inline void AddProperty(XProperty* PropertyIn)
	{
		if (PropertyIn != nullptr)
		{
			PropertyArray.push_back(PropertyIn);
		}
	}

	inline XReflectionInfo* GetSurper()
	{
		return SurperPtr;
	}

	inline XProperty* GetProperty(int index)
	{
		return PropertyArray[index];
	}

	inline XProperty* GetProperty(std::string& PropertyName)
	{
		for (auto& t : PropertyArray)
		{
			if (t->PropertyName == PropertyName)
			{
				return t;
			}
		}
		return nullptr;
	}


	inline std::vector<XProperty*>& GetPropertyArray()
	{

	}
	std::vector<XProperty*>PropertyArray;
};



template<typename T>
class XRangeProperty :public XProperty
{
public:
	XRangeProperty(
		XReflectionInfo* ReflectionInfoIn,
		const std::string& PropertyNameIn,
		unsigned int PropertyOffsetIn,
		T LowValueIn = T(),
		T HightValueIn = T(),
		T StepIn = T())
		:XProperty(ReflectionInfoIn, PropertyNameIn, PropertyOffsetIn),
		LowValue(LowValueIn),
		HightValue(HightValueIn),
		Step(StepIn) {}

	T LowValue;
	T HightValue;
	T Step;
};

template<typename T>
class XValueProperty :public XRangeProperty<T>
{
public:
	XValueProperty(
		XReflectionInfo* ReflectionInfoIn,
		const std::string& PropertyNameIn,
		unsigned int PropertyOffsetIn,
		T LowValueIn = T(),
		T HightValueIn = T(),
		T StepIn = T())
		:XRangeProperty<T>(ReflectionInfoIn, PropertyNameIn, PropertyOffsetIn, LowValueIn, HightValueIn, StepIn) {}

	virtual T& GetPropertyValue(void* ObjectIn)
	{
		return *(T*)(((char*)ObjectIn) + this->PropertyOffset);
	}
};


template<class T>
struct AutoPropertyCreator
{
	XProperty* CreateProperty(
		XReflectionInfo* Owner, 
		const std::string& Name,
		unsigned int Offset)
	{
		XValueProperty<T>* Ptr = (XValueProperty<T>*)GlobalGObjectPermanentMemAlloc.AllocateMem(sizeof(XValueProperty<T>));
		
		CRT_HEAP_TRACK_OFF_BEGIN;
		XProperty* Result = new (Ptr)XValueProperty<T>(Owner, Name, Offset);
		CRT_HEAP_TRACK_OFF_END;

		return Result;
	}
};

class PropertyCreator
{
public:
	template<class ValueType>
	static AutoPropertyCreator<ValueType>& GetAutoPropertyCreator(ValueType& valueTypeDummyRef)
	{
		static AutoPropertyCreator<ValueType> APC;
		return APC;
	}
};