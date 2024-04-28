#ifndef __MATH_HLSL__
#define __MATH_HLSL__

float4 mul_x(float4 VectorIn , float4x4 MatrixIn)
{
    return mul(MatrixIn,VectorIn);
}

float3 mul_x(float3 VectorIn , float3x3 MatrixIn)
{
    return mul(MatrixIn,VectorIn);
}

#endif