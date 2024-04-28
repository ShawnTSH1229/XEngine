#include "DeferredShadingRenderer.h"
#include "imgui.h"
void XDeferredShadingRenderer::TempUIRenderer(XRHICommandList& RHICmdList , XRHITexture* DestTex)
{
	EditorUI.ImGui_Impl_RHI_NewFrame(&RHICmdList);

	ImGui::NewFrame();

	EditorUI.OnTick();
	ImGui::Render();
	EditorUI.ImGui_Impl_RHI_RenderDrawData(ImGui::GetDrawData(), &RHICmdList, DestTex);
}
