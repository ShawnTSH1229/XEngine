#pragma once
#include "Runtime/HAL/PlatformTypes.h"
#include "Runtime/ApplicationCore/ApplicationInput.h"
#include <Runtime\Render\DeferredShadingRenderer.h>

class XApplication
{
public:
	static XApplication* Application;

	uint32 ClientWidth;
	uint32 ClientHeight;
	XAppInput AppInput;
	
	XApplication();
	virtual ~XApplication();
	virtual void* GetPlatformHandle();

	virtual bool CreateAppWindow();
	virtual void ApplicationLoop() = 0;;

	virtual bool UISetup() = 0;
	virtual bool UINewFrame() = 0;

	virtual void SetRenderer(XDeferredShadingRenderer* InDeferredShadingRenderer) = 0;
};