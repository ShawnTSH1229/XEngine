
#include "SVOGICommon.hlsl"
#include "SVOGINodeCountBuffer.hlsl"

// 1.0 / 1024
#define ShadowMapWidth 0.0009765625
#define ThreadBlockSize 128
StructuredBuffer<Voxel> VoxelArrayR; 

//[numthreads(ThreadBlockSize, 1, 1)]
//void ResetOctreeFlagsCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
//{
//    uint NodeOffset = (groupId.x * ThreadBlockSize) + groupIndex;
//
//    uint NodesCountCurrentLevel = GetNodesCountByLevel(OctreeHeight );
//    uint NodesCountCurrentLevel = GetNodesCountByLevel(OctreeHeight - 1);
//    if( NodeOffset > NodesCountCurrentLevel)
//    {
//        return;
//    }
//
//    uint CurrentOctreeLevelOffsetInBuffer = LevelOffsetInBuffer( OctreeHeight - 1 );
//    uint NodeIndex = (CurrentOctreeLevelOffsetInBuffer + NodeOffset) * SIZE_OF_NODE_STRUCT;
//
//    uint2 FlagCoords = GetFlagCoords( NodeIndex );
//    if ( (SpaseVoxelOctreeRW[FlagCoords] & NODE_LIT ) != 0x0)
//    {
//        SpaseVoxelOctreeRW[FlagCoords] = NODE_HASINITED;
//    }
//}

float3 UnpackUintToFloat3( uint value )
{
    return float3( ( ( value )       & 0xff ) * 0.003922,
                   ( ( value >> 8  ) & 0xff ) * 0.003922,
                   ( ( value >> 16 ) & 0xff ) * 0.003922 );
}

#define brickBufferSize 64
#define BRICK_SIZE 3
uint3 NodeIDToTextureCoords( uint id )
{
    uint blocksPerAxis = brickBufferSize / BRICK_SIZE; // 3x3x3 values per brick
    uint blocksPerAxisSq = blocksPerAxis * blocksPerAxis;
    return uint3( id % blocksPerAxis, (id / blocksPerAxis) % blocksPerAxis, id / blocksPerAxisSq ) * BRICK_SIZE;
}

Texture2D SceneDepthInput;
cbuffer cbShadow
{
    row_major float4x4 LightViewProjectMatrix;
    row_major float4x4 LightViewProjectMatrixInv;
}
cbuffer cbVoxelization
{
	float3 MinBound;
    float VoxelBufferDimension;
    float3 MaxBound;
    float cbVoxelization_Padding0;
};

cbuffer cbMainLight
{
    float3 LightDir;
    float LightIntensity;
    float3 LightColor;
    float cbMainLight_padding1;
}

