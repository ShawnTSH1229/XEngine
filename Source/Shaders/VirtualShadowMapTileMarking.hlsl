#include "Common.hlsl"
#include "Math.hlsl"
#include "VirtualShadowMapCommon.hlsl"
// todo: 
// 1. check locked compare
// 2. row major LightViewProjectMatrix


Texture2D TextureSampledInput;
RWTexture2D<uint> VirtualSMFlags;

cbuffer cbShadowViewInfo
{
    //float4x4 LightViewProjectMatrix;
    row_major float4x4 LightViewProjectMatrix;
};

[numthreads(16, 16, 1)]
void VSMTileMaskCS(uint2 DispatchThreadID :SV_DispatchThreadID)
{
    float DeviceZ = TextureSampledInput.Load(int3(DispatchThreadID,0));
    float2 UV = DispatchThreadID * View_BufferSizeAndInvSize.zw;
    
    if(DeviceZ == 0.0 || UV.x > 1.0f || UV.y > 1.0f)
    {
        return;
    }
    
    float2 ScreenPos = UV * 2.0f - 1.0f; ScreenPos.y *= -1.0f;

    float4 NDCPosNoDivdeW = float4(ScreenPos , DeviceZ , 1.0);
    float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse,NDCPosNoDivdeW);
    WorldPosition.xyz/=WorldPosition.w;

    float4 ShadowScreenPOS = mul(float4(WorldPosition.xyz,1.0),LightViewProjectMatrix);
    ShadowScreenPOS.xyz/=ShadowScreenPOS.w;
    float2 UVOut = ShadowScreenPOS.xy; 
    UVOut.y*=-1.0;
    UVOut = UVOut* 0.5 + 0.5f;
    
    VirtualSMFlags[uint2(UVOut * 8 * 8)] = 1;
}