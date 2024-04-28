#include "Common.hlsl"
#include "Random.hlsl"
#include "MonteCarlo.hlsl"
#include "Math.hlsl"

//.r:Intensity in 0..1 range 
//.g:RoughnessMaskMul,
//.b:EnableDiscard for FPostProcessScreenSpaceReflectionsStencilPS,
//.a:(bTemporalAAIsOn?TemporalAAIndex:StateFrameIndexMod8)*1551

cbuffer cbSSR
{
    float4 cbSSR_SSRParams;
}

struct VertexIn
{
	float2 PosIn    : ATTRIBUTE0;
};

void VS(VertexIn vin,
    out float4 PosH: SV_POSITION
)
{
    PosH = float4(vin.PosIn, 0.0f, 1.0f);
}

struct FGBufferData
{
	float3 WorldNormal;
	float3 BaseColor;
	float Metallic;
	float Specular;
	float Roughness;
    uint ShadingModelID;

    float all_sum;
};

Texture2D SceneColor;
Texture2D SceneTexturesStruct_GBufferATexture;
Texture2D SceneTexturesStruct_GBufferBTexture;
Texture2D SceneTexturesStruct_GBufferCTexture;
Texture2D SceneTexturesStruct_GBufferDTexture;

Texture2D SceneTexturesStruct_SceneDepthTexture;
Texture2D HZBTexture;




uint MortonCode( uint x )
{
	x = (x ^ (x <<  2)) & 0x33333333;
	x = (x ^ (x <<  1)) & 0x55555555;
	return x;

}

uint ReverseUIntBits( uint bits )
{
	bits = ( (bits & 0x33333333) << 2 ) | ( (bits & 0xcccccccc) >> 2 );
	bits = ( (bits & 0x55555555) << 1 ) | ( (bits & 0xaaaaaaaa) >> 1 );
	return bits;
}

struct FSSRTRay
{
	float3 RayStartScreen;
	float3 RayStepScreen;
};

//-1<start+step*scale<1
float GetStepScreenFactorToClipAtScreenEdge(float2 RayStartScreen, float2 RayStepScreen)
{
    const float RayStepScreenInvFactor = 0.5 * length(RayStepScreen);
	const float2 S = 1 - max(abs(RayStepScreen + RayStartScreen * RayStepScreenInvFactor) - RayStepScreenInvFactor, 0.0f) / abs(RayStepScreen);
	const float RayStepFactor = min(S.x, S.y) / RayStepScreenInvFactor;
	return RayStepFactor;
}

FSSRTRay InitScreenSpaceRayFromWorldSpace(
    float3 RayOriginTranslatedWorld,
	float3 WorldRayDirection,
	float NDCZ)
{
    float4 RayStartClip	= mul_x(float4(RayOriginTranslatedWorld, 1), cbView_TranslatedViewProjectionMatrix);
	float4 RayEndClip = mul_x(float4(RayOriginTranslatedWorld + WorldRayDirection * NDCZ, 1), cbView_TranslatedViewProjectionMatrix);

	float3 RayStartScreen = RayStartClip.xyz * rcp(RayStartClip.w);
	float3 RayEndScreen = RayEndClip.xyz * rcp(RayEndClip.w);

    FSSRTRay Ray;
	Ray.RayStartScreen = RayStartScreen;
	Ray.RayStepScreen = RayEndScreen - RayStartScreen;
    Ray.RayStepScreen *= GetStepScreenFactorToClipAtScreenEdge(Ray.RayStartScreen.xy, Ray.RayStepScreen.xy);
    return Ray;
}

FGBufferData GetGbufferData(int2 SvPositionXY)
{
    FGBufferData GBuffer=(FGBufferData)0;
    float4 GbufferA = SceneTexturesStruct_GBufferATexture.Load(int3(SvPositionXY, 0));
    float4 GbufferB = SceneTexturesStruct_GBufferBTexture.Load(int3(SvPositionXY, 0));
    float4 GbufferC = SceneTexturesStruct_GBufferCTexture.Load(int3(SvPositionXY, 0));
    float4 GbufferD = SceneTexturesStruct_GBufferDTexture.Load(int3(SvPositionXY, 0));
	GBuffer.WorldNormal=normalize(GbufferA.xyz);
    GBuffer.BaseColor = GbufferC.xyz;
    GBuffer.Metallic = GbufferB.x;
    GBuffer.Specular = GbufferB.y;
    GBuffer.Roughness = GbufferB.z;
    GBuffer.ShadingModelID = uint(GbufferB.w);

    GBuffer.all_sum = GbufferA.x + GbufferB.x + GbufferC.x + GbufferD.x;
    return GBuffer;
}

