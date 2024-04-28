#include "GenericPlatformFile.h"

IFileHandle::~IFileHandle()
{
	FileHandle.close();
}

bool IFileHandle::Read(uint8* Destination, int64 BytesToRead)
{
	if (BytesToRead != 0)
	{
		FileHandle.read((char*)(Destination), BytesToRead);
	}
	return true;
}

bool IFileHandle::Write(const uint8* Source, int64 BytesToWrite)
{
	if (BytesToWrite != 0)
	{
		FileHandle.write((const char*)(Source), BytesToWrite);
	}
	return true;
}

void IFileHandle::OpenWrite(const TCHAR* Filename, bool bAppend, bool bAllowRead)
{
	std::ios::openmode openmode = std::ios::out | std::ios::binary;
	
	if (bAppend == false)
	{
		openmode |= std::ios::trunc;
	}

	if (bAllowRead)
	{
		openmode |= std::ios::in;
	}

	FileHandle.open(Filename, openmode);
	
}

void IFileHandle::OpenRead(const TCHAR* Filename)
{
	std::ios::openmode openmode = std::ios::in | std::ios::binary;
	FileHandle.open(Filename, openmode);
}
