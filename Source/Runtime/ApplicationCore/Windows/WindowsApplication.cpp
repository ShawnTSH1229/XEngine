#ifdef X_PLATFORM_WIN
#include <string>
#include <backends/imgui_impl_win32.h>
#include "WindowsApplication.h"
#include "Runtime/RHI/RHI.h"
#include "Runtime/Core/ComponentNode/GameTimer.h"
#include "Runtime\RHI\RHICommandList.h"



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND XWindowsApplication::WinHandle = nullptr;

static void CalcFPS(HWND mhMainWnd , GameTimer* InGameTimer)
{
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((InGameTimer->TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1

		std::wstring fpsStr = std::to_wstring(fps);

		std::wstring windowText = L"fps: " + fpsStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

XWindowsApplication::XWindowsApplication()
{
}

XWindowsApplication::~XWindowsApplication()
{
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool XWindowsApplication::CreateAppWindow()
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowsMsgProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = nullptr;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = WindowsClassName;

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	RECT R = { 0, 0, ClientWidth, ClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int Width  = R.right - R.left;
	int Height = R.bottom - R.top;
	WinHandle = CreateWindow(WindowsClassName, WindowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, 0, 0, nullptr, 0);
	
	if (!WinHandle)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(WinHandle, SW_SHOW);
	UpdateWindow(WinHandle);

	return true;
}

void XWindowsApplication::ApplicationLoop()
{
	MSG msg = { 0 };

	AppInput.Timer->Reset();
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DeferredShadingRenderer->ViewInfoUpdate(*AppInput.Camera);
			Application->UINewFrame();
			DeferredShadingRenderer->Rendering(GRHICmdList);
		}
		AppInput.Timer->Tick();
	}
}
void* XWindowsApplication::GetPlatformHandle()
{
	return WinHandle;
}
bool XWindowsApplication::UISetup()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingAlwaysTabBar = true;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(1.0, 0);
	style.FramePadding = ImVec2(14.0, 2.0f);
	style.ChildBorderSize = 0.0f;
	style.FrameRounding = 5.0f;
	style.FrameBorderSize = 1.5f;
	ImGui_ImplWin32_Init(WinHandle);
	return true;
}
bool XWindowsApplication::UINewFrame()
{
	ImGui_ImplWin32_NewFrame();
	CalcFPS(WinHandle, AppInput.GetTimer());
	return true;
}

void XWindowsApplication::SetRenderer(XDeferredShadingRenderer* InDeferredShadingRenderer)
{
	DeferredShadingRenderer = InDeferredShadingRenderer;
}

static int32 WindowKeyMap[] =
{
	(int32)EInputKey::KEY_MAX,		//					0x00
	(int32)EInputKey::KEY_MAX,		//VK_LBUTTON        0x01
	(int32)EInputKey::KEY_MAX,		//VK_RBUTTON        0x02
	(int32)EInputKey::KEY_MAX,		//VK_CANCEL         0x03
	(int32)EInputKey::KEY_MAX,		//VK_MBUTTON        0x04
	(int32)EInputKey::KEY_MAX,		//VK_XBUTTON1       0x05
	(int32)EInputKey::KEY_MAX,		//VK_XBUTTON2       0x06
	(int32)EInputKey::KEY_MAX,		//				    0x07
	(int32)EInputKey::KEY_MAX,		//VK_BACK           0x08
	(int32)EInputKey::KEY_MAX,		//VK_TAB            0x09
	(int32)EInputKey::KEY_MAX,		//					0x0A
	(int32)EInputKey::KEY_MAX,		//					0x0B
	(int32)EInputKey::KEY_MAX,		//VK_CLEAR          0x0C
	(int32)EInputKey::KEY_MAX,	//VK_RETURN         0x0D
	(int32)EInputKey::KEY_MAX,		//VK_CLEAR          0x0E
	(int32)EInputKey::KEY_MAX,		//VK_CLEAR          0x0F
	(int32)EInputKey::KEY_MAX,		//VK_SHIFT          0x10
	(int32)EInputKey::KEY_MAX,		//VK_CONTROL        0x11
	(int32)EInputKey::KEY_MAX,		//VK_MENU           0x12
	(int32)EInputKey::KEY_MAX,	//VK_PAUSE          0x13
	(int32)EInputKey::KEY_MAX,	//VK_CAPITAL        0x14
	(int32)EInputKey::KEY_MAX,		//					0x15
	(int32)EInputKey::KEY_MAX,		//					0x16
	(int32)EInputKey::KEY_MAX,		//					0x17
	(int32)EInputKey::KEY_MAX,		//					0x18
	(int32)EInputKey::KEY_MAX,		//					0x19
	(int32)EInputKey::KEY_MAX,		//					0x1A
	(int32)EInputKey::KEY_MAX,	//VK_ESCAPE         0x1B
	(int32)EInputKey::KEY_MAX,		//					0x1C
	(int32)EInputKey::KEY_MAX,		//					0x1D
	(int32)EInputKey::KEY_MAX,		//					0x1E
	(int32)EInputKey::KEY_MAX,		//					0x1F
	(int32)EInputKey::KEY_MAX,	//VK_SPACE          0x20
	(int32)EInputKey::KEY_MAX,		//VK_PRIOR          0x21
	(int32)EInputKey::KEY_MAX,		//VK_NEXT           0x22
	(int32)EInputKey::KEY_MAX,		//VK_END			0x23
	(int32)EInputKey::KEY_MAX,		//VK_HOME           0x24
	(int32)EInputKey::KEY_MAX,		//VK_LEFT           0x25
	(int32)EInputKey::KEY_MAX,		//VK_UP             0x26
	(int32)EInputKey::KEY_MAX,	//VK_RIGHT          0x27
	(int32)EInputKey::KEY_MAX,		//VK_DOWN           0x28
	(int32)EInputKey::KEY_MAX,		//VK_SELECT         0x29
	(int32)EInputKey::KEY_MAX,		//					0x2A
	(int32)EInputKey::KEY_MAX,		//					0x2B
	(int32)EInputKey::KEY_MAX,		//					0x2C
	(int32)EInputKey::KEY_MAX,	//VK_INSERT         0x2D
	(int32)EInputKey::KEY_MAX,	//VK_DELETE         0x2E
	(int32)EInputKey::KEY_MAX,		//					0x2F
	(int32)EInputKey::KEY_MAX,		//VK_0				0x30
	(int32)EInputKey::KEY_MAX,		//VK_1				0x31
	(int32)EInputKey::KEY_MAX,		//VK_2				0x32
	(int32)EInputKey::KEY_MAX,		//VK_3				0x33
	(int32)EInputKey::KEY_MAX,		//VK_4				0x34
	(int32)EInputKey::KEY_MAX,		//VK_5				0x35
	(int32)EInputKey::KEY_MAX,		//VK_6				0x36
	(int32)EInputKey::KEY_MAX,		//VK_7				0x37
	(int32)EInputKey::KEY_MAX,		//VK_8				0x38
	(int32)EInputKey::KEY_MAX,		//VK_9				0x39
	(int32)EInputKey::KEY_MAX,		//					0x4A
	(int32)EInputKey::KEY_MAX,		//					0x4B
	(int32)EInputKey::KEY_MAX,		//					0x4C
	(int32)EInputKey::KEY_MAX,		//					0x4D
	(int32)EInputKey::KEY_MAX,		//					0x4E
	(int32)EInputKey::KEY_MAX,		//					0x4F
	(int32)EInputKey::KEY_MAX,		//					0x40
	(int32)EInputKey::BK_A,		//VK_A				0x41
	(int32)EInputKey::KEY_MAX,		//VK_B				0x42
	(int32)EInputKey::KEY_MAX,		//VK_C				0x43
	(int32)EInputKey::BK_D,		//VK_D				0x44
	(int32)EInputKey::KEY_MAX,		//VK_E				0x45
	(int32)EInputKey::KEY_MAX,		//VK_F				0x46
	(int32)EInputKey::KEY_MAX,		//VK_G				0x47
	(int32)EInputKey::KEY_MAX,		//VK_H				0x48
	(int32)EInputKey::KEY_MAX,		//VK_I				0x49
	(int32)EInputKey::KEY_MAX,		//VK_J				0x4A
	(int32)EInputKey::KEY_MAX,		//VK_K				0x4B
	(int32)EInputKey::KEY_MAX,		//VK_L				0x4C
	(int32)EInputKey::KEY_MAX,		//VK_M				0x4D
	(int32)EInputKey::KEY_MAX,		//VK_N				0x4E
	(int32)EInputKey::KEY_MAX,		//VK_O				0x4F
	(int32)EInputKey::KEY_MAX,		//VK_P				0x50
	(int32)EInputKey::KEY_MAX,		//VK_Q				0x51
	(int32)EInputKey::KEY_MAX,		//VK_R				0x52
	(int32)EInputKey::BK_S,		//VK_S				0x53
	(int32)EInputKey::KEY_MAX,		//VK_T				0x54
	(int32)EInputKey::KEY_MAX,		//VK_U				0x55
	(int32)EInputKey::KEY_MAX,		//VK_V				0x56
	(int32)EInputKey::BK_W,		//VK_W				0x57
	(int32)EInputKey::KEY_MAX,		//VK_X				0x58
	(int32)EInputKey::KEY_MAX,		//VK_Y				0x59
	(int32)EInputKey::KEY_MAX,		//VK_Z				0x5A
};
LRESULT WINAPI XWindowsApplication::WindowsMsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_ACTIVATE:
		return 0; 
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);
		
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
		Application->AppInput.InputMsgProcesss(EInputType::MOUSE, EInputEvent::DOWN, EInputKey::MOUSE_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		SetCapture(WinHandle);
		return 0;

	case WM_LBUTTONUP:
		Application->AppInput.InputMsgProcesss(EInputType::MOUSE, EInputEvent::UP, EInputKey::MOUSE_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		ReleaseCapture();
		return 0;

	case WM_RBUTTONDOWN:
		Application->AppInput.InputMsgProcesss(EInputType::MOUSE, EInputEvent::DOWN, EInputKey::MOUSE_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		SetCapture(WinHandle);
		return 0;

	case WM_RBUTTONUP:
		Application->AppInput.InputMsgProcesss(EInputType::MOUSE, EInputEvent::UP, EInputKey::MOUSE_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		Application->AppInput.InputMsgProcesss(EInputType::MOUSE, EInputEvent::MOUSE_MOVE, EInputKey::MOUSE_MAX, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
		return 0;

	case WM_KEYDOWN:
		Application->AppInput.InputMsgProcesss(EInputType::KEYBOARD, EInputEvent::DOWN, (EInputKey)WindowKeyMap[wParam], 0, 0, 0);
		return 0;
	case WM_KEYUP:
		Application->AppInput.InputMsgProcesss(EInputType::KEYBOARD, EInputEvent::UP, (EInputKey)WindowKeyMap[wParam], 0, 0, 0);

		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif
