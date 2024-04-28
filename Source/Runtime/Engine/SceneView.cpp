#include "SceneView.h"

XVector4 CreateInvDeviceZToWorldZTransform(const XMatrix ProjMatrix)
{
	// DeviceZ = A + B / ViewZ
	// ViewZ = 1.0/((DeviceZ - A)/B)
	// ViewZ = 1.0/(DeviceZ /B - A/B)

	float DepthMul = ProjMatrix.m[2][2];
	float DepthAdd = ProjMatrix.m[3][2];

	if (DepthAdd == 0.f) { DepthAdd = 0.00000001f; }

	// combined perspective and ortho equation in shader to handle either SceneView.cpp line 416
	bool bIsPerspectiveProjection = ProjMatrix.m[3][3] < 1.0f;
	if (bIsPerspectiveProjection)
	{
		float SubtractValue = DepthMul / DepthAdd;
		SubtractValue -= 0.00000001f;
		return XVector4(0.0f, 0.0f, 1.0f / DepthAdd, SubtractValue);
	}
	else
	{
		return XVector4(1.0f / ProjMatrix.m[2][2], -ProjMatrix.m[3][2] / ProjMatrix.m[2][2] + 1.0f, 0.0f, 1.0f);
	}
}

void XViewMatrices::Create(const XMatrix& ProjectionMatrixIn, const XVector3& ViewLocation, const XVector3& ViewTargetPosition)
{
	ProjectionMatrix = ProjectionMatrixIn;
	UpdateViewMatrix(ViewLocation, ViewTargetPosition);
}

/*
	a					a'
	b					b'
(	c	) = ViewProj(	c'	)
	d					d'

	= aT * VP T
*/

//in frustum is positive
void XViewMatrices::GetPlanes(XPlane Planes[(int)ECameraPlane::CP_MAX])
{
	//Right Plane x = 1;  
	XVector4 RightPlane = XVector4::Zero; RightPlane.x = 1; RightPlane.w = -1;
	Planes[(int)ECameraPlane::CP_RIGHT] = XVector4::Transform(RightPlane, ViewProjectionMatrixTranspose);
	
	//Left Plane x = -1;  -> reflect normal
	XVector4 LeftPlane = XVector4::Zero; LeftPlane.x = 1; LeftPlane.w = 1;
	Planes[(int)ECameraPlane::CP_LEFT] = XVector4::Transform(LeftPlane, ViewProjectionMatrixTranspose);
	Planes[(int)ECameraPlane::CP_LEFT].NegNormal();

	{
		//Near Plane z = 1;   -> reflect normal <- error ,do culling in Cartesian coordinates ,whic is right hand
		XVector4 NearPlane = XVector4::Zero; NearPlane.z = 1; NearPlane.w = -1;
		Planes[(int)ECameraPlane::CP_NEAR] = XVector4::Transform(NearPlane, ViewProjectionMatrixTranspose);
		Planes[(int)ECameraPlane::CP_NEAR];

		// FarPlane z = 0; -> reflect normal <- right
		XVector4 FarPlane = XVector4::Zero; FarPlane.z = 1;
		Planes[(int)ECameraPlane::CP_FAR] = XVector4::Transform(FarPlane, ViewProjectionMatrixTranspose);
		Planes[(int)ECameraPlane::CP_FAR].NegNormal();
	}

	//Top Plane y = 1 ;
	XVector4 TopPlane = XVector4::Zero; TopPlane.y = 1; TopPlane.w = -1;
	Planes[(int)ECameraPlane::CP_TOP] = XVector4::Transform(TopPlane, ViewProjectionMatrixTranspose);
	

	//Bottom Plane y = -1; -> reflect normal
	XVector4 BottomPlane = XVector4::Zero; BottomPlane.y = 1; BottomPlane.w = 1;
	Planes[(int)ECameraPlane::CP_BOTTOM] = XVector4::Transform(BottomPlane, ViewProjectionMatrixTranspose);
	Planes[(int)ECameraPlane::CP_BOTTOM].NegNormal();
}

