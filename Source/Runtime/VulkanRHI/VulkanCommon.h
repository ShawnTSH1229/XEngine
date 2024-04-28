#pragma once

namespace VulkanBindless
{
	enum EDescriptorSets
	{
		None = 0,
		BindlesssStorageBufferSet = 1,
		BindlessAccelerationStructureSet = 2,
		NumBindLessSet
	};
};