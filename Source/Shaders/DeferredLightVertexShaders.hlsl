#include "Common.hlsl"

struct VertexIn
{
	float2 PosIn    : ATTRIBUTE0;
	float2 Tex    : ATTRIBUTE1;
};

void DeferredLightVertexMain(
    VertexIn Input,
    //float2 PosIn    : POSITION,
	//float2 TexC : TEXCOORD,
	out float2 OutTexCoord : TEXCOORD0,
	out float3 OutScreenVector : TEXCOORD1,
	out float4 OutPosition : SV_POSITION
)
{
    OutTexCoord=Input.Tex;
    OutPosition=float4(Input.PosIn,0.0f,1.0f);
    OutScreenVector=mul_x(float4(OutPosition.xy,1.0f,0.0f),cbView_ScreenToTranslatedWorld).xyz;
}

 