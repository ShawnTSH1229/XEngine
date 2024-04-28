#include "VulkanContext.h"
#include "VulkanRHIPrivate.h"
#include "VulkanPendingState.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResource.h"

void XVulkanCommandListContext::RHISetShaderTexture(EShaderType ShaderType, uint32 TextureIndex, XRHITexture* NewTextureRHI)
{
	XVulkanTextureBase* Texture = GetVulkanTextureFromRHITexture(NewTextureRHI);
	VkImageLayout Layout = GlobalLayoutManager.FindLayout(Texture->Surface.Image);

	PendingGfxState->SetTextureForStage(ShaderType, TextureIndex, Texture, Layout);
}

void XVulkanCommandListContext::RHISetShaderConstantBuffer(EShaderType ShaderType, uint32 BufferIndex, XRHIConstantBuffer* RHIConstantBuffer)
{
	XVulkanConstantBuffer* ConstantBuffer = static_cast<XVulkanConstantBuffer*>(RHIConstantBuffer);
	PendingGfxState->SetUniformBuffer(0, BufferIndex, ConstantBuffer);
}

void XVulkanCommandListContext::SetVertexBuffer(XRHIBuffer* RHIVertexBuffer, uint32 VertexBufferSlot, uint32 OffsetFormVBBegin)
{
	XVulkanResourceMultiBuffer* VertexBuffer = static_cast<XVulkanResourceMultiBuffer*>(RHIVertexBuffer);
	if (VertexBuffer != nullptr)
	{
		PendingGfxState->SetStreamSource(0, VertexBuffer->Buffer.VulkanHandle, OffsetFormVBBegin + VertexBuffer->Buffer.Offset);
	}
}


void XVulkanCommandListContext::RHIDrawIndexedPrimitive(XRHIBuffer* IndexBuffer, uint32 IndexCountPerInstance, uint32 InstanceCount, uint32 StartIndexLocation, uint32 BaseVertexLocation, uint32 StartInstanceLocation)
{
	XVulkanCmdBuffer* Cmd = CmdBufferManager->GetActiveCmdBuffer();
	PendingGfxState->PrepareForDraw(Cmd);

	
	XVulkanResourceMultiBuffer* VkIndexBuffer = static_cast<XVulkanResourceMultiBuffer*>(IndexBuffer);
	vkCmdBindIndexBuffer(Cmd->GetHandle(), VkIndexBuffer->Buffer.VulkanHandle, VkIndexBuffer->Buffer.Offset,
		VkIndexBuffer->GetStride() == 4 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(Cmd->GetHandle(), IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}