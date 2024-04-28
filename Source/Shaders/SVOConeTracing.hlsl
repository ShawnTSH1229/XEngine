#include "Common.hlsl"
#include "SVOGICommon.hlsl"
struct VertexIn
{
	float2 PosIn    : ATTRIBUTE0;
	float2 Tex    : ATTRIBUTE1;
};

cbuffer cbVoxelization
{
	float3 MinBound;
    float VoxelBufferDimension;
    float3 MaxBound;
    float cbVoxelization_Padding0;
};

Texture2D SceneDepth;
Texture2D NormalTexture;//SceneTexturesStruct_GBufferATexture

static const uint conesNum = 5;
void RotateConesDir( float3 normal, inout float3 coneDir[conesNum] )
{
    // find rotation between normal and half-sphere orientation (coneDir[0])
    float cosRotAngle = dot( normal, coneDir[0] );
    
    // also we can add random rotates to cones
    
    // don't rotate if vecs are co-directional
    if ( cosRotAngle < 0.995f )
    {
        float3 rotVec = float3( 1.0f, 0.0f, 0.0f );
        float sinRotAngle = 0.01f;

        // use default rotVec for rotation if vecs are opposite
        if ( cosRotAngle > -0.995f )
        {
            rotVec = cross( coneDir[0], normal );
            sinRotAngle = length( rotVec );
            rotVec = normalize( rotVec );
        }

        // rotate half-sphere
        [unroll]
        for ( uint i = 0; i < conesNum; i++ )
        {
            float3 a = coneDir[i];
            float3 v = rotVec;
            float cosAV = dot( a, v );

            // don't rotate if vecs are co-directional
            if ( cosAV < 0.995f )
            {
                float3 aParrV = v * cosAV;
                float3 aPerpV = a - aParrV;
                
                float3 aPerpVNorm = normalize( aPerpV );
                
                float3 w = normalize( cross( v, aPerpVNorm ) );
                float3 daPerpV = ( aPerpVNorm * cosRotAngle + w * sinRotAngle ) * length( aPerpV );

                coneDir[i] = aParrV + daPerpV;
            }
        }
    }
}

float GetNodeWidth( in uint octreeLevel )
{
    return ( MaxBound.x - MinBound.x ) / ( uint(OctreeResolution) >> uint( OctreeHeight - octreeLevel -1) );
}

uint PackUint3ToUint( uint3 Value )
{
    return ( ( Value.x & 0x3ff ) )       |
           ( ( Value.y & 0x3ff ) << 10 ) |
             ( Value.z & 0x3ff ) << 20;
}

#define brickBufferSize 64
#define BRICK_SIZE 3
#define SAMPLING_AREA_SIZE ( 2.0f / brickBufferSize )
#define SAMPLING_OFFSET ( 0.5f / brickBufferSize )

uint3 NodeIDToTextureCoords( uint id )
{
    uint blocksPerAxis = brickBufferSize / BRICK_SIZE; // 3x3x3 values per brick
    uint blocksPerAxisSq = blocksPerAxis * blocksPerAxis;
    return uint3( id % blocksPerAxis, (id / blocksPerAxis) % blocksPerAxis, id / blocksPerAxisSq ) * BRICK_SIZE;
}

bool WorldToBrickPosition( in float3 worldPos, in uint octreeLevel, out float3 brickPos )
{
    if ( any( worldPos.xyz < MinBound ) || any( worldPos.xyz >  MaxBound ) )
       return false;

    uint3 VoxelSceneIndex = (( worldPos - MinBound ) / ( MaxBound - MinBound ) ) * VoxelBufferDimension;    
    uint octreePos = PackUint3ToUint(VoxelSceneIndex);

    uint nodeIndex;
    uint3 CurrentMinPosIndex;
    if ( SearchOctreeR( octreePos, octreeLevel, nodeIndex, CurrentMinPosIndex ) )
    {
        uint nodeID = nodeIndex / SIZE_OF_NODE_STRUCT;
        float3 coordsStart = float3( NodeIDToTextureCoords( nodeID ) ) / brickBufferSize;

        float nodeWidth = GetNodeWidth( octreeLevel );
        float3 startWPos = float3( CurrentMinPosIndex ) / OctreeResolution * ( MaxBound - MinBound ) + MinBound;

        float3 coordsOffset = ( worldPos - startWPos ) / nodeWidth * SAMPLING_AREA_SIZE;
        brickPos = coordsStart + coordsOffset + SAMPLING_OFFSET;
        return true;
    }
    return false;
}

