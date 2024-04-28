struct SceneConstantBuffer
{
    row_major float4x4 gWorld;
    float3 BoundingBoxCenter;//BoundingBox in World  Space
    float a;
    float3 BoundingBoxExtent;
    float b;
};

struct UINT64
{
    uint LowAddress;
    uint HighAddress;
};

UINT64 UINT64_ADD(UINT64 InValue , uint InAdd)
{
    UINT64 Ret = InValue;
    uint C= InValue.LowAddress + InAdd;
    bool OverFlow = (C < InValue.HighAddress) || (C < InAdd);
    if(OverFlow)
    {
        Ret.HighAddress += 1;
    }
    Ret.LowAddress = C;
    return Ret;
}

struct ShadowIndirectCommand
{
    uint2 CbWorldAddress;
    UINT64 CbGlobalShadowViewProjectAddressVS; // Used For VSM
    UINT64 CbGlobalShadowViewProjectAddressPS; // Used For VSM
    
    uint2 VertexBufferLoacation;
    uint VertexSizeInBytes;
    uint VertexStrideInBytes;
    
    uint2 IndexBufferLoacation;
    uint IndexSizeInBytes;
    uint IndexFormat;

    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    
    uint StartInstanceLocation;
    uint Padding;
};

StructuredBuffer<SceneConstantBuffer> SceneConstantBufferIN;
StructuredBuffer<ShadowIndirectCommand> inputCommands; 
AppendStructuredBuffer<ShadowIndirectCommand> outputCommands;
Texture2D<uint> VirtualSMFlags;

cbuffer cbCullingParameters
{
    row_major float4x4 ShadowViewProject;
    float4 Planes[6];
    float commandCount; 
}

static float4 BoundingBoxOffset[8] = 
{
 float4(    -1.0f,  -1.0f,  1.0f,   0.0f     ),
 float4(    1.0f,   -1.0f,  1.0f,   0.0f     ),
 float4(    1.0f,   1.0f,   1.0f,   0.0f     ),
 float4(    -1.0f,  1.0f,   1.0f,   0.0f     ), 
 float4(    -1.0f,  -1.0f,  -1.0f,  0.0f     ), 
 float4(    1.0f,   -1.0f,  -1.0f,  0.0f     ),
 float4(    1.0f,   1.0f,   -1.0f,  0.0f     ),  
 float4(    -1.0f,  1.0f,   -1.0f,  0.0f     ),
};

#define TileWidth  64
#define ThreadBlockSize 128
[numthreads(ThreadBlockSize, 1, 1)]
void ShadowCommandBuildCS(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    uint index = (groupId.x * ThreadBlockSize) + groupIndex;

    if(index >= uint(commandCount))
    return;

    float4 Corner[8];
    float3 Center = SceneConstantBufferIN[index].BoundingBoxCenter;
    float3 Extent = SceneConstantBufferIN[index].BoundingBoxExtent;
    float4 Temp= float4(Center,0)  + float4(Extent,0) * BoundingBoxOffset[0];
    Corner[0]  = Temp;


    [unroll]
    for(uint i = 1; i < 8 ; i++)
    {
        Corner[i] = float4(SceneConstantBufferIN[index].BoundingBoxCenter ,0)  + float4(SceneConstantBufferIN[index].BoundingBoxExtent,0) * BoundingBoxOffset[i];
    }

    float2 UVMin = float2(1.1,1.1);
    float2 UVMax = float2(-0.1,-0.1);
    
    {
        float4 ScreenPosition = mul(float4(Corner[0].xyz,1.0f),ShadowViewProject);
        ScreenPosition.xyz/=ScreenPosition.w;

        float2 ScreenUV = ScreenPosition.xy;
        ScreenUV.y*=-1.0;
        ScreenUV = ScreenUV* 0.5 + 0.5f;

        UVMin.x = ScreenUV.x > UVMin.x ? UVMin.x : ScreenUV.x;
        UVMin.y = ScreenUV.y > UVMin.y ? UVMin.y : ScreenUV.y;

        UVMax.x = ScreenUV.x > UVMax.x ? ScreenUV.x : UVMax.x;
        UVMax.y = ScreenUV.y > UVMax.y ? ScreenUV.y : UVMax.y;
    }

    [unroll]
    for(uint j = 1; j < 8 ; j++)
    {
        float4 ScreenPosition = mul(float4(Corner[j].xyz,1.0f),ShadowViewProject);
        ScreenPosition.xyz/=ScreenPosition.w;

        float2 ScreenUV = ScreenPosition.xy;
        ScreenUV.y*=-1.0;
        ScreenUV = ScreenUV* 0.5 + 0.5f;

        UVMin.x = ScreenUV.x > UVMin.x ? UVMin.x : ScreenUV.x;
        UVMin.y = ScreenUV.y > UVMin.y ? UVMin.y : ScreenUV.y;

        UVMax.x = ScreenUV.x > UVMax.x ? ScreenUV.x : UVMax.x;
        UVMax.y = ScreenUV.y > UVMax.y ? ScreenUV.y : UVMax.y;
    }

    uint2 TileIndexMin = uint2(UVMin * TileWidth);
    uint2 TileIndexMax = uint2(UVMax * TileWidth);

    //if(TileIndexMax.x == TileIndexMin.x)
    //{
    //    TileIndexMax.x = TileIndexMin.x +1;
    //}
    //
    //if(TileIndexMax.y == TileIndexMin.y)
    //{
    //    TileIndexMax.y = TileIndexMin.y +1;
    //}

    for(uint IndexX = TileIndexMin.x ; IndexX <= TileIndexMax.x ; IndexX++)
    {
        for(uint indexY = TileIndexMin.y ; indexY <= TileIndexMax.y ; indexY++)
        {
            if(IndexX < 64 && IndexX >0 && indexY < 64 && indexY >0)
            {
                uint Masked = VirtualSMFlags[uint2(IndexX,indexY)];
                if(Masked == 1)
                {
                    uint AddValue = indexY * TileWidth * (4*4*4 + 4*4) + IndexX * (4*4*4 + 4*4);
                    ShadowIndirectCommand cmd = inputCommands[index];
                    cmd.CbGlobalShadowViewProjectAddressVS = UINT64_ADD(inputCommands[index].CbGlobalShadowViewProjectAddressVS,AddValue);
                    cmd.CbGlobalShadowViewProjectAddressPS = cmd.CbGlobalShadowViewProjectAddressVS;
                    outputCommands.Append(cmd);
                }
            }
        }
    }
}