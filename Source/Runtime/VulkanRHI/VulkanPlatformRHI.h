#pragma once
#include <Runtime\HAL\Mch.h>
#include <vulkan\vulkan_core.h>
#include "Runtime\RHI\PlatformRHI.h"


#define VULKAN_VARIFY(x)										\
    {															\
        VkResult Result = (x);									\
        if(Result != VkResult::VK_SUCCESS) { __debugbreak(); }	\
    }
class XVulkanViewport;
class XVulkanDevice;


class XVulkanPlatformRHI :public XPlatformRHI
{
public:
	

	XVulkanPlatformRHI();
	~XVulkanPlatformRHI();
	void Init() override;

	//CreateVertexLayout
	std::shared_ptr<XRHIVertexLayout> RHICreateVertexDeclaration(const XRHIVertexLayoutArray& Elements) final override;

	//Create buffer
	virtual std::shared_ptr<XRHIBuffer>RHICreateBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override;
	[[deprecated]]
	std::shared_ptr<XRHIBuffer>RHICreateVertexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override;
	[[deprecated]]
	std::shared_ptr<XRHIBuffer>RHICreateIndexBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override { return RHICreateVertexBuffer(Stride, Size, Usage, ResourceData); };
	[[deprecated]]
	std::shared_ptr<XRHIStructBuffer>RHICreateStructBuffer(uint32 Stride, uint32 Size, EBufferUsage Usage, XRHIResourceCreateData ResourceData)final override { XASSERT_TEMP(false); return nullptr; };
	
	std::shared_ptr<XRHIUnorderedAcessView> RHICreateUnorderedAccessView(XRHIStructBuffer* StructuredBuffer, bool bUseUAVCounter, bool bAppendBuffer, uint64 CounterOffsetInBytes) final override { XASSERT_TEMP(false); return nullptr; };
	std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIStructBuffer* StructuredBuffer) final override { XASSERT_TEMP(false); return nullptr; };
	std::shared_ptr<XRHIShaderResourceView> RHICreateShaderResourceView(XRHIBuffer* StructuredBuffer) final override { XASSERT_TEMP(false); return nullptr; };
	void RHIResetStructBufferCounter(XRHIStructBuffer* RHIStructBuffer, uint32 CounterOffset)final override { XASSERT_TEMP(false);};
	void RHICopyTextureRegion(XRHITexture* RHITextureDst, XRHITexture* RHITextureSrc, uint32 DstX, uint32 DstY, uint32 DstZ, uint32 OffsetX, uint32 OffsetY, uint32 OffsetZ)final override { XASSERT_TEMP(false);};

	//Command Signature
	XRHIUnorderedAcessView* GetRHIUAVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0)override { XASSERT(false); return nullptr; };
	XRHIShaderResourceView* GetRHISRVFromTexture(XRHITexture* RHITexture, uint32 MipIndex = 0)override { XASSERT(false); return nullptr; };
	uint64 RHIGetCmdBufferOffset(XRHIStructBuffer* RHIStructBuffer)override { XASSERT(false); return 0; };
	void* RHIGetCommandDataPtr(std::vector<XRHICommandData>& RHICmdData, uint32& OutCmdDataSize)override { XASSERT(false); return nullptr; };
	std::shared_ptr<XRHICommandSignature> RHICreateCommandSignature(XRHIIndirectArg* RHIIndirectArg, uint32 ArgCount, XRHIVertexShader* VertexShader, XRHIPixelShader* PixelShader) override { XASSERT(false); return nullptr; };

	//Create State
	std::shared_ptr<XRHISamplerState> RHICreateSamplerState(const XSamplerStateInitializerRHI& Initializer) final override;
	std::shared_ptr<XRHIRasterizationState> RHICreateRasterizationStateState(const XRasterizationStateInitializerRHI& Initializer)final override;
	std::shared_ptr<XRHIDepthStencilState> RHICreateDepthStencilState(const XDepthStencilStateInitializerRHI& Initializer)final override;
	std::shared_ptr<XRHIBlendState> RHICreateBlendState(const XBlendStateInitializerRHI& Initializer)final override;

	//Create Shader
	std::shared_ptr<XRHIVertexShader> RHICreateVertexShader(XArrayView<uint8> Code)final override;
	 
	std::shared_ptr<XRHIPixelShader> RHICreatePixelShader(XArrayView<uint8> Code)final override;
	std::shared_ptr<XRHIComputeShader> RHICreateComputeShader(XArrayView<uint8> Code)final override { XASSERT(false); return nullptr; };

	//CreatePSO
	std::shared_ptr<XRHIGraphicsPSO> RHICreateGraphicsPipelineState(const XGraphicsPSOInitializer& PSOInit)final override;
	std::shared_ptr<XRHIComputePSO> RHICreateComputePipelineState(const XRHIComputeShader* RHIComputeShader)final override { XASSERT(false); return nullptr; };
