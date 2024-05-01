#include "DeferredShadingRenderer.h"

#define VSM_MIP_NUM 3
#define VSM_TILE_MAX_MIP_NUM_XY 8
#define VSM_TILE_MIN_MIP_NUM_XY (VSM_TILE_MAX_MIP_NUM_XY << (VSM_MIP_NUM - 1)) // MIP 0 : 32

#define VSM_TILE_TEX_PHYSICAL_SIZE 128
#define VSM_MIN_LEVEL_DISTANCE 128.0


