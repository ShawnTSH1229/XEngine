#include "D3D12PipelineLibrary.h"
#include "D3D12PhysicDevice.h"
#include "Runtime/Core/Misc/Path.h"

#define CACHE_PATH BOOST_PP_CAT(L, BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/Cache/graphics_pso_cache.cache"))

//https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12PipelineStateCache/src/MemoryMappedPipelineLibrary.cpp
static const std::wstring FileName = CACHE_PATH;

void XD3D12PipelineLibrary::DeserializingPSOLibrary(XD3D12Device* InPhyDevice)
{
	PhyDevice = InPhyDevice;
	MMappedFile.Init(FileName);
    ThrowIfFailed(PhyDevice->GetDXDevice1()->CreatePipelineLibrary(MMappedFile.GetData(), MMappedFile.GetSize(), IID_PPV_ARGS(&m_pipelineLibrary)));
	Changed = false;
}

XD3D12PipelineLibrary::~XD3D12PipelineLibrary()
{
    SerializingPSOLibrary();
}

void XD3D12PipelineLibrary::SerializingPSOLibrary()
{
	const UINT librarySize = static_cast<UINT>(m_pipelineLibrary->GetSerializedSize());
	if (librarySize > 0)
	{
		const size_t neededSize = sizeof(UINT) + librarySize;
		if (Changed)
		{
			void* pTempData = new BYTE[librarySize];
			if (pTempData)
			{
				ThrowIfFailed(m_pipelineLibrary->Serialize(pTempData, librarySize));
                MMappedFile.GrowMapping(librarySize);
			
				memcpy(MMappedFile.GetData(), pTempData, librarySize);
                MMappedFile.SetSize(librarySize);
				delete[] pTempData;
				pTempData = nullptr;
			}
            else
            {
                ThrowIfFailed(m_pipelineLibrary->Serialize(MMappedFile.GetData(), librarySize));
                MMappedFile.SetSize(librarySize);
            }
		}
	}
    MMappedFile.Destroy(false);
}

bool XD3D12PipelineLibrary::LoadPSOFromLibrary(
	LPCWSTR pName,
	const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc,
	ID3D12PipelineState** PtrAddress)
{
	HRESULT hr = m_pipelineLibrary->LoadGraphicsPipeline(pName, pDesc, IID_PPV_ARGS(PtrAddress));
	if (hr == E_INVALIDARG)
	{
		return false;
	}
	return true;
}

bool XD3D12PipelineLibrary::LoadPSOFromLibrary(LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc, ID3D12PipelineState** PtrAddress)
{
    HRESULT hr = m_pipelineLibrary->LoadComputePipeline(pName, pDesc, IID_PPV_ARGS(PtrAddress));
    if (hr == E_INVALIDARG)
    {
        return false;
    }
    return true;
}

void XD3D12PipelineLibrary::StorePSOToLibrary(
	LPCWSTR pName,
	ID3D12PipelineState* pPipeline)
{
	Changed = true;
	ThrowIfFailed(m_pipelineLibrary->StorePipeline(pName, pPipeline));
}




MemoryMappedFile::MemoryMappedFile() :
    m_mapFile(INVALID_HANDLE_VALUE),
    m_file(INVALID_HANDLE_VALUE),
    m_mapAddress(nullptr),
    m_currentFileSize(0)
{
}

MemoryMappedFile::~MemoryMappedFile()
{
}

void MemoryMappedFile::Init(std::wstring filename, UINT fileSize)
{
    m_filename = filename;
    WIN32_FIND_DATA findFileData;
    HANDLE handle = FindFirstFileEx(filename.c_str(), FindExInfoBasic, &findFileData, FindExSearchNameMatch, nullptr, 0);
    bool found = handle != INVALID_HANDLE_VALUE;

    if (found)
    {
        FindClose(handle);
    }

    m_file = CreateFile2(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, (found) ? OPEN_EXISTING : CREATE_NEW, nullptr);
    XASSERT(m_file != INVALID_HANDLE_VALUE);


    LARGE_INTEGER realFileSize = {};
    BOOL flag = GetFileSizeEx(m_file, &realFileSize); 
    XASSERT(flag == true);
    XASSERT(realFileSize.HighPart == 0);
    
    m_currentFileSize = realFileSize.LowPart;

    if (fileSize > m_currentFileSize)
    {
        m_currentFileSize = fileSize;// Grow to the specified size.
    }

    m_mapFile = CreateFileMapping(m_file, nullptr, PAGE_READWRITE, 0, m_currentFileSize, nullptr);
    XASSERT(m_mapFile != nullptr);

    m_mapAddress = MapViewOfFile(m_mapFile, FILE_MAP_ALL_ACCESS, 0, 0, m_currentFileSize);
    XASSERT(m_mapAddress != nullptr);
}

void MemoryMappedFile::Destroy(bool deleteFile)
{
    if (m_mapAddress)
    {
        BOOL flag = UnmapViewOfFile(m_mapAddress); XASSERT(flag == true);
        m_mapAddress = nullptr;
        flag = CloseHandle(m_mapFile); XASSERT(flag == true);   // Close the file mapping object.
        flag = CloseHandle(m_file); XASSERT(flag == true);       // Close the file itself.
    }

    if (deleteFile)
    {
        DeleteFile(m_filename.c_str());
    }
}

void MemoryMappedFile::GrowMapping(UINT size)
{
    // Add space for the extra size at the beginning of the file.
    size += sizeof(UINT);

    // Check the size.
    if (size <= m_currentFileSize)
    {
        return;// Don't shrink.
    }

    // Flush.
    BOOL flag = FlushViewOfFile(m_mapAddress, 0);
	XASSERT(flag == true);

    // Close the current mapping.
    Destroy(false);

    // Update the size and create a new mapping.
    m_currentFileSize = size;
    Init(m_filename, m_currentFileSize);
}
