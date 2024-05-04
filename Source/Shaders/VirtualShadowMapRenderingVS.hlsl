#include "VirtualShadowMapCommon.hlsl"

cbuffer cbPerObject
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter;
    uint isDynamicObejct;
    float3 BoundingBoxExtent;
    float padding1;
};

cbuffer LightSubProjectMatrix
{
    row_major float4x4 LightViewProjectMatrix;
    uint VirtualTableIndexX;
    uint VirtualTableIndexY;
    uint MipLevel;
    uint padding2;
};

struct SVSinput
{
    float4	Position	: ATTRIBUTE0;
};

void VSMVS(SVSinput Input, out float4 Position : SV_POSITION)
{
    float4 PositionW = mul( Input.Position, gWorld);
    Position = mul(PositionW, LightViewProjectMatrix);
}



