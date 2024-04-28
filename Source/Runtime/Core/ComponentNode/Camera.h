#pragma once
#include "Runtime/Core/Math/Math.h"

#define MouseSensitivity 0.1

class GCamera
{
public:
	GCamera() :RotateXDegree(0), RotateYDegree(0), RotateZDegree(0) 
	{
		Updata();
	}
	
	inline void ProcessMouseMove(float MouseXOffset, float MouseYOffset)
	{
		//origin is left top
		MouseYOffset *= -1.0;

		MouseXOffset *= MouseSensitivity;
		MouseYOffset *= MouseSensitivity;

		RotateXDegree += MouseYOffset;
		RotateYDegree += MouseXOffset;

		if (RotateXDegree > 89.0f)
			RotateXDegree = 89.0f;
		if (RotateXDegree < -89.0f)
			RotateXDegree = -89.0f;

		Updata();
	}

	inline void Updata()
	{
		mLook.x = cos(XDegreeToRadians(RotateXDegree)) * sin(XDegreeToRadians(RotateYDegree));
		mLook.y = sin(XDegreeToRadians(RotateXDegree));
		mLook.z = cos(XDegreeToRadians(RotateXDegree))* cos(XDegreeToRadians(RotateYDegree));

		//Cartesian coordinates is right hand
		mRight = -mLook.Cross(XVector3(0.0, 1.0, 0.0));
		mRight.Normalize();

		mUp = mRight.Cross(mLook);
		mUp.Normalize();
	}

	inline void WalkAD(float d)
	{
		mPosition += d * mRight;
	}

	inline void WalkWS(float d)
	{
		mPosition += d * mLook;// mPosition += d*mLook
	}

	inline XVector3 GetEyePosition()
	{
		return mPosition;
	}

	inline XVector3 GetTargetPosition()
	{
		return mPosition + mLook;
	}

	inline void SetPerspective(float FovAngleYIn,float AspectRatioIn,float NearIn,float FarIn)
	{
		FovAngleY = FovAngleYIn;
		AspectRatio = AspectRatioIn;
		Near = NearIn;
		Far = FarIn;

		ProjectMatrix = XMatrix::MMatrixPerspectiveFovLH(FovAngleYIn, AspectRatioIn, NearIn, FarIn);
	}

	inline XMatrix& GetProjectMatrix()
	{
		return ProjectMatrix;
	}
private:
	XMatrix ProjectMatrix;

	float FovAngleY;
	float Near;
	float Far;
	float AspectRatio;

	float RotateXDegree;
	float RotateYDegree;
	float RotateZDegree;

	XVector3 mPosition = { 0.0f, 3.0f, -3.0f };
	
	XVector3 mRight;	// { 1.0f, 0.0f, 0.0f };
	XVector3 mUp;		// { 0.0f, 1.0f, 0.0f };
	XVector3 mLook;		// { 0.0f, 0.0f, 1.0f }; when RotateYDegree = 0 and RotateXDegree x = 0;
};