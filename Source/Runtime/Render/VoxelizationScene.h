#pragma once
#include "Runtime/RenderCore/RenderResource.h"

#define VoxelDimension 512
#define OctreeRealBufferSize 1024
#define OctreeHeight 9

//1 8 64 512
//1 2 4 8 16 32 64 128 512

#define BrickBufferSize 64

#define ApproxVoxelDimension (512*512*16)

#define MinBoundX -48
#define MinBoundY -48
#define MinBoundZ -48

#define MaxBoundX 48
#define MaxBoundY 48
#define MaxBoundZ 48

class XSVOGIResourece :public XRenderResource
{
public:

	//Voxel Scene
	std::shared_ptr<XRHIConstantBuffer>VoxelSceneVSCBuffer;
	std::shared_ptr<XRHIConstantBuffer>VoxelScenePSCBuffer;

	std::shared_ptr <XRHITexture2D> VoxelSceneRTPlaceHolder;

	std::shared_ptr <XRHITexture2D> NodeCountAndOffsetBuffer;

	std::shared_ptr<XRHIStructBuffer>VoxelArrayRW;
	std::shared_ptr<XRHIUnorderedAcessView> VoxelArrayUnorderedAcessView;
	std::shared_ptr<XRHIShaderResourceView> VoxelArrayShaderResourceView;

	//SVO Build
	std::shared_ptr <XRHITexture2D> SpaseVoxelOctree;
	std::vector<std::shared_ptr<XRHIConstantBuffer>>cbSVOBuildBufferLevels;

	std::shared_ptr<XRHIStructBuffer>SVOCounterBuffer;
	std::shared_ptr<XRHIUnorderedAcessView> SVOCounterBufferUnorderedAcessView;

	//SVO Inject Light
	std::shared_ptr <XRHITexture3D> IrradianceBrickBufferRWUAV;
	std::shared_ptr <XRHITexture3D> IrradianceBrickBufferPinPongUAV;
	std::shared_ptr<XRHIConstantBuffer>RHIcbMainLight;

	XRHITexture3D* BrickBufferPingPong[2];
	int PingPongIndex ;
	void InitRHI()override;
	void ReleaseRHI()override;
};
struct cbMainLightStruct
{
	XVector3 LightDir;
	float LightIntensity;
	XVector3 LightColor;
	float cbMainLight_padding1;
};

struct cbSVOBuildBufferStruct
{
	uint32 CurrentLevel;
};

struct VoxelScenePSBufferStruct
{
	XVector3 MinBound;
	float VoxelBufferDimension;
	XVector3 MaxBound;
	float Pad;
};

extern TGlobalResource<XSVOGIResourece>SVOGIResourece;;
extern bool VoxelSceneInitialize;