bool CastRay(FSSRTRay SSRRay,uint NumSteps,float CompareTolerance ,float Roughness,out float3 OutHitUVz)
{
    const float3 RayStartScreen = SSRRay.RayStartScreen;
	float3 RayStepScreen = SSRRay.RayStepScreen;
    float3 RayStartUVz = float3( (RayStartScreen.xy * float2( 0.5, -0.5 ) + 0.5), RayStartScreen.z );
	float3 RayStepUVz  = float3(  RayStepScreen.xy  * float2( 0.5, -0.5 )		, RayStepScreen.z );

    const float Step = 1.0 / NumSteps;
    RayStepUVz *= Step;
        
    float3 RayUVz = RayStartUVz + RayStepUVz;
    float Level = 0, lastDepthDiff = 0, DepthDiff = 0;
        
    bool bHit = false;
    uint i;

    [loop]
    for (i = 0; i < NumSteps; i++)
    {
        float2 RaySamplesUV = RayUVz.xy + float(i) * RayStepUVz.xy;
        float RaySamplesZ = RayUVz.z + float(i) * RayStepUVz.z;
        float SampleDepth = HZBTexture.SampleLevel(gsamPointWarp, RaySamplesUV, Level).r;
        Level += (4.0 / NumSteps) * Roughness;

        DepthDiff = SampleDepth - RaySamplesZ;
        if (abs(DepthDiff + CompareTolerance) < CompareTolerance)
        {
            bHit = true;
            break;
        }
        lastDepthDiff = DepthDiff;
    }

        
    [branch]
    if(bHit)//lerp_t/d0=uv-t/d1
    {
        float lerp_t=/*length(RayStepUVz.xy)**/saturate(lastDepthDiff/(DepthDiff-lastDepthDiff));
        OutHitUVz = RayUVz + RayStepUVz * (float(i)+lerp_t);
    }
    else
    {
        OutHitUVz = float3(0,0,0);
    }
    return bHit;
}

float GetRoughnessFade(in float Roughness)
{
	//return min(Roughness * SSRParams.y + 2, 1.0);
	return Roughness * (-2.0f) + 2;
}

void PS(float4 SvPosition : SV_POSITION
    , out float4 OutColor : SV_Target)
{
    float2 UV = SvPosition.xy * View_BufferSizeAndInvSize.zw;
    float2 ScreenPos = UV * 2.0f - 1.0f; ScreenPos.y *= -1.0f;

    float DeviceZ = SceneTexturesStruct_SceneDepthTexture.Sample(gsamPointWarp, UV).r;
    const float ViewZ = ConvertFromDeviceZ_To_ViewZBeforeDivdeW(DeviceZ);
    const float3 PositionTranslatedWorld = mul_x(float4(ScreenPos * ViewZ, ViewZ, 1), cbView_ScreenToTranslatedWorld).xyz;
    const float3 V = normalize(float3(0, 0, 0) - PositionTranslatedWorld);

    FGBufferData GBuffer = GetGbufferData(SvPosition.xy);
    float Roughness = GBuffer.Roughness;
    const float3 N = GBuffer.WorldNormal;

    uint2 Random = Rand3DPCG16(int3(SvPosition.xy, View_StateFrameIndexMod8)).xy;
    float3x3 TangentBasis = GetTangentBasis(N);
    float3 TangentV = mul(TangentBasis, V);

    uint NumRays = 8;
    uint NumSteps=12;
    uint NumHit = 0;

    OutColor = float4(0.0, 0.0, 0.0, 0.0);
    [loop]
    for (uint i = 0; i < NumRays;++i)
    {
        float3 OutHitUVz;
        float2 E = Hammersley(i, NumRays, Random);
        float3 H = mul(ImportanceSampleVisibleGGX(UniformSampleDisk(E), Roughness * Roughness, TangentV).xyz, TangentBasis);
        float3 L = 2 * dot(V, H) * H - V;
        FSSRTRay SSRRay = InitScreenSpaceRayFromWorldSpace(PositionTranslatedWorld, L, ViewZ);
        bool bHit = CastRay(SSRRay, NumSteps, 0.001f * abs(dot(L, N)), Roughness, OutHitUVz);
        if (bHit)
        {
            OutColor += SceneColor.Sample(gsamLinearWarp, OutHitUVz.xy).xyzw;
            NumHit++;
        }
    }

    float2 absScreenXY = float2(abs(ScreenPos.x), abs(ScreenPos.y));
    float maxScreenXY=max(absScreenXY.x,absScreenXY.y);
    
    float UVFade = 1.0;
    if (maxScreenXY > 0.9)
    {
        UVFade = 0.0;
    }
        
    OutColor.xyz /= float(NumRays);
    OutColor.xyz *= UVFade;
    OutColor.xyz *= GetRoughnessFade(Roughness);
    OutColor.a = float(NumHit) / float(NumRays);
    
    if(OutColor.a<0.5)
    {
        OutColor.xyz=float3(0,0,0);
    }


    //----------------------------------------------
    float temp=0;
    temp += cbSSR_SSRParams.x + GBuffer.all_sum;
    OutColor.x+=(temp*0.0f);
}
