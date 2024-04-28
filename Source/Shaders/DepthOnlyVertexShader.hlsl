#include "Math.hlsl"
cbuffer cbPerObject
{
    float4x4 gWorld;
    float3 BoundingBoxCenter;
    float padding0;
    float3 BoundingBoxExtent;
    float padding1;
}

//cbuffer NoUse
//{
//    float4 PlaceHolder[4];
//}

#include "common.hlsl"

struct FVertexFactoryInput
{
    float4	Position	: ATTRIBUTE0;
};


void VS(FVertexFactoryInput Input,
    out float4 Position : SV_POSITION)
{
    float4 PositionW=mul_x(Input.Position, gWorld);
    Position=mul_x(PositionW, cbView_ViewPorjectionMatrix);

    //float Temp = PlaceHolder[0].x;
    //Position.x += Temp*0.0;
}

void PS()
{

}
    