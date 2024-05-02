#include "VirtualShadowMapCommon.hlsl"
StructuredBuffer<uint> VirtualShadowMapTileState;
StructuredBuffer<uint> VirtualShadowMapTileStateCacheMiss;

StructuredBuffer<uint> VirtualShadowMapPreTileState;
StructuredBuffer<uint> VirtualShadowMapPreTileTable;

// [0 : 8 ) MipLevel
// [8 : 16) Physical Tile Index X
// [16: 24) Physical Tile Index Y
RWStructuredBuffer<uint> VirtualShadowMapTileTable;
RWStructuredBuffer<uint> VirtualShadowMapTileAction;

// Mip0 4 * 4 = 16 Group
// Mip1 2 * 2 = 4 Group
// Mip2 1 * 1 = 1 Group
// Total 21 Group

// GroupSize X 21
// Group Size 8 * 8
[numthreads(VSM_TILE_MAX_MIP_NUM_XY, VSM_TILE_MAX_MIP_NUM_XY , 1)]
void VSMUpdateTileActionCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint GroupIdxX = GroupID.x;
    const uint MipLevel = GroupIdxX >= MipLevelGroupStart[2] ? 2 : (GroupIdxX >= MipLevelGroupStart[1] ? 1 : 0);
    const uint2 TileIndexXY = GroupThreadID.xy;

    const uint GlobalMipOffset = MipLevelGroupOffset[MipLevel];
    const uint SubMipOffset = (GroupIdxX - MipLevelGroupStart[MipLevel]) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY;
    const uint SubTileOffset = TileIndexXY.y * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.x;

    const uint GlobalTileIndex = GlobalMipOffset + SubMipOffset + SubTileOffset;

    uint TileState = VirtualShadowMapTileState[GlobalTileIndex];
    uint CacheMissAction = VirtualShadowMapTileStateCacheMiss[GlobalTileIndex];
    uint PreTileState = VirtualShadowMapPreTileState[GlobalTileIndex];

    bool bTileUsed = (TileState == TILE_STATE_USED);
    bool bPreTileUsed = (PreTileState == TILE_STATE_USED);

    bool bNewTile = (TileState == TILE_STATE_USED) && (PreTileState == TILE_STATE_UNUSED);
    bool bCacheMissTile = (CacheMissAction == TILE_STATE_CACHE_MISS) && (PreTileState == TILE_STATE_USED);
    
    bool bTileRemove = (TileState == TILE_STATE_UNUSED) && (PreTileState == TILE_STATE_USED);

    bool bTileActionCached = (bPreTileUsed) && (bTileUsed) && (!bCacheMissTile);
    bool bTileActionNeedUpdate = bNewTile || bCacheMissTile;
    bool bTileActionNeedRemove = bTileRemove;

    uint TileAction = 0;

    if(bTileActionCached)
    {
        VirtualShadowMapTileTable[GlobalTileIndex] = VirtualShadowMapPreTileTable[GlobalTileIndex];
        TileAction = TILE_ACTION_CACHED;
    }
    
    if(bTileActionNeedUpdate)
    {
        TileAction = TILE_ACTION_NEED_UPDATE;
    }

    if(bTileActionNeedRemove)
    {
        TileAction = TILE_ACTION_NEED_REMOVE;
    }

    VirtualShadowMapTileAction[GlobalTileIndex] = TileAction;
}
