#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

#include "Runtime/Core/MainInit.h"
#include "Runtime/Engine/ResourcecConverter.h"
#include "Runtime/Render/DeferredShadingRenderer.h"
#include "Runtime/Core/ComponentNode/GameTimer.h"

#include "Runtime/ApplicationCore/Windows/WindowsApplication.h"
#include "Runtime/ApplicationCore/GlfwApp/GlfwApplication.h"
XApplication* XApplication::Application = nullptr;

#if !USE_DX12
#include <fstream>
#include "Runtime\VulkanRHI\VulkanResource.h"
#include <Runtime\VulkanRHI\VulaknHack.h>
#include "Runtime\RHI\RHIStaticStates.h"
#include "Runtime\RHI\PipelineStateCache.h"
const int MAX_FRAMES_IN_FLIGHT = 1;
#include "Runtime\RenderCore\GlobalShader.h"
#include <chrono>
#include <Runtime\RenderCore\ShaderParameter.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Runtime\VulkanRHI\VulkanResource.h"

#include "Runtime\Render\RayTracingInstanceBufferUtil.h"

#include "Runtime\Render\RayTracingBasicShaders.h"

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class XVkTestResourece :public XRenderResource
{
public:
    std::shared_ptr<XRHIConstantBuffer>RHICbVkTest;

    void InitRHI()override
    {
        RHICbVkTest = RHICreateConstantBuffer(sizeof(UniformBufferObject));
    }
};

TGlobalResource<XVkTestResourece>VkTestCBResourece;


class XHLSL2SPIRPS :public XGloablShader
{
public:
    static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
    {
        return new XHLSL2SPIRPS(Initializer);
    }

    static ShaderInfos StaticShaderInfos;
    static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

    XHLSL2SPIRPS(const XShaderInitlizer& Initializer)
        :XGloablShader(Initializer)
    {
        TestTexture.Bind(Initializer.ShaderParameterMap, "texSampler");
    }

    void SetParameter(
        XRHICommandList& RHICommandList,
        XRHITexture* InTexture)
    {
        SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, TestTexture, InTexture);
    }

    TextureParameterType    TestTexture;
};

class XHLSL2SPIRVS :public XGloablShader
{
public:
    static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
    {
        return new XHLSL2SPIRVS(Initializer);
    }
    static ShaderInfos StaticShaderInfos;
    static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

    XHLSL2SPIRVS(const XShaderInitlizer& Initializer) :XGloablShader(Initializer) 
    {
        cbView.Bind(Initializer.ShaderParameterMap, "cbView");
    }

    void SetParameter(
        XRHICommandList& RHICommandList,
        XRHIConstantBuffer* IncbView)
    {
        SetShaderConstantBufferParameter(RHICommandList, EShaderType::SV_Vertex, cbView, IncbView);
    }

    CBVParameterType cbView;
};
XHLSL2SPIRVS::ShaderInfos XHLSL2SPIRVS::StaticShaderInfos(
    "XHLSL2SPIRVS", GET_SHADER_PATH("VulkanShaderTest/hlsl2spirtest.hlsl"),
    "VS", EShaderType::SV_Vertex, XHLSL2SPIRVS::CustomConstrucFunc,
    XHLSL2SPIRVS::ModifyShaderCompileSettings);
XHLSL2SPIRPS::ShaderInfos XHLSL2SPIRPS::StaticShaderInfos(
    "XHLSL2SPIRPS", GET_SHADER_PATH("VulkanShaderTest/hlsl2spirtest.hlsl"),
    "PS", EShaderType::SV_Pixel, XHLSL2SPIRPS::CustomConstrucFunc,
    XHLSL2SPIRPS::ModifyShaderCompileSettings);

class XTestVertexLayout : public XRenderResource
{
public:
    std::shared_ptr<XRHIVertexLayout> RHIVertexLayout;
    virtual void InitRHI()override
    {
        XRHIVertexLayoutArray LayoutArray;
        LayoutArray.push_back(XVertexElement(0, EVertexElementType::VET_Float3, 0, 0));
        LayoutArray.push_back(XVertexElement(1, EVertexElementType::VET_Float3, 0, 0 + sizeof(DirectX::XMFLOAT2)));
        LayoutArray.push_back(XVertexElement(2, EVertexElementType::VET_Float2, 0, 0 + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMFLOAT3)));
        RHIVertexLayout = RHICreateVertexLayout(LayoutArray);
    }

    virtual void ReleaseRHI()override
    {
        RHIVertexLayout.reset();
    }
};
TGlobalResource<XTestVertexLayout> GTestVtxLayout;

