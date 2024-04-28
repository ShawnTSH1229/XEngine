cbuffer cbPerObject
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter;
    float padding0;
    float3 BoundingBoxExtent;
    float padding1;
}

cbuffer LightProjectMatrix
{
    row_major float4x4 LightViewProjectMatrix;
    row_major float4x4 LightViewProjectMatrixInv;
}

struct FVertexFactoryInput
{
    float4	Position	: ATTRIBUTE0;
};

void VS(FVertexFactoryInput Input,
    out float4 Position : SV_POSITION)
{
    float4 PositionW=mul(Input.Position, gWorld);
    Position=mul(PositionW, LightViewProjectMatrix);
}

void PS()
{
    
}