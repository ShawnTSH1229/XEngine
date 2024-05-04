#include "Runtime/RenderCore/ShaderParameter.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RenderCore/RenderResource.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RHI/RHIStaticStates.h"

#include "VirtualShadowMapArray.h"
#include "DeferredShadingRenderer.h"

static int GFrameNum = 0;

__declspec(align(256)) struct SShadowViewInfo
{
	XMatrix LighgViewProject;
	XVector3 WorldCameraPosition;
	float SViewPadding0;
	XVector3 ShadowLightDir;
};

__declspec(align(256)) struct SVSMClearParameters
{
	uint32 VirtualShadowMapTileStateSize;
};

__declspec(align(256)) struct SVSMVisualizeParameters
{
	uint32 VisualizeType;

	uint32 DispatchSizeX;
	uint32 DispatchSizeY;
	uint32 DispatchSizeZ;
};

__declspec(align(256)) struct SCullingParameters
{
	XMatrix ShadowViewProject;
	uint32 MeshCount;
};

struct SLightSubProjectMatrix
{
	XMatrix LightViewProjectMatrix;
	uint32 VirtualTableIndexX;
	uint32 VirtualTableIndexY;
	uint32 MipLevel;
	uint32 padding2;
};

struct SVirtualShadowMapStructBuffer
{
	std::shared_ptr <XRHIStructBuffer> Buffer;
	std::shared_ptr<XRHIUnorderedAcessView> BufferUAV;
	std::shared_ptr<XRHIShaderResourceView> BufferSRV;

	std::shared_ptr <XRHIStructBuffer> GetBuffer() { return Buffer; }
	XRHIUnorderedAcessView* GetUAV() { return BufferUAV.get(); }
	XRHIShaderResourceView* GetSRV() { return BufferSRV.get(); }

	void Create(uint32 Stride, uint32 Size, XRHIResourceCreateData SrcData)
	{
		Buffer = RHIcreateStructBuffer(Stride, Size, EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), SrcData);
		BufferUAV = RHICreateUnorderedAccessView(Buffer.get(), false, false, 0);
		BufferSRV = RHICreateShaderResourceView(Buffer.get());
	}
};

class XVirtualShadowMapResource :public XRenderResource
{
public:
	std::shared_ptr<XRHIConstantBuffer> VSMClearParameters;
	std::shared_ptr<XRHIConstantBuffer> VSMVisualizeParameters;
	std::shared_ptr<XRHIConstantBuffer> VSMTileShadowViewConstantBuffer;
	std::shared_ptr<XRHIConstantBuffer> VSMCullingParameters;

	std::shared_ptr <XRHIStructBuffer> VirtualShadowMapTileStateCacheMiss;
	std::shared_ptr<XRHIUnorderedAcessView> VirtualShadowMapTileStateCacheMissUAV;
	std::shared_ptr<XRHIShaderResourceView> VirtualShadowMapTileStateCacheMissSRV;

	TResourceVector<uint32> FreeListInitData;
	SVirtualShadowMapStructBuffer VirtualShadowMapTileAction;
	SVirtualShadowMapStructBuffer VirtualShadowMapFreeTileList;
	SVirtualShadowMapStructBuffer VirtualShadowMapFreeListStart;

	std::shared_ptr<XRHICommandSignature>RHIShadowCommandSignature;
	std::shared_ptr<XRHITexture2D>PlaceHodeltarget;

	SVirtualShadowMapStructBuffer VirtualShadowMapCommnadBufferUnculled;
	SVirtualShadowMapStructBuffer VirtualShadowMapCommnadBufferCulled;
	SVirtualShadowMapStructBuffer VirtualShadowMapCommnadCounter;	

	std::shared_ptr<XRHITexture2D> PhysicalShadowDepthTexture;

	// Clear Physiacl Shadow Texture
	SVirtualShadowMapStructBuffer VirtualShadowMapTileTablePacked;
	SVirtualShadowMapStructBuffer TileNeedUpdateCounter;

	std::shared_ptr<XRHIConstantBuffer> LightSubProjectMatrix;
	void InitRHI()override
	{
		BufferIndex = 0;
		uint32 MipNum = VSM_MIP_NUM;
		uint32 TotalTileNum = 0;
		for (uint32 Index = 0; Index < MipNum; Index++)
		{
			uint32 MipTileWidth = (VSM_TILE_MAX_MIP_NUM_XY << (VSM_MIP_NUM - (Index + 1)));
			TotalTileNum += MipTileWidth * MipTileWidth;
		}
		VSMClearParameters = RHICreateConstantBuffer(sizeof(SVSMClearParameters));
		VSMVisualizeParameters = RHICreateConstantBuffer(sizeof(SVSMVisualizeParameters));
		VSMTileShadowViewConstantBuffer = RHICreateConstantBuffer(sizeof(SShadowViewInfo));
		VSMCullingParameters = RHICreateConstantBuffer(sizeof(SCullingParameters));

		uint32 ResourceSize = sizeof(SLightSubProjectMatrix) * (32 * 32 + 16 * 16 + 8 * 8);
		LightSubProjectMatrix = RHICreateConstantBuffer(ResourceSize);
		for (int Index = 0; Index < 2; Index++)
		{
			VirtualShadowMapTileState[Index] = RHIcreateStructBuffer(sizeof(uint32), sizeof(uint32) * TotalTileNum, EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);
			VirtualShadowMapTileStateUAV[Index] = RHICreateUnorderedAccessView(VirtualShadowMapTileState[Index].get(), false, false, 0);
			VirtualShadowMapTileStateSRV[Index] = RHICreateShaderResourceView(VirtualShadowMapTileState[Index].get());
		}

		for (int Index = 0; Index < 2; Index++)
		{
			VirtualShadowMapTileTable[Index] = RHIcreateStructBuffer(sizeof(uint32), sizeof(uint32) * TotalTileNum, EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);
			VirtualShadowMapTileTableUAV[Index] = RHICreateUnorderedAccessView(VirtualShadowMapTileTable[Index].get(), false, false, 0);
			VirtualShadowMapTileTableSRV[Index] = RHICreateShaderResourceView(VirtualShadowMapTileTable[Index].get());
		}


		VirtualShadowMapTileStateCacheMiss = RHIcreateStructBuffer(sizeof(uint32), sizeof(uint32) * TotalTileNum, EBufferUsage(int(EBufferUsage::BUF_StructuredBuffer) | int(EBufferUsage::BUF_UnorderedAccess)), nullptr);
		VirtualShadowMapTileStateCacheMissUAV = RHICreateUnorderedAccessView(VirtualShadowMapTileStateCacheMiss.get(), false, false, 0);
		VirtualShadowMapTileStateCacheMissSRV = RHICreateShaderResourceView(VirtualShadowMapTileStateCacheMiss.get());

		VirtualShadowMapTileAction.Create(sizeof(uint32), sizeof(uint32) * TotalTileNum, nullptr);

		for (uint32 PhysicalIndexY = 0; PhysicalIndexY < VSM_TEX_PHYSICAL_WH; PhysicalIndexY++)
		{
			for (uint32 PhysicalIndexX = 0; PhysicalIndexX < VSM_TEX_PHYSICAL_WH; PhysicalIndexX++)
			{
				FreeListInitData.PushBack(PhysicalIndexX | (PhysicalIndexY << 16));
			}
		}
		
		XRHIResourceCreateData SrcData(&FreeListInitData);
		VirtualShadowMapFreeTileList.Create(sizeof(uint32), sizeof(uint32) * VSM_TEX_PHYSICAL_WH* VSM_TEX_PHYSICAL_WH, SrcData);

		VirtualShadowMapFreeListStart.Create(sizeof(uint32), 4096, nullptr);
		VirtualShadowMapCommnadCounter.Create(sizeof(uint32), 4096, nullptr);

		VirtualShadowMapTileTablePacked.Create(sizeof(uint32), 4096, nullptr);
		TileNeedUpdateCounter.Create(sizeof(uint32), 4096, nullptr);

		PlaceHodeltarget = RHICreateTexture2D(VSM_TILE_TEX_PHYSICAL_SIZE, VSM_TILE_TEX_PHYSICAL_SIZE, 1, false, false, EPixelFormat::FT_R8G8B8A8_UNORM, ETextureCreateFlags(TexCreate_RenderTargetable), 1, nullptr);
	}

