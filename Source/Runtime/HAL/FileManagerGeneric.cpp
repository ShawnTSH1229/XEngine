#include "FileManagerGeneric.h"

XFArchiveFileReaderGeneric::~XFArchiveFileReaderGeneric()
{
	FileHandle.reset(nullptr);
}

XFArchiveFileReaderGeneric::XFArchiveFileReaderGeneric(IFileHandle* InHandle, const TCHAR* InFilename)
	:FileHandle(InHandle),
	//FileName(InFilename),
	Pos(0)
{
	this->SetIsLoad(true);
}

void XFArchiveFileReaderGeneric::Archive_Impl(void* V, int64 Length)
{
	FileHandle->Read((uint8*)V, Length);
}







XFArchiveFileWriterGeneric::~XFArchiveFileWriterGeneric()
{
	Close();
}

XFArchiveFileWriterGeneric::XFArchiveFileWriterGeneric(IFileHandle* InHandle, const TCHAR* InFilename)
	:FileHandle(InHandle),
	//FileName(InFilename),
	Pos(0)
{
	BufferSize = 4096;
	BufferArray.reserve(4096);
	this->SetIsSave(true);
}

void XFArchiveFileWriterGeneric::Close()
{
	WriteCurrentBufferToFileAndClear();
	FileHandle.reset(nullptr);
}

void XFArchiveFileWriterGeneric::Archive_Impl(void* V, int64 Length)
{
	Pos += Length;

	// if length is greater than buffer size , write directl
	if (Length > BufferSize)
	{
		WriteCurrentBufferToFileAndClear();
		WriteBufferToFile(static_cast<const uint8*>(V), Length);
	}
	else
	{
		int64 FreeSize;
		while (Length > (FreeSize = BufferSize - BufferArray.size()))
		{
			BufferArray.insert(BufferArray.end(), static_cast<uint8*>(V), static_cast<uint8*>(V) + FreeSize);
			Length -= FreeSize;
			V = static_cast<uint8*>(V) + FreeSize;
			WriteCurrentBufferToFileAndClear();
		}

		//if has free size
		if (Length)
		{
			BufferArray.insert(BufferArray.end(), static_cast<uint8*>(V), static_cast<uint8*>(V) + Length);
		}
	}
}


bool XFArchiveFileWriterGeneric::WriteBufferToFile(const uint8* Src, int64 CountToWrite)
{
	return FileHandle->Write(Src, CountToWrite);
}

bool XFArchiveFileWriterGeneric::WriteCurrentBufferToFileAndClear()
{
	int64 WriteSize = BufferArray.size();
	bool Result = WriteBufferToFile(BufferArray.data(), WriteSize);
	BufferArray.clear();
	return Result;
}


std::shared_ptr<XArchiveBase> XFileManagerGeneric::CreateFileWriter(const TCHAR* Filename)
{
	IFileHandle* FilHandle = new IFileHandle();
	FilHandle->OpenWrite(Filename);
	return std::shared_ptr<XArchiveBase>(new XFArchiveFileWriterGeneric(FilHandle, Filename));
}

std::shared_ptr<XArchiveBase> XFileManagerGeneric::CreateFileReader(const TCHAR* Filename)
{
	IFileHandle* FilHandle = new IFileHandle();
	FilHandle->OpenRead(Filename);
	return std::shared_ptr<XArchiveBase>(new XFArchiveFileReaderGeneric(FilHandle, Filename));
}