struct XTestVertex
{
    XVector3 Position;
    XVector3 Color;
    XVector2 TexCoord;
    struct XTestVertex
    (XVector3 PositionIn, XVector3 ColorIn, XVector2 TexCoordIn)
        :Position(PositionIn), Color(ColorIn), TexCoord(TexCoordIn){}
};

class XTestVertexBuffer :public RVertexBuffer
{
public:
    void InitRHI()override
    {
        TResourceVector<XTestVertex>Vertices;
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, -0.5f, 0.0f), XVector3(1.0f, 0.0f, 0.0f), XVector2(1.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, -0.5f, 0.0f), XVector3(0.0f, 1.0f, 0.0f), XVector2(0.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, 0.5f, 0.0f), XVector3(0.0f, 0.0f, 1.0f), XVector2(0.0f, 1.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, 0.5f, 0.0f), XVector3(1.0f, 1.0f, 1.0f), XVector2(1.0f, 1.0f)));

        Vertices.PushBack(XTestVertex(XVector3(-0.5f, -0.5f, -0.5f), XVector3(1.0f, 0.0f, 0.0f), XVector2(1.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, -0.5f, -0.5f), XVector3(0.0f, 1.0f, 0.0f), XVector2(0.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, 0.5f, -0.5f), XVector3(0.0f, 0.0f, 1.0f), XVector2(0.0f, 1.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, 0.5f, -0.5f), XVector3(1.0f, 1.0f, 1.0f), XVector2(1.0f, 1.0f)));

        Vertices.PushBack(XTestVertex(XVector3(0.5f, 0.5f, 0.0f), XVector3(1.0f, 0.0f, 0.0f), XVector2(1.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, -0.5f, 0.0f), XVector3(0.0f, 1.0f, 0.0f), XVector2(0.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, -0.5f, -0.5f), XVector3(0.0f, 0.0f, 1.0f), XVector2(0.0f, 1.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, 0.5f, -0.5f), XVector3(1.0f, 1.0f, 1.0f), XVector2(1.0f, 1.0f)));

        Vertices.PushBack(XTestVertex(XVector3(-0.5f, -0.5f, 0.0f), XVector3(1.0f, 0.0f, 0.0f), XVector2(1.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, 0.5f, 0.0f), XVector3(0.0f, 1.0f, 0.0f), XVector2(0.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, 0.5f, -0.5f), XVector3(0.0f, 0.0f, 1.0f), XVector2(0.0f, 1.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, -0.5f, -0.5f), XVector3(1.0f, 1.0f, 1.0f), XVector2(1.0f, 1.0f)));

        Vertices.PushBack(XTestVertex(XVector3(-0.5f, 0.5f, 0.0f), XVector3(1.0f, 0.0f, 0.0f), XVector2(1.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3( 0.5f, 0.5f, 0.0f), XVector3(0.0f, 1.0f, 0.0f), XVector2(0.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3( 0.5f, 0.5f, -0.5f), XVector3(0.0f, 0.0f, 1.0f), XVector2(0.0f, 1.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, 0.5f, -0.5f), XVector3(1.0f, 1.0f, 1.0f), XVector2(1.0f, 1.0f)));

        Vertices.PushBack(XTestVertex(XVector3(0.5f, -0.5f, 0.0f), XVector3(1.0f, 0.0f, 0.0f), XVector2(1.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, -0.5f, 0.0f), XVector3(0.0f, 1.0f, 0.0f), XVector2(0.0f, 0.0f)));
        Vertices.PushBack(XTestVertex(XVector3(-0.5f, -0.5f, -0.5f), XVector3(0.0f, 0.0f, 1.0f), XVector2(0.0f, 1.0f)));
        Vertices.PushBack(XTestVertex(XVector3(0.5f, -0.5f, -0.5f), XVector3(1.0f, 1.0f, 1.0f), XVector2(1.0f, 1.0f)));

        XRHIResourceCreateData CreateData(&Vertices);
        RHIVertexBuffer = RHIcreateVertexBuffer(sizeof(XTestVertex), 4 * sizeof(XTestVertex), EBufferUsage::BUF_Static, CreateData);
    }
};

