#include "VirtualShadowMapCommon.hlsl"

cbuffer cbPerObject
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter;
    uint isDynamicObejct;
    float3 BoundingBoxExtent;
    float padding1;
};

StructuredBuffer<SShadowViewInfo> ShadowTileViewParameters;

struct SVSinput
{
    float4	Position	: ATTRIBUTE0;
};

void VSMVS(SVSinput Input, uint InstanceID : SV_InstanceID, out float4 Position : SV_POSITION, out nointerpolation uint InstanceIndex : TEXCOORD0)
{
    float4 PositionW = mul( Input.Position, gWorld);
    SShadowViewInfo ShadowViewInfo = ShadowTileViewParameters[InstanceID];
    Position = mul(PositionW, ShadowViewInfo.ShadowViewProject);
    InstanceIndex = InstanceID;
}

RWTexture2D<uint> PhysicalShadowDepthTexture; // 4096 * 4096
StructuredBuffer<uint> VirtualShadowMapTileTable; // 24 bit

void VSMPS(in float4 PositionIn: SV_POSITION, in nointerpolation uint InstanceIndex : TEXCOORD0, out float PlaceHolderTarget : SV_TARGET)
{
    if( PositionIn.z < 0.0f || PositionIn.z > 1.0f)
    {
        PlaceHolderTarget = float4(0.0,1,0,0);
        return;
    }

    uint TileInfo = VirtualShadowMapTileTable[InstanceIndex];
    uint PhysicalTileIndexX = (TileInfo >>  0) & 0xFFFF;
    uint PhysicalTileIndexY = (TileInfo >> 16) & 0xFFFF;

    float2 IndexXY = uint2(PhysicalTileIndexX, PhysicalTileIndexY) * VSM_TILE_TEX_PHYSICAL_SIZE;
    float2 WritePos = IndexXY + PositionIn.xy;
    double FixedPointDepth = double(PositionIn.z) * uint(0xFFFFFFFF);
    uint UintDepth = FixedPointDepth; //float Point to fixed point

    InterlockedMax(PhysicalShadowDepthTexture[uint2(WritePos)],UintDepth);

    PlaceHolderTarget = isDynamicObejct ? 0.0f : 1.0f;
}




