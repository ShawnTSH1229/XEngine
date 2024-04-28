#pragma once
#include <vector>
#include "Runtime/Core/Serialization/Archive.h"
#include "Runtime/Core/GenericPlatform/GenericPlatformFile.h"

class XFArchiveFileReaderGeneric :public XArchiveBase
{
private:
	int64 Pos;
	std::unique_ptr<IFileHandle>FileHandle;
public:
	~XFArchiveFileReaderGeneric();
	XFArchiveFileReaderGeneric(IFileHandle* InHandle, const TCHAR* InFilename);
	virtual void Archive_Impl(void* V, int64 Length) override;
};

class XFArchiveFileWriterGeneric :public XArchiveBase
{
private:
	int64 Pos;

	//std::wstring FileName;
	std::unique_ptr<IFileHandle>FileHandle;

	int64 BufferSize;
	std::vector<uint8>BufferArray;
public:
	virtual ~XFArchiveFileWriterGeneric();
	XFArchiveFileWriterGeneric(IFileHandle* InHandle, const TCHAR* InFilename);
	void Close()override;
	virtual void Archive_Impl(void* V, int64 Length) override;
protected:
	bool WriteBufferToFile(const uint8* Src, int64 CountToWrite);
	bool WriteCurrentBufferToFileAndClear();

};

class XFileManagerGeneric
{
public:
	static std::shared_ptr<XArchiveBase> CreateFileWriter(const TCHAR* Filename);
	static std::shared_ptr<XArchiveBase> CreateFileReader(const TCHAR* Filename);
};