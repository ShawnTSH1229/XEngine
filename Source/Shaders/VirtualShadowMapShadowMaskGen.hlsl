#include "Common.hlsl"
#include "VirtualShadowMapCommon.hlsl"

Texture2D GBufferNormal;
Texture2D SceneDepthInput;
StructuredBuffer<uint> VirtualShadowMapTileTable;
Texture2D<uint> PhysicalShadowDepthTexture;

RWTexture2D<float> VirtualShadowMapMaskTexture;

cbuffer CbShadowViewInfo
{
    row_major float4x4 LightViewProjectMatrix;
    float3 WorldCameraPosition;
    float SViewPadding0;
    float3 ShadowLightDir;
};

static const float MipLevelVirtualTextureSize[3] = { 1.0 / (8 * 1024) , 1.0 / (4 * 1024), 1.0 / (1 * 1024)};
static float2 UVOffset[5] = 
{
    float2(0,0),
    float2(1.0f, 1.0f),
    float2(-1.0f, -1.0f),
    float2(-1.0f, 1.0f),
    float2(-1.0f, -1.0f)
};

float ComputeShadowFactor(float2 UVShadowSpace ,uint ObjectShadowDepth, uint Bias, uint MipLevel)
{
    uint2 VSMTileIndex = uint2(UVShadowSpace * MipLevelSize[MipLevel]);
    int TableIndex = MipLevelOffset[MipLevel] + VSMTileIndex.y * MipLevelSize[MipLevel] + VSMTileIndex.x;
    uint TileInfo = VirtualShadowMapTileTable[TableIndex];
    uint PhysicalTileIndexX = (TileInfo >>  0) & 0xFFFF;
    uint PhysicalTileIndexY = (TileInfo >> 16) & 0xFFFF;

    float2 IndexXY = uint2(PhysicalTileIndexX, PhysicalTileIndexY) * VSM_TILE_TEX_PHYSICAL_SIZE;
    float2 SubTileIndex = (float2(UVShadowSpace * MipLevelSize[MipLevel]) - VSMTileIndex) * VSM_TILE_TEX_PHYSICAL_SIZE;
    float2 ReadPos = IndexXY + SubTileIndex;

    uint ShadowDepth = PhysicalShadowDepthTexture[uint2(ReadPos)].x;

    if((ObjectShadowDepth + Bias ) < ShadowDepth) 
    {
        return 1.0f;
    }

    return 0.0;
}

[numthreads(16, 16, 1)]
void VSMShadowMaskGenCS(uint2 DispatchThreadID :SV_DispatchThreadID)
{
    float DeviceZ = SceneDepthInput.Load(int3(DispatchThreadID,0)).x;
    float2 UV = DispatchThreadID * View_BufferSizeAndInvSize.zw;

    if(UV.x > 1.0f || UV.y > 1.0f)
    {
        return;
    }

    if(DeviceZ == 0.0)
    {
        VirtualShadowMapMaskTexture[DispatchThreadID] =0.0f;
        return;
    }
    float4 Normal = GBufferNormal.Load(int3(DispatchThreadID,0));

    float2 ScreenPos = UV * 2.0f - 1.0f; ScreenPos.y *= -1.0f;

    float4 NDCPosNoDivdeW = float4(ScreenPos , DeviceZ , 1.0);
    float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse,NDCPosNoDivdeW);
    WorldPosition.xyz /= WorldPosition.w;

    float4 ShadowScreenPOS = mul(float4(WorldPosition.xyz,1.0),LightViewProjectMatrix);
    ShadowScreenPOS.xyz /= ShadowScreenPOS.w;

    float2 UVShadowSpace = ShadowScreenPOS.xy; 
    UVShadowSpace.y *= -1.0;
    UVShadowSpace = UVShadowSpace * 0.5 + 0.5f;

    float Distance = length(WorldCameraPosition - WorldPosition.xyz);
    float Log2Distance = log2(Distance + 1);
    int MipLevel = clamp(Log2Distance - VSM_CLIPMAP_MIN_LEVEL, 0, VSM_MIP_NUM - 1);

    float ObjectShadowDepth = float(ShadowScreenPOS.z) * uint(0xFFFFFFFF);

    float3 LightDir = ShadowLightDir;
    LightDir = normalize(LightDir);
    float NoL = dot(LightDir , Normal.xyz);
    uint Bias = NoL * (0.0001) * uint(0xFFFFFFFF) + (0.0005) *  uint(0xFFFFFFFF);

    float Total = 0.0f;
    for(int i = 0; i < 5; i++)
    {
        Total += ComputeShadowFactor(UVShadowSpace + UVOffset[i] * MipLevelVirtualTextureSize[MipLevel], ObjectShadowDepth,Bias, MipLevel);
    }
    VirtualShadowMapMaskTexture[DispatchThreadID] = (Total / 5.0f);
}