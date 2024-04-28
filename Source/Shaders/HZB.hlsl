
Texture2D TextureSampledInput;
RWTexture2D<float> FurthestHZBOutput_0;
RWTexture2D<float> FurthestHZBOutput_1;
RWTexture2D<float> FurthestHZBOutput_2;
RWTexture2D<float> FurthestHZBOutput_3;
RWTexture2D<float> FurthestHZBOutput_4;

//RWTexture2D<uint> VirtualSMFlags;

SamplerState gsamPointWarp  : register(s0,space1000);

cbuffer cbHZB
{
    float4 DispatchThreadIdToBufferUV;
}

#define MAX_MIP_BATCH_SIZE 5
uint SignedRightShift(uint x, const int bitshift)
{
	if (bitshift > 0)
	{
		return x << asuint(bitshift);
	}
	else if (bitshift < 0)
	{
		return x >> asuint(-bitshift);
	}
	return x;
}


// Returns the pixel pos [[0; N[[^2 in a two dimensional tile size of N=2^TileSizeLog2, to
// store at a given SharedArrayId in [[0; N^2[[, so that a following recursive 2x2 pixel
// block reduction stays entirely LDS memory banks coherent.
uint2 InitialTilePixelPositionForReduction2x2(const uint TileSizeLog2, uint SharedArrayId)
{
	uint x = 0;
	uint y = 0;

	[unroll]
	for (uint i = 0; i < TileSizeLog2; i++)
	{
		const uint DestBitId = TileSizeLog2 - 1 - i;
		const uint DestBitMask = 1u << DestBitId;
		x |= DestBitMask & SignedRightShift(SharedArrayId, int(DestBitId) - int(i * 2 + 0));
		y |= DestBitMask & SignedRightShift(SharedArrayId, int(DestBitId) - int(i * 2 + 1));
	}

	#if 0
	const uint N = 1 << TileSizeLog2;
	return uint2(SharedArrayId / N, SharedArrayId - N * (SharedArrayId / N));
	#endif

	return uint2(x, y);
}

#define GROUP_TILE_SIZE 16
#define DIM_MIP_LEVEL_COUNT 5
groupshared float SharedFurthestDeviceZ[GROUP_TILE_SIZE * GROUP_TILE_SIZE];

void OutputMipLevel(uint MipLevel, uint2 OutputPixelPos, float FurthestDeviceZ)
{
	if (MipLevel == 1)
	{
		FurthestHZBOutput_1[OutputPixelPos] = FurthestDeviceZ;
	}
	else if (MipLevel == 2)
	{
		FurthestHZBOutput_2[OutputPixelPos] = FurthestDeviceZ;
	}
	else if (MipLevel == 3)
	{
		FurthestHZBOutput_3[OutputPixelPos] = FurthestDeviceZ;
	}
    else if (MipLevel == 4)
	{
		FurthestHZBOutput_4[OutputPixelPos] = FurthestDeviceZ;
	}			
}


[numthreads(GROUP_TILE_SIZE, GROUP_TILE_SIZE, 1)]
void HZBBuildCS(
	uint2 GroupId : SV_GroupID,
	uint GroupThreadIndex : SV_GroupIndex)
{
    
    uint2 GroupThreadId = InitialTilePixelPositionForReduction2x2(MAX_MIP_BATCH_SIZE - 1, GroupThreadIndex);
    uint2 DispatchThreadId = GROUP_TILE_SIZE * GroupId + GroupThreadId;

    float2 BufferUV=(DispatchThreadId.xy+0.5f)*DispatchThreadIdToBufferUV.xy;
    float4 DeviceZ = TextureSampledInput.Gather(gsamPointWarp,BufferUV);
    float FurthestDeviceZ = min(min(DeviceZ.x, DeviceZ.y), min(DeviceZ.z, DeviceZ.w));

    uint2 OutputPixelPos=DispatchThreadId;

    FurthestHZBOutput_0[OutputPixelPos.xy]=FurthestDeviceZ;
    SharedFurthestDeviceZ[GroupThreadIndex] = FurthestDeviceZ;

    [unroll]
	for (uint MipLevel = 1; MipLevel < DIM_MIP_LEVEL_COUNT; MipLevel++)
	{
		const uint TileSize = GROUP_TILE_SIZE / (1u << MipLevel);
		const uint ReduceBankSize = TileSize * TileSize;
		
		if (MipLevel == 1)
			GroupMemoryBarrierWithGroupSync();
		[branch]
		if (GroupThreadIndex < ReduceBankSize)
		{
			float4 ParentFurthestDeviceZ;
			ParentFurthestDeviceZ[0] = FurthestDeviceZ;
			[unroll]
			for (uint i = 1; i < 4; i++)
			{
				uint LDSIndex = GroupThreadIndex + i * ReduceBankSize;
				ParentFurthestDeviceZ[i] = SharedFurthestDeviceZ[LDSIndex];
			}
			
			FurthestDeviceZ = min(min(ParentFurthestDeviceZ.x, ParentFurthestDeviceZ.y), min(ParentFurthestDeviceZ.z, ParentFurthestDeviceZ.w));

			OutputPixelPos = OutputPixelPos >> 1;
			OutputMipLevel(MipLevel, OutputPixelPos, FurthestDeviceZ);
			
			SharedFurthestDeviceZ[GroupThreadIndex] = FurthestDeviceZ;
		}
	} 
}    