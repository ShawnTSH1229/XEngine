#pragma once
#include <vector>
#include "Archive.h"

class XMemoryReader :public XArchiveBase 
{

public:
	XMemoryReader(const std::vector<uint8>& BytesIn)
		:Offset(0), Bytes(BytesIn)
	{
		this->SetIsLoad(true);
	}

	void Archive_Impl(void* Data, int64 Num)override
	{
		memcpy(Data, &Bytes[Offset], Num);
		Offset += Num;
	}
private:
	std::vector<uint8> Bytes;
	int64 Offset;
};