#if RHI_RAYTRACING
	std::shared_ptr<XRHIRayTracingPSO> RHICreateRayTracingPipelineState(const XRayTracingPipelineStateInitializer& OriginalInitializer);
#endif
	//RHICreateUniformBuffer
	std::shared_ptr<XRHIConstantBuffer> RHICreateConstantBuffer(uint32 size)override;

	std::shared_ptr<XRHITexture2D> RHICreateTexture2D(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray,
		bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size = 0) override {
		XASSERT_TEMP(false)
			return nullptr;
	};

	std::shared_ptr<XRHITexture2D> RHICreateTexture2D2(uint32 width, uint32 height, uint32 SizeZ, bool bTextureArray,
		bool bCubeTexture, EPixelFormat Format, ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data, uint32 Size = 0) override;

	std::shared_ptr<XRHITexture3D> RHICreateTexture3D(uint32 width, uint32 height, uint32 SizeZ, EPixelFormat Format,
		ETextureCreateFlags flag, uint32 NumMipsIn, uint8* tex_data)override {
		XASSERT_TEMP(false);
		return nullptr;
	};

	XRHITexture* RHIGetCurrentBackTexture()override;

	//Lock/UnLock Vertex Buffer
	void* LockVertexBuffer(XRHIBuffer* VertexBuffer, uint32 Offset, uint32 SizeRHI)override{ XASSERT(false); return nullptr;}
	void UnLockVertexBuffer(XRHIBuffer* VertexBuffer)override { XASSERT(false); };

	void* LockIndexBuffer(XRHIBuffer* IndexBuffer, uint32 Offset, uint32 SizeRHI)override { XASSERT(false); return nullptr;};
	void UnLockIndexBuffer(XRHIBuffer* IndexBuffer)override { XASSERT(false); };

#if RHI_RAYTRACING
	virtual XRayTracingAccelerationStructSize RHICalcRayTracingGeometrySize(const XRayTracingGeometryInitializer& Initializer);
	virtual XRayTracingAccelerationStructSize RHICalcRayTracingSceneSize(uint32 MaxInstances, ERayTracingAccelerationStructureFlags Flags);
	virtual std::shared_ptr<XRHIRayTracingGeometry> RHICreateRayTracingGeometry(const XRayTracingGeometryInitializer& Initializer);
	virtual std::shared_ptr<XRHIRayTracingScene> RHICreateRayTracingScene(XRayTracingSceneInitializer Initializer) ;
	virtual std::shared_ptr<XRHIRayTracingShader> RHICreateRayTracingShader(XArrayView<uint8> Code, EShaderType ShaderType);
#endif

	//Vulkan
	static void GetInstanceLayersAndExtensions(std::vector<const ACHAR*>& OutLayer, std::vector<const ACHAR*>& OutExtension);
	static VKAPI_ATTR VkBool32 VKAPI_CALL VkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData); 
	XVulkanViewport* GetVulkanViewport() { return VulkanViewport; }
	inline XVulkanDevice* GetDevice() { return Device; }
private:
	friend class VkHack;
	VkInstance Instance;
	VkDebugUtilsMessengerEXT DebugMessenger;
	XVulkanDevice* Device;
	XVulkanViewport* VulkanViewport;
	void SelectAndInitDevice();
};