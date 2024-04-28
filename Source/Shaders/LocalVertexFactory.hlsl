
#include "Math.hlsl"

//Local Vertex Factory , if create Landscape , then this file chnage to LandSacpeVertextFactory.ush
struct FVertexFactoryInput
{
    float4	Position	: ATTRIBUTE0;
    float3	TangentX	: ATTRIBUTE1;
    float4	TangentY	: ATTRIBUTE2;
    float2	TexCoord    : ATTRIBUTE3;
};

struct FVertexFactoryInterpolantsVSToPS
{
	float4 TangentToWorld0 : TEXCOORD10; 
    float4 TangentToWorld2 : TEXCOORD11;
	float4 TexCoords : TEXCOORD0;
    float3 TestWorldPosition : TEXCOORD1;
};

void VsToPsCompute(FVertexFactoryInput Input ,in out FVertexFactoryInterpolantsVSToPS Output,in out float4 Position)
{
    Output.TangentToWorld0 = float4(mul_x(Input.TangentX, (float3x3)gWorld),0.0f);
    Output.TangentToWorld2 = float4(mul_x(Input.TangentY.xyz, (float3x3)gWorld),1.0f);
    Output.TexCoords=float4(Input.TexCoord.xy,0.0f,0.0f);
    Output.TestWorldPosition= mul_x(Input.Position,gWorld).xyz;
    float4 PositionW=mul_x(Input.Position, gWorld);
    Position=mul_x(PositionW, cbView_ViewPorjectionMatrix);
}