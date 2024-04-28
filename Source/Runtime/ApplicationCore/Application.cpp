#include "Application.h"
//XAppInput* XAppInput::InputPtr = nullptr;

XApplication::XApplication()
{
	ClientWidth = 1540;
	ClientHeight = 845;
}

XApplication::~XApplication()
{
}


bool XApplication::CreateAppWindow()
{
	return false;
}

void* XApplication::GetPlatformHandle()
{
	return nullptr;
}


