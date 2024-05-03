#include "VirtualShadowMapCommon.hlsl"
//todo: clear the command counter buffer

struct SceneConstantBuffer
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter;//BoundingBox in World  Space
    uint isDynamicObejct;
    float3 BoundingBoxExtent;
    float b;
};

// this constant buffer is used for cache miss test
cbuffer CBDynamicObjectParameters
{
    row_major float4x4 DynamicgWorld;
    float3 DynamicBoundingBoxCenter; //world space
    uint DynamicIsDynamicObejct;
    float3 DynamicBoundingBoxExtent;
    float Dynamicpadding1;
};

cbuffer CBCullingParameters
{
    row_major float4x4 ShadowViewProject[3];
    uint MeshCount; 
};

struct ShadowIndirectCommand
{
    // cbv
    uint2 CbWorldAddress;
    
    // vb
    uint2 VertexBufferLoacation;
    
    uint VertexSizeInBytes;
    uint VertexStrideInBytes;
    
    // ib
    uint2 IndexBufferLoacation;
    
    // draw instanced index
    uint IndexSizeInBytes;
    uint IndexFormat;
    uint IndexCountPerInstance;
    uint InstanceCount;
    
    uint StartIndexLocation;
    int BaseVertexLocation;
    
    uint StartInstanceLocation;
    uint Padding;
};

StructuredBuffer<uint> VirtualShadowMapTileAction;
StructuredBuffer<SceneConstantBuffer> SceneConstantBufferIn;
StructuredBuffer<ShadowIndirectCommand> InputCommands;

RWStructuredBuffer<ShadowIndirectCommand> OutputCommands; 
RWStructuredBuffer<uint> CommandCounterBuffer;

// GroupSize X 21 : Tile Group Num
// GroupSize Y 50 : Mesh Batch Num
// Group Size 8 * 8
[numthreads(VSM_TILE_MAX_MIP_NUM_XY, VSM_TILE_MAX_MIP_NUM_XY , 1)]
void VSMTileCmdBuildCS(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID, uint2 DispatchThreadID: SV_DispatchThreadID)
{
    uint GroupIdxX = GroupID.x;
    const uint MipLevel = GroupIdxX >= MipLevelGroupStart[2] ? 2 : (GroupIdxX >= MipLevelGroupStart[1] ? 1 : 0);
    const uint2 TileIndexXY = GroupThreadID.xy;
    const uint MipTileIndex = (GroupIdxX - MipLevelGroupStart[MipLevel]) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.y * VSM_TILE_MAX_MIP_NUM_XY + TileIndexXY.x;
    const uint GlobalTileIndex = MipLevelGroupOffset[MipLevel] + MipTileIndex;
    const uint2 MipTileIndexXY = uint2(MipTileIndex % MipLevelSize[MipLevel], MipTileIndex / MipLevelSize[MipLevel]);

    uint VirtualShadowMapAction = VirtualShadowMapTileAction[GlobalTileIndex];
    if(VirtualShadowMapAction == TILE_ACTION_NEED_UPDATE)
    {
        uint StartBatchIndex = GroupID.y * 10;
        uint EndBatchIndex = StartBatchIndex + 10;

        for(uint Index = StartBatchIndex;  (Index < EndBatchIndex) && (Index < MeshCount); Index++)
        {
            SceneConstantBuffer SceneConstantBuffer = SceneConstantBufferIn[Index];
            uint isDynamicObejct = SceneConstantBuffer.isDynamicObejct;
            float3 BoundingBoxCenter = SceneConstantBuffer.BoundingBoxCenter;
            float3 BoundingBoxExtent = SceneConstantBuffer.BoundingBoxExtent;
            if(isDynamicObejct)
            {
                BoundingBoxCenter = DynamicBoundingBoxCenter;
                BoundingBoxExtent = DynamicBoundingBoxExtent;
            }

            float4 Corner[8];

            [unroll]
            for(uint i = 0; i < 8 ; i++)
            {
                Corner[i] = float4(BoundingBoxCenter ,0)  + float4(BoundingBoxExtent,0) * BoundingBoxOffset[i];
            }

            float2 UVMin = float2(1.1,1.1);
            float2 UVMax = float2(-0.1,-0.1);

            [unroll]
            for(uint j = 0; j < 8 ; j++)
            {
                float4 ScreenPosition = mul(float4(Corner[j].xyz,1.0f), ShadowViewProject[MipLevel]);
                ScreenPosition.xyz /= ScreenPosition.w;

                float2 ScreenUV = ScreenPosition.xy;
                ScreenUV.y *= -1.0;
                ScreenUV = ScreenUV * 0.5 + 0.5f;

                UVMin.x = ScreenUV.x > UVMin.x ? UVMin.x : ScreenUV.x;
                UVMin.y = ScreenUV.y > UVMin.y ? UVMin.y : ScreenUV.y;

                UVMax.x = ScreenUV.x > UVMax.x ? ScreenUV.x : UVMax.x;
                UVMax.y = ScreenUV.y > UVMax.y ? ScreenUV.y : UVMax.y;
            }


            uint2 TileIndexMin = uint2( UVMin * MipLevelSize[MipLevel]);
            uint2 TileIndexMax = uint2( UVMax * MipLevelSize[MipLevel]);

            if(TileIndexMin.x < MipTileIndexXY.x && MipTileIndexXY.y < MipTileIndexXY.y && TileIndexMax.x > MipTileIndexXY.x && TileIndexMax.y > MipTileIndexXY.y)
            {
                ShadowIndirectCommand InputCommand = InputCommands[Index];
                InputCommand.StartInstanceLocation = GlobalTileIndex;

                uint OriginalValue = 0;
                InterlockedAdd(CommandCounterBuffer[0], 1, OriginalValue);
                OutputCommands[OriginalValue] = InputCommand;
            }
        }
    }
}