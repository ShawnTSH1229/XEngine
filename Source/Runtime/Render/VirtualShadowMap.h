#pragma once
#define PhysicalTileWidth 8
#define NumPhysicalTexPerVirtualSM 8
#define VirtualTileWidthNum (8*8)
#define PhysicalShadowDepthTextureSize (1024)
#define PhysicalTileSize 128


#define VSM_MIP_NUM 3
#define VSM_TILE_NUM_XY 8
#define VSM_FULL_RES_TILE_NUM (VSM_TILE_NUM_XY << (VSM_MIP_NUM - 1))
#define VSM_MIP0_RESOLUTION 128.0

