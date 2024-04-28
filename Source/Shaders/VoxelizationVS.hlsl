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

cbuffer cbPerObject
{
    float4x4 gWorld;
    float3 BoundBoxMax;
    float padding0;
    float3 BoundBoxMin;
    float padding1;
}

#include "Common.Hlsl"

cbuffer OrientMatrix
{
    row_major float4x4 OrientToXView;
    row_major float4x4 OrientToYView;
    row_major float4x4 OrientToZView;
    row_major float4x4 ProjMatrix;
}


void VS(FVertexFactoryInput Input,
    out FVertexFactoryInterpolantsVSToPS Output,
    out float4 Position : SV_POSITION
    )
{
    Output.TangentToWorld0 = float4(mul((float3x3)gWorld,Input.TangentX),0.0f);
    Output.TangentToWorld2 = float4(mul((float3x3)gWorld,Input.TangentY.xyz),1.0f);
    Output.TexCoords = float4(Input.TexCoord,0,0);

    float3 AbsNormal = abs(normalize(Output.TangentToWorld2.xyz));

    float4 PosW = mul(gWorld,Input.Position);
    Output.TestWorldPosition = PosW.xyz;

    
    float4 PosH;
    if((AbsNormal.x>AbsNormal.y)&&(AbsNormal.x>AbsNormal.z))
    {
        PosH = mul(PosW,OrientToXView);
    }
    else if((AbsNormal.y>AbsNormal.x)&&(AbsNormal.y>AbsNormal.z))
    {
        PosH = mul(PosW,OrientToYView);
    }
    else
    {
        PosH = mul(PosW,OrientToZView);
    }

    Position = mul(PosH,ProjMatrix);
}
