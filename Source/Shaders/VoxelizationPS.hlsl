struct FVertexFactoryInterpolantsVSToPS
{
	float4 TangentToWorld0 : TEXCOORD10; 
    float4 TangentToWorld1 : TEXCOORD11;
	float4 TexCoords : TEXCOORD0;
    float3 TestWorldPosition : TEXCOORD1;
};

#include "SVOGINodeCountBuffer.hlsl"
#include "Generated/Material.hlsl"

cbuffer cbVoxelization
{
	float3 MinBound;
    float VoxelBufferDimension;
    float3 MaxBound;
    float cbVoxelization_Padding0;
};

struct Voxel
{
    uint Position;
    uint Color;
    uint Normal;
    uint Pad; // 128 bits aligment
};
AppendStructuredBuffer<Voxel> VoxelArrayRW; 

uint PackUint3ToUint( uint3 Value )
{
    return ( ( Value.x & 0x3ff ) )       |
           ( ( Value.y & 0x3ff ) << 10 ) |
             ( Value.z & 0x3ff ) << 20;
}

uint PackFloat3ToUint( float3 Value )
{
    float3 V = saturate( Value );
    return ( uint( V.x * 255 ) )      |
           ( uint( V.y * 255 ) << 8 ) |
           ( uint( V.z * 255 ) << 16 );
}


void PS(FVertexFactoryInterpolantsVSToPS Input,
    out float4 PlaceHolderRT : SV_Target)
{
    ParametersIn Parameters=(ParametersIn)0;
    Parameters.TexCoords = Input.TexCoords.xy;
    Parameters.TangentToWorld0 = Input.TangentToWorld0.xyz;
    Parameters.TangentToWorld1 = Input.TangentToWorld1.xyz;

    GBufferdataOutput OutputGbufferData=(GBufferdataOutput)0;
    CalcMaterialParameters(Parameters,OutputGbufferData);

    float3 Normal = ( OutputGbufferData.Normal + 1.0f ) * 0.5f;
    uint3 VoxelSceneIndex = (( Input.TestWorldPosition - MinBound ) / ( MaxBound - MinBound ) ) * VoxelBufferDimension;     
    float3 Color = OutputGbufferData.BaseColor;

    Voxel VoxelOut = (Voxel)0;
    VoxelOut.Position = PackUint3ToUint(VoxelSceneIndex);
    VoxelOut.Color = PackFloat3ToUint(Color);
    VoxelOut.Normal = PackFloat3ToUint(Normal);
    VoxelOut.Pad = 42; //Flag

    //uint BufferSize = VoxelArrayRW.IncrementCounter();
    VoxelArrayRW.Append(VoxelOut);
    IncmentVoxelCount();
    NodeCountAndOffsetBuffer[uint2(2,0)] = 1;
    NodeCountAndOffsetBuffer[uint2(3,0)] = 0;
    PlaceHolderRT = float4(Color,1.0);
}