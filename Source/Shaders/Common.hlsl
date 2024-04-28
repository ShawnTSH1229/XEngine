#ifndef __COMMON_HLSL__
#define __COMMON_HLSL__

#include "Math.hlsl"
cbuffer cbView
{
    float4x4 cbView_TranslatedViewProjectionMatrix;
    float4x4 cbView_ScreenToTranslatedWorld;
    float4x4 cbView_ViewToClip;
    float4x4 cbView_ScreenToWorld;
    float4x4 cbView_ViewPorjectionMatrix;
    float4x4 cbView_ViewPorjectionMatrixInverse;
    
    float4 View_InvDeviceZToWorldZTransform;
    float3 View_WorldCameraOrigin;
    uint View_StateFrameIndexMod8;

    float4 View_BufferSizeAndInvSize;
    
    float4 View_AtmosphereLightDirection;

    float3 View_SkyWorldCameraOrigin;
    float cbView_padding1;

    float4 View_SkyPlanetCenterAndViewHeight;
    float4x4 cbView_SkyViewLutReferential;

    float4 View_ViewSizeAndInvSize;
    
    float  Far;
    float Near;
};

#define PI 3.141592653589
const static float Max10BitsFloat = 64512.0f;

SamplerState gsamPointWarp  : register(s0,space1000);
SamplerState gsamLinearWarp  : register(s4,space1000);
SamplerState gsamLinearClamp  : register(s5,space1000);

float Luminance( float3 LinearColor )
{
	return dot( LinearColor, float3( 0.3, 0.59, 0.11 ) );
}
float Pow5( float x )
{
	float xx = x*x;
	return xx * xx * x;
}

float Pow2(float x)
{
    return x*x;
}

//https://blog.csdn.net/dengyibing/article/details/80793209
float ConvertFromDeviceZ_To_ViewZBeforeDivdeW(float DeviceZ)
{
    //from near to far
    //DeviceZ * View_InvDeviceZToWorldZTransform[0] + View_InvDeviceZToWorldZTransform[1] orth
    //1.0f / (DeviceZ * View_InvDeviceZToWorldZTransform[2] - View_InvDeviceZToWorldZTransform[3]); persipective
    //https://zhuanlan.zhihu.com/p/509056079
	return DeviceZ * View_InvDeviceZToWorldZTransform[0] + View_InvDeviceZToWorldZTransform[1] + 1.0f / (DeviceZ * View_InvDeviceZToWorldZTransform[2] - View_InvDeviceZToWorldZTransform[3]);
}

#define POW_CLAMP 0.000001f

// Clamp the base, so it's never <= 0.0f (INF/NaN).
float ClampedPow(float X,float Y)
{
	return pow(max(abs(X),POW_CLAMP),Y);
}

#endif