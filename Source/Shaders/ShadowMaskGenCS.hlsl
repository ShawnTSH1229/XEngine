#include "Common.hlsl"

RWTexture2D<float>VsmShadowMaskTex;

Texture2D GBufferNormal;
Texture2D SceneDepthInput;
Texture2D<uint4> PagetableInfos; //64*64
Texture2D<uint>PhysicalShadowDepthTexture; // 1024 * 1024

cbuffer cbShadowViewInfo
{
    row_major float4x4 LightViewProjectMatrix;
    float3 ShadowLightDir;
    float padding0;
    //float ClientWidth;
    //float ClientHeight;
    //float padding0;
    //float padding1;
}

#define PageNum (8*8)
#define PhysicalTileWidthNum 8
#define PhysicalSize 1024

#define VirtualTileSise (1024 * 8)
static float2 UVOffset[5] = 
{
    float2(0,0),
    float2(1.0f/VirtualTileSise,1.0f/VirtualTileSise),
    float2(1.0f/VirtualTileSise,-1.0f/VirtualTileSise),
    float2(-1.0f/VirtualTileSise,1.0f/VirtualTileSise),
    float2(-1.0f/VirtualTileSise,-1.0f/VirtualTileSise)
};

float ComputeShadowFactor(float2 UVShadowSpace ,uint ObjectShadowDepth , uint Bias)
{
    uint2 TileIndexXY = uint2(UVShadowSpace * PageNum /*- 0.5f*/);
    uint PageTableIndex = PagetableInfos[TileIndexXY].x;

    uint PhysicalIndexX = PageTableIndex % uint(PhysicalTileWidthNum);
    uint PhysicalIndexY = PageTableIndex / uint(PhysicalTileWidthNum);

    float2 SubTileUV = (UVShadowSpace * PageNum) - uint2(UVShadowSpace * PageNum);
    float2 ShadowDepthPos = float2(PhysicalIndexX,PhysicalIndexY) + SubTileUV; 
    ShadowDepthPos/=PhysicalTileWidthNum;
    ShadowDepthPos*=PhysicalSize;

    uint ShadowDepth = PhysicalShadowDepthTexture[uint2(ShadowDepthPos)].x;

    if((ObjectShadowDepth + Bias )< ShadowDepth) 
    {
        return 1.0f;
    }

    return 0.0;
}

[numthreads(16, 16, 1)]
void ShadowMaskGenCS(uint2 DispatchThreadID :SV_DispatchThreadID)
{
    float DeviceZ = SceneDepthInput.Load(int3(DispatchThreadID,0)).x;
    float2 UV = DispatchThreadID * View_BufferSizeAndInvSize.zw;

    if(UV.x > 1.0f || UV.y > 1.0f)
    {
        return;
    }

    if(DeviceZ == 0.0)
    {
        VsmShadowMaskTex[DispatchThreadID] =0.0f;
        return;
    }
    float4 Normal = GBufferNormal.Load(int3(DispatchThreadID,0));

    float2 ScreenPos = UV * 2.0f - 1.0f; ScreenPos.y *= -1.0f;

    float4 NDCPosNoDivdeW = float4(ScreenPos , DeviceZ , 1.0);
    float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse,NDCPosNoDivdeW);
    WorldPosition.xyz/=WorldPosition.w;

    float4 ShadowScreenPOS = mul(float4(WorldPosition.xyz,1.0),LightViewProjectMatrix);
    ShadowScreenPOS.xyz/=ShadowScreenPOS.w;

    float2 UVShadowSpace = ShadowScreenPOS.xy; 
    UVShadowSpace.y*=-1.0;
    UVShadowSpace = UVShadowSpace* 0.5 + 0.5f;

    uint ObjectShadowDepth = (ShadowScreenPOS.z)* (1<<30);

    float3 LightDir = ShadowLightDir;
    LightDir = normalize(LightDir);
    float NoL = dot(LightDir,Normal.xyz);
    uint Bias = NoL * 500 * (1 << 10) + 300 * (1 << 10);
    
    //if(abs(NoL) < 0.25)
    //{
    //    Bias += 500 * (1 << 10);
    //}

    float Total = 0.0f;
    for(int i = 0; i < 5; i++)
    {
        Total+=ComputeShadowFactor(UVShadowSpace + UVOffset[i],ObjectShadowDepth,Bias);
    }

    //VsmShadowMaskTex[DispatchThreadID] = ComputeShadowFactor(UVShadowSpace,ObjectShadowDepth,Bias);
    VsmShadowMaskTex[DispatchThreadID] = (Total / 5.0f);
}