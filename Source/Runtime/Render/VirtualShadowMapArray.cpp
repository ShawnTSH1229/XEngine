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

	std::shared_ptr <XRHIStructBuffer> VirtualShadowMapTileStateCacheMiss;
	std::shared_ptr<XRHIUnorderedAcessView> VirtualShadowMapTileStateCacheMissUAV;
	std::shared_ptr<XRHIShaderResourceView> VirtualShadowMapTileStateCacheMissSRV;

	TResourceVector<uint32> FreeListInitData;
	SVirtualShadowMapStructBuffer VirtualShadowMapTileAction;
	SVirtualShadowMapStructBuffer VirtualShadowMapFreeTileList;
	SVirtualShadowMapStructBuffer VirtualShadowMapFreeListStart;


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

	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapResourceClear(Initializer);
	}

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
	};

	XVirtualShadowMapResourceClear(const XShaderInitlizer& Initializer) :XGloablShader(Initializer)
	{
		VSMClearParameters.Bind(Initializer.ShaderParameterMap, "VSMClearParameters");
		VirtualShadowMapTileState.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileState");
		VirtualShadowMapTileStateCacheMiss.Bind(Initializer.ShaderParameterMap, "VirtualShadowMapTileStateCacheMiss");
	}

	void SetParameters(XRHICommandList& RHICommandList, SParameters Parameters)
	{
		SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Compute, VSMClearParameters, Parameters.VirtualShadowmapClearParameters);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, Parameters.VirtualShadowMapTileState);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileStateCacheMiss, Parameters.VirtualShadowMapTileStateCacheMiss);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileState, Parameters.VirtualShadowMapTileState);
		SetShaderUAVParameter(RHICommandList, EShaderType::SV_Compute, VirtualShadowMapTileStateCacheMiss, Parameters.VirtualShadowMapTileStateCacheMiss);
	}

	CBVParameterType VSMClearParameters;
	UAVParameterType VirtualShadowMapTileState;
	UAVParameterType VirtualShadowMapTileStateCacheMiss;
};
XVirtualShadowMapResourceClear::ShaderInfos XVirtualShadowMapResourceClear::StaticShaderInfos("VSMResourceClear", GET_SHADER_PATH("VirtualShadowMapResourceClear.hlsl"), "VSMResourceClear", EShaderType::SV_Compute, XVirtualShadowMapResourceClear::CustomConstrucFunc, XVirtualShadowMapResourceClear::ModifyShaderCompileSettings);


class XVirtualShadowMapTileMark :public XGloablShader
{
public:
	
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapTileMark(Initializer);
	}

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

	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapTileCacheMiss(Initializer);
	}

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

	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapUpdateTileAction(Initializer);
	}

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
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapPhysicalTileAlloc(Initializer);
	}

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
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapPhysicalTileRelease(Initializer);
	}

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
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XVirtualShadowMapCmdBuild(Initializer);
	}

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



void XDeferredShadingRenderer::VirutalShadowMapSetup()
{
	std::vector<XRHIIndirectArg> IndirectShadowArgs;

	IndirectShadowArgs.resize(4);
	IndirectShadowArgs[0].type = IndirectArgType::Arg_CBV;
	IndirectShadowArgs[0].CBV.RootParameterIndex = 2;
	IndirectShadowArgs[1].type = IndirectArgType::Arg_VBV;
	IndirectShadowArgs[2].type = IndirectArgType::Arg_IBV;
	IndirectShadowArgs[3].type = IndirectArgType::Arg_Draw_Indexed;
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
	VirtualShadowMapResource.VSMTileShadowViewConstantBuffer->UpdateData(&VirtualShadowMapInfo, sizeof(SShadowViewInfo), 0);

	SVSMClearParameters VSMClearParameters;
	VSMClearParameters.VirtualShadowMapTileStateSize = 32 * 32 + 16 * 16 + 8 * 8;
	VirtualShadowMapResource.VSMClearParameters->UpdateData(&VSMClearParameters, sizeof(SVSMClearParameters), 0);

	VirtualShadowMapResource.UpdateBufferIndex();

	
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

		VSMResourceClear->SetParameters(RHICmdList, Parameters);
		RHICmdList.RHIDispatchComputeShader((VSMClearParameters.VirtualShadowMapTileStateSize + ((16 * 16) - 1)) / (16 * 16), 1, 1);

		// Insert a barrier for waw resource
		RHICmdList.TransitionResource(VirtualShadowMapResource.GetVSMTileStateBuffer(false), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.VirtualShadowMapTileStateCacheMiss, EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.GetVSMTileTableBuffer(false), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);
		RHICmdList.TransitionResource(VirtualShadowMapResource.VirtualShadowMapTileAction.GetBuffer(), EResourceAccessFlag::RAF_UNKOWN, EResourceAccessFlag::RAF_COMMON);

		RHICmdList.RHIEventEnd();
	}

	VirtualShadowMapTileMark(RHICmdList);
	VirtualShadowMapUpdateTileAction(RHICmdList);
	VirtualShadowMapPhysicalTileManage(RHICmdList);
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

void XDeferredShadingRenderer::VirtualShadowMapVisualize(XRHICommandList& RHICmdList)
{
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