RWTexture3D<float4> IrradianceBrickBufferRW;
bool PhotonTraverseOctree2( in uint3 VoxelSpaceIndex, out uint NodeIndex, out uint parentIndex, out uint3 SubOctreeIndex )
{
    if ( any( step( OctreeResolution, VoxelSpaceIndex ) ) )
        return false;

    uint3 CurrentMinPosIndex = 0;
    uint HalfSize = OctreeResolution;

    NodeIndex = 0;
    parentIndex = 0;
    SubOctreeIndex = 0;
    
    uint2 flagC = 0;
    [unroll]
    for ( uint treeLevel = 0; treeLevel < OctreeHeight - 1; treeLevel++ )
    {
        parentIndex = NodeIndex;
        GetSubNodeIndex(VoxelSpaceIndex,SubOctreeIndex,HalfSize,CurrentMinPosIndex,NodeIndex);
        NodeIndex = SpaseVoxelOctreeRW[IndexToCoords( NodeIndex )].r;
        
        if(NodeIndex == NODE_UNKOWN)
        {
            return false;
        }

        flagC = GetFlagCoords( parentIndex );
        SpaseVoxelOctreeRW[flagC] = NODE_LIT;
    }

    return true;
}
[numthreads(16, 16, 1)]
void ShadowMapInjectLightCS(uint2 ThreadID : SV_DispatchThreadID)
{
    float DeviceZ = SceneDepthInput.Load(int3(ThreadID,0));
    float2 UV = ThreadID * float(ShadowMapWidth);

    if(DeviceZ == 0.0)
    {
        return;
    }

    float2 ScreenPos = UV * 2.0f - 1.0f; ScreenPos.y *= -1.0f;
    float4 NDCPosNoDivdeW = float4(ScreenPos , DeviceZ , 1.0);
    float4 WorldPosition = mul(NDCPosNoDivdeW,LightViewProjectMatrixInv);
    WorldPosition.xyz/=WorldPosition.w;

    uint3 VoxelSpaceIndex = (( WorldPosition.xyz - MinBound ) / ( MaxBound - MinBound ) ) * VoxelBufferDimension; 

    uint NodeIndex;
    uint parentIndex;
    uint3 SubOctreeIndex;
    if(PhotonTraverseOctree2(VoxelSpaceIndex,NodeIndex,parentIndex,SubOctreeIndex))
    {
        Voxel v = VoxelArrayR[NodeIndex];
        float3 Color = UnpackUintToFloat3(v.Color);
        float3 Normal = UnpackUintToFloat3(v.Normal) * 2.0f - 1.0f;
        float4 outputEnergy = float4( saturate( dot( Normal, LightDir) ) * Color.rgb * LightIntensity * LightColor.rgb, 1.0f ); 
        
        uint3 brickCoords = NodeIDToTextureCoords( parentIndex / SIZE_OF_NODE_STRUCT );
        uint3 localOffset = SubOctreeIndex * 2;
        IrradianceBrickBufferRW[brickCoords + localOffset] = outputEnergy;
    }
}




