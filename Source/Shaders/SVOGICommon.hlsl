struct Voxel
{
    uint Position;
    uint Color;
    uint Normal;
    uint Pad;
};

RWTexture2D<uint> SpaseVoxelOctreeRW;
Texture2D<uint> SpaseVoxelOctreeR;
#define OctreeHeight 9 // statrt index is 0
#define OctreeResolution 512
#define OctreeRealBufferSize 1024
#define SIZE_OF_NODE_STRUCT 16

#define NODE_UNKOWN         0xffffffff
#define NODE_NEEDSUBDIVIDE  0xff00ff00
#define NODE_HASINITED      0x00ff00ff
#define NODE_LIT            0x00000100 

uint3 UnpackUintToUint3( uint Value )
{
    return uint3( ( Value ) & 0x3ff, 
                  ( Value >> 10 ) & 0x3ff, 
                  ( Value >> 20 ) & 0x3ff );
}


uint2 IndexToCoords( uint Index )
{
    return uint2( Index % OctreeRealBufferSize, Index / OctreeRealBufferSize );
}
uint GetChildIndexInNode( uint nodeIndex, uint offset )
{
    return SpaseVoxelOctreeRW[IndexToCoords( nodeIndex + offset )];
}

uint GetChildIndexInNode( uint NodeIndex, uint3 SubOctreeIndex )
{
    uint Offset = SubOctreeIndex.z * 4 + SubOctreeIndex.y * 2 + SubOctreeIndex.x;
    return SpaseVoxelOctreeRW[IndexToCoords( NodeIndex + Offset )];
}

uint GetChildIndexInBuffer( uint NodeIndex, uint3 SubOctreeIndex )
{
    uint Offset = SubOctreeIndex.z * 4 + SubOctreeIndex.y * 2 + SubOctreeIndex.x;
    return NodeIndex + Offset;
}

uint2 GetChildCoordsInBuffer( uint nodeIndex, uint offset )
{
    return IndexToCoords( nodeIndex + offset );
}
uint NeighborMaskToOffset( uint3 mask )
{
    // -x +x -y +y -z +z
    // mask.x == 0 - not a x-neighbor
    // mask.x == 1 - negative x
    // mask.x == 2 - positive x
    
    //        x-offset              y-offset                         z-offset
    return ( 0 + mask.x ) + ( 2 * step( 1, mask.y ) + mask.y ) + ( 4 * step( 1, mask.z ) + mask.z );
}
uint2 GetNodeNeighborCoords( uint nodeIndex, uint3 mask )
{
    uint offset = NeighborMaskToOffset( mask );
    return IndexToCoords( nodeIndex + 7 + offset );
}
uint GetNodeNeighbor( uint nodeIndex, uint3 mask )
{
    uint offset = NeighborMaskToOffset( mask );
    return SpaseVoxelOctreeRW[IndexToCoords( nodeIndex + 7 + offset )];
}
uint2 GetParentCoords( uint nodeIndex )
{
    return IndexToCoords( nodeIndex + 14 );
}
uint GetFlagOfNode(uint nodeIndex)
{
    return SpaseVoxelOctreeRW[IndexToCoords( nodeIndex + 15 )];
}
uint2 GetFlagCoords( uint NodeIndex )
{
    return IndexToCoords( NodeIndex + 15 );
}

void GetSubNodeIndex(in uint3 PositionIndex,in out uint3 SubOctreeIndex , in out uint HalfSize , in out uint3 CurrentMinPosIndex ,in out uint NodeIndex)
{
    HalfSize = HalfSize / 2 ;
    int3 SubOctreePos  = PositionIndex - (CurrentMinPosIndex + HalfSize);
    SubOctreeIndex = uint3(step(0,sign(SubOctreePos)));

    CurrentMinPosIndex = CurrentMinPosIndex + HalfSize * SubOctreeIndex;
    NodeIndex = GetChildIndexInBuffer(NodeIndex,SubOctreeIndex);
}