class XTestIndexBuffer :public RVertexBuffer
{
public:
    void InitRHI()override
    {
        TResourceVector<uint16> Indecies;
        Indecies.PushBack(0);
        Indecies.PushBack(1);
        Indecies.PushBack(2);
        Indecies.PushBack(2);
        Indecies.PushBack(3);
        Indecies.PushBack(0);

        Indecies.PushBack(4);
        Indecies.PushBack(5);
        Indecies.PushBack(6);
        Indecies.PushBack(6);
        Indecies.PushBack(7);
        Indecies.PushBack(4);

        Indecies.PushBack(8);
        Indecies.PushBack(9);
        Indecies.PushBack(10);
        Indecies.PushBack(10);
        Indecies.PushBack(11);
        Indecies.PushBack(8);

        Indecies.PushBack(12);
        Indecies.PushBack(13);
        Indecies.PushBack(14);
        Indecies.PushBack(14);
        Indecies.PushBack(15);
        Indecies.PushBack(12);


        Indecies.PushBack(16);
        Indecies.PushBack(17);
        Indecies.PushBack(18);
        Indecies.PushBack(18);
        Indecies.PushBack(19);
        Indecies.PushBack(16);

        Indecies.PushBack(20);
        Indecies.PushBack(21);
        Indecies.PushBack(22);
        Indecies.PushBack(22);
        Indecies.PushBack(23);
        Indecies.PushBack(20);

        XRHIResourceCreateData CreateData(&Indecies);
        RHIVertexBuffer = RHIcreateVertexBuffer(sizeof(uint16), 36 * sizeof(uint16), EBufferUsage::BUF_Static, CreateData);
    }
};
TGlobalResource<XTestVertexBuffer> GTestVertexRHI;
TGlobalResource<XTestIndexBuffer> GTestIndexRHI;


#endif

class XSandBox
{
public:
#if !USE_DX12
    VkHack mVkHack;
    std::shared_ptr<XRHITexture2D> RHITestTexture2D;
    std::shared_ptr<XRHITexture2D> RHITestTexture2DDepth;
#endif

    XRHICommandList RHICmdList;

#if USE_DX12
    XBoundSphere SceneBoundingSphere;
    XDeferredShadingRenderer DeferredShadingRenderer;
    std::vector<std::shared_ptr<GGeomertry>> RenderGeos;
#endif

    //Timer
    GameTimer mTimer;
    
    //Camera
    GCamera CamIns;
    float Far = 1000.0f;
    float Near = 1.0f;
    float FoVAngleY = 0.25f * X_PI;

    //Light
    XVector3 LightDir = {1 / sqrtf(3.0f), 1 / sqrtf(3.0f), -1 / sqrtf(3.0f)};
    XVector3 LightColor = {1, 1, 1};
    float LightIntensity = 7.0f;
#if USE_DX12
    void SceneBuild()
    {
        std::shared_ptr<GGeomertry> DefaultSphere = TempCreateSphereGeoWithMat();
        DefaultSphere->SetWorldTranslate(XVector3(0, -0.5, 0));
    
        std::shared_ptr<GGeomertry> DefaultCube = TempCreateCubeGeoWithMat();
        DefaultCube->SetWorldTranslate(XVector3(-1, 1.5, 0));
        DefaultCube->GetMaterialInstance()->SetMaterialValueFloat("ConstantMetatllic", 0.8);
        DefaultCube->GetMaterialInstance()->SetMaterialValueFloat("ConstantRoughness", 0.6);
    
        std::shared_ptr<GGeomertry> DefaultCubeRight = DefaultCube->CreateGeoInstancewithMat();
        DefaultCubeRight->SetWorldTranslate(XVector3(1, 1.5, 0));
    
        std::shared_ptr<GGeomertry> DefaultQuad = TempCreateQuadGeoWithMat();
        DefaultQuad->SetWorldTranslate(XVector3(0.0, 1.0, 0.0));
    
        std::shared_ptr<GGeomertry> LeftQuad = DefaultQuad->CreateGeoInstancewithMat();
        LeftQuad->SetWorldRotate(XVector3(0, 0, 1), -(3.14159 * 0.5));
        LeftQuad->SetWorldTranslate(XVector3(-2.0, 0.0, 0.0));
    
        std::shared_ptr<GGeomertry> FrontQuad = DefaultQuad->CreateGeoInstancewithMat();
        FrontQuad->SetWorldRotate(XVector3(1, 0, 0), -(3.14159 * 0.5));
        FrontQuad->SetWorldTranslate(XVector3(0.0, 0.0, 2.0));
    
        RenderGeos.push_back(DefaultCube);
        RenderGeos.push_back(DefaultCubeRight);
        RenderGeos.push_back(DefaultSphere);
        RenderGeos.push_back(DefaultQuad);
        RenderGeos.push_back(LeftQuad);
        RenderGeos.push_back(FrontQuad);
    
        //OBjLoaderTest(RenderGeos);
        for (auto& t : RenderGeos)
        {
            t->GetGVertexBuffer()->CreateRHIBufferChecked();
            t->GetGIndexBuffer()->CreateRHIBufferChecked();
        }
    }
#else

