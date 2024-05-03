
#include "VirtualShadowMapCommon.hlsl"
//todo:
// 1. pack tile action buffer

StructuredBuffer<uint> VirtualShadowMapTileAction;

// [0 : 16) Physical Tile Index X
// [16: 32) Physical Tile Index Y

RWStructuredBuffer<uint> VirtualShadowMapTileTable; // allocate physical tile

RWStructuredBuffer<uint> VirtualShadowMapFreeTileList;
RWStructuredBuffer<int> VirtualShadowMapFreeListStart; // 0 - 8* 8

[numthreads(VSM_TILE_MAX_MIP_NUM_XY, VSM_TILE_MAX_MIP_NUM_XY , 1)]
void VSMTilePhysicalTileAllocCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint GroupIdxX = GroupID.x;
    const uint MipLevel = GroupIdxX >= MipLevelGroupStart[2] ? 2 : (GroupIdxX >= MipLevelGroupStart[1] ? 1 : 0);
    const uint2 TileIndexXY = GroupThreadID.xy;
    const uint GlobalTileIndex = MipLevelGroupOffset[MipLevel] + (GroupIdxX - MipLevelGroupStart[MipLevel]) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.y * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.x;

    uint VirtualShadowMapAction = VirtualShadowMapTileAction[GlobalTileIndex];

    if(VirtualShadowMapAction & TILE_ACTION_NEED_ALLOCATE)
    {
        uint FreeListIndex = 0;
        InterlockedAdd(VirtualShadowMapFreeListStart[0],1,FreeListIndex);
        if(FreeListIndex < VSM_TEX_PHYSICAL_WH * VSM_TEX_PHYSICAL_WH)
        {
            VirtualShadowMapTileTable[GlobalTileIndex] = VirtualShadowMapFreeTileList[FreeListIndex];
        }
    }
}

StructuredBuffer<uint> VirtualShadowMapPreTileTable; // release physical tile

[numthreads(VSM_TILE_MAX_MIP_NUM_XY, VSM_TILE_MAX_MIP_NUM_XY , 1)]
void VSMTilePhysicalTileReleaseCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint GroupIdxX = GroupID.x;
    const uint MipLevel = GroupIdxX >= MipLevelGroupStart[2] ? 2 : (GroupIdxX >= MipLevelGroupStart[1] ? 1 : 0);
    const uint2 TileIndexXY = GroupThreadID.xy;
    const uint GlobalTileIndex = MipLevelGroupOffset[MipLevel] + (GroupIdxX - MipLevelGroupStart[MipLevel]) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.y * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.x;

    uint VirtualShadowMapAction = VirtualShadowMapTileAction[GlobalTileIndex];

    if(VirtualShadowMapAction & TILE_ACTION_NEED_REMOVE)
    {
        int FreeListIndex = 0;
        InterlockedAdd(VirtualShadowMapFreeListStart[0],-1,FreeListIndex);
        if(FreeListIndex > 0)
        {
            uint RemoveTileIndex = VirtualShadowMapPreTileTable[GlobalTileIndex];
            VirtualShadowMapFreeTileList[FreeListIndex - 1] = RemoveTileIndex;
        }
    }
}