	std::shared_ptr <XRHIStructBuffer> GetVSMTileStateBuffer(bool bPrevious) { return VirtualShadowMapTileState[(bPrevious ? ((BufferIndex + 1) % 2) : BufferIndex)]; };
	XRHIUnorderedAcessView* GetVSMTileStateUAV(bool bPrevious) { return VirtualShadowMapTileStateUAV[(bPrevious ? ((BufferIndex + 1) % 2) : BufferIndex)].get(); };
	XRHIShaderResourceView* GetVSMTileStateSRV(bool bPrevious) { return VirtualShadowMapTileStateSRV[(bPrevious ? ((BufferIndex + 1) % 2) : BufferIndex)].get(); }

	std::shared_ptr <XRHIStructBuffer> GetVSMTileTableBuffer(bool bPrevious) { return VirtualShadowMapTileTable[(bPrevious ? ((BufferIndex + 1) % 2) : BufferIndex)]; };
	XRHIUnorderedAcessView* GetVSMTileTableUAV(bool bPrevious) { return VirtualShadowMapTileTableUAV[(bPrevious ? ((BufferIndex + 1) % 2) : BufferIndex)].get(); };
	XRHIShaderResourceView* GetVSMTileTableSRV(bool bPrevious) { return VirtualShadowMapTileTableSRV[(bPrevious ? ((BufferIndex + 1) % 2) : BufferIndex)].get(); }

	void UpdateBufferIndex()
	{
		BufferIndex = (BufferIndex + 1) % 2;
	}

private:
	std::shared_ptr <XRHIStructBuffer> VirtualShadowMapTileState[2];
	std::shared_ptr<XRHIUnorderedAcessView> VirtualShadowMapTileStateUAV[2];
	std::shared_ptr<XRHIShaderResourceView> VirtualShadowMapTileStateSRV[2];

	std::shared_ptr <XRHIStructBuffer> VirtualShadowMapTileTable[2];
	std::shared_ptr<XRHIUnorderedAcessView> VirtualShadowMapTileTableUAV[2];
	std::shared_ptr<XRHIShaderResourceView> VirtualShadowMapTileTableSRV[2];

	uint32 BufferIndex;
};

TGlobalResource<XVirtualShadowMapResource> VirtualShadowMapResource;

class XVirtualShadowMapResourceClear :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapResourceClear(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	struct SParameters
	{
		XRHIConstantBuffer* VirtualShadowmapClearParameters;

		XRHIUnorderedAcessView* VirtualShadowMapTileState;
		XRHIUnorderedAcessView* VirtualShadowMapTileStateCacheMiss;
		
		XRHIUnorderedAcessView* VirtualShadowMapTileTable;
		XRHIUnorderedAcessView* VirtualShadowMapTileAction;

		XRHIUnorderedAcessView* CommandCounterBuffer;
		XRHIUnorderedAcessView* TileNeedUpdateCounter_UAV;
	};

	XVirtualShadowMapResourceClear(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VSMClearParameters.Bind(Initializer.ShaderParameterMap, "VSMClearParameters");
		VirtualShadowMapTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileState");
		VirtualShadowMapTileStateCacheMiss.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileStateCacheMiss");
		VirtualShadowMapTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTable");
		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");
		CommandCounterBuffer.Bind(Initializer.ShaderParameterMap, "CommandCounterBuffer");
		TileNeedUpdateCounter_UAV.Bind(Initializer.ShaderParameterMap, "TileNeedUpdateCounter_UAV");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters Parameters)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, VSMClearParameters, Parameters.VirtualShadowmapClearParameters);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, Parameters.VirtualShadowMapTileState);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileStateCacheMiss, Parameters.VirtualShadowMapTileStateCacheMiss);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTable, Parameters.VirtualShadowMapTileTable);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, Parameters.VirtualShadowMapTileAction);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, CommandCounterBuffer, Parameters.CommandCounterBuffer);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, TileNeedUpdateCounter_UAV, Parameters.TileNeedUpdateCounter_UAV);
	}

	CBVParameterType VSMClearParameters;
	UAVParameterType VirtualShadowMapTileState;
	UAVParameterType VirtualShadowMapTileStateCacheMiss;
	UAVParameterType VirtualShadowMapTileTable;
	UAVParameterType VirtualShadowMapTileAction;
	UAVParameterType CommandCounterBuffer;
	UAVParameterType TileNeedUpdateCounter_UAV;
};
XVirtualShadowMapResourceClear::ShaderInfos XVirtualShadowMapResourceClear::StaticShaderInfos("VSMResourceClear", GET_SHADER_PATH("VirtualShadowMapResourceClear.hlsl"), "VSMResourceClear", EShaderType::SV_Compute, XVirtualShadowMapResourceClear::CustomConstrucFunc, XVirtualShadowMapResourceClear::ModifyShaderCompileSettings);


class XVirtualShadowMapTileMark :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapTileMark(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XVirtualShadowMapTileMark(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		CBView.Bind(Initializer.ShaderParameterMap, "cbView");
		CBShadowViewInfo.Bind(Initializer.ShaderParameterMap, "CbShadowViewInfo");
		SceneDepthTexture.Bind(Initializer.ShaderParameterMap, "SceneDepthTexture");
		VirtualShadowMapTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileState");
	}

	void SetParameters(XRHICommandList& RHICommandList, XRHIConstantBuffer* InputSceneViewBuffer, XRHIConstantBuffer* InputShadowViewInfoBuffer, XRHIUnorderedAcessView* InputVirtualShadowmapTileState, XRHITexture* InputSceneDepthTexture)
	{
		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthTexture, InputSceneDepthTexture);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBView, InputSceneViewBuffer);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBShadowViewInfo, InputShadowViewInfoBuffer);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, InputVirtualShadowmapTileState);
	}

	CBVParameterType CBView;
	CBVParameterType CBShadowViewInfo;
	TextureParameterType SceneDepthTexture;
	UAVParameterType VirtualShadowMapTileState;
};

XVirtualShadowMapTileMark::ShaderInfos XVirtualShadowMapTileMark::StaticShaderInfos("VSMTileMaskCS", GET_SHADER_PATH("VirtualShadowMapTileMark.hlsl"), "VSMTileMaskCS", EShaderType::SV_Compute, XVirtualShadowMapTileMark::CustomConstrucFunc, XVirtualShadowMapTileMark::ModifyShaderCompileSettings);

class XVirtualShadowMapTileCacheMiss :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapTileCacheMiss(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	XVirtualShadowMapTileCacheMiss(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		CBDynamicObjectParameters.Bind(Initializer.ShaderParameterMap, "CBDynamicObjectParameters");
		CBShadowViewInfo.Bind(Initializer.ShaderParameterMap, "CbShadowViewInfo");
		VirtualShadowMapTileStateCacheMiss.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileStateCacheMiss");
	}

	void SetParameters(XRHICommandList& RHICommandList, XRHIConstantBuffer* InputDynamicObjectParameters, XRHIConstantBuffer* InputShadowViewInfoBuffer, XRHIUnorderedAcessView* InputVirtualShadowMapTileStateCacheMiss)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBDynamicObjectParameters, InputDynamicObjectParameters);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBShadowViewInfo, InputShadowViewInfoBuffer);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileStateCacheMiss, InputVirtualShadowMapTileStateCacheMiss);
	}

	CBVParameterType CBDynamicObjectParameters;
	CBVParameterType CBShadowViewInfo;
	UAVParameterType VirtualShadowMapTileStateCacheMiss;
};

