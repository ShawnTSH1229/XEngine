#pragma once
#include "vulkan\vulkan.h"
namespace VulkanExtension
{
	extern PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	extern PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	extern PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	extern PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	extern PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	extern PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
	extern PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	
	//typedef void (VKAPI_PTR *PFN_vkGetDescriptorEXT)(VkDevice device, const VkDescriptorGetInfoEXT* pDescriptorInfo, size_t dataSize, void* pDescriptor);
	extern PFN_vkGetDescriptorEXT vkGetDescriptorEXT;

	//void vkCmdTraceRaysKHR(
	//	VkCommandBuffer                             commandBuffer,
	//	const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
	//	const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable,
	//	const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
	//	const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable,
	//	uint32_t                                    width,
	//	uint32_t                                    height,
	//	uint32_t                                    depth);
	extern PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;

	extern PFN_vkGetDescriptorSetLayoutSizeEXT vkGetDescriptorSetLayoutSizeEXT;

	void InitExtensionFunction(VkDevice Device);
}