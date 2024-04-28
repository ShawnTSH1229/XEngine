#include "VulkanPlatformRHI.h"
#include "VulkanRayTraing.h"
#include "VulkanDevice.h"
#include "VulkanLoader.h"
#include "VulkanCommandBuffer.h"
#include "Runtime\RHI\RHICommandList.h"

#if RHI_RAYTRACING
VkDeviceAddress XVulkanResourceMultiBuffer::GetDeviceAddress() const
{
	VkBufferDeviceAddressInfoKHR DeviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	DeviceAddressInfo.buffer = Buffer.VulkanHandle;
	return VulkanExtension::vkGetBufferDeviceAddressKHR(Device->GetVkDevice(), &DeviceAddressInfo);
}

static ERayTracingAccelerationStructureFlags GetRayTracingAccelerationStructureBuildFlags(const XRayTracingGeometryInitializer& Initializer)
{
	ERayTracingAccelerationStructureFlags BuildFlags = ERayTracingAccelerationStructureFlags::None;
	if (Initializer.bPreferBuild)
	{
		BuildFlags = ERayTracingAccelerationStructureFlags::PreferBuild;
	}
	else
	{
		BuildFlags = ERayTracingAccelerationStructureFlags::PreferTrace;
	}

	if (Initializer.bAllowUpdate)
	{
		BuildFlags = ERayTracingAccelerationStructureFlags(uint32(BuildFlags) | uint32(ERayTracingAccelerationStructureFlags::AllowUpdate));
	}

	if(!Initializer.bPreferBuild && !Initializer.bAllowUpdate && Initializer.bAllowCompaction )
	{
		BuildFlags = ERayTracingAccelerationStructureFlags(uint32(BuildFlags) | uint32(ERayTracingAccelerationStructureFlags::AllowCompaction));
	}

	return BuildFlags;
}

static void AddAccelerationStructureBuildBarrier(VkCommandBuffer CommandBuffer)
{
	VkMemoryBarrier Barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
	Barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	Barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
	vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &Barrier, 0, nullptr, 0, nullptr);
}

static void GetBLASBuildData(
	const VkDevice Device, 
	const std::vector<XRayTracingGeometrySegment>& Segments, 

	std::shared_ptr<XRHIBuffer>IndexBuffer,
	const uint32 IndexBufferOffset,
	const uint32 IndexStrideInBytes,

	ERayTracingAccelerationStructureFlags BuildFlag, 
	EAccelerationStructureBuildMode BuildMode,
	XVkRTBLASBuildData& BuildData)
{
	VkDeviceOrHostAddressConstKHR IndexBufferDeviceAddress = {};
	IndexBufferDeviceAddress.deviceAddress = (IndexBuffer.get() != nullptr) ? static_cast<XVulkanResourceMultiBuffer*>(IndexBuffer.get())->GetDeviceAddress() + IndexBufferOffset : 0;

	std::vector<uint32>PrimitiveCount;
	for (int32 Index = 0; Index < Segments.size(); Index++)
	{
		const XRayTracingGeometrySegment& Segment = Segments[Index];

		XVulkanResourceMultiBuffer* const VertexBuffer = static_cast<XVulkanResourceMultiBuffer*>(Segment.VertexBuffer.get());

		VkDeviceOrHostAddressConstKHR VertexBufferDeviceAddress = {};
		VertexBufferDeviceAddress.deviceAddress = VertexBuffer->GetDeviceAddress() + Segment.VertexBufferOffset;

		VkAccelerationStructureGeometryKHR SegmentGeometry = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		
		if (Segment.bForceOpaque)
		{
			SegmentGeometry.flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
		}

		SegmentGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

		SegmentGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		SegmentGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		SegmentGeometry.geometry.triangles.vertexData = VertexBufferDeviceAddress;
		SegmentGeometry.geometry.triangles.maxVertex = Segment.MaxVertices;
		SegmentGeometry.geometry.triangles.vertexStride = Segment.VertexBufferStride;
		SegmentGeometry.geometry.triangles.indexData = IndexBufferDeviceAddress;

		// No support for segment transform
		SegmentGeometry.geometry.triangles.transformData.deviceAddress = 0;
		SegmentGeometry.geometry.triangles.transformData.hostAddress = nullptr;

		uint32 PrimitiveOffset = 0;
		if (IndexBuffer.get() != nullptr)
		{
			SegmentGeometry.geometry.triangles.indexType = (IndexStrideInBytes == 2) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
			PrimitiveOffset = Segment.FirstPrimitive * 3 * IndexStrideInBytes;
		}
		else
		{
			XASSERT(false);
		}

		BuildData.Segments.push_back(SegmentGeometry);

		VkAccelerationStructureBuildRangeInfoKHR RangeInfo = {};
		RangeInfo.firstVertex = 0;
		RangeInfo.primitiveCount = (Segment.bEnabled) ? Segment.NumPrimitives : 0;
		RangeInfo.transformOffset = 0;
		BuildData.Ranges.push_back(RangeInfo);

		PrimitiveCount.push_back(Segment.NumPrimitives);
	}
	
	BuildData.GeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	BuildData.GeometryInfo.flags = (uint32(BuildFlag) & uint32(ERayTracingAccelerationStructureFlags::PreferBuild)) != 0 ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	BuildData.GeometryInfo.mode = (BuildMode == EAccelerationStructureBuildMode::Build) ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
	BuildData.GeometryInfo.geometryCount = BuildData.Segments.size();
	BuildData.GeometryInfo.pGeometries = BuildData.Segments.data();

	VulkanExtension::vkGetAccelerationStructureBuildSizesKHR(Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &BuildData.GeometryInfo, PrimitiveCount.data(), &BuildData.SizesInfo);
}

