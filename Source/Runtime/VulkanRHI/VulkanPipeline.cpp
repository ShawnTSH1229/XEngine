#include "VulkanPipeline.h"
#include "VulkanPlatformRHI.h"
#include "VulkanDevice.h"
#include "VulkanState.h"
#include "VulkanRHIPrivate.h"
#include "VulkanResource.h"
#include <Runtime\Core\Math\Math.h>
#include "VulaknHack.h"

XVulkanPipelineStateCacheManager::XVulkanPipelineStateCacheManager(XVulkanDevice* InDevice)
    :Device(InDevice)
{
}

std::size_t XGfxPipelineDesc::CreateKey()
{
    return std::hash<std::string>{}(std::string((char*)this, sizeof(XGfxPipelineDesc)));
}


void GetVulkanShaders(const XRHIBoundShaderStateInput& BSI, XVulkanShader* OutShaders[(uint32)EShaderType::SV_ShaderCount])
{
    OutShaders[(uint32)EShaderType::SV_Vertex] = static_cast<XVulkanVertexShader*>(BSI.RHIVertexShader);
    OutShaders[(uint32)EShaderType::SV_Pixel] = static_cast<XVulkanPixelShader*>(BSI.RHIPixelShader); 
}

std::shared_ptr<XRHIGraphicsPSO> XVulkanPipelineStateCacheManager::RHICreateGraphicsPipelineState(const XGraphicsPSOInitializer& PSOInit)
{
    std::size_t Key;
    XGfxPipelineDesc Desc;
    XVulkanDescriptorSetsLayoutInfo DescriptorSetLayoutInfo;//UnUsed For Now

    CreateGfxEntry(PSOInit, DescriptorSetLayoutInfo, &Desc);
    Key = Desc.CreateKey();

    XVulkanRHIGraphicsPipelineState* NewPSO = nullptr;
    auto iter = GraphicsPSOMap.find(Key);
    if (iter != GraphicsPSOMap.end())
    {
        return  std::shared_ptr<XVulkanRHIGraphicsPipelineState>(iter->second);
    }
    
    NewPSO = new XVulkanRHIGraphicsPipelineState(Device, PSOInit, Desc, Key);
    NewPSO->RenderPass = Device->GetGfxContex()->PrepareRenderPassForPSOCreation(PSOInit);


    XVulkanLayout* Layout = FindOrAddLayout(DescriptorSetLayoutInfo, true);;
    NewPSO->Layout = Layout;

    XVulkanShader* VulkanShaders[(uint32)EShaderType::SV_ShaderCount];
    memset(VulkanShaders, 0, sizeof(XVulkanShader*) * (uint32)EShaderType::SV_ShaderCount);
    GetVulkanShaders(PSOInit.BoundShaderState, VulkanShaders);
   
    //TODO VS PS Shader Layout
    CreateGfxPipelineFromEntry(NewPSO, VulkanShaders, &NewPSO->VulkanPipeline);

    GraphicsPSOMap[Key] = std::shared_ptr<XVulkanRHIGraphicsPipelineState>(NewPSO);
    
    return GraphicsPSOMap[Key];
}

