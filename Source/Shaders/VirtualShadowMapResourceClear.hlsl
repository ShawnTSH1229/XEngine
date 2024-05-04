cbuffer VSMClearParameters
{
   uint VirtualShadowMapTileStateSize;
};

RWStructuredBuffer<uint> VirtualShadowMapTileState; // 32 * 32 + 16 * 16 + 8 * 8
RWStructuredBuffer<uint> VirtualShadowMapTileStateCacheMiss;
RWStructuredBuffer<uint> VirtualShadowMapTileTable;
RWStructuredBuffer<uint> VirtualShadowMapTileAction;
RWStructuredBuffer<uint> CommandCounterBuffer;

//Dispatch size 64 + 16 + 2 = 82

[numthreads(16, 16, 1)]
void VSMResourceClear(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    if(DispatchThreadID.x == 0 && DispatchThreadID.y == 0)
    {
        CommandCounterBuffer[0] = 0;
    }

    uint GlobalTileIndex = GroupID.x * 16 * 16 + GroupThreadID.y * 16 + GroupThreadID.x;
    if(GlobalTileIndex < VirtualShadowMapTileStateSize)
    {
        VirtualShadowMapTileState[GlobalTileIndex] = 0;
        VirtualShadowMapTileStateCacheMiss[GlobalTileIndex] = 0;
        VirtualShadowMapTileTable[GlobalTileIndex] = 0;
        VirtualShadowMapTileAction[GlobalTileIndex] = 0;
    }
}