XRayTracingAccelerationStructSize XVulkanPlatformRHI::RHICalcRayTracingGeometrySize(const XRayTracingGeometryInitializer& Initializer)
{
	const uint32 IndexStrideInBytes = Initializer.IndexBuffer ? Initializer.IndexBuffer->GetStride() : 0;

	XVkRTBLASBuildData BuildData;
	GetBLASBuildData(GetDevice()->GetVkDevice(), Initializer.Segments, Initializer.IndexBuffer, Initializer.IndexBufferOffset, IndexStrideInBytes, GetRayTracingAccelerationStructureBuildFlags(Initializer), EAccelerationStructureBuildMode::Build, BuildData);
	
	XRayTracingAccelerationStructSize Result;
	Result.ResultSize = AlignArbitrary(BuildData.SizesInfo.accelerationStructureSize, GRHIRayTracingAccelerationStructureAlignment);
	Result.BuildScratchSize = AlignArbitrary(BuildData.SizesInfo.buildScratchSize, GRHIRayTracingScratchBufferAlignment);
	Result.UpdateScratchSize = AlignArbitrary(BuildData.SizesInfo.updateScratchSize, GRHIRayTracingScratchBufferAlignment);
	return Result;
}

static void GetTLASBuildData(
	const VkDevice Device,
	const uint32 NumInstances,
	const VkDeviceAddress InstanceBufferAddress,
	XVkRTTLASBuildData& BuildData)
{
	VkDeviceOrHostAddressConstKHR InstanceBufferDeviceAddress = {};
	InstanceBufferDeviceAddress.deviceAddress = InstanceBufferAddress;

	BuildData.Geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	BuildData.Geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	BuildData.Geometry.geometry.instances.arrayOfPointers = VK_FALSE;
	BuildData.Geometry.geometry.instances.data = InstanceBufferDeviceAddress;

	BuildData.GeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	BuildData.GeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	BuildData.GeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	BuildData.GeometryInfo.geometryCount = 1;
	BuildData.GeometryInfo.pGeometries = &BuildData.Geometry;

	VulkanExtension::vkGetAccelerationStructureBuildSizesKHR(
		Device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&BuildData.GeometryInfo,
		&NumInstances,
		&BuildData.SizesInfo);
}


XRayTracingAccelerationStructSize XVulkanPlatformRHI::RHICalcRayTracingSceneSize(uint32 MaxInstances, ERayTracingAccelerationStructureFlags Flags)
{
	XVkRTTLASBuildData BuildData;
	const VkDeviceAddress InstanceBufferAddress = 0; // No device address available when only querying TLAS size

	GetTLASBuildData(Device->GetVkDevice(), MaxInstances, InstanceBufferAddress, BuildData);

	XRayTracingAccelerationStructSize Result;
	Result.ResultSize = BuildData.SizesInfo.accelerationStructureSize;
	Result.BuildScratchSize = BuildData.SizesInfo.buildScratchSize;
	Result.UpdateScratchSize = BuildData.SizesInfo.updateScratchSize;

	return Result;
}

std::shared_ptr<XRHIRayTracingGeometry> XVulkanPlatformRHI::RHICreateRayTracingGeometry(const XRayTracingGeometryInitializer& Initializer)
{
	return std::make_shared<XVulkanRayTracingGeometry>(Initializer, GetDevice());
}

std::shared_ptr<XRHIRayTracingScene> XVulkanPlatformRHI::RHICreateRayTracingScene(XRayTracingSceneInitializer Initializer)
{
	return std::make_shared<XVulkanRayTracingScene>(Initializer, GetDevice());
}

