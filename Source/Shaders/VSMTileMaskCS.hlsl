#include "Common.hlsl"
#include "Math.hlsl"
Texture2D TextureSampledInput;
RWTexture2D<uint> VirtualSMFlags;

cbuffer cbShadowViewInfo
{
    //float4x4 LightViewProjectMatrix;
    row_major float4x4 LightViewProjectMatrix;

    float ClientWidth;
    float ClientHeight;
    float padding0;
    float padding1;
}

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

    //https://blog.csdn.net/dengyibing/article/details/80793209
    float4 NDCPosNoDivdeW = float4(ScreenPos , DeviceZ , 1.0);
    float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse,NDCPosNoDivdeW);
    WorldPosition.xyz/=WorldPosition.w;

    float4 ShadowScreenPOS = mul(float4(WorldPosition.xyz,1.0),LightViewProjectMatrix);
    ShadowScreenPOS.xyz/=ShadowScreenPOS.w;
    float2 UVOut = ShadowScreenPOS.xy; 
    UVOut.y*=-1.0;
    UVOut = UVOut* 0.5 + 0.5f;
    
    //float a = ClientWidth + UVOut.x + UVOut.y;
    //a*=0.0;
    //VirtualSMFlags[DispatchThreadID] = 0 + a;
    
    InterlockedCompareStore(VirtualSMFlags[uint2(UVOut * 8 * 8)],0,1);
}