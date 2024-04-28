#pragma once
#include "Runtime/Core/Spatial/Spatial.h"
#include "GeomertyData.h"

class GGeomertyNode :public GNode
{
public:
	std::shared_ptr<GGeomertry>GetGeometryData(unsigned int i)const;
private:
};