#pragma once
#include <fstream>
#include "Runtime/HAL/PlatformTypes.h"


//Temp ,No Virual Now
class IFileHandle
{
public:

	//Close File
	~IFileHandle();

	
	bool Read(uint8* Destination, int64 BytesToRead);
	bool Write(const uint8* Source, int64 BytesToWrite);
	void OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false);
	void OpenRead(const TCHAR* Filename);
private:

	//temp
	std::fstream FileHandle;
};