bool SearchOctree(in uint VoxelPosition,in uint CurrentOctreeLevel , out uint NodeIndex)
{
    NodeIndex = 0;

    if (CurrentOctreeLevel >= OctreeHeight )
        return false;

    uint3 PositionIndex = UnpackUintToUint3(VoxelPosition);
    if (any(step( OctreeResolution, PositionIndex ))) //step(x,y) : if x < y, return true
        return false;

    uint3 CurrentMinPosIndex = 0;
    uint3 SubOctreeIndex = 0;
    uint HalfSize = OctreeResolution;

    for (uint TreeLevel = 0; TreeLevel < CurrentOctreeLevel; TreeLevel++ )
    {
        GetSubNodeIndex(PositionIndex,SubOctreeIndex,HalfSize,CurrentMinPosIndex,NodeIndex);
        if(TreeLevel != (OctreeHeight - 2))
        {
            NodeIndex = SpaseVoxelOctreeRW[IndexToCoords( NodeIndex )].r;
            if(NodeIndex == NODE_UNKOWN)
            {
                return false;
            }
        }
    }
    return true;
}

bool SearchOctreeR(in uint VoxelPosition,in uint CurrentOctreeLevel , out uint NodeIndex ,out uint3 CurrentMinPosIndex)
{
    NodeIndex = 0;

    if (CurrentOctreeLevel >= OctreeHeight )
        return false;

    uint3 PositionIndex = UnpackUintToUint3(VoxelPosition);
    if (any(step( OctreeResolution, PositionIndex ))) //step(x,y) : if x < y, return true
        return false;

    CurrentMinPosIndex = 0;
    uint3 SubOctreeIndex = 0;
    uint HalfSize = OctreeResolution;

    for (uint TreeLevel = 0; TreeLevel < CurrentOctreeLevel; TreeLevel++ )
    {
        GetSubNodeIndex(PositionIndex,SubOctreeIndex,HalfSize,CurrentMinPosIndex,NodeIndex);
        if(TreeLevel != (OctreeHeight - 2))
        {
            NodeIndex = SpaseVoxelOctreeR[IndexToCoords( NodeIndex )].r;
            if(NodeIndex == NODE_UNKOWN)
            {
                return false;
            }
        }
    }
    return true;
}

//bool PhotonTraverseOctree( in uint3 VoxelSpaceIndex, out uint NodeIndex, out uint parentIndex, out uint3 SubOctreeIndex )
//{
//    if ( any( step( OctreeResolution, VoxelSpaceIndex ) ) )
//        return false;
//
//    uint3 CurrentMinPosIndex = 0;
//    uint HalfSize = OctreeResolution;
//
//    NodeIndex = 0;
//    parentIndex = 0;
//    SubOctreeIndex = 0;
//    
//    uint2 flagC = 0;
//    [unroll]
//    for ( uint treeLevel = 0; treeLevel < OctreeHeight - 1; treeLevel++ )
//    {
//        parentIndex = NodeIndex;
//        GetSubNodeIndex(VoxelSpaceIndex,SubOctreeIndex,HalfSize,CurrentMinPosIndex,NodeIndex);
//        NodeIndex = SpaseVoxelOctreeRW[IndexToCoords( NodeIndex )].r;
//        
//        if(NodeIndex == NODE_UNKOWN)
//        {
//            return false;
//        }
//
//        flagC = GetFlagCoords( parentIndex );
//        SpaseVoxelOctreeRW[flagC] = NODE_LIT;
//        //if ( treeLevel != OctreeHeight - 1 ) 
//        //{
//        //    // write values to octree flag that we was here (write flag for the last level after the loop)
//        //
//        //}
//
// 
//    }
//
//    return true;
//}

// octree node struct
//    -x -y -z <----------- 0 // subnodes for current node
//    +x -y -z                // at the last level leaf is a pointer (index) to voxel array
//    -x +y -z
//    +x +y -z
//    -x -y +z
//    +x -y +z
//    -x +y +z
//    +x +y +z
//    -x <----------------- 8 // neighbors - pointer (index) to octree
//    +x
//    -y
//    +y
//    -y
//    +z
//    parent <------------- 14
//    flags <-------------- 15 // see NODE_XXX values