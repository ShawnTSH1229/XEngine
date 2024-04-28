#pragma once
#pragma once
#include <vector>
#include "Archive.h"

class XMemoryWriter :public XArchiveBase
{

public:
	XMemoryWriter(std::vector<uint8>& BytesIn)
		:Offset(0), Bytes(BytesIn)
	{
		this->SetIsSave(true);
	}

	void Archive_Impl(void* Data, int64 Num)override
	{
		const int64 NumAdd = Offset + Num - Bytes.size();
		if (NumAdd > 0)
		{
			Bytes.resize(Offset + Num);
		}

		memcpy(&Bytes[Offset], Data, Num);
		Offset += Num;
	}

	std::vector<uint8>& TempGetData()
	{
		return Bytes;
	}
private:
	std::vector<uint8> Bytes;
	int64 Offset;
};