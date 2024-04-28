#pragma once
#include <d3d12.h>
#include <string>
#include "Runtime/HAL/Mch.h"

class XD3D12Device;
class MemoryMappedFile
{
public:
    MemoryMappedFile();
    ~MemoryMappedFile();

    void Init(std::wstring filename, UINT filesize = 64);
    void Destroy(bool deleteFile);
    void GrowMapping(UINT size);

    inline void SetSize(UINT size)
    {
        if (m_mapAddress)
        {
            static_cast<UINT*>(m_mapAddress)[0] = size;
        }
    }

    inline UINT GetSize() const
    {
        if (m_mapAddress)
        {
            return static_cast<UINT*>(m_mapAddress)[0];
        }
        return 0;
    }

    inline void* GetData()
    {
        if (m_mapAddress)
        {
            return &static_cast<UINT*>(m_mapAddress)[1];
        }
        return nullptr;
    }

    inline bool IsMapped() const 
    { 
        return m_mapAddress != nullptr; 
    }

protected:
    HANDLE m_mapFile;
    HANDLE m_file;
    LPVOID m_mapAddress;
    UINT m_currentFileSize;

    std::wstring m_filename;
};

class XD3D12PipelineLibrary
{
public:
    ~XD3D12PipelineLibrary();
	
    //ID3D12PipelineLibrary::LoadGraphics/ComputePipeline
    bool LoadPSOFromLibrary(LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc, ID3D12PipelineState** PtrAddress);
    bool LoadPSOFromLibrary(LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC* pDesc, ID3D12PipelineState** PtrAddress);
	
    void StorePSOToLibrary(LPCWSTR pName,ID3D12PipelineState* pPipeline);   //ID3D12PipelineLibrary::StorePipeline
	void SerializingPSOLibrary();                                           //ID3D12PipelineLibrary::Serialize			
	void DeserializingPSOLibrary(XD3D12Device* PhyDevice);            //ID3D12Device1::CreatePipelineLibrary

	inline ID3D12PipelineLibrary* GetID3D12PipelineLibrary() { return m_pipelineLibrary.Get(); }
private:
	bool Changed;
    MemoryMappedFile MMappedFile;
	XD3D12Device* PhyDevice;
	XDxRefCount<ID3D12PipelineLibrary> m_pipelineLibrary;
};