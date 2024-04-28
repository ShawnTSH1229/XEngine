#include "VulkanPlatformRHI.h"
#include "VulkanResource.h"

static std::map<uint32, XVulkanVertexLayout*>VertexLayoutCache;
std::shared_ptr<XRHIVertexLayout> XVulkanPlatformRHI::RHICreateVertexDeclaration(const XRHIVertexLayoutArray& Elements)
{
	std::size_t Hash = std::hash<std::string>{}(std::string((char*)&Elements, sizeof(XVertexElement) * Elements.size()));
	auto iter = VertexLayoutCache.find(Hash);
	if (iter != VertexLayoutCache.end())
	{
		return std::shared_ptr<XVulkanVertexLayout>(iter->second);
	}
	return std::make_shared<XVulkanVertexLayout>(Elements);
}