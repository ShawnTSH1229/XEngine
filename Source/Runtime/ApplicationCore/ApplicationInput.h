#pragma once
#include "Runtime/HAL/PlatformTypes.h"

class GCamera;
class GameTimer;

enum class EInputType
{
	KEYBOARD,
	MOUSE,
	MAX
};

enum class EInputEvent
{
	DOWN,
	UP,
	MOUSE_MOVE,
	WHEEL_MOVE,
	MAX
};

enum class EInputKey
{
	MOUSE_RIGHT = 0,
	MOUSE_LEFT,
	MOUSE_MIDDLE,
	MOUSE_MAX,

	ESCAPE,
	BK_W,
	BK_A,
	BK_S,
	BK_D,
	KEY_MAX
};

struct XMousePos
{
	int32 X;
	int32 Y;
};

class XAppInput
{
	friend class XApplication;
public:
	XAppInput();
	XMousePos& GetMouseDelta() { return MouseDelta; }
	bool GetMousePressed(EInputKey InputKey) { return bMousePressed[int32(InputKey)]; }
	bool GetKeyPressed(EInputKey InputKey) { return bBoardKeyPressed[int32(InputKey)]; }
	
	void SetCamera(GCamera* InCamera){Camera = InCamera;}
	void SetTimer(GameTimer* InTimera) { Timer = InTimera; }
	GameTimer* GetTimer() { return Timer; }

	void InputMsgProcesss(EInputType InputType, EInputEvent InputEvent, EInputKey InputKey, int32 PosX, int32 PosY, int32 WHEEL);
	
private:
	inline void SetMousePos(int32 X, int32 Y) { MouseX = X; MouseY = Y; }
	void OnRMouseDown(int32 X, int32 Y);
	void OnLMouseDown(int32 X, int32 Y);
	void OnRMouseUp(int32 X, int32 Y);
	void OnMouseMove(int32 X, int32 Y);
	void OnKeyDown(EInputKey InputKey);
	void OnKeyUp(EInputKey InputKey);

	bool bMousePressed[int32(EInputKey::MOUSE_MAX)];
	bool bBoardKeyPressed[int32(EInputKey::KEY_MAX)];
	XMousePos MouseDelta;
	int32 MouseX;
	int32 MouseY;
public:
	GCamera* Camera;
	GameTimer* Timer;
};