XVirtualShadowMapTileCacheMiss::ShaderInfos XVirtualShadowMapTileCacheMiss::StaticShaderInfos("VSMTileCacheMissCS", GET_SHADER_PATH("VirtualShadowMapTileCacheMiss.hlsl"), "VSMTileCacheMissCS", EShaderType::SV_Compute, XVirtualShadowMapTileCacheMiss::CustomConstrucFunc, XVirtualShadowMapTileCacheMiss::ModifyShaderCompileSettings);

class XVirtualShadowMapUpdateTileAction :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapUpdateTileAction(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	struct SParameters
	{
		XRHIShaderResourceView* VirtualShadowMapTileState;
		XRHIShaderResourceView* VirtualShadowMapTileStateCacheMiss;

		XRHIShaderResourceView* VirtualShadowMapPreTileState;
		XRHIShaderResourceView* VirtualShadowMapPreTileTable;

		XRHIUnorderedAcessView* VirtualShadowMapTileTable;
		XRHIUnorderedAcessView* VirtualShadowMapTileAction;
	};

	XVirtualShadowMapUpdateTileAction(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualShadowMapTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileState");
		VirtualShadowMapTileStateCacheMiss.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileStateCacheMiss");

		VirtualShadowMapPreTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapPreTileState");
		VirtualShadowMapPreTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapPreTileTable");

		VirtualShadowMapTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTable");
		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, ShaderParameters.VirtualShadowMapTileState);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileStateCacheMiss, ShaderParameters.VirtualShadowMapTileStateCacheMiss);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapPreTileState, ShaderParameters.VirtualShadowMapPreTileState);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapPreTileTable, ShaderParameters.VirtualShadowMapPreTileTable);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTable, ShaderParameters.VirtualShadowMapTileTable);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, ShaderParameters.VirtualShadowMapTileAction);
	}


	SRVParameterType VirtualShadowMapTileState;
	SRVParameterType VirtualShadowMapTileStateCacheMiss;
	
	SRVParameterType VirtualShadowMapPreTileState;
	SRVParameterType VirtualShadowMapPreTileTable;
	
	UAVParameterType VirtualShadowMapTileTable;
	UAVParameterType VirtualShadowMapTileAction;
};
XVirtualShadowMapUpdateTileAction::ShaderInfos XVirtualShadowMapUpdateTileAction::StaticShaderInfos("VSMUpdateTileActionCS", GET_SHADER_PATH("VirtualShadowMapUpdateTileAction.hlsl"), "VSMUpdateTileActionCS", EShaderType::SV_Compute, XVirtualShadowMapUpdateTileAction::CustomConstrucFunc, XVirtualShadowMapUpdateTileAction::ModifyShaderCompileSettings);

class XVirtualShadowMapPhysicalTileAlloc :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapPhysicalTileAlloc(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	struct SParameters
	{
		XRHIShaderResourceView* VirtualShadowMapTileAction;
		XRHIUnorderedAcessView* VirtualShadowMapTileTable;
		XRHIUnorderedAcessView* VirtualShadowMapFreeTileList;
		XRHIUnorderedAcessView* VirtualShadowMapFreeListStart;
	};

	XVirtualShadowMapPhysicalTileAlloc(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");

		VirtualShadowMapTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTable");
		VirtualShadowMapFreeTileList.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapFreeTileList");
		VirtualShadowMapFreeListStart.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapFreeListStart");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, ShaderParameters.VirtualShadowMapTileAction);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTable, ShaderParameters.VirtualShadowMapTileTable);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapFreeTileList, ShaderParameters.VirtualShadowMapFreeTileList);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapFreeListStart, ShaderParameters.VirtualShadowMapFreeListStart);
	}


	SRVParameterType VirtualShadowMapTileAction;

	UAVParameterType VirtualShadowMapTileTable;
	UAVParameterType VirtualShadowMapFreeTileList;
	UAVParameterType VirtualShadowMapFreeListStart;
};
XVirtualShadowMapPhysicalTileAlloc::ShaderInfos XVirtualShadowMapPhysicalTileAlloc::StaticShaderInfos("VSMTilePhysicalTileAllocCS", GET_SHADER_PATH("VirtualShadowMapPhysicalMemoryManage.hlsl"), "VSMTilePhysicalTileAllocCS", EShaderType::SV_Compute, XVirtualShadowMapPhysicalTileAlloc::CustomConstrucFunc, XVirtualShadowMapPhysicalTileAlloc::ModifyShaderCompileSettings);

class XVirtualShadowMapPhysicalTileRelease :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapPhysicalTileRelease(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	struct SParameters
	{
		XRHIShaderResourceView* VirtualShadowMapTileAction;
		XRHIShaderResourceView* VirtualShadowMapPreTileTable;

		XRHIUnorderedAcessView* VirtualShadowMapFreeTileList;
		XRHIUnorderedAcessView* VirtualShadowMapFreeListStart;
	};

	XVirtualShadowMapPhysicalTileRelease(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");
		VirtualShadowMapPreTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapPreTileTable");

		VirtualShadowMapFreeTileList.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapFreeTileList");
		VirtualShadowMapFreeListStart.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapFreeListStart");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, ShaderParameters.VirtualShadowMapTileAction);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapPreTileTable, ShaderParameters.VirtualShadowMapPreTileTable);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapFreeTileList, ShaderParameters.VirtualShadowMapFreeTileList);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapFreeListStart, ShaderParameters.VirtualShadowMapFreeListStart);
	}


	SRVParameterType VirtualShadowMapTileAction;
	SRVParameterType VirtualShadowMapPreTileTable;

	UAVParameterType VirtualShadowMapFreeTileList;
	UAVParameterType VirtualShadowMapFreeListStart;
};
XVirtualShadowMapPhysicalTileRelease::ShaderInfos XVirtualShadowMapPhysicalTileRelease::StaticShaderInfos("VSMTilePhysicalTileReleaseCS", GET_SHADER_PATH("VirtualShadowMapPhysicalMemoryManage.hlsl"), "VSMTilePhysicalTileReleaseCS", EShaderType::SV_Compute, XVirtualShadowMapPhysicalTileRelease::CustomConstrucFunc, XVirtualShadowMapPhysicalTileRelease::ModifyShaderCompileSettings);

class XVirtualShadowMapCmdBuild :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapCmdBuild(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	struct SParameters
	{
		XRHIConstantBuffer* CBDynamicObjectParameters;
		XRHIConstantBuffer* CBCullingParameters;

		XRHIShaderResourceView* VirtualShadowMapTileAction;
		XRHIShaderResourceView* SceneConstantBufferIn;
		XRHIShaderResourceView* InputCommands;

		XRHIUnorderedAcessView* OutputCommands;
		XRHIUnorderedAcessView* CommandCounterBuffer;
	};

	XVirtualShadowMapCmdBuild(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		CBDynamicObjectParameters.Bind(Initializer.ShaderParameterMap, "CBDynamicObjectParameters");
		CBCullingParameters.Bind(Initializer.ShaderParameterMap, "CBCullingParameters");

		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");
		SceneConstantBufferIn.Bind(Initializer.ShaderParameterMap, "SceneConstantBufferIn");
		InputCommands.Bind(Initializer.ShaderParameterMap, "InputCommands");

		OutputCommands.Bind(Initializer.ShaderParameterMap, "OutputCommands");
		CommandCounterBuffer.Bind(Initializer.ShaderParameterMap, "CommandCounterBuffer");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBDynamicObjectParameters, ShaderParameters.CBDynamicObjectParameters);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBCullingParameters, ShaderParameters.CBCullingParameters);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, ShaderParameters.VirtualShadowMapTileAction);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SceneConstantBufferIn, ShaderParameters.SceneConstantBufferIn);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, InputCommands, ShaderParameters.InputCommands);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, OutputCommands, ShaderParameters.OutputCommands);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, CommandCounterBuffer, ShaderParameters.CommandCounterBuffer);
	}

	CBVParameterType CBDynamicObjectParameters;
	CBVParameterType CBCullingParameters;

	SRVParameterType VirtualShadowMapTileAction;
	SRVParameterType SceneConstantBufferIn;
	SRVParameterType InputCommands;

	UAVParameterType OutputCommands;
	UAVParameterType CommandCounterBuffer;
};
XVirtualShadowMapCmdBuild::ShaderInfos XVirtualShadowMapCmdBuild::StaticShaderInfos("VSMTileCmdBuildCS", GET_SHADER_PATH("VirtualShadowMapBuildTileCmd.hlsl"), "VSMTileCmdBuildCS", EShaderType::SV_Compute, XVirtualShadowMapCmdBuild::CustomConstrucFunc, XVirtualShadowMapCmdBuild::ModifyShaderCompileSettings);

class XVSMTileTableUpdatedPackCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVSMTileTableUpdatedPackCS(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	struct SParameters
	{
		XRHIShaderResourceView* VirtualShadowMapTileTable;
		XRHIShaderResourceView* VirtualShadowMapTileAction;

		XRHIUnorderedAcessView* VirtualShadowMapTileTablePacked_UAV;
		XRHIUnorderedAcessView* TileNeedUpdateCounter_UAV;
	};
	
	XVSMTileTableUpdatedPackCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualShadowMapTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTable");
		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");

		VirtualShadowMapTileTablePacked_UAV.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTablePacked_UAV");
		TileNeedUpdateCounter_UAV.Bind(Initializer.ShaderParameterMap, "TileNeedUpdateCounter_UAV");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTable, ShaderParameters.VirtualShadowMapTileTable);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, ShaderParameters.VirtualShadowMapTileAction);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTablePacked_UAV, ShaderParameters.VirtualShadowMapTileTablePacked_UAV);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, TileNeedUpdateCounter_UAV, ShaderParameters.TileNeedUpdateCounter_UAV);
	}

	SRVParameterType VirtualShadowMapTileTable;
	SRVParameterType VirtualShadowMapTileAction;

	UAVParameterType VirtualShadowMapTileTablePacked_UAV;
	UAVParameterType TileNeedUpdateCounter_UAV;
};
XVSMTileTableUpdatedPackCS::ShaderInfos XVSMTileTableUpdatedPackCS::StaticShaderInfos("VSMTileTableUpdatedPackCS", GET_SHADER_PATH("VirtualShadowMapPhysicalTileClear.hlsl"), "VSMTileTableUpdatedPackCS", EShaderType::SV_Compute, XVSMTileTableUpdatedPackCS::CustomConstrucFunc, XVSMTileTableUpdatedPackCS::ModifyShaderCompileSettings);

class XVSMPhysicalTileClearCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVSMPhysicalTileClearCS(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	struct SParameters
	{
		XRHIShaderResourceView* VirtualShadowMapTileTablePacked_SRV;
		XRHIShaderResourceView* TileNeedUpdateCounter_SRV;
		XRHIUnorderedAcessView* PhysicalShadowDepthTexture;
	};

	XVSMPhysicalTileClearCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VirtualShadowMapTileTablePacked_SRV.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTablePacked_SRV");
		TileNeedUpdateCounter_SRV.Bind(Initializer.ShaderParameterMap, "TileNeedUpdateCounter_SRV");
		PhysicalShadowDepthTexture.Bind(Initializer.ShaderParameterMap, "PhysicalShadowDepthTexture");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTablePacked_SRV, ShaderParameters.VirtualShadowMapTileTablePacked_SRV);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, TileNeedUpdateCounter_SRV, ShaderParameters.TileNeedUpdateCounter_SRV);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, PhysicalShadowDepthTexture, ShaderParameters.PhysicalShadowDepthTexture);
	}

	SRVParameterType VirtualShadowMapTileTablePacked_SRV;
	SRVParameterType TileNeedUpdateCounter_SRV;

	UAVParameterType PhysicalShadowDepthTexture;
};
XVSMPhysicalTileClearCS::ShaderInfos XVSMPhysicalTileClearCS::StaticShaderInfos("VSMPhysicalTileClearCS", GET_SHADER_PATH("VirtualShadowMapPhysicalTileClear.hlsl"), "VSMPhysicalTileClearCS", EShaderType::SV_Compute, XVSMPhysicalTileClearCS::CustomConstrucFunc, XVSMPhysicalTileClearCS::ModifyShaderCompileSettings);

class XVSMShadowMaskGenCS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVSMShadowMaskGenCS(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	struct SParameters
	{
		XRHIConstantBuffer* cbView;
		XRHIConstantBuffer* CbShadowViewInfo;

		XRHIShaderResourceView* GBufferNormal;
		XRHIShaderResourceView* SceneDepthInput;
		XRHIShaderResourceView* VirtualShadowMapTileTable;
		XRHIShaderResourceView* PhysicalShadowDepthTexture;

		XRHIUnorderedAcessView* VirtualShadowMapMaskTexture;
	};

	XVSMShadowMaskGenCS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		cbView.Bind(Initializer.ShaderParameterMap, "cbView");
		CbShadowViewInfo.Bind(Initializer.ShaderParameterMap, "CbShadowViewInfo");

		GBufferNormal.Bind(Initializer.ShaderParameterMap, "GBufferNormal");
		SceneDepthInput.Bind(Initializer.ShaderParameterMap, "SceneDepthInput");
		VirtualShadowMapTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTable");
		PhysicalShadowDepthTexture.Bind(Initializer.ShaderParameterMap, "PhysicalShadowDepthTexture");

		VirtualShadowMapMaskTexture.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapMaskTexture");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, cbView, ShaderParameters.cbView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CbShadowViewInfo, ShaderParameters.CbShadowViewInfo);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, GBufferNormal, ShaderParameters.GBufferNormal);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthInput, ShaderParameters.SceneDepthInput);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileTable, ShaderParameters.VirtualShadowMapTileTable);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, PhysicalShadowDepthTexture, ShaderParameters.PhysicalShadowDepthTexture);
		
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapMaskTexture, ShaderParameters.VirtualShadowMapMaskTexture);
	}

	CBVParameterType cbView;
	CBVParameterType CbShadowViewInfo;

	SRVParameterType GBufferNormal;
	SRVParameterType SceneDepthInput;
	SRVParameterType VirtualShadowMapTileTable;
	SRVParameterType PhysicalShadowDepthTexture;

	UAVParameterType VirtualShadowMapMaskTexture;
};
XVSMShadowMaskGenCS::ShaderInfos XVSMShadowMaskGenCS::StaticShaderInfos("VSMShadowMaskGenCS", GET_SHADER_PATH("VirtualShadowMapShadowMaskGen.hlsl"), "VSMShadowMaskGenCS", EShaderType::SV_Compute, XVSMShadowMaskGenCS::CustomConstrucFunc, XVSMShadowMaskGenCS::ModifyShaderCompileSettings);


class XVirtualShadowMapRenderingVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapRenderingVS(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:

	XVirtualShadowMapRenderingVS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer) {}
	void SetParameters(XRHICommandList& RHICommandList) {}
};
XVirtualShadowMapRenderingVS::ShaderInfos XVirtualShadowMapRenderingVS::StaticShaderInfos("VSMVS", GET_SHADER_PATH("VirtualShadowMapRenderingVS.hlsl"), "VSMVS", EShaderType::SV_Vertex, XVirtualShadowMapRenderingVS::CustomConstrucFunc, XVirtualShadowMapRenderingVS::ModifyShaderCompileSettings);

class XVirtualShadowMapRenderingPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer) { return new XVirtualShadowMapRenderingPS(Initializer); }
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}
public:
	struct SParameters
	{
		XRHIShaderResourceView* VirtualShadowMapTileTable;
		XRHIUnorderedAcessView* PhysicalShadowDepthTexture;
	};

	XVirtualShadowMapRenderingPS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer) 
	{ 
		VirtualShadowMapTileTable.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileTable");
		PhysicalShadowDepthTexture.Bind(Initializer.ShaderParameterMap, "PhysicalShadowDepthTexture");
	}
	void SetParameters(XRHICommandList& RHICommandList, SParameters ShaderParameters) 
	{ 
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Pixel, VirtualShadowMapTileTable, ShaderParameters.VirtualShadowMapTileTable);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Pixel, PhysicalShadowDepthTexture, ShaderParameters.PhysicalShadowDepthTexture);
	}

	SRVParameterType VirtualShadowMapTileTable;
	UAVParameterType PhysicalShadowDepthTexture;
};
XVirtualShadowMapRenderingPS::ShaderInfos XVirtualShadowMapRenderingPS::StaticShaderInfos("VSMPS", GET_SHADER_PATH("VirtualShadowMapRenderingPS.hlsl"), "VSMPS", EShaderType::SV_Pixel, XVirtualShadowMapRenderingPS::CustomConstrucFunc, XVirtualShadowMapRenderingPS::ModifyShaderCompileSettings);


