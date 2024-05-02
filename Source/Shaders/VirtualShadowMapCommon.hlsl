#define VSM_MIP_NUM 3
#define VSM_TILE_MAX_MIP_NUM_XY 8
#define VSM_TILE_MIN_MIP_NUM_XY (VSM_TILE_MAX_MIP_NUM_XY << (VSM_MIP_NUM - 1)) // MIP 0 : 32

#define VSM_TILE_TEX_PHYSICAL_SIZE 128
#define VSM_MIN_LEVEL_DISTANCE 64.0 // Radius = 64, Range = 64 * 2 = 128

#define VSM_CLIPMAP_MIN_LEVEL 6 // 2 ^ (6 + 1) = 128

#define TILE_STATE_UNUSED 0
#define TILE_STATE_USED 1
#define TILE_STATE_CACHE_MISS 2

#define TILE_ACTION_NONE 0
#define TILE_ACTION_NEED_UPDATE 1
#define TILE_ACTION_NEED_REMOVE 2
#define TILE_ACTION_CACHED 4

struct SShadowViewInfo
{
    row_major float4x4 ShadowViewProject;
};

static float4 BoundingBoxOffset[8] = 
{
 float4(    -1.0f,  -1.0f,  1.0f,   0.0f     ),
 float4(    1.0f,   -1.0f,  1.0f,   0.0f     ),
 float4(    1.0f,   1.0f,   1.0f,   0.0f     ),
 float4(    -1.0f,  1.0f,   1.0f,   0.0f     ), 
 float4(    -1.0f,  -1.0f,  -1.0f,  0.0f     ), 
 float4(    1.0f,   -1.0f,  -1.0f,  0.0f     ),
 float4(    1.0f,   1.0f,   -1.0f,  0.0f     ),  
 float4(    -1.0f,  1.0f,   -1.0f,  0.0f     ),
};

static const uint MipLevelGroupStart[3] = {0, 16, 4 + 16};
static const uint MipLevelGroupOffset[3] = {0, 16 * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY , (4 + 16) * VSM_TILE_MAX_MIP_NUM_XY * VSM_TILE_MAX_MIP_NUM_XY};

static const uint MipLevelOffset[3] = {0,32 * 32, 32 * 32 + 16 * 16};
static const uint MipLevelSize[3] = {32,16,8};



