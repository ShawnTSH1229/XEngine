#include "VirtualShadowMapCommon.hlsl"

// this constant buffer is used for cache miss test
cbuffer CBDynamicObjectParameters
{
    row_major float4x4 DynamicgWorld;
    float3 DynamicBoundingBoxMax; //world space
    uint DynamicIsDynamicObejct;
    float3 DynamicBoundingBoxMin;
    float Dynamicpadding1;
};

cbuffer CBCullingParameters
{
    row_major float4x4 ShadowViewProject;
    uint MeshCount; 
};

struct UINT64
{
    uint LowAddress;
    uint HighAddress;
};

UINT64 UINT64_ADD(UINT64 InValue , uint InAdd)
{
    UINT64 Ret = InValue;
    uint C= InValue.LowAddress + InAdd;
    bool OverFlow = (C < InValue.HighAddress) || (C < InAdd);
    if(OverFlow)
    {
        Ret.HighAddress += 1;
    }
    Ret.LowAddress = C;
    return Ret;
}

struct ShadowIndirectCommand
{
    // cbv
    uint2 CbWorldAddress;
    
    UINT64 CbGlobalShadowViewProjectAddressVS; // Used For VSM
    UINT64 CbGlobalShadowViewProjectAddressPS; // Used For VSM

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

struct SceneConstantBuffer
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter;//BoundingBox in World  Space
    uint isDynamicObejct;
    float3 BoundingBoxExtent;
    float b;
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
    if(VirtualShadowMapAction & TILE_ACTION_NEED_UPDATE)
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
                BoundingBoxCenter = (DynamicBoundingBoxMax + DynamicBoundingBoxMin) * 0.5;
                BoundingBoxExtent = DynamicBoundingBoxMax - BoundingBoxCenter;
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
                float4 ScreenPosition = mul(float4(Corner[j].xyz,1.0f), ShadowViewProject);
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

            if(TileIndexMin.x <= MipTileIndexXY.x && TileIndexMin.y <= MipTileIndexXY.y && TileIndexMax.x >= MipTileIndexXY.x && TileIndexMax.y >= MipTileIndexXY.y)
            {
                uint PointerOffset = GlobalTileIndex * ( 4 * 4 * 4 + 4 * 4); // float4x4 + uint4
                ShadowIndirectCommand InputCommand = InputCommands[Index];
                InputCommand.CbGlobalShadowViewProjectAddressVS = UINT64_ADD(InputCommand.CbGlobalShadowViewProjectAddressVS,PointerOffset);
                InputCommand.CbGlobalShadowViewProjectAddressPS = InputCommand.CbGlobalShadowViewProjectAddressVS;
                InputCommand.StartInstanceLocation = GlobalTileIndex;

                uint OriginalValue = 0;
                InterlockedAdd(CommandCounterBuffer[0], 1, OriginalValue);
                OutputCommands[OriginalValue] = InputCommand;
            }
        }
    }
}