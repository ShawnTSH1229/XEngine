

cbuffer cbPerObject
{
    float4x4 gWorld;
    float3 BoundBoxMax;
    float padding0;
    float3 BoundBoxMin;
    float padding1;
}

//cbuffer cbPass
//{
//    float4x4 gViewProj;
//}

#include "Common.Hlsl"

//#include "Generated/VertexFactory.hlsl"

#include "Generated/LocalVertexFactory.hlsl"

void VS(FVertexFactoryInput Input,
    out FVertexFactoryInterpolantsVSToPS Output,
    out float4 Position : SV_POSITION
    )
{
    VsToPsCompute(Input,Output,Position);
}