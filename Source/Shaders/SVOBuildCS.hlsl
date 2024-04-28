#include "SVOGICommon.hlsl"
#include "SVOGINodeCountBuffer.hlsl"

StructuredBuffer<Voxel> VoxelArray; 

cbuffer cbSVOBuildBuffer
{
    uint CurrentOctreeLevel;
}

#define ThreadBlockSize 128

///////////////////////////////////////////////////////////////////
[numthreads(ThreadBlockSize, 1, 1)]
void CheckSVOFlagCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint VoxelID = (groupId.x * ThreadBlockSize) + groupIndex;
    if(VoxelArray[VoxelID].Pad != 42)
    {
        return;
    }

    uint NodeIndex = 0;
    if ( SearchOctree( VoxelArray[VoxelID].Position, CurrentOctreeLevel, NodeIndex ) )
    {
        uint2 FlagCoords = GetFlagCoords(NodeIndex);
        if ( SpaseVoxelOctreeRW[FlagCoords] != NODE_HASINITED )
        {
            SpaseVoxelOctreeRW[FlagCoords] = NODE_NEEDSUBDIVIDE;
        }
    }
}
///////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
RWStructuredBuffer<uint> NodesCounter;
uint AllocateNodes( in uint NodeIndex )
{
    uint ParentIndex = NodeIndex;
    uint NodeCount = NodesCounter.IncrementCounter();
    uint Index = NodeCount * 8 + 1; // 1 is for preallocated root node
    
    [unroll]
    for ( uint i = 0; i < 8; i++ )
    {
        // convert index to octree adress
        uint ChildIndex = ( Index + i ) * SIZE_OF_NODE_STRUCT;
        SpaseVoxelOctreeRW[GetChildCoordsInBuffer( NodeIndex, i )] = ChildIndex;
        SpaseVoxelOctreeRW[GetParentCoords( ChildIndex )] = ParentIndex;

        for(uint j = 0 ; j < 8; j++ )
        {
            SpaseVoxelOctreeRW[IndexToCoords( ChildIndex + j )] = NODE_UNKOWN;
        }


    }
    return ( NodeCount + 1 ) * 8 + 1;
}

[numthreads(ThreadBlockSize, 1, 1)]
void SubDivideNodeCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint NodeOffset = (groupId.x * ThreadBlockSize) + groupIndex;
    uint NodesCountCurrentLevel = GetNodesCountByLevelForW(CurrentOctreeLevel);
    if( NodeOffset > NodesCountCurrentLevel)
    {
        return;
    }

    uint CurrentOctreeLevelOffsetInBuffer = LevelOffsetInBufferForW( CurrentOctreeLevel );
    uint NodeIndex = (CurrentOctreeLevelOffsetInBuffer + NodeOffset) * SIZE_OF_NODE_STRUCT;

    uint2 FlagCoords = GetFlagCoords( NodeIndex );
    if ( SpaseVoxelOctreeRW[FlagCoords] == NODE_NEEDSUBDIVIDE )
    {
        uint NodesCount = AllocateNodes( NodeIndex );
        SpaseVoxelOctreeRW[FlagCoords] = NODE_HASINITED;

        for(uint i = 8 ; i< 14; i++)
        {
            if(SpaseVoxelOctreeRW[IndexToCoords(NodeIndex + i)] == 0)
                SpaseVoxelOctreeRW[IndexToCoords(NodeIndex + i)] = NODE_UNKOWN;
        }
        
        
        uint NextLevelOffset = NodeOffset + NodesCountCurrentLevel;
        uint NodeCountNextLevel  = NodesCount > NextLevelOffset ? NodesCount - NextLevelOffset : 0;

        uint NextOctreeLevel = CurrentOctreeLevel + 1;
        StoreNodeCountLevelMax(NextOctreeLevel,NodeCountNextLevel);
        StoreNodeOffsetLevelMax(NextOctreeLevel,NextLevelOffset);
    }
}
///////////////////////////////////////////////////////////////////