Texture3D<float4> IrradianceBrickBufferROnly;
#define FIRST_LEVEL 5
#define LAST_LEVEL 1
float4 XConeTracingPS(
    in float4 PosH: SV_POSITION,
    in float2 Tex: TEXCOORD0
): SV_Target
{
    float4 GbufferA= NormalTexture.Load(int3(PosH.xy,0));
    float3 Normal = normalize(GbufferA.xyz);
    float  Depth  =  SceneDepth.Load( int3(PosH.xy,0)).r;

    if(Depth < 0.01)
    {
        return float4(0,0,0,0);
    }

    float2 ScreenPos = Tex * 2.0f - 1.0f; ScreenPos.y *= -1.0f;
    float4 NDCPosNoDivdeW = float4(ScreenPos , Depth , 1.0);
    float4 WorldPosition = mul(cbView_ViewPorjectionMatrixInverse,NDCPosNoDivdeW);
    WorldPosition.xyz/=WorldPosition.w;

    if ( any( WorldPosition.xyz < MinBound ) || any( WorldPosition.xyz >  MaxBound ) )
        discard;

    float4 coneCol[conesNum] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };
    float3 coneDir[conesNum] = {
        float3(  0.0f,      1.0f,  0.0f      ),
        float3(  0.374999f, 0.5f,  0.374999f ),
        float3(  0.374999f, 0.5f, -0.374999f ),
        float3( -0.374999f, 0.5f,  0.374999f ),
        float3( -0.374999f, 0.5f, -0.374999f )
    };

    RotateConesDir( Normal, coneDir );
    float4 sampleCol = 0.0f;
    float opacity = 0.0f;

    float4 preCol = 0.0f;
    float4 curCol = 0.0f;
    for ( uint i = 0; i < conesNum; i++ )
    {
        for ( uint octreeLevel = FIRST_LEVEL; octreeLevel > LAST_LEVEL; octreeLevel-- )
        {
            float nodeWidth = GetNodeWidth( octreeLevel);
            float sampleOffset = 0.2 + nodeWidth * 1.01f;
            float3 worldSamplePos = WorldPosition.xyz + coneDir[i] * sampleOffset;

            float3 brickSamplePos;
            if ( WorldToBrickPosition( worldSamplePos, octreeLevel, brickSamplePos ) )
            {
                sampleCol = IrradianceBrickBufferROnly.Sample( gsamPointWarp, brickSamplePos );
                opacity = sampleCol.a;

                curCol = float4( sampleCol.rgb * opacity, opacity );
                preCol = coneCol[i];

                coneCol[i].rgb = preCol.rgb + curCol.rgb * ( 1.0f - preCol.a );
                coneCol[i].a = preCol.a + ( 1.0f - preCol.a ) * curCol.a;
            }
            else if ( any( worldSamplePos > MaxBound ) || any( worldSamplePos < MinBound ) )
            {
                coneCol[i].a = 1.0f;
            }

            if ( coneCol[i].a >= 1.0f )
                break;

        }
    }

    float weight = 1.0f / conesNum;
    float4 result = 0.0f;

    for ( i = 0; i < conesNum; i++ )
    {
        result += float4( coneCol[i].rgb, 1 ) * weight;
    }

    //float TempRes = Normal.x + Depth.x + WorldPosition.x ;
    //TempRes*= 0.0f;
    //return float4(TempRes,0,0,0);
    return float4(result.xyz / 9,1.0f);
}