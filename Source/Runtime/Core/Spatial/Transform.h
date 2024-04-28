#pragma once
#include "Runtime/Core/Math/Math.h"
#include "Runtime/HAL/Mch.h"

class XTransform
{
public:
	XMatrix CombinedMatrix;
	XVector3 RotateAxis;
	float Angle;

	XVector3 Translation;
	XVector3 Scale;
	bool bNeedRecombine;

	XTransform();
	
	inline XMatrix& GetCombineMatrix()
	{
		Combine();
		return CombinedMatrix;
	}

	inline XMatrix GetCombineMatrixTranspose()
	{
		return GetCombineMatrix().Transpose();
	}

	void Combine();

	inline void SetTranslation(const XVector3& TrasnlationIn)
	{
		Translation = TrasnlationIn;
		bNeedRecombine = true;
	}

	inline void SetScale(const XVector3& ScaleIn)
	{
		XASSERT(((ScaleIn.x == ScaleIn.y) && (ScaleIn.y == ScaleIn.z)));
		Scale = ScaleIn;
		bNeedRecombine = true;
	}

	inline void SetRotate(const XVector3& AxisIn, float AngleIn)
	{
		RotateAxis = AxisIn;
		Angle = AngleIn;
		bNeedRecombine = true;
	}
};