   void createTextureImage()
   {
       int texWidth, texHeight, texChannels;
       stbi_uc* pixels = stbi_load("G:/XEngineF/XEnigine/ContentSave/TextureNoAsset/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
       VkDeviceSize imageSize = texWidth * texHeight * 4;
   
       RHITestTexture2D = RHICreateTexture2D2(texWidth, texHeight, 1,
           false, false, EPixelFormat::FT_R8G8B8A8_UNORM_SRGB, ETextureCreateFlags((uint32)TexCreate_ShaderResource | (uint32)TexCreate_SRGB), 1, pixels, imageSize);

       RHITestTexture2DDepth = RHICreateTexture2D2(mVkHack.GetBkBufferExtent().width, mVkHack.GetBkBufferExtent().height, 1, false, false,
           EPixelFormat::FT_R24G8_TYPELESS, ETextureCreateFlags(TexCreate_DepthStencilTargetable | TexCreate_ShaderResource)
           , 1, nullptr, 0);

       stbi_image_free(pixels);
   }
   


   void PrePareRayTracingScene()
   {
       //1. create raytracing vb
       std::shared_ptr<XRHIBuffer> RayTracingVB;
       //2. create raytracing ib
       std::shared_ptr<XRHIBuffer> RayTracingIB;
       //3. create ray buffer
       std::shared_ptr<XRHIBuffer> RayTracingRB;
       std::shared_ptr<XRHIShaderResourceView> RBView;
       //4. create bottom level acceleration structure
       std::shared_ptr<XRHIRayTracingGeometry> Geometry;
       //5. create top level acceleration structure
       std::shared_ptr<XRHIBuffer> ScratchBuffer;
       std::shared_ptr<XRHIBuffer> SceneBuffer;
       std::shared_ptr<XRHIShaderResourceView> SceneBufferView;
       std::shared_ptr<XRHIBuffer> InstanceBuffer;
       XRayTracingSceneWithGeometryInstance RayTracingScene;
       //6. create result buffer
       std::shared_ptr<XRHIStructBuffer>OcclusionBuffer;
       std::shared_ptr<XRHIUnorderedAcessView>OcclusionUAV;

       constexpr int32 NumRays = 4;

       //1. create raytracing vb
       {
           TResourceVector<XVector3>Vertices;
           Vertices.PushBack(XVector3(1, -1, 0));
           Vertices.PushBack(XVector3(1, 1, 0));
           Vertices.PushBack(XVector3(-1, 1, 0));
           XRHIResourceCreateData CreateData(&Vertices);
           RayTracingVB = RHIcreateVertexBuffer2(sizeof(XVector3), 3 * sizeof(XVector3), EBufferUsage::BUF_Static, CreateData);
       }

       //2. create raytracing ib
       {
           TResourceVector<uint16>IndexData;
           IndexData.PushBack(0);
           IndexData.PushBack(1);
           IndexData.PushBack(2);
           XRHIResourceCreateData CreateData(&IndexData);
           RayTracingIB = RHICreateIndexBuffer2(sizeof(uint16), 3 * sizeof(uint16), EBufferUsage::BUF_Static, CreateData);
       }
      
       // (-1,-1,0) --------- (1,-1,0)
       //       \               |
       //        \              |
       //         \             |
       //          \            |
       //           \           |
       //              .........         
       //                    (1,1,0)

       //3. create ray buffer
       {
           TResourceVector<XBasicRayTracingRay>RayData;
           RayData.PushBack(XBasicRayTracingRay({ 0.75f, 0.0f,-1.0f }, 0xFFFFFFFF, { 0.0f,0.0f, 1.0f }, 10000.0f)); // expected to hit
           RayData.PushBack(XBasicRayTracingRay({ 0.75f, 0.0f,-1.0f }, 0xFFFFFFFF, { 0.0f,0.0f, 1.0f },     0.5f)); // expected to miss (short ray)
           RayData.PushBack(XBasicRayTracingRay({ 0.75f, 0.0f, 1.0f }, 0xFFFFFFFF, { 0.0f,0.0f,-1.0f }, 10000.0f)); // expected to hit  (should hit back face)
           RayData.PushBack(XBasicRayTracingRay({-0.75f, 0.0f,-1.0f }, 0xFFFFFFFF, { 0.0f,0.0f, 1.0f }, 10000.0f)); // expected to miss (doesn't intersect)
           XRHIResourceCreateData CreateData(&RayData);
           RayTracingRB = RHIcreateStructBuffer2(sizeof(XBasicRayTracingRay), sizeof(XBasicRayTracingRay) * 4, EBufferUsage::BUF_Static | EBufferUsage::BUF_ShaderResource, CreateData);
           RBView = RHICreateShaderResourceView2(RayTracingRB.get());
       }
       
       //4. create bottom level acceleration structure
       XRayTracingGeometryInitializer GeometryInitializer;
       {
           GeometryInitializer.IndexBuffer = RayTracingIB;
           GeometryInitializer.IndexBufferOffset = 0;
           GeometryInitializer.bPreferBuild = false;

           XRayTracingGeometrySegment Segment;
           Segment.VertexBuffer = RayTracingVB;
           Segment.VertexBufferOffset = 0;
           Segment.MaxVertices = 3;
           Segment.FirstPrimitive = 0;
           Segment.NumPrimitives = 1;
           Segment.VertexBufferStride = sizeof(XVector3);
           GeometryInitializer.Segments.push_back(Segment);

           Geometry = RHICreateRayTracingGeometry(GeometryInitializer);
           RHICmdList.BuildAccelerationStructure(Geometry);
       }
    
       //5. create top level acceleration structure
       {
           //5.1 create instance data
           std::vector<XRayTracingGeometryInstance> Instances;
           {
               Instances.resize(1);
               memset(Instances[0].Trasnforms, 0, sizeof(float) * 3 * 4);

               Instances[0].GeometryRHI = Geometry;
               Instances[0].NumTransforms = 1;
               Instances[0].Trasnforms[0][0] = 1;
               Instances[0].Trasnforms[1][1] = 1;
               Instances[0].Trasnforms[2][2] = 1;
           }

           //5.2 create ray tracing scene
           RayTracingScene = CreateRayTracingSceneWithGeometryInstance(Instances, RAY_TRACING_NUM_SHADER_SLOTS, 1);
           
           //5.3 create instance buffer
           { 
               // must match D3D12_RAYTRACING_INSTANCE_DESC / VkAccelerationStructureInstanceKHR
               struct FRayTracingInstanceDescriptor
               {
                   float Transform[3][4];
                   uint32 InstanceIdAndMask; // 24 + 8 
                   uint32 InstanceContributionToHitGroupIndexAndFlags; // 24 + 8
                   uint64 AccelerationStructure;
               };

               TResourceVector<FRayTracingInstanceDescriptor> InstanceBufferData;
               FRayTracingInstanceDescriptor InstanceDescriptor;
               memcpy(InstanceDescriptor.Transform, Instances[0].Trasnforms, sizeof(float) * 3 * 4);

               // Each geometry copy can receive a user-provided integer, which can be used to retrieve extra shader parameters or customize appearance.
               InstanceDescriptor.InstanceIdAndMask = (Instances[0].RayMask << 24) | (0 & (0x00FFFFFF));//little endian (nv *) / big endian ? 
               uint32 HitGroupIndex = RAY_TRACING_NUM_SHADER_SLOTS;
               //VkGeometryInstanceFlagBitsKHR;
               //InstanceDescriptor.InstanceContributionToHitGroupIndexAndFlags = ((uint32)ERayTracingInstanceFlags::None << 24) | (RayTracingScene.Scene->SceneInitializer.SegmentPrefixSum[0] & (0x00FFFFFF));VkGeometryInstanceFlagBitsKHR
               //Counterclockwise is the default for UE. Reversing culling is achieved by *not* setting this flag.
               InstanceDescriptor.InstanceContributionToHitGroupIndexAndFlags = ((uint32)ERayTracingInstanceFlags::TriangleCullReverse << 24) | (RayTracingScene.Scene->GetRayTracingSceneInitializer().SegmentPrefixSum[0] & (0x00FFFFFF));
               InstanceDescriptor.AccelerationStructure = Geometry->GetAccelerationStructureAddress();
               InstanceBufferData.PushBack(std::move(InstanceDescriptor));

               XRHIResourceCreateData ResourceCreateData(&InstanceBufferData);
               InstanceBuffer = RHIcreateStructBuffer2(sizeof(FRayTracingInstanceDescriptor), 1, EBufferUsage::BUF_ShaderResource, ResourceCreateData);
           }

           //5.4 create ray tracing scene buffer
           XRayTracingAccelerationStructSize SceneSizeInfo = RHICalcRayTracingSceneSize(1, ERayTracingAccelerationStructureFlags::PreferTrace);
           SceneBuffer = RHICreateBuffer(0, SceneSizeInfo.ResultSize, EBufferUsage::BUF_AccelerationStructure, XRHIResourceCreateData());
           ScratchBuffer = RHICreateBuffer(0, SceneSizeInfo.BuildScratchSize, EBufferUsage::BUF_UnorderedAccess, XRHIResourceCreateData());

           //5.5 bind ray tracing scene with scene packed buffer and create ray tracing view
           RHICmdList.BindAccelerationStructureMemory(RayTracingScene.Scene.get(), SceneBuffer, 0);

           //5.6 build acceleration structure
           XRayTracingSceneBuildParams Params;
           Params.Scene = RayTracingScene.Scene.get();
           Params.ScratchBuffer = ScratchBuffer.get();
           Params.ScratchBufferOffset = 0;
           Params.InstanceBuffer = InstanceBuffer.get();
           Params.InstanceBufferOffset = 0;

           RHICmdList.BuildAccelerationStructure(Params);
       }
       
       //6. create result buffer
       {
           RHICmdList.Transition(XRHITransitionInfo(RayTracingScene.Scene.get(), XRHITransitionInfo::EType::BVH, ERHIAccess::BVHWrite, ERHIAccess::BVHRead));
           //SceneBufferView = RHICreateShaderResourceView();
           OcclusionBuffer = RHIcreateStructBuffer(sizeof(uint32), sizeof(uint32) * NumRays, EBufferUsage::BUF_Static | EBufferUsage::BUF_UnorderedAccess, XRHIResourceCreateData());
           OcclusionUAV = RHICreateUnorderedAccessView(OcclusionBuffer.get(), false, false, 0);
       }

       DispatchBasicOcclusionRays(RHICmdList, RayTracingScene.Scene.get(), SceneBufferView.get(), RBView.get(), OcclusionUAV.get(), 4);
   }

   
    void drawFrame() {
        RHICmdList.Open();

        {
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            UniformBufferObject ubo{};
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj = glm::perspective(glm::radians(45.0f), float(XApplication::Application->ClientWidth) / float(XApplication::Application->ClientHeight), 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;

            VkTestCBResourece.RHICbVkTest->UpdateData(&ubo, sizeof(UniformBufferObject), 0);
        }

        XRHITexture* BackTex = RHIGetCurrentBackTexture();

        XRHIRenderPassInfo RPInfos(1, &BackTex, ERenderTargetLoadAction::EClear, RHITestTexture2DDepth.get(), EDepthStencilLoadAction::EClear);
        RHICmdList.RHIBeginRenderPass(RPInfos, "VulkanTestRP", sizeof("VulkanTestRP"));
        RHICmdList.CacheActiveRenderTargets(RPInfos);

        XGraphicsPSOInitializer GraphicsPSOInit;
        GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
        GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, ECompareFunction::CF_Less>::GetRHI();
        GraphicsPSOInit.RasterState = TStaticRasterizationState<>::GetRHI();

        TShaderReference<XHLSL2SPIRVS> VertexShader = GetGlobalShaderMapping()->GetShader<XHLSL2SPIRVS>();
        TShaderReference<XHLSL2SPIRPS> PixelShader = GetGlobalShaderMapping()->GetShader<XHLSL2SPIRPS>();

        GraphicsPSOInit.BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
        GraphicsPSOInit.BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();
        GraphicsPSOInit.BoundShaderState.RHIVertexLayout = GTestVtxLayout.RHIVertexLayout.get();

        RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
        SetGraphicsPipelineStateFromPSOInit(RHICmdList, GraphicsPSOInit);

        RHICmdList.SetVertexBuffer(GTestVertexRHI.RHIVertexBuffer.get(), 0, 0);

        //TODO:SetSampler
        VertexShader->SetParameter(RHICmdList, VkTestCBResourece.RHICbVkTest.get());
        PixelShader->SetParameter(RHICmdList, RHITestTexture2D.get());

        RHICmdList.RHIDrawIndexedPrimitive(GTestIndexRHI.RHIVertexBuffer.get(), 36, 1, 0, 0, 0);
        RHICmdList.RHIEndRenderPass();
        
        //TODO:FixMe
        //RHICmdList.Execute();
        RHICmdList.Close();

        RHICmdList.RHIEndFrame();
    
    }
#endif
    void Init()
    {
        MainInit::Init();

#if USE_DX12
        XApplication::Application = new XWindowsApplication();
#else
        XApplication::Application = new XGlfwApplication();
#endif
        XApplication::Application->CreateAppWindow();
#if USE_DX12
        XApplication::Application->SetRenderer(&DeferredShadingRenderer);
#endif
        XApplication::Application->AppInput.SetCamera(&CamIns);
        XApplication::Application->AppInput.SetTimer(&mTimer);

        RHIInit(XApplication::Application->ClientWidth, XApplication::Application->ClientHeight, USE_DX12);

        RHICmdList = GRHICmdList;
        RHICmdList.Open();
#if USE_DX12
        SceneBuild();
#endif
        MainInit::InitAfterRHI();
#if USE_DX12
        XApplication::Application->UISetup();
        SceneBoundingSphere.Center = XVector3(0, 0, 0);
        SceneBoundingSphere.Radius = 48.0f;
        float AspectRatio = static_cast<float>(XApplication::Application->ClientWidth) / static_cast<float>(XApplication::Application->
            ClientHeight);
        CamIns.SetPerspective(FoVAngleY, AspectRatio, Near, Far);
        DeferredShadingRenderer.ViewInfoSetup(XApplication::Application->ClientWidth, XApplication::Application->ClientHeight, CamIns);
        DeferredShadingRenderer.Setup(RenderGeos,
            XBoundSphere{ SceneBoundingSphere.Center, SceneBoundingSphere.Radius },
            LightDir, LightColor, LightIntensity, RHICmdList);
        RHICmdList.Execute();
#else
        createTextureImage();
        
        RHICmdList.Execute();
#endif
    }

    void Destroy()
    {
        MainInit::Destroy();
        RHIRelease();
    }
};

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(10686);
    {
        XSandBox SandBox;
        SandBox.Init();
#if !USE_DX12
        while (!glfwWindowShouldClose((GLFWwindow*)XApplication::Application->GetPlatformHandle())) {
            glfwPollEvents();
            SandBox.drawFrame();
        }
#else
        XApplication::Application->ApplicationLoop();
#endif
        SandBox.Destroy();
    }
    {
        delete XWindowsApplication::Application;
        return 0;
    }
    _CrtDumpMemoryLeaks();
}