void XVulkanPipelineStateCacheManager::CreateGfxEntry(const XGraphicsPSOInitializer& PSOInitializer, XVulkanDescriptorSetsLayoutInfo& DescriptorSetLayoutInfo, XGfxPipelineDesc* Desc)
{
    XGfxPipelineDesc* OutGfxEntry = Desc;

    //Depth Stencil State
    VkPipelineDepthStencilStateCreateInfo DSStateInfo = static_cast<XVulkanDepthStencilState*>(PSOInitializer.DepthStencilState)->DepthStencilState;
    OutGfxEntry->DepthStencil.bDepthWriteEnable = DSStateInfo.depthWriteEnable;
    OutGfxEntry->DepthStencil.bDepthTestEnable = DSStateInfo.depthTestEnable;
    OutGfxEntry->DepthStencil.DepthCompareOp = DSStateInfo.depthCompareOp;

    //FinalizeBindings
    DescriptorSetLayoutInfo.FinalizeBindings_Gfx(
        static_cast<XVulkanVertexShader*>(PSOInitializer.BoundShaderState.RHIVertexShader),
        static_cast<XVulkanPixelShader*>(PSOInitializer.BoundShaderState.RHIPixelShader));

    //Blend State
    for (int32 Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
    {
        VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState = static_cast<XVulkanBlendState*>(PSOInitializer.BlendState)->BlendStates[Index];
        XGfxPipelineDesc::XBlendAttachment BlendAttachment;
        BlendAttachment.bBlendEnable = PipelineColorBlendAttachmentState.blendEnable;

        BlendAttachment.AlphaBlendOp = PipelineColorBlendAttachmentState.alphaBlendOp;
        BlendAttachment.SrcAlphaBlendFactor = PipelineColorBlendAttachmentState.srcAlphaBlendFactor;
        BlendAttachment.DstAlphaBlendFactor = PipelineColorBlendAttachmentState.dstAlphaBlendFactor;

        BlendAttachment.ColorBlendOp = PipelineColorBlendAttachmentState.colorBlendOp;
        BlendAttachment.SrcColorBlendFactor = PipelineColorBlendAttachmentState.srcColorBlendFactor;
        BlendAttachment.DstColorBlendFactor = PipelineColorBlendAttachmentState.dstColorBlendFactor;

        OutGfxEntry->ColorAttachmentStates.push_back(BlendAttachment);
    }


    OutGfxEntry->RTNums = PSOInitializer.RTNums;

    for (int32 Index = 0; Index < PSOInitializer.RTNums; Index++)
    {
        OutGfxEntry->RenderTargets.RtFotmats[Index] = VkFormat(GPixelFormats[(int)PSOInitializer.RT_Format[Index]].PlatformFormat);
    }
    OutGfxEntry->RenderTargets.DsFotmats = VkFormat(GPixelFormats[(int)PSOInitializer.DS_Format].PlatformFormat);

    XVulkanVertexLayout* const VtxLayout = static_cast<XVulkanVertexLayout* const>(PSOInitializer.BoundShaderState.RHIVertexLayout);
    
    int32 Stride = 0;
    for (int32 Index = 0; Index < VtxLayout->VertexElements.size(); Index++)
    {
        switch (VtxLayout->VertexElements[Index].Format)
        {
        case EVertexElementType::VET_Float1:		Stride += sizeof(float); break;
        case EVertexElementType::VET_Float2:		Stride += sizeof(float)*2; break;
        case EVertexElementType::VET_Float3:		Stride += sizeof(float)*3; break;
        case EVertexElementType::VET_Float4:		Stride += sizeof(float)*4; break;
        case EVertexElementType::VET_Color:			Stride += sizeof(int32); break;
        case EVertexElementType::VET_PackedNormal:	Stride += sizeof(int32); break;
        default:XASSERT(false);
        }
    }
    OutGfxEntry->VertexBinding.Stride = Stride;

    for (int32 Index = 0; Index < VtxLayout->VertexElements.size(); Index++)
    {
        XGfxPipelineDesc::XVertexAttribute VertexAttribute = {};
        VertexAttribute.Location = Index;
        VertexAttribute.Binding = 0;

        switch (VtxLayout->VertexElements[Index].Format)
        {
        case EVertexElementType::VET_Float1:		VertexAttribute.Format = VK_FORMAT_R32_SFLOAT; break;
        case EVertexElementType::VET_Float2:		VertexAttribute.Format = VK_FORMAT_R32G32_SFLOAT; break;
        case EVertexElementType::VET_Float3:		VertexAttribute.Format = VK_FORMAT_R32G32B32_SFLOAT; break;
        case EVertexElementType::VET_Float4:		VertexAttribute.Format = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        case EVertexElementType::VET_Color:			VertexAttribute.Format = VK_FORMAT_R8G8B8A8_UNORM; break;
        case EVertexElementType::VET_PackedNormal:	VertexAttribute.Format = VK_FORMAT_R8G8B8A8_SNORM; break;
        default:XASSERT(false);
        }

        VertexAttribute.Offset = VtxLayout->VertexElements[Index].AlignedByteOffset;
        OutGfxEntry->VertexAttributes.push_back(VertexAttribute);
    }
}


static VkDescriptorSetLayout descriptorSetLayout;
static VkPipelineLayout pipelineLayout;;

VkDescriptorSetLayout VkHack::GetVkDescriptorSetLayout()
{
    return descriptorSetLayout;
}
VkImageView VkHack::GetTexView(XRHITexture2D* Tex)
{
    return static_cast<XVulkanTexture2D*>(Tex)->DefaultView.View;
}
VkPipelineLayout VkHack::GetVkPipelineLayout()
{
    return pipelineLayout;
}

void XVulkanPipelineStateCacheManager::CreateGfxPipelineFromEntry(XVulkanRHIGraphicsPipelineState* PSO, XVulkanShader* Shaders[(uint32)EShaderType::SV_ShaderCount], VkPipeline* Pipeline)
{
    XGfxPipelineDesc* GfxEntry = &PSO->Desc;
    PSO->GetOrCreateShaderModules(Shaders);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = PSO->ShaderModules[(uint32)EShaderType::SV_Vertex];

    //std::string VSName = Shaders[(uint32)EShaderType::SV_Vertex]->GetEntryPoint();
    //vertShaderStageInfo.pName = VSName.c_str();

    //UE vulkan Shader Entry point
    //1.set EntryPoint = main_00000000_00000000
    //2.compile Shader
    //3.get ShaderCode EntryPoint to"main_%0.8x_%0.8x", SpirvByteSize, CRC

    std::string pNameVS = Shaders[(uint32)EShaderType::SV_Vertex]->GetEntryPoint();
    vertShaderStageInfo.pName = pNameVS.c_str();

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = PSO->ShaderModules[(uint32)EShaderType::SV_Pixel];

    std::string pNamePS = Shaders[(uint32)EShaderType::SV_Pixel]->GetEntryPoint();
    fragShaderStageInfo.pName = pNamePS.c_str();

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkGraphicsPipelineCreateInfo PipelineInfo;
    PipelineInfo = {};
    PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.stageCount = 2;
    PipelineInfo.pStages = shaderStages;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = PSO->Desc.VertexBinding.Stride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription>attributeDescriptions;
    for (int32 Index = 0; Index < PSO->Desc.VertexAttributes.size(); Index++)
    {
        VkVertexInputAttributeDescription InputAttributeDescription = {};
        InputAttributeDescription.binding = 0;
        InputAttributeDescription.location = PSO->Desc.VertexAttributes[Index].Location;
        InputAttributeDescription.format = PSO->Desc.VertexAttributes[Index].Format;
        InputAttributeDescription.offset = PSO->Desc.VertexAttributes[Index].Offset;
        attributeDescriptions.push_back(InputAttributeDescription);
    }

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    PipelineInfo.pVertexInputState = &vertexInputInfo;
    XASSERT_TEMP(false);
   
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    PipelineInfo.pInputAssemblyState = &inputAssembly;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    PipelineInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
   
    XASSERT_TEMP(false);
    //rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    
    rasterizer.depthBiasEnable = VK_FALSE;
    PipelineInfo.pRasterizationState = &rasterizer;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    PipelineInfo.pMultisampleState = &multisampling;

    VkPipelineColorBlendAttachmentState BlendStates[MaxSimultaneousRenderTargets];
    for (int32 Index = 0; Index < MaxSimultaneousRenderTargets; Index++)
    {
        PSO->Desc.ColorAttachmentStates[Index].WriteInto(BlendStates[Index]);
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; 
    colorBlending.attachmentCount = PSO->Desc.RTNums;
    colorBlending.pAttachments = BlendStates;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    PipelineInfo.pColorBlendState = &colorBlending;

    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{};
    PSO->Desc.DepthStencil.WriteInto(DepthStencilInfo);
    PipelineInfo.pDepthStencilState = &DepthStencilInfo;

    std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    PipelineInfo.pDynamicState = &dynamicState;
    
    PipelineInfo.layout = PSO->Layout->PipelineLayout;
    
    {
        descriptorSetLayout = PSO->Layout->DescriptorSetLayout.LayoutHandle;
        pipelineLayout = PSO->Layout->PipelineLayout;
    }

    PipelineInfo.renderPass = PSO->RenderPass->GetRenderPass();
    PipelineInfo.subpass = 0;
    PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VULKAN_VARIFY(vkCreateGraphicsPipelines(Device->GetVkDevice(), VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, Pipeline));
}

XVulkanPipelineStateCacheManager::~XVulkanPipelineStateCacheManager()
{
    for (auto iter = LayoutMap.begin(); iter != LayoutMap.end(); iter++)
    {
        if (iter->second)
        {
            delete iter->second;
        }
    }
}

XVulkanLayout* XVulkanPipelineStateCacheManager::FindOrAddLayout(const XVulkanDescriptorSetsLayoutInfo& DescriptorSetLayoutInfo, bool bGfxLayout)
{
    uint32 Hash = DescriptorSetLayoutInfo.GetHash();
    auto iter = LayoutMap.find(Hash);
    if (iter != LayoutMap.end())
    {
        return iter->second;
    }

    XVulkanLayout* Layout = nullptr;
    XVulkanGfxLayout* GfxLayout = nullptr;
    if (bGfxLayout)
    {
        GfxLayout = new XVulkanGfxLayout(Device);
        Layout = GfxLayout;
    }
    else
    {
        XASSERT(false);
    }
    //TODO:
    static_assert(sizeof(XVulkanDescriptorSetsLayoutInfo) == 112);
    Layout->DescriptorSetLayout.LayoutTypes = DescriptorSetLayoutInfo.LayoutTypes;
    Layout->DescriptorSetLayout.RemappingInfo = DescriptorSetLayoutInfo.RemappingInfo;
    Layout->DescriptorSetLayout.SetLayout = DescriptorSetLayoutInfo.SetLayout;
    Layout->DescriptorSetLayout.Hash = DescriptorSetLayoutInfo.Hash;
    Layout->Compile(DSetLayoutMap);
    
    LayoutMap[Hash] = Layout;

    if (GfxLayout)
    {
        GfxLayout->GfxPipelineDescriptorInfo.Initialize(GfxLayout->GetDescriptorSetsLayout()->RemappingInfo);
    }

    return Layout;
}

std::shared_ptr<XRHIGraphicsPSO> XVulkanPlatformRHI::RHICreateGraphicsPipelineState(const XGraphicsPSOInitializer& PSOInit)
{
    return Device->PipelineStateCache->RHICreateGraphicsPipelineState(PSOInit);
}



XVulkanRHIGraphicsPipelineState::XVulkanRHIGraphicsPipelineState(XVulkanDevice* Device, const XGraphicsPSOInitializer& PSOInitializer, XGfxPipelineDesc& InDesc, std::size_t Key)
    :Desc(InDesc)
{

}

void XVulkanRHIGraphicsPipelineState::GetOrCreateShaderModules(XVulkanShader* const* Shaders)
{
    for (int32 Index = 0; Index < (int32)EShaderType::SV_ShaderCount; ++Index)
    {
        XVulkanShader* Shader = Shaders[Index];
        if (Shader)
        {
            ShaderModules[Index] = Shader->GetOrCreateHandle(Desc, Index);
        }
    }
}

XVulkanRHIGraphicsPipelineState::~XVulkanRHIGraphicsPipelineState()
{

}

void XGfxPipelineDesc::XBlendAttachment::WriteInto(VkPipelineColorBlendAttachmentState& Out) const
{
    Out.blendEnable = bBlendEnable ? VK_TRUE : VK_FALSE;
    Out.colorBlendOp = (VkBlendOp)ColorBlendOp;
    Out.srcColorBlendFactor = (VkBlendFactor)SrcColorBlendFactor;
    Out.dstColorBlendFactor = (VkBlendFactor)DstColorBlendFactor;
    Out.alphaBlendOp = (VkBlendOp)AlphaBlendOp;
    Out.srcAlphaBlendFactor = (VkBlendFactor)SrcAlphaBlendFactor;
    Out.dstAlphaBlendFactor = (VkBlendFactor)DstAlphaBlendFactor;
    Out.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
}

void XGfxPipelineDesc::XDepthStencil::WriteInto(VkPipelineDepthStencilStateCreateInfo& Out) const
{
    Out = {};
    Out.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    Out.depthCompareOp = (VkCompareOp)DepthCompareOp;
    Out.depthTestEnable = bDepthTestEnable;
    Out.depthWriteEnable = bDepthWriteEnable;
    Out.depthBoundsTestEnable = VK_FALSE;
    Out.stencilTestEnable = VK_FALSE;
}
