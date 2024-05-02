#include "Common.hlsl"
#include "Math.hlsl"
#include "VirtualShadowMapCommon.hlsl"

// todo:
// clear virtual shadow map tile state
// add tile state visualize pass

cbuffer CbShadowViewInfo
{
    row_major float4x4 LightViewProjectMatrix;
    float3 WorldCameraPosition;
};

cbuffer CbVisualizeParameters
{
    // 0 : Visualize Mip Level
    // 1 : Visualize Tile State
    uint VisualizeType;
};

Texture2D SceneDepthTexture;
StructuredBuffer<uint> VirtualShadowMapTileState;
StructuredBuffer<uint> VirtualShadowMapTileStateCacheMiss;
StructuredBuffer<uint> VirtualShadowMapTileAction;

RWTexture2D<float4> OutputVisualizeTexture; // 32 * 32 + 16 * 16 + 8 * 8

void VisualizeTileState(uint2 TileIndex,uint MipLevel, in out float3 VisualizeColor)
{
   int DestTileInfoIndex = MipLevelOffset[MipLevel] + TileIndex.y * MipLevelSize[MipLevel] + TileIndex.x;
   uint TileState = VirtualShadowMapTileState[DestTileInfoIndex];
   uint CacheMissState = VirtualShadowMapTileStateCacheMiss[DestTileInfoIndex];

   if(TileState == TILE_STATE_USED)
   {
       VisualizeColor.x = 1.0;
       if(CacheMissState == TILE_STATE_CACHE_MISS)
        {
            VisualizeColor.y = 1.0;
        }
   }
   else
   {
       if((TileIndex.x % 2) == (TileIndex.y % 2))
       {
           VisualizeColor.z = 1.0;
       }
       else
       {
           VisualizeColor.z = 0.5;
       }
   }
}

void VisualizeTileAction(uint2 TileIndex,uint MipLevel, in out float3 VisualizeColor)
{
   int DestTileInfoIndex = MipLevelOffset[MipLevel] + TileIndex.y * MipLevelSize[MipLevel] + TileIndex.x;
   uint TileAction = VirtualShadowMapTileAction[DestTileInfoIndex];

   if(TileAction == TILE_ACTION_NEED_UPDATE)
   {
       VisualizeColor.x = 1.0;
   }
   else if(TileAction == TILE_ACTION_NEED_REMOVE)
   {
        VisualizeColor.y = 1.0;
   }
   else if(TileAction == TILE_ACTION_CACHED)
   {
        VisualizeColor.yz = 1.0;
   }
   else
   {
       if((TileIndex.x % 2) == (TileIndex.y % 2))
       {
           VisualizeColor.z = 1.0;
       }
       else
       {
           VisualizeColor.z = 0.5;
       }
   }
}

[numthreads(16, 16, 1)]
void VSMVisualizeCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID,uint2 DispatchThreadID :SV_DispatchThreadID)
{
    float3 VisualizeColor = 0;
    float2 UV = DispatchThreadID * View_BufferSizeAndInvSize.zw;
    if(UV.x > 1.0f || UV.y > 1.0f)
    {
        return;
    }

    //OutputVisualizeTexture[DispatchThreadID] = float4(VisualizeColor.xyz,1.0);

    if(VisualizeType == 0)
    {
        float DeviceZ = SceneDepthTexture.Load(int3(DispatchThreadID.xy,0));
        if(DeviceZ == 0.0)
        {
            return;
        }

        float2 ScreenPos = UV * 2.0f - 1.0f; 
        ScreenPos.y *= -1.0f;

        float4 NDCPosNoDivdeW = float4(ScreenPos, DeviceZ, 1.0);
        float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse, NDCPosNoDivdeW);
        WorldPosition.xyz /= WorldPosition.w;

        float4 ShadowScreenPOS = mul(float4(WorldPosition.xyz,1.0), LightViewProjectMatrix);
        ShadowScreenPOS.xyz /= ShadowScreenPOS.w;

        float2 UVOut = ShadowScreenPOS.xy; 
        UVOut.y *=- 1.0;
        UVOut = UVOut* 0.5 + 0.5f;

        float Distance = length(WorldCameraPosition - WorldPosition.xyz);
        float Log2Distance = log2(Distance + 1);

        int MipLevel = clamp(Log2Distance - VSM_CLIPMAP_MIN_LEVEL, 0, VSM_MIP_NUM - 1);
        uint2 VSMTileIndex = uint2(UVOut * MipLevelSize[MipLevel]);


        if(MipLevel == 0)
        {
            VisualizeColor.x = 1.0;
        }
        else if(MipLevel == 1)
        {
            VisualizeColor.y = 1.0;
        }
        else if(MipLevel == 2)
        {
            VisualizeColor.z = 1.0;
        }

        OutputVisualizeTexture[DispatchThreadID] += float4(VisualizeColor.xyz,1.0);
    }

    if(VisualizeType == 1)
    {
        if(GroupID.x < 32 && GroupID.y < 32)
        {   
            VisualizeTileState(GroupID, 0, VisualizeColor);
        }

        if(GroupID.x >= 32 && GroupID.x < (32 + 16) && GroupID.y < 16)
        {   
            VisualizeTileState(GroupID - uint2(32,0), 1, VisualizeColor);
        }

        if(GroupID.x >= (32 + 16) && GroupID.x < (32 + 16 + 8) && GroupID.y < 8)
        {   
            VisualizeTileState(GroupID - uint2(32 + 16,0), 2, VisualizeColor);
        }

        OutputVisualizeTexture[DispatchThreadID] += float4(VisualizeColor.xyz,1.0);
    }
    
    if(VisualizeType == 2)
    {
        if(GroupID.x < 32 && GroupID.y < 32)
        {   
            VisualizeTileAction(GroupID, 0, VisualizeColor);
        }

        if(GroupID.x >= 32 && GroupID.x < (32 + 16) && GroupID.y < 16)
        {   
            VisualizeTileAction(GroupID - uint2(32,0), 1, VisualizeColor);
        }

        if(GroupID.x >= (32 + 16) && GroupID.x < (32 + 16 + 8) && GroupID.y < 8)
        {   
            VisualizeTileAction(GroupID - uint2(32 + 16,0), 2, VisualizeColor);
        }

        OutputVisualizeTexture[DispatchThreadID] += float4(VisualizeColor.xyz,1.0);
    }
}