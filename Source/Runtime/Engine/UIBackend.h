#pragma once
#include "Runtime/RHI/RHICommandList.h"

class ImDrawData;

class XUIRender
{
public:
	inline void InitIOInfo(uint32 WidthIn, uint32 HeightIn)
	{
		WindowWidth = WidthIn;
		WindowHeight = HeightIn;
	}
		
	void ImGui_Impl_RHI_Init();
	void ImGui_Impl_RHI_Shutdown();

	void ImGui_Impl_RHI_NewFrame(XRHICommandList* RHICmdList);
	void ImGui_Impl_RHI_RenderDrawData(ImDrawData* draw_data, XRHICommandList* RHICmdList,XRHITexture* SceneColorTex);

	//
	void SetDefaltStyle();
protected:
	uint32 WindowWidth;
	uint32 WindowHeight;
};