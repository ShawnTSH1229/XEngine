#pragma once
#include "VulkanRHIPrivate.h"
#include "VulkanResource.h"

class XVulkanRayTracingScene;
class XVulkanRayTracingPipelineState;

struct XVkRTBLASBuildData
{
	std::vector<VkAccelerationStructureBuildRangeInfoKHR>Ranges;

	std::vector<VkAccelerationStructureGeometryKHR> Segments;
	VkAccelerationStructureBuildGeometryInfoKHR GeometryInfo;
	VkAccelerationStructureBuildSizesInfoKHR SizesInfo;
};

struct XVkRTTLASBuildData
{
	XVkRTTLASBuildData()
	{
		Geometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		GeometryInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		SizesInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	}

	VkAccelerationStructureGeometryKHR Geometry;
	VkAccelerationStructureBuildGeometryInfoKHR GeometryInfo;
	VkAccelerationStructureBuildSizesInfoKHR SizesInfo;
};

struct XVulkanHitGroupSystemParameters
{
	uint32 BindlessHitGroupSystemIndexBufferIndex;
	uint32 BindlessHitGroupSystemVertexBufferIndex;

	//uint32 BindlessUniformBuffers[16];
};

class XVulkanRayTracingGeometry : public XRHIRayTracingGeometry
{
public:
	XVulkanRayTracingGeometry(const XRayTracingGeometryInitializer& Initializer, XVulkanDevice* InDevice);

	std::shared_ptr<XRHIBuffer>AccelerationStructureBuffer;

	VkAccelerationStructureKHR Handle = nullptr;
	VkDeviceAddress Address = 0;

	void SetupHitGroupSystemParameters();
	void ReleaseBindlessHandles();

	virtual XRayTracingAccelerationStructureAddress GetAccelerationStructureAddress() const
	{
		XASSERT(false);
		return Address;
	}

	std::vector< XVulkanHitGroupSystemParameters>HitGroupSystemParameters;
	std::vector<XRHIDescriptorHandle>HitGroupSystemVertexViews;
	XRHIDescriptorHandle HitGroupSystemIndexView;

private:
	XVulkanDevice* const Device = nullptr;
};

class XVulkanRayTracingShaderTable
{
public:
	XVulkanRayTracingShaderTable(XVulkanDevice* InDevice);

	const VkStridedDeviceAddressRegionKHR* GetRegion(EShaderType ShaderType);

	void Init(const XVulkanRayTracingScene* Scene, const XVulkanRayTracingPipelineState* Pipeline);
	void SetSlot(EShaderType ShaderType, uint32 DstSlot, uint32 SrcHandleIndex, const std::vector<uint8>& SrcHandleData);

private:
	struct XVulkanShaderTableAllocation
	{
		XVulkanShaderTableAllocation()
		{
			memset(&Region, 0, sizeof(VkStridedDeviceAddressRegionKHR));
		}
		XVulkanAllocation Allocation;
		uint32 HandleCount = 0;
		bool UseLocalRecord = false;
		uint8* MappedBufferMemory = nullptr;
		VkStridedDeviceAddressRegionKHR Region;
		VkBuffer Buffer;
	};

	XVulkanShaderTableAllocation& GetAlloc(EShaderType ShaderType);

	XVulkanShaderTableAllocation RayGen;
	XVulkanShaderTableAllocation Miss;
	XVulkanShaderTableAllocation HitGroup;

	const uint32 HandleSize;
	const uint32 HandleSizeAligned;

	XVulkanDevice* Device;
};

class XVulkanRayTracingPipelineState : public XRHIRayTracingPSO
{
public:
	XVulkanRayTracingPipelineState(XVulkanDevice* const InDevice, const XRayTracingPipelineStateInitializer& Initializer);



	struct ShaderData
	{
		std::vector<std::shared_ptr<XRHIRayTracingShader>>Shaders;
		std::vector<uint8>ShaderHandles;
	};

	ShaderData GetShaderData(EShaderType ShaderType);
	int32 GetShaderIndex(XVulkanRayTracingShader* Shader);
	const std::vector<uint8>& GetShaderHandles(EShaderType ShaderType);

	ShaderData RayGen;
	ShaderData Miss;
	ShaderData HitGroup;
	ShaderData Callable;

	VkPipeline Pipeline;


	bool bAllowHitGroupIndexing = true;
};

class XVulkanRayTracingScene : public XRHIRayTracingScene
{
public:
	XVulkanRayTracingScene(XRayTracingSceneInitializer Initializer, XVulkanDevice* InDevice);
	~XVulkanRayTracingScene();

	//assign in bind buffer
	std::shared_ptr<XVulkanShaderResourceView> ShaderResourceView;

	void BindBuffer(std::shared_ptr<XRHIBuffer> InBuffer, uint32 InBufferOffset);
	
	void BuildAccelerationStructure(XVulkanCommandListContext& CmdContext, XVulkanResourceMultiBuffer* ScratchBuffer, uint32 ScratchOffset, XVulkanResourceMultiBuffer* InstanceBuffer, uint32 InstanceOffset);
	void BuildPerInstanceGeometryParameterBuffer(XVulkanCommandListContext& CmdContext);

	XVulkanRayTracingShaderTable* FindOrCreateShaderTable(const XVulkanRayTracingPipelineState* PipelineState);

	virtual const XRayTracingSceneInitializer GetRayTracingSceneInitializer()const
	{
		return Initializer;
	}

	//layer
	std::shared_ptr<XVulkanShaderResourceView>PerInstanceGeometryParameterSRV;
	std::shared_ptr<XVulkanResourceMultiBuffer> PerInstanceGeometryParameterBuffer;
	std::shared_ptr<XRHIBuffer>AccelerationStructureBuffer;
	
	std::map<const XVulkanRayTracingPipelineState*, XVulkanRayTracingShaderTable*>ShaderTables;

	const XRayTracingSceneInitializer Initializer;
private:
	XVulkanDevice* const Device = nullptr;
};




