#pragma once
#include "Runtime/Engine/UIBackend.h"

class XEditorUI :public XUIRender
{
private:
	void ShowEditorUI();

	void ShowEditorWorldObjectsWindow(bool* bOpen);
	void ShowEditorGameWindow(bool* bOpen);
	void ShowEditorMenu(bool* bOpen);
	void ShowEditorFileContentWindow(bool* bOpen);
	void ShowEditorDetialWindow(bool* bOpen);
public:
	void OnTick();
};