Texture3D<float4> IrradianceBrickBufferROnly;
RWTexture3D<float4> IrradianceBrickBufferWOnly;
void AverageTexelsArray( inout float4 texels[27] )
{
//
//                Z                                          interpolate values between corners
//               /\
//  Y           /
//  \        5.0----|*|----6.0                         5.0----5.5----6.0                         5.0----5.5----6.0                         5.0----5.5----6.0
//  |\       /      /      /|                          /      /      /|                          /      /      /|                          /      /      /|
//  |      /      /      /  |                        /      /      /  |                        /      /      /  |                        /      /      /  |
//  |   |*|----|*|----|*|  |*|                    |*|----|*|----|*|  |*|                    |*|----|*|----|*|  7.0                    3.0----3.5----4.0  7.0
//  |   /      /      /|   /|   Interpolate       /      /      /|   /|   Interpolate       /      /      /|   /|   Interpolate       /      /      /|   /|
//  | /      /      /  | /  |  edges along X    /      /      /  | /  |  edges along Y    /      /      /  | /  |  edges along Z    /      /      /  | /  |
// 1.0----|*|----2.0  |*|  8.0 ------------> 1.0----1.5----2.0  |*|  8.0 ------------> 1.0----1.5----2.0  |*|  8.0 ------------> 1.0----1.5----2.0  5.0  8.0
//  |      |      |   /|   /     4 values     |      |      |   /|   /     6 values     |      |      |   /|   /     9 values     |      |      |   /|   /
//  |      |      | /  | /                    |      |      | /  | /                    |      |      | /  | /                    |      |      | /  | /
// |*|----|*|----|*|  |*|                    |*|----|*|----|*|  |*|                    2.0----2.5----3.0  |*|                    2.0----2.5----3.0  6.0
//  |      |      |   /                       |      |      |   /                       |      |      |   /                       |      |      |   /
//  |      |      | /                         |      |      | /                         |      |      | /                         |      |      | /
// 3.0----|*|----4.0------> X                3.0----3.5----4.0                         3.0----3.5----4.0                         3.0----3.5----4.0
//

    // along X
    const uint sizeAlongX = 4;
    const uint3 coordsAlongX[sizeAlongX] = {
        uint3( 1, 0, 0 ), 
        uint3( 1, 2, 0 ),
        uint3( 1, 0, 2 ),
        uint3( 1, 2, 2 )
    };
    
    [unroll]
    for ( uint i = 0; i < sizeAlongX; i++ )
    {
        uint index = coordsAlongX[i].x + coordsAlongX[i].y * BRICK_SIZE + coordsAlongX[i].z * BRICK_SIZE * BRICK_SIZE;
        uint3 InterpolateA = uint3( 0, coordsAlongX[i].yz );
        uint3 InterpolateB = uint3( 2, coordsAlongX[i].yz );
        uint indexA = InterpolateA.x + InterpolateA.y * BRICK_SIZE + InterpolateA.z * BRICK_SIZE * BRICK_SIZE;
        uint indexB = InterpolateB.x + InterpolateB.y * BRICK_SIZE + InterpolateB.z * BRICK_SIZE * BRICK_SIZE;
        texels[index] = ( texels[indexA] + texels[indexB] ) * 0.5f;
    }

    // along Y
    const uint sizeAlongY = 6;
    const uint3 coordsAlongY[sizeAlongY] = {
        uint3( 0, 1, 0 ),
        uint3( 1, 1, 0 ),
        uint3( 2, 1, 0 ),
        uint3( 0, 1, 2 ),
        uint3( 1, 1, 2 ),
        uint3( 2, 1, 2 )
    };
    
    [unroll]
    for ( i = 0; i < sizeAlongY; i++ )
    {
        uint index = coordsAlongY[i].x + coordsAlongY[i].y * BRICK_SIZE + coordsAlongY[i].z * BRICK_SIZE * BRICK_SIZE;
        uint3 InterpolateA = uint3( coordsAlongY[i].x, 0, coordsAlongY[i].z );
        uint3 InterpolateB = uint3( coordsAlongY[i].x, 2, coordsAlongY[i].z );
        uint indexA = InterpolateA.x + InterpolateA.y * BRICK_SIZE + InterpolateA.z * BRICK_SIZE * BRICK_SIZE;
        uint indexB = InterpolateB.x + InterpolateB.y * BRICK_SIZE + InterpolateB.z * BRICK_SIZE * BRICK_SIZE;
        texels[index] = ( texels[indexA] + texels[indexB] ) * 0.5f;
    }

    // along Z
    const uint sizeAlongZ = 9;
    const uint3 coordsAlongZ[sizeAlongZ] = {
        uint3( 0, 0, 1 ), uint3( 1, 0, 1 ), uint3( 2, 0, 1 ),
        uint3( 0, 1, 1 ), uint3( 1, 1, 1 ), uint3( 2, 1, 1 ),
        uint3( 0, 2, 1 ), uint3( 1, 2, 1 ), uint3( 2, 2, 1 )
    };
    
    [unroll]
    for ( i = 0; i < sizeAlongZ; i++ )
    {
        uint index = coordsAlongZ[i].x + coordsAlongZ[i].y * BRICK_SIZE + coordsAlongZ[i].z * BRICK_SIZE * BRICK_SIZE;
        uint3 InterpolateA = uint3( coordsAlongZ[i].xy, 0 );
        uint3 InterpolateB = uint3( coordsAlongZ[i].xy, 2 );
        uint indexA = InterpolateA.x + InterpolateA.y * BRICK_SIZE + InterpolateA.z * BRICK_SIZE * BRICK_SIZE;
        uint indexB = InterpolateB.x + InterpolateB.y * BRICK_SIZE + InterpolateB.z * BRICK_SIZE * BRICK_SIZE;
        texels[index] = ( texels[indexA] + texels[indexB] ) * 0.5f;
    }
}