//cbuffer
//RW SVO
///////////////////////////////////////////////////////////////////
void ConnectNeighbors( uint nodeIndex )
{
    [unroll]
    for ( uint i = 0; i < 8; i++ )
    {
        uint childIndex = GetChildIndexInNode( nodeIndex, i );

        // x, y, z: 0 is negative, 1 is positive
        const uint RelativePosToCenter[3] = { ( i & 0x1 ) == 1, ( i & 0x2 ) == 0x2, ( i & 0x4 ) == 0x4 };
        
        // 0 - no axis, 1 - negative axis, 2 - positive axis
        const uint3 NeighborMask[6] = {
            uint3( 1, 0, 0 ),
            uint3( 2, 0, 0 ),
            uint3( 0, 1, 0 ),
            uint3( 0, 2, 0 ),
            uint3( 0, 0, 1 ),
            uint3( 0, 0, 2 )
        };

        // 0 - negative axis, 1 - positive axis
        const uint3 ChildsMask[6] = {
            uint3( 0, RelativePosToCenter[1], RelativePosToCenter[2] ),
            uint3( 1, RelativePosToCenter[1], RelativePosToCenter[2] ),
            uint3( RelativePosToCenter[0], 0, RelativePosToCenter[2] ),
            uint3( RelativePosToCenter[0], 1, RelativePosToCenter[2] ),
            uint3( RelativePosToCenter[0], RelativePosToCenter[1], 0 ),
            uint3( RelativePosToCenter[0], RelativePosToCenter[1], 1 )
        };

        // find and save -x, +x, -y, +y, -z, +z neighbors coords
        uint2 neighborCoords = 0;

        [unroll]
        for ( int j = 0; j < 3; j++ )
        {
            //     next code follow this pseudocode:
            //
            //    if (-axis)    // relative axis (x,y,z) of child node inside current (parent) node
            //    {
            //        child.neighbor[-axis] = neighbor[-axis].child[+axis]  // I
            //        child.neighbor[+axis] = child[+axis]                  // II
            //    }
            //    else
            //    {
            //        child.neighbor[+axis] = neighbor[+axis].child[-axis]  // I
            //        child.neighbor[-axis] = child[-axis]                  // II
            //    }

            uint3 NeighborMask1 = NeighborMask[j * 2];
            uint3 NeighborMask2 = NeighborMask[j * 2 + 1];

            uint3 relativeChildIndex = ChildsMask[j * 2 + 1];
            
            if ( RelativePosToCenter[j] )
            {
                NeighborMask1 = NeighborMask[j * 2 + 1];
                NeighborMask2 = NeighborMask[j * 2];

                relativeChildIndex = ChildsMask[j * 2];
            }

            // I
            uint parentNeighborIndex = GetNodeNeighbor( nodeIndex, NeighborMask1 );
            if ( parentNeighborIndex != NODE_UNKOWN && GetFlagOfNode( parentNeighborIndex ) == NODE_HASINITED )
            {
                neighborCoords = GetNodeNeighborCoords( childIndex, NeighborMask1 );
                SpaseVoxelOctreeRW[neighborCoords] = GetChildIndexInNode( parentNeighborIndex, relativeChildIndex );
            }

            // II
            neighborCoords = GetNodeNeighborCoords( childIndex, NeighborMask2 );
            SpaseVoxelOctreeRW[neighborCoords] = GetChildIndexInNode( nodeIndex, relativeChildIndex );
        }
       
    }
}

[numthreads(ThreadBlockSize, 1, 1)]
void ConnectNeighborsCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint NodeOffset = (groupId.x * ThreadBlockSize) + groupIndex;
    uint NodesCountCurrentLevel = GetNodesCountByLevel(CurrentOctreeLevel);
    if( NodeOffset > NodesCountCurrentLevel)
    {
        return;
    }

    uint CurrentOctreeLevelOffsetInBuffer = LevelOffsetInBuffer( CurrentOctreeLevel );
    uint NodeIndex = (CurrentOctreeLevelOffsetInBuffer + NodeOffset) * SIZE_OF_NODE_STRUCT;

    if ( GetFlagOfNode( NodeIndex ) == NODE_HASINITED )
    {
        ConnectNeighbors( NodeIndex );
    }
}
///////////////////////////////////////////////////////////////////

//StructuredBuffer<Voxel> VoxelArray; 
//SpaseVoxelOctreeRW
[numthreads(ThreadBlockSize, 1, 1)]
void ConnectNodesToVoxelsCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint VoxelID = (groupId.x * ThreadBlockSize) + groupIndex;
    if(VoxelArray[VoxelID].Pad != 42)
    {
        return;
    }

    uint leafIndex = 0;
    if ( SearchOctree( VoxelArray[VoxelID].Position, OctreeHeight - 1, leafIndex ) )
    {
        SpaseVoxelOctreeRW[IndexToCoords( leafIndex )] = VoxelID;
    }
}