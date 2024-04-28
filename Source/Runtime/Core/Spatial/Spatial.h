#pragma once
#include <vector>
#include "Runtime/CoreGObject/GObjtect/Object.h"
#include "Runtime/Core/Math/Math.h"
#include "Runtime/Core/Spatial/Transform.h"

class GSpatial :public GObject
{
protected:
	XTransform WorldTransform;
	GSpatial* ParentSpatial;
	XBoundingBox BoundingBox;
public:
	inline XBoundingBox& GetBoudingBoxNoTrans()
	{
		return BoundingBox;
	}

	inline XBoundingBox& GetBoudingBoxWithTrans()
	{
		return BoundingBox.Transform(WorldTransform.GetCombineMatrix());
	}


	inline void SetBoundingBox(XBoundingBox& BoundingBoxIn)
	{
		BoundingBox = BoundingBoxIn;
	}

	inline void SetBoundingBox(XVector3 Center,XVector3 Extent)
	{
		BoundingBox.Center = Center;
		BoundingBox.Extent = Extent;
	}



	inline XTransform& GetWorldTransform()
	{
		return WorldTransform;
	}

	inline void SetWorldRotate(const XVector3& Axis,float Angle)
	{
		WorldTransform.SetRotate(Axis, Angle);
	}

	inline void SetWorldTranslate(const XVector3& TranslationIn)
	{
		WorldTransform.SetTranslation(TranslationIn);
	}

	inline void SetWorldScale(const XVector3& WorldScale)
	{
		WorldTransform.SetScale(WorldScale);
	}
};

class GNode :public  GSpatial
{
protected:
	std::vector<GSpatial*> ChildSpatial;
};