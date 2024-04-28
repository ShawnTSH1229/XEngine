#pragma once
#include "Runtime/Core/Math/Math.h"
#include "Runtime/RHI/RHIResource.h"

XVector4 CreateInvDeviceZToWorldZTransform(const XMatrix ProjMatrix);

enum class ECameraPlane
{
	CP_RIGHT = 0,
	CP_LEFT,
	CP_TOP,
	CP_BOTTOM,
	CP_FAR,
	CP_NEAR,
	CP_MAX = 6
};

struct XViewMatrices
{
	//3D Game Programming With DirectX12 Page163

	//Local - > World -> Translated World -> View -> Clip Space -> Screen
	//World -> Translated : World Translation
	//Translated World -> View : TranslatedViewMatrix
	//World -> View : ViewMatrix
	//View -> Clip ProjectionMatrix
public:
	void Create(
		const XMatrix& ProjectionMatrixIn,
		const XVector3& ViewLocation,
		const XVector3& ViewTargetPosition);
private:
	
	XVector3		ViewOrigin;
	XVector3		PreViewTranslation;
	XVector3		EyeForwardDir;

	XMatrix		ProjectionMatrix;
	XMatrix		ViewMatrix;
	XMatrix		ViewProjectionMatrix;
	XMatrix		TranslatedViewMatrix;
	XMatrix		TranslatedViewProjectionMatrix;
	XMatrix		TranslatedWorldToClip;

	XMatrix		ViewProjectionMatrixTranspose;
	XMatrix		TranslatedViewProjectionMatrixTranspose;

	XMatrix		ViewProjectionMatrixInverse;
	XMatrix		TranslatedViewProjectionMatrixInverse;

public:
	void GetPlanes(XPlane Planes[(int)ECameraPlane::CP_MAX]);


	void UpdateViewMatrix(const XVector3& ViewLocation, const XVector3& ViewTargetPosition);

//TODO
//#define GetMatrix_COMMON_TRANSPOSE_INVERSE(MAT_NAME)\
//	inline const DirectX::XMFLOAT4X4& GetMAT_NAME()const { return MAT_NAME; };\
//	inline const DirectX::XMFLOAT4X4& GetMAT_NAMETranspose()const { return MAT_NAMETranspose; };\
//	inline const DirectX::XMFLOAT4X4& GetMAT_NAMEInverse()const { return MAT_NAMEInverse; };\

	inline const XVector3& GetViewOrigin()const { return ViewOrigin; };
	inline const XVector3& GetPreViewTranslation()const { return PreViewTranslation; };
	inline const XVector3& GetEyeForwarddDir()const { return EyeForwardDir; };

	//common
	inline const XMatrix& GetProjectionMatrix()const				{ return ProjectionMatrix; };
	inline const XMatrix& GetViewMatrix()const						{ return ViewMatrix; };
	inline const XMatrix& GetViewProjectionMatrix()const			{ return ViewProjectionMatrix; };
	inline const XMatrix& GetTranslatedViewMatrix()const			{ return TranslatedViewMatrix; };
	inline const XMatrix& GetTranslatedViewProjectionMatrix()const	{ return TranslatedViewProjectionMatrix; };
	

	//inverse
	inline const XMatrix& GetViewProjectionMatrixInverse()const { return ViewProjectionMatrixInverse; };
	inline const XMatrix& GetTranslatedViewProjectionMatrixInverse()const { return TranslatedViewProjectionMatrixInverse; };

	//misc
	XMatrix GetScreenToTranslatedWorld();
	XMatrix GetScreenToWorld();
};

struct ViewConstantBufferData
{
	XMatrix TranslatedViewProjectionMatrix;
	XMatrix ScreenToTranslatedWorld;
	XMatrix ViewToClip;
	XMatrix ScreenToWorld;
	XMatrix ViewProjectionMatrix;
	XMatrix ViewProjectionMatrixInverse;

	XVector4 InvDeviceZToWorldZTransform;
	XVector3 WorldCameraOrigin;
	uint32 StateFrameIndexMod8 = 0;

	XVector4 BufferSizeAndInvSize;
	XVector4 AtmosphereLightDirection;

	XVector3 SkyWorldCameraOrigin;
	float padding1 = 0.0;

	XVector4 SkyPlanetCenterAndViewHeight;
	XMatrix SkyViewLutReferential;

	XVector4 ViewSizeAndInvSize;
};

class XSceneView
{
public:
	std::shared_ptr<XRHIConstantBuffer>ViewConstantBuffer;
	ViewConstantBufferData ViewCBCPUData;
	XViewMatrices ViewMats;

	uint32 ViewWidth;
	uint32 ViewHeight;
};