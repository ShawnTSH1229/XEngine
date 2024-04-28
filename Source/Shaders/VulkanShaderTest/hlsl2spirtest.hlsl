//static float2 positions[3] = {
//    float2(0.0, -0.5),
//    float2(0.5, 0.5),
//    float2(-0.5, 0.5)
//};
//
//static float3 colors[3] = {
//    float3(1.0, 0.0, 0.0),
//    float3(0.0, 1.0, 0.0),
//    float3(0.0, 0.0, 1.0)
//};
//
//void VS(
//    in uint VID : SV_VertexID,
//    out float4 fragColor : TEXCOORD0,
//    out float4 Position : SV_POSITION
//    )
//{
//    Position = float4(positions[VID], 0.0, 1.0);
//    fragColor = float4(colors[VID],1.0);
//}

//struct VertexIn
//{
//	float2 PosIn        : ATTRIBUTE0;
//	float3 ColorIn      : ATTRIBUTE1;
//};
//
//void VS(VertexIn vin,
//    out float3 fragColor : TEXCOORD0,
//    out float4 Position : SV_POSITION
//)
//{
//    Position = float4(vin.PosIn, 1.0f, 1.0f);
//    fragColor = vin.ColorIn;
//}
//
//void PS(
//	in float3 fragColor		: TEXCOORD0,
//	out float4 OutColor		: SV_Target0
//) 
//{
//    OutColor = float4(fragColor,1.0);
//}

#include "../Math.hlsl"
struct VertexIn
{
    float3 PosIn        : ATTRIBUTE0;
    float3 ColorIn      : ATTRIBUTE1;
    float2 inTexCoord   : ATTRIBUTE2;
};

cbuffer cbView
{
    float4x4 cbView_model;
    float4x4 cbView_view;
    float4x4 cbView_proj;
};

void VS(VertexIn vin,
    out float3 fragColor : TEXCOORD0,
    out float2 fragTexCoord : TEXCOORD1,
    out float4 Position : SV_POSITION
)
{
    float4 PosW = mul_x(float4(vin.PosIn,1.0), cbView_model);
    float4 PosV = mul_x(PosW, cbView_view);
    float4 PosC = mul_x(PosV, cbView_proj);
    Position = PosC;
    //Position = cbView_proj * cbView_view* cbView_model* float4(inPosition, 0.0, 1.0);
    //Position = float4(vin.PosIn, 1.0f, 1.0f);
    fragColor = vin.ColorIn;
    fragTexCoord = vin.inTexCoord;
}


Texture2D texSampler;
SamplerState gsamPointWarp;//  : register(s0, space1000);


void PS(
	in float3 fragColor		: TEXCOORD0,
    in float2 fragTexCoord : TEXCOORD1,
	out float4 OutColor		: SV_Target0
) 
{
    //OutColor= texSampler.Sample(mySampler, fragTexCoord);
    OutColor = texSampler.Sample(gsamPointWarp, fragTexCoord)*float4(fragColor,1.0);
    //OutColor = float4(fragColor,1.0);
}
//dxc G:\XEngineF\XEnigine\Source\Shaders\VulkanShaderTest\hlsl2spirtest.hlsl -E VS -T vs_6_1 -Zi -Qstrip_reflect -spirv -fspv-target-env=vulkan1.1