XVulkanRayTracingPipelineState::XVulkanRayTracingPipelineState(XVulkanDevice* const InDevice, const XRayTracingPipelineStateInitializer& Initializer)
{
	const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InitializerRayGenShaders = Initializer.RayGenTable;
	const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InitializerRayMissShaders = Initializer.MissTable;
	const std::vector<std::shared_ptr<XRHIRayTracingShader>>& InitializerHitGroupShaders = Initializer.HitGroupTable;

	std::vector<VkPipelineShaderStageCreateInfo>ShaderStages;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR>ShaderGroups;

	std::vector<std::string>EntryPointName;

	for (auto const RayGenShaderRHI : InitializerRayGenShaders)
	{
		XASSERT(RayGenShaderRHI->GetShaderType() == EShaderType::SV_RayGen);

		XVulkanRayTracingShader* const RayGenShader = static_cast<XVulkanRayTracingShader*>(RayGenShaderRHI.get());
		EntryPointName.push_back(RayGenShader->GetEntryPoint());

		VkPipelineShaderStageCreateInfo ShaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ShaderStage.module = RayGenShader->GetOrCreateHandle();
		ShaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		ShaderStage.pName = EntryPointName.back().data();
		ShaderStages.push_back(ShaderStage);

		VkRayTracingShaderGroupCreateInfoKHR ShaderGroup = {VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
		ShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		ShaderGroup.generalShader = ShaderStages.size() - 1;//store general shader index
		ShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		ShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		ShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		ShaderGroups.push_back(ShaderGroup);

		RayGen.Shaders.push_back(RayGenShaderRHI);
	}

	for (auto const MissShaderRHI : InitializerRayMissShaders)
	{
		XASSERT(MissShaderRHI->GetShaderType() == EShaderType::SV_RayMiss);

		XVulkanRayTracingShader* const RayMissShader = static_cast<XVulkanRayTracingShader*>(MissShaderRHI.get());
		EntryPointName.push_back(RayMissShader->GetEntryPoint());

		VkPipelineShaderStageCreateInfo ShaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ShaderStage.module = RayMissShader->GetOrCreateHandle();
		ShaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		ShaderStage.pName = EntryPointName.back().data();
		ShaderStages.push_back(ShaderStage);

		VkRayTracingShaderGroupCreateInfoKHR ShaderGroup = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		ShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		ShaderGroup.generalShader = ShaderStages.size() - 1;//store general shader index
		ShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		ShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		ShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		ShaderGroups.push_back(ShaderGroup);

		Miss.Shaders.push_back(MissShaderRHI);
	}

	for (auto const HitGroupShaderRHI : InitializerHitGroupShaders)
	{
		XASSERT(HitGroupShaderRHI->GetShaderType() == EShaderType::SV_HitGroup);

		XVulkanRayTracingShader* const HitGroupShader = static_cast<XVulkanRayTracingShader*>(HitGroupShaderRHI.get());
		EntryPointName.push_back(HitGroupShader->GetEntryPoint());

		VkPipelineShaderStageCreateInfo ShaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ShaderStage.module = HitGroupShader->GetOrCreateHandle();
		ShaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		ShaderStage.pName = EntryPointName.back().data();
		ShaderStages.push_back(ShaderStage);

		VkRayTracingShaderGroupCreateInfoKHR ShaderGroup = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		ShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		ShaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		ShaderGroup.closestHitShader = ShaderStages.size() - 1;//store general shader index
		ShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		ShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		ShaderGroups.push_back(ShaderGroup);

		RayGen.Shaders.push_back(HitGroupShaderRHI);
	}

	VkRayTracingPipelineCreateInfoKHR RayTracingPipelineCreateInfo = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
	RayTracingPipelineCreateInfo.stageCount = ShaderStages.size();
	RayTracingPipelineCreateInfo.pStages = ShaderStages.data();
	RayTracingPipelineCreateInfo.groupCount = ShaderGroups.size();
	RayTracingPipelineCreateInfo.pGroups = ShaderGroups.data();
	RayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;//todo:fixme
	
	RayTracingPipelineCreateInfo.layout = InDevice->GetBindlessDescriptorMannager()->BindlessPipelineLayout;
	RayTracingPipelineCreateInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
	
	VULKAN_VARIFY(VulkanExtension::vkCreateRayTracingPipelinesKHR(InDevice->GetVkDevice(), {}, {}, 1, &RayTracingPipelineCreateInfo, nullptr, &Pipeline));

	{
		const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& RayTracingPipelineProps = InDevice->GetDeviceExtensionProperties().RayTracingPipeline;
		const uint32 HandleSize = RayTracingPipelineProps.shaderGroupHandleSize;

		uint32 HandleOffset = 0;

		auto FetchShaderhandle = [&HandleOffset, HandleSize, InDevice](VkPipeline RTPipeline, uint32 HandleCount)
		{
			std::vector<uint8> OutHandleStorage;
			uint32 OutHandleStorageSize = HandleCount * HandleSize;
			OutHandleStorage.resize(OutHandleStorageSize);

			if (HandleCount)
			{
				VulkanExtension::vkGetRayTracingShaderGroupHandlesKHR(InDevice->GetVkDevice(), RTPipeline, HandleOffset, HandleCount, OutHandleStorageSize, OutHandleStorage.data());
			}

			HandleOffset += HandleCount;

			return OutHandleStorage;
		};

		RayGen.ShaderHandles = FetchShaderhandle(Pipeline, InitializerRayGenShaders.size());
		Miss.ShaderHandles = FetchShaderhandle(Pipeline, InitializerRayMissShaders.size());
		HitGroup.ShaderHandles = FetchShaderhandle(Pipeline, InitializerHitGroupShaders.size());
	}

	bAllowHitGroupIndexing = Initializer.HitGroupTable.size() ? true : false;
}

int32 XVulkanRayTracingPipelineState::GetShaderIndex(XVulkanRayTracingShader* Shader)
{
	size_t Hash = Shader->GetHash();
	const auto& ShaderArray = GetShaderData(Shader->GetShaderType()).Shaders;

	for (int32 Index = 0; Index < ShaderArray.size(); Index++)
	{
		if (Hash == ShaderArray[Index]->GetHash())
		{
			return Index;
		}
	}
	XASSERT(false);
	return 0;
}

const std::vector<uint8>& XVulkanRayTracingPipelineState::GetShaderHandles(EShaderType ShaderType)
{
	return GetShaderData(ShaderType).ShaderHandles;
}


XVulkanRayTracingPipelineState::ShaderData XVulkanRayTracingPipelineState::GetShaderData(EShaderType ShaderType)
{
	switch (ShaderType)
	{
	case EShaderType::SV_RayGen:
		return RayGen;
	case EShaderType::SV_RayMiss:
		return Miss;
	case EShaderType::SV_HitGroup:
		return HitGroup;
	default:
		XASSERT(false);
	}
	return ShaderData();
}

std::shared_ptr<XRHIRayTracingPSO> XVulkanPlatformRHI::RHICreateRayTracingPipelineState(const XRayTracingPipelineStateInitializer& OriginalInitializer)
{
	XASSERT(false);
	return std::shared_ptr<XRHIRayTracingPSO>();
}

void XVulkanCommandListContext::RHIBuildAccelerationStructures(const std::span<const XRayTracingGeometryBuildParams> Params, const XRHIBufferRange& ScratchBufferRange)
{
	uint32 ScratchBufferSize = ScratchBufferRange.Size ? ScratchBufferRange.Size : ScratchBufferRange.Buffer->GetSize();
	XVulkanResourceMultiBuffer* ScratchBuffer = static_cast<XVulkanResourceMultiBuffer*>(ScratchBufferRange.Buffer);
	uint32 ScratchBufferOffset = ScratchBufferRange.Offset;
	
	std::vector<XVkRTBLASBuildData> TempBuildData;
	std::vector<VkAccelerationStructureBuildGeometryInfoKHR> BuildGeometryInfos;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> BuildRangeInfos;

	for (const XRayTracingGeometryBuildParams& P : Params)
	{
		XVulkanRayTracingGeometry* const Geometry = static_cast<XVulkanRayTracingGeometry*>(P.Geometry.get());
		const bool bIsUpdate = P.BuildMode == EAccelerationStructureBuildMode::Update;

		uint64 ScratchBufferRequiredSize = bIsUpdate ? Geometry->GetSizeInfo().UpdateScratchSize : Geometry->GetSizeInfo().BuildScratchSize;
		TempBuildData.push_back(XVkRTBLASBuildData());

		XVkRTBLASBuildData& BuildData = TempBuildData[TempBuildData.size() - 1];

		GetBLASBuildData(
			Device->GetVkDevice(), 
			Geometry->Initializer.Segments, 
			Geometry->Initializer.IndexBuffer, 
			Geometry->Initializer.IndexBufferOffset, 
			Geometry->Initializer.IndexBuffer ? Geometry->Initializer.IndexBuffer->GetStride() : 0, 
			GetRayTracingAccelerationStructureBuildFlags(Geometry->Initializer), EAccelerationStructureBuildMode::Build, 
			BuildData);

		VkDeviceAddress ScratchBufferAddress = ScratchBuffer->GetDeviceAddress() + ScratchBufferOffset;
		ScratchBufferOffset += ScratchBufferRequiredSize;

		//---------------------------------------------------------------
		BuildData.GeometryInfo.dstAccelerationStructure = Geometry->Handle;
		BuildData.GeometryInfo.srcAccelerationStructure = bIsUpdate ? Geometry->Handle : VK_NULL_HANDLE;
		BuildData.GeometryInfo.scratchData.deviceAddress = ScratchBufferAddress;
		//---------------------------------------------------------------

		VkAccelerationStructureBuildRangeInfoKHR* const pBuildRanges = BuildData.Ranges.data();
		BuildGeometryInfos.push_back(BuildData.GeometryInfo);
		BuildRangeInfos.push_back(pBuildRanges);

		Geometry->SetupHitGroupSystemParameters();
	}

	XVulkanCmdBuffer* Cmd = CmdBufferManager->GetActiveCmdBuffer();
	VkCommandBuffer CmdBuffer = Cmd->GetHandle();

	VulkanExtension::vkCmdBuildAccelerationStructuresKHR(CmdBuffer, Params.size(), BuildGeometryInfos.data(), BuildRangeInfos.data());
	AddAccelerationStructureBuildBarrier(CmdBuffer);

	CmdBufferManager->SubmitActiveCmdBuffer();
	CmdBufferManager->PrepareForNewActiveCommandBuffer();
}

XVulkanRayTracingGeometry::XVulkanRayTracingGeometry(const XRayTracingGeometryInitializer& Initializer, XVulkanDevice* InDevice)
	:XRHIRayTracingGeometry(Initializer)
	,Device(InDevice)
{
	SizeInfo = RHICalcRayTracingGeometrySize(Initializer);

	XRHIResourceCreateData ResourceCreateData;
	AccelerationStructureBuffer = RHICreateBuffer(0, SizeInfo.ResultSize, EBufferUsage::BUF_AccelerationStructure, ResourceCreateData);
	
	VkAccelerationStructureCreateInfoKHR CreateInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	CreateInfo.buffer = static_cast<XVulkanResourceMultiBuffer*>(AccelerationStructureBuffer.get())->Buffer.VulkanHandle;
	CreateInfo.offset = static_cast<XVulkanResourceMultiBuffer*>(AccelerationStructureBuffer.get())->Buffer.Offset;
	CreateInfo.size = SizeInfo.ResultSize;
	CreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	VulkanExtension::vkCreateAccelerationStructureKHR(Device->GetVkDevice(), &CreateInfo, nullptr, &Handle);

	VkAccelerationStructureDeviceAddressInfoKHR DeviceAddressInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
	DeviceAddressInfo.accelerationStructure = Handle;
	Address = VulkanExtension::vkGetAccelerationStructureDeviceAddressKHR(Device->GetVkDevice(), &DeviceAddressInfo);
}

void XVulkanRayTracingGeometry::SetupHitGroupSystemParameters()
{
	auto* BindlessDescriptorManager = Device->GetBindlessDescriptorMannager();
	auto GetBindLessHandle = [BindlessDescriptorManager](XVulkanResourceMultiBuffer* Buffer, uint32 ExtraOffset)
	{
		if (Buffer)
		{
			XRHIDescriptorHandle BindlessHandle = BindlessDescriptorManager->ReserveDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			return BindlessHandle;
		}
		return XRHIDescriptorHandle();
	};

	ReleaseBindlessHandles();

	HitGroupSystemParameters.resize(Initializer.Segments.size());
	XVulkanResourceMultiBuffer* IndexBuffer = static_cast<XVulkanResourceMultiBuffer*>(Initializer.IndexBuffer.get());
	HitGroupSystemIndexView = GetBindLessHandle(IndexBuffer, 0);

	for (const XRayTracingGeometrySegment& Segment : Initializer.Segments)
	{
		XVulkanResourceMultiBuffer* VertexBuffer = static_cast<XVulkanResourceMultiBuffer*>(Segment.VertexBuffer.get());
		const XRHIDescriptorHandle VBHandle = GetBindLessHandle(VertexBuffer, Segment.VertexBufferOffset);
		HitGroupSystemVertexViews.push_back(VBHandle);


		XVulkanHitGroupSystemParameters SystemParameters = {};
		SystemParameters.BindlessHitGroupSystemVertexBufferIndex = VBHandle.Index;
		if (IndexBuffer != nullptr)
		{
			SystemParameters.BindlessHitGroupSystemIndexBufferIndex = HitGroupSystemIndexView.Index;
		}
	}

}

void XVulkanRayTracingGeometry::ReleaseBindlessHandles()
{
	XVulkanBindlessDescriptorManager* BindlessDescriptorManager = Device->GetBindlessDescriptorMannager();

	for (XRHIDescriptorHandle BindlessHandle : HitGroupSystemVertexViews)
	{
		BindlessDescriptorManager->Unregister(BindlessHandle);
	}

	HitGroupSystemVertexViews.clear();
	HitGroupSystemVertexViews.resize(Initializer.Segments.size());

	if (HitGroupSystemIndexView.IsValid())
	{
		BindlessDescriptorManager->Unregister(HitGroupSystemIndexView);
		HitGroupSystemIndexView = XRHIDescriptorHandle();
	}

}

void XVulkanRayTracingScene::BindBuffer(std::shared_ptr<XRHIBuffer> InBuffer, uint32 InBufferOffset)
{
	AccelerationStructureBuffer = InBuffer;
	ShaderResourceView = std::make_shared<XVulkanShaderResourceView>(Device, static_cast<XVulkanResourceMultiBuffer*>(AccelerationStructureBuffer.get()), InBufferOffset);
}

void XVulkanCommandListContext::BindAccelerationStructureMemory(XRHIRayTracingScene* Scene, std::shared_ptr<XRHIBuffer> Buffer, uint32 BufferOffset)
{
	static_cast<XVulkanRayTracingScene*>(Scene)->BindBuffer(Buffer, BufferOffset);
}
void XVulkanCommandListContext::RHIBuildAccelerationStructure(const XRayTracingSceneBuildParams& SceneBuildParams)
{
	XVulkanRayTracingScene* const Scene = static_cast<XVulkanRayTracingScene*>(SceneBuildParams.Scene);
	XVulkanResourceMultiBuffer* const ScratchBuffer = static_cast<XVulkanResourceMultiBuffer*>(SceneBuildParams.ScratchBuffer);
	XVulkanResourceMultiBuffer* const InstanceBuffer = static_cast<XVulkanResourceMultiBuffer*>(SceneBuildParams.InstanceBuffer);
	Scene->BuildAccelerationStructure(*this, ScratchBuffer, SceneBuildParams.ScratchBufferOffset, InstanceBuffer, SceneBuildParams.InstanceBufferOffset);
}
// It is designed to be used to access vertex and index buffers during inline ray tracing.
struct XVulkanRayTracingGeometryParameters
{
	union
	{
		struct
		{
			uint32 IndexStride : 8; // Can be just 1 bit to indicate 16 or 32 bit indices
			uint32 VertexStride : 8; // Can be just 2 bits to indicate float3, float2 or half2 format
			uint32 Unused : 16;
		} Config;
		uint32 ConfigBits = 0;
	};
	uint32 IndexBufferOffsetInBytes = 0;
	uint64 IndexBuffer = 0;		//ib address
	uint64 VertexBuffer = 0;	//vb address
};

XVulkanRayTracingScene::XVulkanRayTracingScene(XRayTracingSceneInitializer InInitializer, XVulkanDevice* InDevice)
	:Device(InDevice)
	, Initializer(InInitializer)
{
	const ERayTracingAccelerationStructureFlags BuildFlags = ERayTracingAccelerationStructureFlags::PreferTrace; 
	SizeInfo = RHICalcRayTracingSceneSize(Initializer.NumNativeInstance, BuildFlags);
	const uint32 ParameterBufferSize = std::max<uint32>(1, Initializer.NumTotalSegments) * sizeof(XVulkanRayTracingGeometryParameters);
	XRHIResourceCreateData ResourceCreateData;
	PerInstanceGeometryParameterBuffer = std::make_shared<XVulkanResourceMultiBuffer>(Device,
		sizeof(XVulkanRayTracingGeometryParameters), ParameterBufferSize, EBufferUsage(uint32(EBufferUsage::BUF_StructuredBuffer) | uint32(EBufferUsage::BUF_ShaderResource)),
		ResourceCreateData);

	PerInstanceGeometryParameterSRV = std::make_shared <XVulkanShaderResourceView>(Device, PerInstanceGeometryParameterBuffer.get());
}

XVulkanRayTracingScene::~XVulkanRayTracingScene()
{
	for (auto iter : ShaderTables)
	{
		delete iter.second;
	}
}


void XVulkanRayTracingScene::BuildAccelerationStructure(XVulkanCommandListContext& CmdContext, XVulkanResourceMultiBuffer* InScratchBuffer, uint32 InScratchOffset, XVulkanResourceMultiBuffer* InInstanceBuffer, uint32 InInstanceOffset)
{
	BuildPerInstanceGeometryParameterBuffer(CmdContext);

	std::shared_ptr<XRHIBuffer>ScratchBuffer;

	if (InScratchBuffer == nullptr)
	{
		ScratchBuffer = (RHICreateBuffer(0, SizeInfo.BuildScratchSize, EBufferUsage(uint32(EBufferUsage::BUF_StructuredBuffer) | uint32(EBufferUsage::BUF_AccelerationStructure)), XRHIResourceCreateData()));
		InScratchBuffer = static_cast<XVulkanResourceMultiBuffer*>(ScratchBuffer.get());
		InScratchOffset = 0;
	}

	XVkRTTLASBuildData BuildData;

	{
		VkDeviceAddress InstanceBufferAddress = InInstanceBuffer->GetDeviceAddress() + InInstanceOffset;
		GetTLASBuildData(Device->GetVkDevice(), Initializer.NumNativeInstance, InstanceBufferAddress, BuildData);
		BuildData.GeometryInfo.dstAccelerationStructure = ShaderResourceView->AccelerationStructureHandle;
		BuildData.GeometryInfo.scratchData.deviceAddress = InScratchBuffer->GetDeviceAddress() + InScratchOffset;
	}

	VkAccelerationStructureBuildGeometryInfoKHR GeometryInfo = BuildData.GeometryInfo;

	VkAccelerationStructureBuildRangeInfoKHR RangeInfo;
	RangeInfo.primitiveCount = Initializer.NumNativeInstance;
	RangeInfo.primitiveOffset = 0;
	RangeInfo.transformOffset = 0;
	RangeInfo.firstVertex = 0;

	VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &RangeInfo;

	XVulkanCommandBufferManager& CmdBufferManager = *CmdContext.GetCommandBufferManager();
	XVulkanCmdBuffer* const CmdBuffer = CmdBufferManager.GetActiveCmdBuffer();
	AddAccelerationStructureBuildBarrier(CmdBuffer->GetHandle());

	VulkanExtension::vkCmdBuildAccelerationStructuresKHR(CmdBuffer->GetHandle(), 1, &GeometryInfo, &pRangeInfo);

	AddAccelerationStructureBuildBarrier(CmdBuffer->GetHandle());

	CmdBufferManager.SubmitActiveCmdBuffer();
	CmdBufferManager.PrepareForNewActiveCommandBuffer();
}

XVulkanRayTracingShaderTable* XVulkanRayTracingScene::FindOrCreateShaderTable(const XVulkanRayTracingPipelineState* PipelineState)
{
	auto iter = ShaderTables.find(PipelineState);
	if (iter != ShaderTables.end())
	{
		return iter->second;
	}

	XVulkanRayTracingShaderTable* NewST = new XVulkanRayTracingShaderTable(Device);
	NewST->Init(this, PipelineState);
	ShaderTables[PipelineState] = NewST;
	return NewST;
}

void XVulkanRayTracingScene::BuildPerInstanceGeometryParameterBuffer(XVulkanCommandListContext& CmdContext)
{
	const uint32 ParameterBufferSize = std::max<uint32>(1, Initializer.NumTotalSegments) * sizeof(XVulkanRayTracingGeometryParameters);
	void* MappedBuffer = PerInstanceGeometryParameterBuffer->Lock(EResourceLockMode::RLM_WriteOnly, ParameterBufferSize, 0);
	XVulkanRayTracingGeometryParameters* MappedParameters = reinterpret_cast<XVulkanRayTracingGeometryParameters*>(MappedBuffer);
	uint32 ParameterIndex = 0;
	for (XRHIRayTracingGeometry* GeometryRHI : Initializer.PerInstanceGeometres)
	{
		const XVulkanRayTracingGeometry* Geometry = static_cast<XVulkanRayTracingGeometry*>(GeometryRHI);
		const XRayTracingGeometryInitializer& GeometryInitializer = Geometry->Initializer;
		const XVulkanResourceMultiBuffer* IndexBuffer = static_cast<XVulkanResourceMultiBuffer*>(GeometryInitializer.IndexBuffer.get());
		
		const uint32 IndexStride = IndexBuffer ? IndexBuffer->GetStride() : 0;
		const uint32 IndexOffsetInBytes = GeometryInitializer.IndexBufferOffset;
		const VkDeviceAddress IndexBufferAddress = IndexBuffer ? IndexBuffer->GetDeviceAddress() : VkDeviceAddress(0);//IB

		for (const XRayTracingGeometrySegment& Segment : GeometryInitializer.Segments)
		{
			const XVulkanResourceMultiBuffer* VertexBuffer = static_cast<XVulkanResourceMultiBuffer*>(Segment.VertexBuffer.get());
			const VkDeviceAddress VertexBufferAddress = VertexBuffer->GetDeviceAddress();//VB

			XVulkanRayTracingGeometryParameters SegmentParameters;
			SegmentParameters.Config.IndexStride = IndexStride;
			SegmentParameters.Config.VertexStride = Segment.VertexBufferStride;

			if (IndexStride)
			{
				SegmentParameters.IndexBufferOffsetInBytes = IndexOffsetInBytes + IndexStride * Segment.FirstPrimitive * 3;
				SegmentParameters.IndexBuffer = static_cast<uint64>(IndexBufferAddress);
			}
			else
			{
				SegmentParameters.IndexBuffer = 0;
			}
			SegmentParameters.VertexBuffer = static_cast<uint64>(VertexBufferAddress);
			MappedParameters[ParameterIndex] = SegmentParameters;
			ParameterIndex++;
		}
	}
	PerInstanceGeometryParameterBuffer->UnLock(&CmdContext);
}
#endif

XVulkanBindlessDescriptorManager::XUniformBufferDescriptorArrays GetStageUBs(XVulkanDevice* Device, const XRayTracingShaderBinds& InGlobalResoruceBindings)
{
	XASSERT(false);
	return std::array<VkDescriptorAddressInfoEXT, uint32(EShaderType::SV_ShaderCount)>();
}

void XVulkanCommandListContext::RayTraceDispatch(XRHIRayTracingPSO* InPipeline, XRHIRayTracingShader* InRayGenShader, XRHIRayTracingScene* InScene, const XRayTracingShaderBinds& GlobalResourceBindings, uint32 Width, uint32 Height)
{
	XVulkanRayTracingPipelineState* Pipeline = static_cast< XVulkanRayTracingPipelineState*>(InPipeline);
	XVulkanRayTracingScene* Scene = static_cast<XVulkanRayTracingScene*>(InScene);
	XVulkanRayTracingShader* RayGenShader = static_cast<XVulkanRayTracingShader*>(InRayGenShader);
	XVulkanRayTracingShaderTable* ShaderTable = Scene->FindOrCreateShaderTable(Pipeline);

	XVulkanCmdBuffer* const CmdBuffer = GetCommandBufferManager()->GetActiveCmdBuffer();
	vkCmdBindPipeline(CmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, Pipeline->Pipeline);

	//XVulkanBindlessDescriptorManager::XUniformBufferDescriptorArrays StageUBs = ;

	ShaderTable->SetSlot(EShaderType::SV_RayGen, 0, Pipeline->GetShaderIndex(RayGenShader), Pipeline->GetShaderHandles(EShaderType::SV_RayGen));
	
	VulkanExtension::vkCmdTraceRaysKHR(CmdBuffer->GetHandle(),
		ShaderTable->GetRegion(EShaderType::SV_RayGen),
		ShaderTable->GetRegion(EShaderType::SV_RayMiss),
		ShaderTable->GetRegion(EShaderType::SV_HitGroup),
		nullptr,
		Width, Height,1);
}

XVulkanRayTracingShaderTable::XVulkanRayTracingShaderTable(XVulkanDevice* InDevice)
	:Device(InDevice)
	, HandleSize(InDevice->GetDeviceExtensionProperties().RayTracingPipeline.shaderGroupHandleSize)
	, HandleSizeAligned(InDevice->GetDeviceExtensionProperties().RayTracingPipeline.shaderGroupHandleAlignment)
{
}

const VkStridedDeviceAddressRegionKHR* XVulkanRayTracingShaderTable::GetRegion(EShaderType ShaderType)
{
	return &GetAlloc(ShaderType).Region;
}

void XVulkanRayTracingShaderTable::Init(const XVulkanRayTracingScene* Scene, const XVulkanRayTracingPipelineState* Pipeline)
{
	auto InitAlloc = [Device = Device, HandleSize = HandleSize, HandleSizeAligned = HandleSizeAligned](XVulkanShaderTableAllocation& Alloc, uint32 InHandleCount, bool InUseLocalRecord)
	{
		Alloc.HandleCount = InHandleCount;
		Alloc.UseLocalRecord = InUseLocalRecord;

		if (Alloc.HandleCount > 0)
		{
			VkDevice DeviceHandle = Device->GetVkDevice();
			const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& RayTracingPipelineProps = Device->GetDeviceExtensionProperties().RayTracingPipeline;

			// With the exception of RayGen, the stride will be the size of the handle aligned to the shaderGroupHandleAlignment
			Alloc.Region.stride = InUseLocalRecord ? std::min<VkDeviceSize>(RayTracingPipelineProps.maxShaderGroupStride, 4096): HandleSizeAligned;
			Alloc.Region.size = Alloc.HandleCount * Alloc.Region.stride;

			{
				VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
				BufferCreateInfo.size = Alloc.Region.size;
				BufferCreateInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
				VULKAN_VARIFY(vkCreateBuffer(DeviceHandle, &BufferCreateInfo, nullptr, &Alloc.Buffer));
			}

			Device->GetMemoryManager().AllocateBufferMemory(Alloc.Allocation, Alloc.Buffer, EVulkanAllocationFlags::HostVisible | EVulkanAllocationFlags::AutoBind | EVulkanAllocationFlags::Dedicated);

			VkBufferDeviceAddressInfoKHR DeviceAddressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
			DeviceAddressInfo.buffer = Alloc.Buffer;
			Alloc.Region.deviceAddress = VulkanExtension::vkGetBufferDeviceAddressKHR(DeviceHandle, &DeviceAddressInfo);
			Alloc.MappedBufferMemory = (uint8*)Alloc.Allocation.GetMappedPointer(Device);
		}
	};

	const auto& SceneInitializer = Scene->Initializer;
	InitAlloc(RayGen, 1, false);
	InitAlloc(Miss, SceneInitializer.NumMissShaderSlots, true);
	InitAlloc(HitGroup, Pipeline->bAllowHitGroupIndexing ? SceneInitializer.NumTotalSegments * RAY_TRACING_NUM_SHADER_SLOTS : 1, true);
}

void XVulkanRayTracingShaderTable::SetSlot(EShaderType ShaderType, uint32 DstSlot, uint32 SrcHandleIndex, const std::vector<uint8>& SrcHandleData)
{
	XVulkanShaderTableAllocation& Alloc = GetAlloc(ShaderType);
	memcpy(&Alloc.MappedBufferMemory[DstSlot * Alloc.Region.stride], &SrcHandleData[SrcHandleIndex * HandleSize], HandleSize);
}

XVulkanRayTracingShaderTable::XVulkanShaderTableAllocation& XVulkanRayTracingShaderTable::GetAlloc(EShaderType ShaderType)
{
	switch (ShaderType)
	{
	case EShaderType::SV_RayGen:
		return RayGen;
	case EShaderType::SV_RayMiss:
		return Miss;
	case EShaderType::SV_HitGroup:
		return HitGroup;
	default:
		XASSERT(false);
	}

	XVulkanRayTracingShaderTable::XVulkanShaderTableAllocation EmptyAlloc;
	return EmptyAlloc;
}
