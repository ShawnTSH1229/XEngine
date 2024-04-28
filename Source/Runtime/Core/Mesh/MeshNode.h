#include "Runtime/Core/Spatial/Spatial.h"
#include "Runtime/Engine/Classes/ResourceBase.h"

class GMeshNode :public GNode
{
private:
	bool bCastShadow;
};

class GStaticMeshNode :public GMeshNode,public XResourceBase
{
private:

public:
	constexpr static const char* ResourceSuffix = "xmesh";
};