void XViewMatrices::UpdateViewMatrix(const XVector3& ViewLocation, const XVector3& ViewTargetPosition)
{
	ViewOrigin = ViewLocation;
	PreViewTranslation = XVector3(-ViewOrigin.x, -ViewOrigin.y, -ViewOrigin.z);

	XVector4 ViewOriginVec(ViewOrigin.x, ViewOrigin.y, ViewOrigin.z, 1.0f);
	XVector4 ViewTargetVec(ViewTargetPosition.x, ViewTargetPosition.y, ViewTargetPosition.z, 1.0f);
	XVector4 EyeDirection = ViewTargetVec - ViewOriginVec;
	EyeForwardDir = XVector3(EyeDirection.x, EyeDirection.y, EyeDirection.z);
	//Cartesian coordinates is right hand
	
	//compute uvw
	XVector3 UpDirection(0.0f, 1.0f, 0.0f);
	XVector3 WNormalize = EyeForwardDir; WNormalize.Normalize();
	XVector3 UNormalize = UpDirection.Cross(WNormalize); UNormalize.Normalize();
	XVector3 VNormalize = WNormalize.Cross(UNormalize);

	//compute -qu
	XVector3 NegEyePosition = XVector3(-ViewOriginVec.x, -ViewOriginVec.y, -ViewOriginVec.z);
	float NegQU = UNormalize.Dot(NegEyePosition);
	float NegQV = VNormalize.Dot(NegEyePosition);
	float NegQW = WNormalize.Dot(NegEyePosition);

	//
	XMatrix TranslatedViewMatrixCom(
		UNormalize.x, UNormalize.y, UNormalize.z,0,
		VNormalize.x, VNormalize.y, VNormalize.z,0,
		WNormalize.x, WNormalize.y, WNormalize.z,0,
		0,0,0,1);

	TranslatedViewMatrix = TranslatedViewMatrixCom.Transpose();

	XMatrix ViewMatrixCom(
		UNormalize.x, UNormalize.y, UNormalize.z, NegQU,
		VNormalize.x, VNormalize.y, VNormalize.z, NegQV,
		WNormalize.x, WNormalize.y, WNormalize.z, NegQW,
		0, 0, 0, 1
	);
	ViewMatrix = ViewMatrixCom.Transpose();

	ViewProjectionMatrix = ViewMatrix * ProjectionMatrix;
	ViewProjectionMatrixTranspose = ViewProjectionMatrix.Transpose();
	ViewProjectionMatrixInverse = ViewProjectionMatrix.Invert();

	TranslatedViewProjectionMatrix = TranslatedViewMatrix * ProjectionMatrix;
	TranslatedViewProjectionMatrixTranspose = ViewProjectionMatrix.Transpose();
	TranslatedViewProjectionMatrixInverse = ViewProjectionMatrix.Invert();
}

XMatrix XViewMatrices::GetScreenToTranslatedWorld()
{
	XMatrix ScreenToClip ;
	ScreenToClip.m[2][2] = ProjectionMatrix.m[2][2];
	ScreenToClip.m[3][2] = ProjectionMatrix.m[3][2];
	ScreenToClip.m[2][3] = 1.0f;
	ScreenToClip.m[3][3] = 0.0f;
	
	return ScreenToClip * TranslatedViewProjectionMatrixInverse;
}

XMatrix XViewMatrices::GetScreenToWorld()
{
	XMatrix ScreenToClip;
	ScreenToClip.m[2][2] = ProjectionMatrix.m[2][2];
	ScreenToClip.m[3][2] = ProjectionMatrix.m[3][2];
	ScreenToClip.m[2][3] = 1.0f;
	ScreenToClip.m[3][3] = 0.0f;
	return ScreenToClip * ViewProjectionMatrixInverse;
}

