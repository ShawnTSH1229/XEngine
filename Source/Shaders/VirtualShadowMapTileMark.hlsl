#include "Common.hlsl"
#include "Math.hlsl"
#include "VirtualShadowMapCommon.hlsl"

// todo:
// clear virtual shadow map tile state
// add tile state visualize pass

cbuffer CbShadowViewInfo
{
    row_major float4x4 LightViewProjectMatrix;
    float3 WorldCameraPosition;
};

Texture2D SceneDepthTexture;
RWStructuredBuffer<uint> VirtualShadowMapTileState; // 32 * 32 + 16 * 16 + 8 * 8

[numthreads(16, 16, 1)]
void VSMTileMaskCS(uint2 DispatchThreadID :SV_DispatchThreadID)
{
    float DeviceZ = SceneDepthTexture.Load(int3(DispatchThreadID.xy,0));
    float2 UV = DispatchThreadID * View_BufferSizeAndInvSize.zw;
    
    if(DeviceZ == 0.0 || UV.x > 1.0f || UV.y > 1.0f)
    {
        return;
    }
    
    float2 ScreenPos = UV * 2.0f - 1.0f; 
    ScreenPos.y *= -1.0f;

    float4 NDCPosNoDivdeW = float4(ScreenPos, DeviceZ, 1.0);
    float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse, NDCPosNoDivdeW);
    WorldPosition.xyz /= WorldPosition.w;

    float4 ShadowScreenPOS = mul(float4(WorldPosition.xyz,1.0), LightViewProjectMatrix);
    ShadowScreenPOS.xyz /= ShadowScreenPOS.w;
    
    float2 UVOut = ShadowScreenPOS.xy; 
    UVOut.y *=- 1.0;
    UVOut = UVOut* 0.5 + 0.5f;

    float Distance = length(WorldCameraPosition - WorldPosition.xyz);
    float Log2Distance = log2(Distance + 1);
    
    int MipLevel = clamp(Log2Distance - VSM_CLIPMAP_MIN_LEVEL, 0, VSM_MIP_NUM - 1);
    uint2 VSMTileIndex = uint2(UVOut * MipLevelSize[MipLevel]);

    int DestTileInfoIndex = MipLevelOffset[MipLevel] + VSMTileIndex.y * MipLevelSize[MipLevel] + VSMTileIndex.x;

    VirtualShadowMapTileState[DestTileInfoIndex] = TILE_STATE_USED;
}