class XVirtualShadowMapVisualize :public XGloablShader
{
public:

	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapVisualize(Initializer);
	}

	static ShaderInfos StaticShaderInfos;

	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	struct SParameters
	{
		XRHIConstantBuffer* CbVisualizeParameters;
		XRHIConstantBuffer* CBView;
		XRHIConstantBuffer* CBShadowViewInfo;

		XRHIShaderResourceView* VirtualShadowMapTileState;
		XRHIShaderResourceView* VirtualShadowMapTileStateCacheMiss;
		XRHIShaderResourceView* VirtualShadowMapTileAction;

		XRHIShaderResourceView* VirtualShadowMapFreeTileList;
		XRHIShaderResourceView* VirtualShadowMapFreeListStart;

		XRHITexture* SceneDepthTexture;
		XRHIUnorderedAcessView* OutputVisualizeTexture;
	};

	XVirtualShadowMapVisualize(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		CbVisualizeParameters.Bind(Initializer.ShaderParameterMap, "CbVisualizeParameters");
		CBView.Bind(Initializer.ShaderParameterMap, "cbView");
		CBShadowViewInfo.Bind(Initializer.ShaderParameterMap, "CbShadowViewInfo");
		SceneDepthTexture.Bind(Initializer.ShaderParameterMap, "SceneDepthTexture");
		VirtualShadowMapTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileState");
		VirtualShadowMapTileStateCacheMiss.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileStateCacheMiss");
		VirtualShadowMapTileAction.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileAction");
		VirtualShadowMapFreeTileList.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapFreeTileList");
		VirtualShadowMapFreeListStart.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapFreeListStart");
		OutputVisualizeTexture.Bind(Initializer.ShaderParameterMap, "OutputVisualizeTexture");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters Parameters)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CbVisualizeParameters, Parameters.CbVisualizeParameters);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBView, Parameters.CBView);
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, CBShadowViewInfo, Parameters.CBShadowViewInfo);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, Parameters.VirtualShadowMapTileState);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileStateCacheMiss, Parameters.VirtualShadowMapTileStateCacheMiss);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileAction, Parameters.VirtualShadowMapTileAction);

		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapFreeTileList, Parameters.VirtualShadowMapFreeTileList);
		SetShaderSRVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapFreeListStart, Parameters.VirtualShadowMapFreeListStart);

		SetTextureParameter(RHICommandList, EShaderType::SV_Compute, SceneDepthTexture, Parameters.SceneDepthTexture);

		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, OutputVisualizeTexture, Parameters.OutputVisualizeTexture);
	}

	CBVParameterType CbVisualizeParameters;
	CBVParameterType CBView;
	CBVParameterType CBShadowViewInfo;
	
	SRVParameterType VirtualShadowMapTileState;
	SRVParameterType VirtualShadowMapTileStateCacheMiss;
	SRVParameterType VirtualShadowMapTileAction;

	SRVParameterType VirtualShadowMapFreeTileList;
	SRVParameterType VirtualShadowMapFreeListStart;

	TextureParameterType SceneDepthTexture;
	UAVParameterType OutputVisualizeTexture;
};
XVirtualShadowMapVisualize::ShaderInfos XVirtualShadowMapVisualize::StaticShaderInfos("VSMVisualizeCS", GET_SHADER_PATH("VirtualShadowMapVisualize.hlsl"), "VSMVisualizeCS", EShaderType::SV_Compute, XVirtualShadowMapVisualize::CustomConstrucFunc, XVirtualShadowMapVisualize::ModifyShaderCompileSettings);

static const uint32 MipLevelSize[3] = { 32,16,8 };
static const uint32 MipLevelOffset[3] = { 0,32 * 32, 32 * 32 + 16 * 16 };
static bool bInit = false;

static void MipTilesShadowViewProjMatrixBuild(XBoundSphere& SceneBoundingSphere, XMatrix& LightViewMatIn, XVector3 MainLightDir)
{
	// See XDeferredShadingRenderer::ViewInfoUpdate(GCamera& CameraIn)
	MainLightDir.Normalize();
	XVector3 LightPos = SceneBoundingSphere.Center + MainLightDir * SceneBoundingSphere.Radius * 1.0;

	float l = SceneBoundingSphere.Center.x - SceneBoundingSphere.Radius;
	float r = SceneBoundingSphere.Center.x + SceneBoundingSphere.Radius;
	float t = SceneBoundingSphere.Center.y + SceneBoundingSphere.Radius;
	float b = SceneBoundingSphere.Center.y - SceneBoundingSphere.Radius;
	float f = SceneBoundingSphere.Radius * 2;
	float n = 0.1f;

	XVector3 UpTemp(0, 1, 0);
	XVector3 WDir = -MainLightDir; WDir.Normalize();
	XVector3 UDir = UpTemp.Cross(WDir); UDir.Normalize();
	XVector3 VDir = WDir.Cross(UDir); VDir.Normalize();

	uint32 ResourceSize = sizeof(SLightSubProjectMatrix) * (32 * 32 + 16 * 16 + 8 * 8);

	std::vector<SLightSubProjectMatrix> ShadowTileViewParametersCPU;
	ShadowTileViewParametersCPU.resize(32 * 32 + 16 * 16 + 8 * 8);

	if (!bInit)
	{
		for (uint32 MipIndex = 0; MipIndex < VSM_MIP_NUM; MipIndex++)
		{
			uint32 MipTileWidth = VSM_TILE_MAX_MIP_NUM_XY << (VSM_MIP_NUM - (MipIndex + 1));
			float TileSize = SceneBoundingSphere.Radius * 2 / (float)(MipTileWidth);

			for (uint32 IndexX = 0; IndexX < MipTileWidth; IndexX++)
			{
				for (uint32 IndexY = 0; IndexY < MipTileWidth; IndexY++)
				{
					XMatrix TempSubLightProj = XMatrix::CreateOrthographicOffCenterLH(
						-TileSize * 0.5, TileSize * 0.5,
						-TileSize * 0.5, TileSize * 0.5,
						f, n);

					float SubL = l + (r - l) * ((IndexX) / float(MipTileWidth));
					float SubR = SubL + TileSize;
					float SubT = t - (t - b) * ((IndexY) / float(MipTileWidth));
					float SubB = SubT - TileSize;
					XVector3 SubLightPos = LightPos + UDir * (SubL + SubR) * 0.5 + VDir * (SubB + SubT) * 0.5;
					XMatrix SubLightMatrix = LightViewMatIn;

					XVector3 NegEyePosition = -SubLightPos;
					float NegQU = UDir.Dot(NegEyePosition);
					float NegQV = VDir.Dot(NegEyePosition);
					float NegQW = WDir.Dot(NegEyePosition);

					SubLightMatrix.m[3][0] = NegQU;
					SubLightMatrix.m[3][1] = NegQV;
					SubLightMatrix.m[3][2] = NegQW;


					XMatrix ViewProject = SubLightMatrix * TempSubLightProj;
					ShadowTileViewParametersCPU[MipLevelOffset[MipIndex] + (IndexY * MipLevelSize[MipIndex] + IndexX)].LightViewProjectMatrix = ViewProject;
					ShadowTileViewParametersCPU[MipLevelOffset[MipIndex] + (IndexY * MipLevelSize[MipIndex] + IndexX)].MipLevel = MipIndex;
					ShadowTileViewParametersCPU[MipLevelOffset[MipIndex] + (IndexY * MipLevelSize[MipIndex] + IndexX)].VirtualTableIndexX = IndexX;
					ShadowTileViewParametersCPU[MipLevelOffset[MipIndex] + (IndexY * MipLevelSize[MipIndex] + IndexX)].VirtualTableIndexY = IndexY;
				}
			}
		}


		VirtualShadowMapResource.LightSubProjectMatrix->UpdateData(ShadowTileViewParametersCPU.data(), ResourceSize, 0);
		bInit = true;
	}
};


