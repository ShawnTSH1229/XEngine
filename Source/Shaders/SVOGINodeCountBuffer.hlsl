RWTexture2D<uint> NodeCountAndOffsetBuffer;
Texture2D<uint> NodeCountAndOffsetBufferR;

#define VOXEL_COUNT_OFFSET 0

//1 8 64 512


// offset: 0------------------2------------------Level0--------------------------------4----------------------Level1-------------------6-Level2--8----Level 3---10
// data:   [ VoxelsCount; UnUsed; ] [ nodesCount for level 0; nodesOffset for level 0;][ nodesCount for level 1; nodesOffset for level 1;]

uint LevelIndex( uint OctreeLevel )
{
    return 2 + OctreeLevel * 2;
}


uint LevelOffsetInBuffer( uint OctreeLevel )
{
    return NodeCountAndOffsetBufferR[ uint2(LevelIndex( OctreeLevel ) + 1 ,0 )];
}

uint GetNodesCountByLevel( uint OctreeLevel )
{
    return NodeCountAndOffsetBufferR[ uint2(LevelIndex( OctreeLevel ),0) ];
}

uint LevelOffsetInBufferForW( uint OctreeLevel )
{
    return NodeCountAndOffsetBuffer[ uint2(LevelIndex( OctreeLevel ) + 1 ,0 )];
}

uint GetNodesCountByLevelForW( uint OctreeLevel )
{
    return NodeCountAndOffsetBuffer[ uint2(LevelIndex( OctreeLevel ),0) ];
}

void StoreNodeOffsetLevelMax( uint OctreeLevel ,uint NodeOffset)
{
    InterlockedMax(NodeCountAndOffsetBuffer[ uint2(LevelIndex( OctreeLevel ) + 1 ,0) ] , NodeOffset);
}

void StoreNodeCountLevelMax( uint OctreeLevel ,uint NodeCount)
{
    InterlockedMax(NodeCountAndOffsetBuffer[ uint2(LevelIndex( OctreeLevel ),0) ],NodeCount);
}
void IncmentVoxelCount()
{
    InterlockedAdd( NodeCountAndOffsetBuffer[uint2(0,0)], 1 );
}