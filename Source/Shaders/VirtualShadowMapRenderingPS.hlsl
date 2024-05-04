#include "VirtualShadowMapCommon.hlsl"
cbuffer LightSubProjectMatrix
{
    row_major float4x4 LightViewProjectMatrix;
    uint VirtualTableIndexX;
    uint VirtualTableIndexY;
    uint MipLevel;
    uint padding2;
};

StructuredBuffer<uint> VirtualShadowMapTileTable; // 16 + 16 bit
RWTexture2D<uint> PhysicalShadowDepthTexture; // 2k * 2k

void VSMPS(in float4 PositionIn: SV_POSITION, out float4 PlaceHolderTarget : SV_TARGET)
{
    if( PositionIn.z < 0.0f || PositionIn.z > 1.0f)
    {
        PlaceHolderTarget = float4(0.0,1,0,0);
        return;
    }

    uint TableIndex = MipLevelOffset[MipLevel] + VirtualTableIndexY * MipLevelSize[MipLevel] + VirtualTableIndexX;
    uint TileInfo = VirtualShadowMapTileTable[TableIndex];
    uint PhysicalTileIndexX = (TileInfo >>  0) & 0xFFFF;
    uint PhysicalTileIndexY = (TileInfo >> 16) & 0xFFFF;

    float2 IndexXY = uint2(PhysicalTileIndexX, PhysicalTileIndexY) * VSM_TILE_TEX_PHYSICAL_SIZE;
    float2 WritePos = IndexXY + PositionIn.xy;
    float FixedPointDepth = float(PositionIn.z) * uint(0xFFFFFFFF);
    uint UintDepth = FixedPointDepth;

    InterlockedMax(PhysicalShadowDepthTexture[uint2(WritePos)],UintDepth);

    PlaceHolderTarget = float4(1.0,0,0,0);
}