void XDeferredShadingRenderer::VirutalShadowMapSetup()
{

	std::vector<XRHIIndirectArg> IndirectShadowArgs;
	IndirectShadowArgs.resize(6);
	// Proority SRV > UAV > CBV
	// We has a srv and a uav, so the cbv is 2
	IndirectShadowArgs[0].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[0].CBV.RootParameterIndex = 2;

	IndirectShadowArgs[1].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[1].CBV.RootParameterIndex = 3;

	IndirectShadowArgs[2].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[2].CBV.RootParameterIndex = 4;

	IndirectShadowArgs[3].type = IndirectArgType::Arg_VBV;
	IndirectShadowArgs[4].type = IndirectArgType::Arg_IBV;
	IndirectShadowArgs[5].type = IndirectArgType::Arg_Draw_Indexed;

	TShaderReference<XVirtualShadowMapRenderingVS> VertexShader = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapRenderingVS>();
	TShaderReference<XVirtualShadowMapRenderingPS> PixelShader = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapRenderingPS>();
	VirtualShadowMapResource.RHIShadowCommandSignature = RHICreateCommandSignature(IndirectShadowArgs.data(), IndirectShadowArgs.size(), VertexShader.GetVertexShader(), PixelShader.GetPixelShader());

	std::vector<XRHICommandData> RHICmdData;
	RHICmdData.resize(RenderGeos.size());
	for (int i = 0; i < RenderGeos.size(); i++)
	{
		auto& it = RenderGeos[i];
		RHICmdData[i].CBVs.push_back(it->GetAndUpdatePerObjectVertexCBuffer().get());
		RHICmdData[i].CBVs.push_back(VirtualShadowMapResource.LightSubProjectMatrix.get());
		RHICmdData[i].CBVs.push_back(VirtualShadowMapResource.LightSubProjectMatrix.get());

		auto VertexBufferPtr = it->GetRHIVertexBuffer();
		auto IndexBufferPtr = it->GetRHIIndexBuffer();
		RHICmdData[i].VB = VertexBufferPtr.get();
		RHICmdData[i].IB = IndexBufferPtr.get();
		RHICmdData[i].IndexCountPerInstance = it->GetIndexCount();
		RHICmdData[i].InstanceCount = 1;
		RHICmdData[i].StartIndexLocation = 0;
		RHICmdData[i].BaseVertexLocation = 0;
		RHICmdData[i].StartInstanceLocation = 0;
	}

	uint32 OutCmdDataSize;
	void* DataPtrret = RHIGetCommandDataPtr(RHICmdData, OutCmdDataSize);
	
	uint32 ShadowCommandUnculledSize = OutCmdDataSize* RenderGeos.size();
	FResourceVectorUint8 ShadowIndirectBufferData;
	ShadowIndirectBufferData.Data = DataPtrret;
	ShadowIndirectBufferData.SetResourceDataSize(ShadowCommandUnculledSize);
	XRHIResourceCreateData IndirectBufferResourceData(&ShadowIndirectBufferData);
	VirtualShadowMapResource.VirtualShadowMapCommnadBufferUnculled.Create(OutCmdDataSize, ShadowCommandUnculledSize, IndirectBufferResourceData);
	VirtualShadowMapResource.VirtualShadowMapCommnadBufferCulled.Create(OutCmdDataSize, ShadowCommandUnculledSize * 16, nullptr);
}


void XDeferredShadingRenderer::VirtualShadowMapUpdate()
{
	GFrameNum++;
	float ZPos = sin((GFrameNum % 512) / 512.0 * 3.1415926535 * 2.0) * 60.0 + 60.0;

	std::shared_ptr<GGeomertry> DynamicGeometry = RenderGeos[0];
	DynamicGeometry->SetWorldTranslate(XVector3(1.0, 1.0, ZPos));
}