[numthreads(ThreadBlockSize, 1, 1)]
void AverageLitNodeValuesCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint NodeOffset = (groupId.x * ThreadBlockSize) + groupIndex;
    uint NodesCountCurrentLevel = GetNodesCountByLevel(OctreeHeight - 2);
    if( NodeOffset > NodesCountCurrentLevel)
    {
        return;
    }

    uint CurrentOctreeLevelOffsetInBuffer = LevelOffsetInBuffer( OctreeHeight - 2);
    uint NodeID  = CurrentOctreeLevelOffsetInBuffer + NodeOffset;
    
    float4 texels[27] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };

    const uint3 cornersCoord[8] = {
        uint3( 0, 0, 0 ), 
        uint3( 2, 0, 0 ),
        uint3( 0, 2, 0 ),
        uint3( 2, 2, 0 ),
        uint3( 0, 0, 2 ),
        uint3( 2, 0, 2 ),
        uint3( 0, 2, 2 ),
        uint3( 2, 2, 2 )
    };

    uint3 brickCoords = NodeIDToTextureCoords( NodeID );

    // copy texels from brickBuffer to average them
    [unroll]
    for ( uint i = 0; i < 8; i++ )
    {
        uint index = cornersCoord[i].x + cornersCoord[i].y * BRICK_SIZE + cornersCoord[i].z * BRICK_SIZE * BRICK_SIZE;
        float4 DebugTemp = IrradianceBrickBufferROnly.Load(int4(brickCoords + cornersCoord[i],0));
        texels[index] = DebugTemp;
    }

    AverageTexelsArray( texels );

    [unroll]
    for ( i = 0; i < 27; i++ )
    {
        uint3 localOffset = uint3( i % BRICK_SIZE, (i / BRICK_SIZE) % BRICK_SIZE, i / (BRICK_SIZE * BRICK_SIZE) );
        IrradianceBrickBufferWOnly[brickCoords + localOffset] = texels[i];
    }
}


cbuffer cbSVOBuildBuffer
{
    uint CurrentOctreeLevel;
}

#define SIDES_COUNT 6
static const uint3 sidesCoords[SIDES_COUNT] = {
    uint3( 0, 1, 1 ), uint3( 1, 0, 1 ), uint3( 1, 1, 0 ),
    uint3( 2, 1, 1 ), uint3( 1, 2, 1 ), uint3( 1, 1, 2 )
};

#define EDGES_COUNT 12
static const uint3 edgesCoords[EDGES_COUNT] = {
    uint3( 1, 0, 0 ), uint3( 1, 2, 0 ), uint3( 1, 0, 2 ), uint3( 1, 2, 2 ), 
    uint3( 0, 1, 0 ), uint3( 2, 1, 0 ), uint3( 0, 1, 2 ), uint3( 2, 1, 2 ),
    uint3( 0, 0, 1 ), uint3( 2, 0, 1 ), uint3( 0, 2, 1 ), uint3( 2, 2, 1 )
};

#define CORNERS_COUNT 8
static const uint3 CornerOffset[CORNERS_COUNT] = {
    uint3( 0, 0, 0 ),
    uint3( 1, 0, 0 ),
    uint3( 0, 1, 0 ),
    uint3( 1, 1, 0 ),
    uint3( 0, 0, 1 ),
    uint3( 1, 0, 1 ),
    uint3( 0, 1, 1 ),
    uint3( 1, 1, 1 )
};



