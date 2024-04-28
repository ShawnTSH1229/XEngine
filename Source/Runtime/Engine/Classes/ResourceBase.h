#pragma once
#include <string>
enum class EResourceType
{
	RT_TEXTURE,
	RT_STATIC_MESH,
	RT_NUM,
};
class XResourceBase
{
public:
private:
	std::string ResourceName;
};