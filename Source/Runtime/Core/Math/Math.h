#pragma once
#include "SimpleMath.h"
#include "Matrix.h"

using XVector2 = DirectX::SimpleMath::Vector2;
using XVector3 = DirectX::SimpleMath::Vector3;
using XVector4 = DirectX::SimpleMath::Vector4;
using XMatrix = DirectX::SimpleMath::LMatrix;
using XPlane = DirectX::SimpleMath::Plane;

struct XBoundSphere
{
	XVector3 Center;
	float Radius;
};

struct XBoundingBox
{
    XVector3 Center;
    XVector3 Extent;

	XBoundingBox& Transform(XMatrix& MatrixIn);
};

constexpr float X_PI = 3.141592654f;
constexpr float X_2PI = 6.283185307f;
inline constexpr float XDegreeToRadians(float Degree)
{
	return Degree * (DirectX::XM_PI / 180.0f);
}

inline constexpr float XRadiansToDegree(float Radians)
{
	return Radians * (180.0f / DirectX::XM_PI);
}