[numthreads(ThreadBlockSize, 1, 1)]
void GatherValuesFromLowLevelCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint NodeOffset = (groupId.x * ThreadBlockSize) + groupIndex;
    uint NodesCountCurrentLevel = GetNodesCountByLevel(CurrentOctreeLevel);
    if( NodeOffset > NodesCountCurrentLevel)
    {
        return;
    }

    uint CurrentOctreeLevelOffsetInBuffer = LevelOffsetInBuffer( CurrentOctreeLevel );
    uint NodeID  = CurrentOctreeLevelOffsetInBuffer + NodeOffset;
    uint CurrentNodeIndex = NodeID * SIZE_OF_NODE_STRUCT;

    uint nodeFlag = SpaseVoxelOctreeR[GetFlagCoords( CurrentNodeIndex )];
        
    if ( nodeFlag & NODE_LIT  == 0x0 )
        return;

    uint3 brickCoords = NodeIDToTextureCoords( NodeID );
    float4 texels[27] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };
    
    static float4 brick[27];

    [unroll]
    for ( uint i = 0; i < 8; i++ )
    {
        float Opacity = 0.0;
        float3 Color = float3(0,0,0);
        int childIndex = SpaseVoxelOctreeR[GetChildCoordsInBuffer( CurrentNodeIndex, i )];
        
        uint nodeFlagLocal = SpaseVoxelOctreeR[GetFlagCoords( childIndex )];
        if ( ( nodeFlagLocal & NODE_LIT ) == 0x0 )
            continue;

        uint3 ChilebrickCoords = NodeIDToTextureCoords( childIndex / SIZE_OF_NODE_STRUCT );

        [unroll]
        for ( uint j = 0; j < 27; j++ )
        {
            uint3 localOffset = uint3( j % BRICK_SIZE, (j / BRICK_SIZE) % BRICK_SIZE, j / (BRICK_SIZE * BRICK_SIZE) );
            float4 DebugTempColor = IrradianceBrickBufferROnly.Load(int4(ChilebrickCoords + localOffset,0));
            brick[j] = DebugTempColor;
            IrradianceBrickBufferWOnly[ChilebrickCoords + localOffset] = DebugTempColor;
        }

        [unroll]
        for ( uint k = 0; k < CORNERS_COUNT; k++ )
        {
            uint3 cornerCoords = CornerOffset[k] * 2;
            uint index = cornerCoords.x + cornerCoords.y * BRICK_SIZE + cornerCoords.z * BRICK_SIZE * BRICK_SIZE;
            Opacity += brick[index].a;
            Color += (brick[index].xyz * ( 8.0f / 512.0f));
        }

        Opacity /= 8.0f;

        [unroll]
        for ( uint l = 0; l < EDGES_COUNT; l++ )
        {
            uint3 edgesCoord = edgesCoords[l];
            uint index = edgesCoord.x + edgesCoord.y * BRICK_SIZE + edgesCoord.z * BRICK_SIZE * BRICK_SIZE;
            Color += (brick[index].xyz * (16.0f / 512.0f));
        }

        [unroll]
        for ( uint m = 0; m < SIDES_COUNT; m++ )
        {
            uint3 sidesCoord = sidesCoords[m];
            uint index = sidesCoord.x + sidesCoord.y * BRICK_SIZE + sidesCoord.z * BRICK_SIZE * BRICK_SIZE;
            Color += (brick[index].xyz * (32.0f / 512.0f));
        }

        uint centerIndex = 1 + 1 * BRICK_SIZE + 1 * BRICK_SIZE * BRICK_SIZE;
        Color += (brick[centerIndex].xyz * (64.0f / 512.0f));

        uint3 cornerIndices = CornerOffset[i] * 2;
        uint cornerIndex = cornerIndices.x + cornerIndices.y * BRICK_SIZE + cornerIndices.z * BRICK_SIZE * BRICK_SIZE;
        texels[cornerIndex] = float4(Color,Opacity);
    }

    AverageTexelsArray( texels );
   
    [unroll]
    for (uint n = 0; n < 27; n++ )
    {
        uint3 localOffset = uint3( n % BRICK_SIZE, (n / BRICK_SIZE) % BRICK_SIZE, n / (BRICK_SIZE * BRICK_SIZE) );
        IrradianceBrickBufferWOnly[brickCoords + localOffset] = texels[n];
    }
}
