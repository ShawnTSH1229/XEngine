#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

#include "Runtime/Core/MainInit.h"
#include "Runtime/Engine/ResourcecConverter.h"
#include "Runtime/Render/DeferredShadingRenderer.h"
#include "Runtime/Core/ComponentNode/GameTimer.h"

#include "Runtime/ApplicationCore/Windows/WindowsApplication.h"
#include "Runtime/ApplicationCore/GlfwApp/GlfwApplication.h"

#include "Test/TestSceneCreate.h"

XApplication* XApplication::Application = nullptr;

class XSandBox
{
public:

    XRHICommandList RHICmdList;
    XBoundSphere SceneBoundingSphere;
    XDeferredShadingRenderer DeferredShadingRenderer;
    std::vector<std::shared_ptr<GGeomertry>> RenderGeos;

    //Timer
    GameTimer mTimer;

    //Camera
    GCamera CamIns;
    float Far = 1000.0f;
    float Near = 1.0f;
    float FoVAngleY = 0.25f * X_PI;

    //Light
    XVector3 LightDir = { 1 / sqrtf(3.0f), 1 / sqrtf(3.0f), -1 / sqrtf(3.0f) };
    XVector3 LightColor = { 1, 1, 1 };
    float LightIntensity = 7.0f;
    void SceneBuild()
    {
        CreateVSMTestScene(RenderGeos, SceneBoundingSphere);
    }

    void Init()
    {
        MainInit::Init();

        XApplication::Application = new XWindowsApplication();

        XApplication::Application->CreateAppWindow();
        XApplication::Application->SetRenderer(&DeferredShadingRenderer);
        XApplication::Application->AppInput.SetCamera(&CamIns);
        XApplication::Application->AppInput.SetTimer(&mTimer);

        RHIInit(XApplication::Application->ClientWidth, XApplication::Application->ClientHeight, USE_DX12);

        RHICmdList = GRHICmdList;
        RHICmdList.Open();
        SceneBuild();
        MainInit::InitAfterRHI();
        XApplication::Application->UISetup();
        float AspectRatio = static_cast<float>(XApplication::Application->ClientWidth) / static_cast<float>(XApplication::Application->
            ClientHeight);
        CamIns.SetPerspective(FoVAngleY, AspectRatio, Near, Far);
        DeferredShadingRenderer.ViewInfoSetup(XApplication::Application->ClientWidth, XApplication::Application->ClientHeight, CamIns);
        DeferredShadingRenderer.Setup(RenderGeos, XBoundSphere{ SceneBoundingSphere.Center, SceneBoundingSphere.Radius }, LightDir, LightColor, LightIntensity, RHICmdList);
        RHICmdList.Execute();
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
        XApplication::Application->ApplicationLoop();
        SandBox.Destroy();
    }
    {
        delete XWindowsApplication::Application;
        return 0;
    }
    _CrtDumpMemoryLeaks();
}