void XDeferredShadingRenderer::VirtualShadowMapRendering(XRHICommandList& RHICmdList)
{
	SShadowViewInfo VirtualShadowMapInfo;
	VirtualShadowMapInfo.LighgViewProject = LightViewProjMat;
	VirtualShadowMapInfo.WorldCameraPosition = RViewInfo.ViewMats.GetViewOrigin();
	VirtualShadowMapInfo.ShadowLightDir = ShadowLightDir;
	VirtualShadowMapResource.VSMTileShadowViewConstantBuffer->UpdateData(&VirtualShadowMapInfo, sizeof(SShadowViewInfo), 0);

	SVSMClearParameters VSMClearParameters;
	VSMClearParameters.VirtualShadowMapTileStateSize = 32 * 32 + 16 * 16 + 8 * 8;
	VirtualShadowMapResource.VSMClearParameters->UpdateData(&VSMClearParameters, sizeof(SVSMClearParameters), 0);

	VirtualShadowMapResource.UpdateBufferIndex();

	SCullingParameters CullParameters;
	CullParameters.ShadowViewProject = LightViewProjMat;
	CullParameters.MeshCount = RenderGeos.size();
	VirtualShadowMapResource.VSMCullingParameters->UpdateData(&CullParameters, sizeof(SCullingParameters), 0);
	
	{
		RHICmdList.RHIEventBegin(1, "VSMResourceClear", sizeof("VSMResourceClear"));
		TShaderReference<XVirtualShadowMapResourceClear> VSMResourceClear = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapResourceClear>();
		SetComputePipelineStateFromCS(RHICmdList, VSMResourceClear.GetComputeShader());
		
		XVirtualShadowMapResourceClear::SParameters Parameters;
		Parameters.VirtualShadowmapClearParameters = VirtualShadowMapResource.VSMClearParameters.get();
		Parameters.VirtualShadowMapTileState = VirtualShadowMapResource.GetVSMTileStateUAV(false);
		Parameters.VirtualShadowMapTileStateCacheMiss = VirtualShadowMapResource.VirtualShadowMapTileStateCacheMissUAV.get();
		Parameters.VirtualShadowMapTileTable = VirtualShadowMapResource.GetVSMTileTableUAV(false);
		Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetUAV();
		Parameters.CommandCounterBuffer = VirtualShadowMapResource.VirtualShadowMapCommnadCounter.GetUAV();
		Parameters.TileNeedUpdateCounter_UAV = VirtualShadowMapResource.TileNeedUpdateCounter.GetUAV();
		VSMResourceClear->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader((VSMClearParameters.VirtualShadowMapTileStateSize + ((16 * 16) - 1)) / (16 * 16), 1, 1);

		// Insert a barrier for waw resource
		RHICmdList.TransitionResource(VirtualShadowMapResource.GetVSMTileStateBuffer(false), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.VirtualShadowMapTileStateCacheMiss, EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.GetVSMTileTableBuffer(false), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.VirtualShadowMapTileAction.GetBuffer(), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.VirtualShadowMapCommnadCounter.GetBuffer(), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.RHIEventEnd();
	}

	
	VirtualShadowMapTileMark(RHICmdList);
	VirtualShadowMapUpdateTileAction(RHICmdList);
	VirtualShadowMapPhysicalTileManage(RHICmdList);
	VirtualShadowMapBuildCmd(RHICmdList);
	VirtualShadowMapProjection(RHICmdList);
}

void XDeferredShadingRenderer::VirtualShadowMapTileMark(XRHICommandList& RHICmdList)
{
	{
		RHICmdList.RHIEventBegin(1, "VSMTileMaskCS", sizeof("VSMTileMaskCS"));
		TShaderReference<XVirtualShadowMapTileMark> VSMTileMaskCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapTileMark>();
		SetComputePipelineStateFromCS(RHICmdList, VSMTileMaskCS.GetComputeShader());
		VSMTileMaskCS->SetParameters(RHICmdList, RViewInfo.ViewConstantBuffer.get(), VirtualShadowMapResource.VSMTileShadowViewConstantBuffer.get(), VirtualShadowMapResource.GetVSMTileStateUAV(false), SceneTargets.TextureDepthStencil.get());
		RHICmdList.RHIDispatchComputeShader(static_cast<uint32>((RViewInfo.ViewWidth + 15) / 16), static_cast<uint32>((RViewInfo.ViewHeight + 15) / 16), 1);
		RHICmdList.RHIEventEnd();
	}

	{
		RHICmdList.RHIEventBegin(1, "VSMTileCacheMiss", sizeof("VSMTileCacheMiss"));
		TShaderReference<XVirtualShadowMapTileCacheMiss> VSMTileCacheMissCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapTileCacheMiss>();
		SetComputePipelineStateFromCS(RHICmdList, VSMTileCacheMissCS.GetComputeShader());
		VSMTileCacheMissCS->SetParameters(RHICmdList, RenderGeos[0]->GetAndUpdatePerObjectVertexCBuffer().get(), VirtualShadowMapResource.VSMTileShadowViewConstantBuffer.get(), VirtualShadowMapResource.VirtualShadowMapTileStateCacheMissUAV.get());
		RHICmdList.RHIDispatchComputeShader(1, 1, 1);
		RHICmdList.RHIEventEnd();
	}
}

void XDeferredShadingRenderer::VirtualShadowMapUpdateTileAction(XRHICommandList& RHICmdList)
{
	RHICmdList.RHIEventBegin(1, "VSMUpdateTileActionCS", sizeof("VSMUpdateTileActionCS"));
	TShaderReference<XVirtualShadowMapUpdateTileAction> VSMUpdateTileActionCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapUpdateTileAction>();
	SetComputePipelineStateFromCS(RHICmdList, VSMUpdateTileActionCS.GetComputeShader());

	XVirtualShadowMapUpdateTileAction::SParameters Parameters;
	Parameters.VirtualShadowMapTileState = VirtualShadowMapResource.GetVSMTileStateSRV(false);
	Parameters.VirtualShadowMapTileStateCacheMiss = VirtualShadowMapResource.VirtualShadowMapTileStateCacheMissSRV.get();
	
	Parameters.VirtualShadowMapPreTileState = VirtualShadowMapResource.GetVSMTileStateSRV(true);
	Parameters.VirtualShadowMapPreTileTable = VirtualShadowMapResource.GetVSMTileTableSRV(true);

	Parameters.VirtualShadowMapTileTable = VirtualShadowMapResource.GetVSMTileTableUAV(false);
	Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetUAV();

	VSMUpdateTileActionCS->SetParameters(RHICmdList, Parameters);
	RHICmdList.RHIDispatchComputeShader(21, 1, 1);
	RHICmdList.RHIEventEnd();
}

void XDeferredShadingRenderer::VirtualShadowMapPhysicalTileManage(XRHICommandList& RHICmdList)
{
	{
		RHICmdList.RHIEventBegin(1, "VSMTilePhysicalTileManageCS", sizeof("VSMTilePhysicalTileManageCS"));
		TShaderReference<XVirtualShadowMapPhysicalTileAlloc> VSMTilePhysicalTileAllocCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapPhysicalTileAlloc>();
		SetComputePipelineStateFromCS(RHICmdList, VSMTilePhysicalTileAllocCS.GetComputeShader());

		XVirtualShadowMapPhysicalTileAlloc::SParameters Parameters;
		Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetSRV();
		Parameters.VirtualShadowMapTileTable = VirtualShadowMapResource.GetVSMTileTableUAV(false);
		Parameters.VirtualShadowMapFreeTileList = VirtualShadowMapResource.VirtualShadowMapFreeTileList.GetUAV();
		Parameters.VirtualShadowMapFreeListStart = VirtualShadowMapResource.VirtualShadowMapFreeListStart.GetUAV();

		VSMTilePhysicalTileAllocCS->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader(21, 1, 1);
		RHICmdList.RHIEventEnd();
	}

	// Insert a barrier for waw resource
	RHICmdList.TransitionResource(VirtualShadowMapResource.VirtualShadowMapFreeTileList.GetBuffer(), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_SRV);
	
	{
		RHICmdList.RHIEventBegin(1, "VSMTilePhysicalTileReleaseCS", sizeof("VSMTilePhysicalTileReleaseCS"));
		TShaderReference<XVirtualShadowMapPhysicalTileRelease> VSMTilePhysicalTileReleaseCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapPhysicalTileRelease>();
		SetComputePipelineStateFromCS(RHICmdList, VSMTilePhysicalTileReleaseCS.GetComputeShader());
	
		XVirtualShadowMapPhysicalTileRelease::SParameters Parameters;
		Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetSRV();
		Parameters.VirtualShadowMapPreTileTable = VirtualShadowMapResource.GetVSMTileTableSRV(true);
		Parameters.VirtualShadowMapFreeTileList = VirtualShadowMapResource.VirtualShadowMapFreeTileList.GetUAV();
		Parameters.VirtualShadowMapFreeListStart = VirtualShadowMapResource.VirtualShadowMapFreeListStart.GetUAV();
	
		VSMTilePhysicalTileReleaseCS->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader(21, 1, 1);
		RHICmdList.RHIEventEnd();
	}
}

void XDeferredShadingRenderer::VirtualShadowMapBuildCmd(XRHICommandList& RHICmdList)
{
	MipTilesShadowViewProjMatrixBuild(SceneBoundingSphere, LightViewMat, ShadowLightDir);

	RHICmdList.RHIEventBegin(1, "VSMTileCmdBuildCS", sizeof("VSMTileCmdBuildCS"));
	TShaderReference<XVirtualShadowMapCmdBuild> VSMTileCmdBuildCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapCmdBuild>();
	SetComputePipelineStateFromCS(RHICmdList, VSMTileCmdBuildCS.GetComputeShader());

	XVirtualShadowMapCmdBuild::SParameters Parameters;
	Parameters.CBDynamicObjectParameters = RenderGeos[0]->GetPerObjectVertexCBuffer().get();
	Parameters.CBCullingParameters = VirtualShadowMapResource.VSMCullingParameters.get();
	Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetSRV();
	Parameters.SceneConstantBufferIn = GlobalObjectStructBufferSRV.get();
	Parameters.InputCommands = VirtualShadowMapResource.VirtualShadowMapCommnadBufferUnculled.GetSRV();
	Parameters.OutputCommands = VirtualShadowMapResource.VirtualShadowMapCommnadBufferCulled.GetUAV();
	Parameters.CommandCounterBuffer = VirtualShadowMapResource.VirtualShadowMapCommnadCounter.GetUAV();
	VSMTileCmdBuildCS->SetParameters(RHICmdList, Parameters);
	RHICmdList.RHIDispatchComputeShader(21, 50, 1);
	RHICmdList.RHIEventEnd();
}

void XDeferredShadingRenderer::VirtualShadowMapProjection(XRHICommandList& RHICmdList)
{
	// Packed The Tile Table Need Updated
	{
		RHICmdList.RHIEventBegin(1, "VSMTileTableUpdatedPackCS", sizeof("VSMTileTableUpdatedPackCS"));
		TShaderReference<XVSMTileTableUpdatedPackCS> VSMTileTableUpdatedPackCS = GetGlobalShaderMapping()->GetShader<XVSMTileTableUpdatedPackCS>();
		SetComputePipelineStateFromCS(RHICmdList, VSMTileTableUpdatedPackCS.GetComputeShader());
		XVSMTileTableUpdatedPackCS::SParameters Parameters;
		Parameters.VirtualShadowMapTileTable = VirtualShadowMapResource.GetVSMTileTableSRV(false);
		Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetSRV();
		Parameters.VirtualShadowMapTileTablePacked_UAV = VirtualShadowMapResource.VirtualShadowMapTileTablePacked.GetUAV();
		Parameters.TileNeedUpdateCounter_UAV = VirtualShadowMapResource.TileNeedUpdateCounter.GetUAV();
		VSMTileTableUpdatedPackCS->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader(21, 1, 1);
		RHICmdList.RHIEventEnd();
	}

	{
		RHICmdList.RHIEventBegin(1, "VSMPhysicalTileClearCS", sizeof("VSMPhysicalTileClearCS"));
		TShaderReference<XVSMPhysicalTileClearCS> VSMPhysicalTileClearCS = GetGlobalShaderMapping()->GetShader<XVSMPhysicalTileClearCS>();
		SetComputePipelineStateFromCS(RHICmdList, VSMPhysicalTileClearCS.GetComputeShader());
		XVSMPhysicalTileClearCS::SParameters Parameters;
		Parameters.VirtualShadowMapTileTablePacked_SRV = VirtualShadowMapResource.VirtualShadowMapTileTablePacked.GetSRV();
		Parameters.TileNeedUpdateCounter_SRV = VirtualShadowMapResource.TileNeedUpdateCounter.GetSRV();
		Parameters.PhysicalShadowDepthTexture = GetRHIUAVFromTexture(SceneTargets.PhysicalShadowDepthTexture.get());
		VSMPhysicalTileClearCS->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader(24, 16, 1);
		RHICmdList.RHIEventEnd();
	}

	// WAW Resource
	RHICmdList.TransitionResource(SceneTargets.PhysicalShadowDepthTexture, EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
	
	static bool IsFirstFram = true;

	//Indirect Draw
	{
		XRHITexture* PlaceHodeltargetTex = VirtualShadowMapResource.PlaceHodeltarget.get();
		XRHIRenderPassInfo RPInfos(1, &PlaceHodeltargetTex, ERenderTargetLoadAction::EClear, nullptr, EDepthStencilLoadAction::ENoAction);
		RHICmdList.RHIBeginRenderPass(RPInfos, "VSMIndirectDraw", sizeof("VSMIndirectDraw"));
		RHICmdList.CacheActiveRenderTargets(RPInfos);
		
		XGraphicsPSOInitializer GraphicsPSOInit;
		GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<>::GetRHI();
		GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();
		
		TShaderReference<XVirtualShadowMapRenderingVS> VertexShader = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapRenderingVS>();
		TShaderReference<XVirtualShadowMapRenderingPS> PixelShader = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapRenderingPS>();
		GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
		std::shared_ptr<XRHIVertexLayout> RefVertexLayout = DefaultVertexFactory.GetLayout(ELayoutType::Layout_Default);
		GraphicsPSOInit.BoundShaderState.RHIVertexLayout = RefVertexLayout.get();
		
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);

		XVirtualShadowMapRenderingPS::SParameters PixelParameters;
		PixelParameters.PhysicalShadowDepthTexture = GetRHIUAVFromTexture(SceneTargets.PhysicalShadowDepthTexture.get());
		PixelParameters.VirtualShadowMapTileTable = VirtualShadowMapResource.GetVSMTileTableSRV(false);
		PixelShader->SetParameters(RHICmdList, PixelParameters);

		RHICmdList.RHIExecuteIndirect(VirtualShadowMapResource.RHIShadowCommandSignature.get(), RenderGeos.size() * 16,
			VirtualShadowMapResource.VirtualShadowMapCommnadBufferCulled.GetBuffer().get(), 0,
			VirtualShadowMapResource.VirtualShadowMapCommnadCounter.GetBuffer().get(), 0);
		RHICmdList.RHIEndRenderPass();
	}

	{
		RHICmdList.RHIEventBegin(1, "VSMShadowMaskGenCS", sizeof("VSMShadowMaskGenCS"));
		TShaderReference<XVSMShadowMaskGenCS> VSMShadowMaskGenCS = GetGlobalShaderMapping()->GetShader<XVSMShadowMaskGenCS>();
		XRHIComputeShader* ComputeShader = VSMShadowMaskGenCS.GetComputeShader();
		SetComputePipelineStateFromCS(RHICmdList, ComputeShader);
		XVSMShadowMaskGenCS::SParameters Parameters;
		Parameters.cbView = RViewInfo.ViewConstantBuffer.get();
		Parameters.CbShadowViewInfo = VirtualShadowMapResource.VSMTileShadowViewConstantBuffer.get();
		Parameters.GBufferNormal = GetRHISRVFromTexture(SceneTargets.TextureGBufferA.get());
		Parameters.SceneDepthInput = GetRHISRVFromTexture(SceneTargets.TextureDepthStencil.get());
		Parameters.VirtualShadowMapTileTable = VirtualShadowMapResource.GetVSMTileTableSRV(false);
		Parameters.PhysicalShadowDepthTexture = GetRHISRVFromTexture(SceneTargets.PhysicalShadowDepthTexture.get());
		Parameters.VirtualShadowMapMaskTexture = GetRHIUAVFromTexture(SceneTargets.VSMShadowMaskTexture.get());
		VSMShadowMaskGenCS->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader(static_cast<uint32>(ceil(RViewInfo.ViewWidth / 16)), static_cast<uint32>(ceil(RViewInfo.ViewHeight / 16)), 1);
		RHICmdList.RHIEventEnd();
	}

}

void XDeferredShadingRenderer::VirtualShadowMapVisualize(XRHICommandList& RHICmdList)
{
	return;
	XRHITexture* TextureSceneColor = SceneTargets.TextureSceneColorDeffered.get();

	SVSMVisualizeParameters VSMVisualizeParameters;
	VSMVisualizeParameters.VisualizeType = 4;
	VSMVisualizeParameters.DispatchSizeX = static_cast<uint32>((RViewInfo.ViewWidth + 15) / 16);
	VSMVisualizeParameters.DispatchSizeY = static_cast<uint32>((RViewInfo.ViewHeight + 15) / 16);
	VirtualShadowMapResource.VSMVisualizeParameters->UpdateData(&VSMVisualizeParameters, sizeof(SVSMVisualizeParameters), 0);
	
	RHICmdList.RHIEventBegin(1, "VSMVisualizeCS", sizeof("VSMVisualizeCS"));
	TShaderReference<XVirtualShadowMapVisualize> VSMVisualizeCS = GetGlobalShaderMapping()->GetShader<XVirtualShadowMapVisualize>();
	SetComputePipelineStateFromCS(RHICmdList, VSMVisualizeCS.GetComputeShader());

	XVirtualShadowMapVisualize::SParameters Parameters;
	Parameters.CbVisualizeParameters = VirtualShadowMapResource.VSMVisualizeParameters.get();
	Parameters.CBView = RViewInfo.ViewConstantBuffer.get();
	Parameters.CBShadowViewInfo = VirtualShadowMapResource.VSMTileShadowViewConstantBuffer.get();

	Parameters.VirtualShadowMapTileState = VirtualShadowMapResource.GetVSMTileStateSRV(false);
	Parameters.VirtualShadowMapTileStateCacheMiss = VirtualShadowMapResource.VirtualShadowMapTileStateCacheMissSRV.get();
	Parameters.VirtualShadowMapTileAction = VirtualShadowMapResource.VirtualShadowMapTileAction.GetSRV();

	Parameters.VirtualShadowMapFreeTileList = VirtualShadowMapResource.VirtualShadowMapFreeTileList.GetSRV();
	Parameters.VirtualShadowMapFreeListStart = VirtualShadowMapResource.VirtualShadowMapFreeListStart.GetSRV();

	Parameters.OutputVisualizeTexture = GetRHIUAVFromTexture(TextureSceneColor);
	Parameters.SceneDepthTexture = SceneTargets.TextureDepthStencil.get();

	VSMVisualizeCS->SetParameters(RHICmdList, Parameters);
	RHICmdList.RHIDispatchComputeShader(VSMVisualizeParameters.DispatchSizeX, VSMVisualizeParameters.DispatchSizeY, 1);
	RHICmdList.RHIEventEnd();
}
