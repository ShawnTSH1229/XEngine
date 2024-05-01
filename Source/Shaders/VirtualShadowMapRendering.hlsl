//todo: add a depth clear pass

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

void VS(SVSinput Input, uint InstanceID : SV_InstanceID, out float4 Position : SV_POSITION, out nointerpolation uint InstanceIndex : TEXCOORD0)
{
    float4 PositionW = mul( Input.Position, gWorld);
    SShadowViewInfo ShadowViewInfo = ShadowTileViewParameters[InstanceID];
    Position = mul(PositionW, ShadowViewInfo.ShadowViewProject);
}

RWTexture2D<uint>PhysicalShadowDepthTexture; // 4096 * 4096
Buffer<uint> VirtualShadowMapTileTable; // 24 bit

void PS(in float4 PositionIn: SV_POSITION, in nointerpolation uint InstanceIndex : TEXCOORD0, out float PlaceHolderTarget : SV_TARGET)
{
    if( PositionIn.z < 0.0f || PositionIn.z > 1.0f)
    {
        PlaceHolderTarget = float4(0.0,1,0,0);
        return;
    }

    uint TileInfo = VirtualShadowMapTileTable[InstanceIndex];
    uint PhysicalTileIndexX = (TileInfo >>  8) & 0xFF;
    uint PhysicalTileIndexY = (TileInfo >> 16) & 0xFF;

    float2 IndexXY = uint2(IndexX,IndexY) * VSM_TILE_TEX_PHYSICAL_SIZE;
    float2 WritePos = IndexXY + PositionIn.xy;
    double FixedPointDepth = double(PositionIn.z) * uint(0xFFFFFFFF);
    uint UintDepth = FixedPointDepth; //float Point to fixed point

    InterlockedMax(PhysicalShadowDepthTexture[uint2(WritePos)],UintDepth);

    PlaceHolderTarget = 0.0f;
}




