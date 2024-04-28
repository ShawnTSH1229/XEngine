#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Runtime/ApplicationCore/Application.h"

class XGlfwApplication :public XApplication
{
private:
	GLFWwindow* window;
	XDeferredShadingRenderer* DeferredShadingRenderer;
	const wchar_t* WindowsClassName = L"MainWnd";
	const wchar_t* WindowName = L"XEngine";
public:
	XGlfwApplication();
	~XGlfwApplication();
	void* GetPlatformHandle()override;
	bool CreateAppWindow()override;
	void ApplicationLoop()override;
	bool UISetup()override;
	bool UINewFrame()override;
	void SetRenderer(XDeferredShadingRenderer* InDeferredShadingRenderer)override;
};