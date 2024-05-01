#include "Math.hlsl"
#include "VirtualShadowMapCommon.hlsl"

// this constant buffer is used for cache miss test
cbuffer cbDynamicObjectParameters
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter; //world space
    uint isDynamicObejct;
    float3 BoundingBoxExtent;
    float padding1;
};

cbuffer cbShadowViewInfo
{
    row_major float4x4 LightViewProjectMatrix;
    float3 WorldCameraPosition;
};

RWBuffer<uint> VirtualShadowMapTileStateCacheMiss;

[numthreads(1, 1, 1)]
void VSMTileCacheMissCS(uint2 DispatchThreadID :SV_DispatchThreadID)
{
    float4 Corner[8];

    [unroll]
    for(uint i = 0; i < 8 ; i++)
    {
        Corner[i] = float4(BoundingBoxCenter ,0)  + float4(BoundingBoxExtent,0) * BoundingBoxOffset[i];
    }

    float2 UVMin = float2(1.1,1.1);
    float2 UVMax = float2(-0.1,-0.1);

    [unroll]
    for(uint j = 0; j < 8 ; j++)
    {
        float4 ScreenPosition = mul(float4(Corner[j].xyz,1.0f), ShadowViewProject);
        ScreenPosition.xyz/=ScreenPosition.w;

        float2 ScreenUV = ScreenPosition.xy;
        ScreenUV.y *= -1.0;
        ScreenUV = ScreenUV* 0.5 + 0.5f;

        UVMin.x = ScreenUV.x > UVMin.x ? UVMin.x : ScreenUV.x;
        UVMin.y = ScreenUV.y > UVMin.y ? UVMin.y : ScreenUV.y;

        UVMax.x = ScreenUV.x > UVMax.x ? ScreenUV.x : UVMax.x;
        UVMax.y = ScreenUV.y > UVMax.y ? ScreenUV.y : UVMax.y;
    }
    
    for(uint MipLevel = 0; MipLevel < VSM_MIP_NUM; MipLevel++)
    {
        uint2 TileIndexMin = uint2(UVMin * MipLevelSize[MipLevel]);
        uint2 TileIndexMax = uint2(UVMax * MipLevelSize[MipLevel]);

        for(uint IndexY = TileIndexMin.y ; IndexY <= TileIndexMax.y ; IndexY++)
        {
            for(uint IndexX = TileIndexMin.x ; IndexX <= TileIndexMax.x ; IndexX++)
            {
                if(IndexX < MipLevelSize[MipLevel] && IndexX >0 && IndexY < MipLevelSize[MipLevel] && indexY >0)
                {
                    int DestTileInfoIndex = MipLevelOffset[MipLevel] + IndexY * MipLevelSize[MipLevel] + IndexX;
                    VirtualShadowMapTileStateCacheMiss[DestTileInfoIndex] = TILE_STATE_CACHE_MISS;
                }
            }
        }
    }

}
