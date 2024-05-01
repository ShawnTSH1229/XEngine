
#include "VirtualShadowMapCommon.hlsl"
//todo:
// 1. init VirtualShadowMapState Buffer
// 2. pack tile action buffer

Buffer<uint> VirtualShadowMapTileAction;


// [0 : 8 ) MipLevel
// [8 : 16) Physical Tile Index X
// [16: 24) Physical Tile Index Y
RWBuffer<uint> VirtualShadowMapTileTable; // 24 bit
RWBuffer<uint> VirtualShadowMapFreeTileList; // 24 bit

RWBuffer<uint> VirtualShadowMapFreeListStart; // 0 - 64

[numthreads(VSM_TILE_MAX_MIP_NUM_XY, VSM_TILE_MAX_MIP_NUM_XY , 1)]
void VSMTileAllocPhyCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint GroupIdxX = GroupID.x;
    const uint MipLevel = GroupIdxX >= MipLevelGroupStart[2] ? 2 : (GroupIdxX >= MipLevelGroupStart[1] ? 1 : 0);
    const uint2 TileIndexXY = GroupThreadID.xy;
    const uint GlobalTileIndex = MipLevelGroupOffset[MipLevel] + (GroupIdxX - MipLevelGroupStart[MipLevel]) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.y * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.x;

    uint VirtualShadowMapAction = VirtualShadowMapTileAction[GlobalTileIndex];

    int UpdateCount = 0;
    if(VirtualShadowMapAction == TILE_ACTION_NEED_UPDATE)
    {
        uint FreeListIndex = 0;
        InterlockedAdd(VirtualShadowMapFreeListStart[0],1,FreeListIndex);
        if(FreeListIndex < VSM_TILE_NUM_XY * VSM_TILE_NUM_XY)
        {
            VirtualShadowMapTileTable[GlobalTileIndex] = VirtualShadowMapFreeTileList[FreeListIndex];
        }
    }
}
