#include "VirtualShadowMapCommon.hlsl"
StructuredBuffer<uint> VirtualShadowMapTileTable;
StructuredBuffer<uint> VirtualShadowMapTileAction;
RWStructuredBuffer<uint> VirtualShadowMapTileTablePacked_UAV;
RWStructuredBuffer<uint> TileNeedUpdateCounter_UAV; //todo: clear

[numthreads(VSM_TILE_MAX_MIP_NUM_XY, VSM_TILE_MAX_MIP_NUM_XY , 1)]
void VSMTileTableUpdatedPackCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint GroupIdxX = GroupID.x;
    const uint MipLevel = GroupIdxX >= MipLevelGroupStart[2] ? 2 : (GroupIdxX >= MipLevelGroupStart[1] ? 1 : 0);
    const uint2 TileIndexXY = GroupThreadID.xy;

    const uint GlobalMipOffset = MipLevelGroupOffset[MipLevel];
    const uint SubMipOffset = (GroupIdxX - MipLevelGroupStart[MipLevel]) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY;
    const uint SubTileOffset = TileIndexXY.y * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.x;

    const uint GlobalTileIndex = GlobalMipOffset + SubMipOffset + SubTileOffset;

    uint TileAction = VirtualShadowMapTileAction[GlobalTileIndex];
    if(TileAction & TILE_ACTION_NEED_UPDATE)
    {
        uint OriginalValue = 0;
        InterlockedAdd(TileNeedUpdateCounter_UAV[0],1,OriginalValue);
        VirtualShadowMapTileTablePacked_UAV[OriginalValue] = VirtualShadowMapTileTable[GlobalTileIndex];
    }
}
 
StructuredBuffer<uint> VirtualShadowMapTileTablePacked_SRV;
StructuredBuffer<uint> TileNeedUpdateCounter_SRV;
RWTexture2D<uint> PhysicalShadowDepthTexture;
// Dispatch Size X: 24 Y: 16 
[numthreads(VSM_TILE_TEX_PHYSICAL_SIZE / 8, VSM_TILE_TEX_PHYSICAL_SIZE / 8 , 1)]
void VSMTileTableClearCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint TiltTableIndex = GroupID.x;
    if(TiltTableIndex < TileNeedUpdateCounter_SRV[0])
    {
        uint PhysicalTileIndex = VirtualShadowMapTileTablePacked_SRV[TiltTableIndex];
        uint PhysicalTileIndexX = (PhysicalTileIndex >>  0) & 0xFFFF;
        uint PhysicalTileIndexY = (PhysicalTileIndex >> 16) & 0xFFFF;

        uint2 StartPos = uint2(PhysicalTileIndexX * VSM_TILE_TEX_PHYSICAL_SIZE, PhysicalTileIndexY * PhysicalTileIndexY);

        uint RawTexelToClearPerThread = 4;
        const uint BrickNumX = 4;
        const uint BrickNumY = 4;
        uint2 BrickIndex = uint2(GroupID.y % BrickNumX,  GroupID.y / BrickNumY);
        
        const uint BrickSizeX = VSM_TILE_TEX_PHYSICAL_SIZE / 4; // 64
        const uint BrickSizeY = VSM_TILE_TEX_PHYSICAL_SIZE / 4; // 64

        uint StartClearX = BrickIndex.x * BrickSizeX + GroupThreadID.x + StartPos.x;
        uint StartClearY = BrickIndex.y * BrickSizeY + GroupThreadID.y + StartPos.y;

        // Brick Texture Szie: 64 * 64
        // Group Size: 32 * 32
        // 2 * 2 Pixels Per Thread
        for(uint ClearIndexY = StartClearY; ClearIndexY < StartClearY + 2; ClearIndexY++)
        {
            for(uint ClearIndexX = StartClearX; ClearIndexX < StartClearX + 2; ClearIndexX++)
            {
                PhysicalShadowDepthTexture[uint2(ClearIndexX, ClearIndexY)] = 0;
            }
        }
    }
}


