#include "GlfwApplication.h"


XGlfwApplication::XGlfwApplication()
{
}

XGlfwApplication::~XGlfwApplication()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void* XGlfwApplication::GetPlatformHandle()
{
    return window;
}

bool XGlfwApplication::CreateAppWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(ClientWidth, ClientHeight, "Vulkan", nullptr, nullptr);
    return true;
}

void XGlfwApplication::ApplicationLoop()
{
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}


bool XGlfwApplication::UISetup()
{
    return true;
}

bool XGlfwApplication::UINewFrame()
{
    return false;
}

void XGlfwApplication::SetRenderer(XDeferredShadingRenderer* InDeferredShadingRenderer)
{
    DeferredShadingRenderer = InDeferredShadingRenderer;
}
