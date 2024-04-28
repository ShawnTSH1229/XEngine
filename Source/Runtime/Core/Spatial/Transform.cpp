#include "Transform.h"

XTransform::XTransform()
{
	bNeedRecombine = true;
	Translation = XVector3::Zero;
	Scale = XVector3::One;
	RotateAxis = XVector3(0, 0, 1);
	Angle = 0;
	CombinedMatrix = XMatrix::Identity;
}

void XTransform::Combine()
{
	if (bNeedRecombine)
	{
		CombinedMatrix = XMatrix::CreateFromAxisAngle(RotateAxis, Angle) * XMatrix::CreateScale(Scale) * XMatrix::CreateTranslation(Translation);
		bNeedRecombine = false;
	}
}
