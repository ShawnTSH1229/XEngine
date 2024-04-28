#include "UIBackend.h"
#include <imgui.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#ifdef _MSC_VER
#pragma comment(lib, "d3dcompiler") 
#endif

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/wstringize.hpp>

#include "Runtime/Core/Math/Math.h"
#include "Runtime/HAL/Mch.h"
#include "Runtime/RenderCore/GlobalShader.h"
#include "Runtime/RHI/RHIResource.h"
#include "Runtime/RHI/RHIStaticStates.h"
#include "Runtime/RHI/PipelineStateCache.h"
#include "Runtime/RenderCore/ShaderParameter.h"

class RUIBackendVertexLayout : public XRenderResource
{
public:
	std::shared_ptr<XRHIVertexLayout> RHIVertexLayout;
	virtual void InitRHI()override
	{
		XRHIVertexLayoutArray LayoutArray;
		LayoutArray.push_back(XVertexElement(0, EVertexElementType::VET_Float2, 0, 0));
		LayoutArray.push_back(XVertexElement(1, EVertexElementType::VET_Float2, 0, 0 + sizeof(XVector2)));
		LayoutArray.push_back(XVertexElement(2, EVertexElementType::VET_Color, 0, 0 + sizeof(XVector2) + sizeof(XVector2)));
		RHIVertexLayout = RHICreateVertexLayout(LayoutArray);
	}

	virtual void ReleaseRHI()override
	{
		RHIVertexLayout.reset();
	}
};

TGlobalResource<RUIBackendVertexLayout>UIBackendVertexLayout;

struct VERTEX_CONSTANT_BUFFER
{
	float   mvp[4][4];
};

class XUIBackendVS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XUIBackendVS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XUIBackendVS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{
		ProjectionMatrix.Bind(Initializer.ShaderParameterMap, "ProjectionMatrix");
	}

	void SetParameter(
		XRHICommandList& RHICommandList, 
		VERTEX_CONSTANT_BUFFER& MatIn)
	{
		SetShaderValue(RHICommandList, EShaderType::SV_Vertex, ProjectionMatrix, MatIn);
	}

	XShaderVariableParameter ProjectionMatrix;
};

class XUIBackendPS :public XGloablShader
{
public:
	static XXShader* CustomConstrucFunc(const XShaderInitlizer& Initializer)
	{
		return new XUIBackendPS(Initializer);
	}
	static ShaderInfos StaticShaderInfos;
	static void ModifyShaderCompileSettings(XShaderCompileSetting& OutSettings) {}

public:
	XUIBackendPS(const XShaderInitlizer& Initializer)
		:XGloablShader(Initializer)
	{
		UITexture.Bind(Initializer.ShaderParameterMap, "texture0");
	}

	void SetParameter(
		XRHICommandList& RHICommandList,
		XRHITexture* InUITexture)
	{
		SetTextureParameter(RHICommandList, EShaderType::SV_Pixel, UITexture, InUITexture);
	}
	TextureParameterType UITexture;
};


;
XUIBackendVS::ShaderInfos XUIBackendVS::StaticShaderInfos(
	"XUIBackendVS", BOOST_PP_CAT(L, BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/Source/Shaders/UIBackend.hlsl")),
	"UI_VS", EShaderType::SV_Vertex, XUIBackendVS::CustomConstrucFunc,
	XUIBackendVS::ModifyShaderCompileSettings);

XUIBackendPS::ShaderInfos XUIBackendPS::StaticShaderInfos(
	"XUIBackendPS", BOOST_PP_CAT(L, BOOST_PP_CAT(BOOST_PP_STRINGIZE(ROOT_DIR_XENGINE), "/Source/Shaders/UIBackend.hlsl")),
	"UI_PS", EShaderType::SV_Pixel, XUIBackendPS::CustomConstrucFunc,
	XUIBackendPS::ModifyShaderCompileSettings);


struct RHIUI_VBIB
{
	std::shared_ptr<XRHIBuffer>VertexBuffer;
	std::shared_ptr<XRHIBuffer>IndexBuffer;

	int VertexBufferSize;
	int IndexBufferSize;
};



struct RHIUI_RenderBuffer
{
	RHIUI_VBIB VBIB;
};

struct ImGui_Impl_RHI_Data
{
	RHIUI_RenderBuffer* RenderBuffer;
	XGraphicsPSOInitializer* PSOInit;
	std::shared_ptr<XRHITexture2D>FontsTexture;

	UINT frameIndex;
	ImGui_Impl_RHI_Data() 
	{ 
		memset((void*)this, 0, sizeof(*this)); 
		frameIndex = UINT_MAX;
	}
};

static ImGui_Impl_RHI_Data* ImGui_Impl_RHI_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_Impl_RHI_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}


void XUIRender::ImGui_Impl_RHI_Init()
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui_Impl_RHI_Data* BackendData = new ImGui_Impl_RHI_Data();
	io.BackendRendererUserData = (void*)BackendData;
	io.BackendRendererName = "ImGui_Impl_RHI";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	BackendData->RenderBuffer = new RHIUI_RenderBuffer();
	BackendData->RenderBuffer->VBIB.VertexBufferSize = 10000;
	BackendData->RenderBuffer->VBIB.IndexBufferSize = 5000;


}

void XUIRender::ImGui_Impl_RHI_Shutdown()
{
	ImGui_Impl_RHI_Data* BackendData = ImGui_Impl_RHI_GetBackendData();
	delete BackendData->RenderBuffer;
	delete BackendData->PSOInit;
	delete BackendData;
}


void XUIRender::ImGui_Impl_RHI_NewFrame(XRHICommandList* RHICmdList)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_Impl_RHI_Data* BackendData = ImGui_Impl_RHI_GetBackendData();
	if (BackendData->PSOInit == nullptr)
	{
		BackendData->PSOInit = new XGraphicsPSOInitializer();
		BackendData->PSOInit->BlendState = TStaticBlendState<
			true,
			EBlendOperation::BO_Add,
			EBlendFactor::BF_SourceAlpha,
			EBlendFactor::BF_InverseSourceAlpha,
			EBlendOperation::BO_Add,
			EBlendFactor::BF_One,
			EBlendFactor::BF_InverseSourceAlpha
		>::GetRHI();
		BackendData->PSOInit->DepthStencilState = TStaticDepthStencilState<false, ECompareFunction::CF_Always>::GetRHI();
		BackendData->PSOInit->RasterState = TStaticRasterizationState<>::GetRHI();
		TShaderReference<XUIBackendVS> VertexShader = GetGlobalShaderMapping()->GetShader<XUIBackendVS>();
		TShaderReference<XUIBackendPS> PixelShader = GetGlobalShaderMapping()->GetShader<XUIBackendPS>();
		BackendData->PSOInit->BoundShaderState.RHIVertexShader = VertexShader.GetVertexShader();
		BackendData->PSOInit->BoundShaderState.RHIPixelShader = PixelShader.GetPixelShader();

		BackendData->PSOInit->BoundShaderState.RHIVertexLayout = UIBackendVertexLayout.RHIVertexLayout.get();


		
		unsigned char* pixels; int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		BackendData->FontsTexture = RHICreateTexture2D(width, height, 1, false, false, EPixelFormat::FT_R8G8B8A8_UNORM, ETextureCreateFlags(TexCreate_None), 1, pixels);
	}


}

static void ImGui_Impl_RHI_SetupRenderState(ImDrawData* draw_data, XRHICommandList* RHICmdList, XRHITexture* SceneColorTex)
{
	ImGui_Impl_RHI_Data* BackendData = ImGui_Impl_RHI_GetBackendData();
	VERTEX_CONSTANT_BUFFER vertex_constant_buffer;
	{
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&vertex_constant_buffer.mvp, mvp, sizeof(mvp));
	}


	XRHIRenderPassInfo RPInfos(1, &SceneColorTex, ERenderTargetLoadAction::ELoad, nullptr, EDepthStencilLoadAction::ENoAction);
	RHICmdList->RHIBeginRenderPass(RPInfos, "IMGUI_PASS",sizeof("IMGUI_PASS"));
	RHICmdList->CacheActiveRenderTargets(RPInfos);
	RHICmdList->ApplyCachedRenderTargets(*BackendData->PSOInit);
	SetGraphicsPipelineStateFromPSOInit(*RHICmdList, *BackendData->PSOInit);

	TShaderReference<XUIBackendVS> VertexShader = GetGlobalShaderMapping()->GetShader<XUIBackendVS>();
	TShaderReference<XUIBackendPS> PixelShader = GetGlobalShaderMapping()->GetShader<XUIBackendPS>();
	
	VertexShader->SetParameter(*RHICmdList, vertex_constant_buffer);
	PixelShader->SetParameter(*RHICmdList, BackendData->FontsTexture.get());
}

void XUIRender::ImGui_Impl_RHI_RenderDrawData(ImDrawData* draw_data, XRHICommandList* RHICmdList, XRHITexture* SceneColorTex)
{
	ImGui_Impl_RHI_Data* BackendData = ImGui_Impl_RHI_GetBackendData();

	RHIUI_VBIB* VBIB = &BackendData->RenderBuffer->VBIB;
	
	// Create and grow vertex/index buffers if needed
	if (VBIB->VertexBuffer.get() == nullptr || VBIB->VertexBufferSize < draw_data->TotalVtxCount)
	{
		XRHIResourceCreateData NullData;
		VBIB->VertexBufferSize = draw_data->TotalVtxCount + 5000;
		VBIB->VertexBuffer = RHIcreateVertexBuffer(sizeof(ImDrawVert), sizeof(ImDrawVert) * VBIB->VertexBufferSize, EBufferUsage::BUF_Dynamic, NullData);
	}

	if (VBIB->IndexBuffer.get() == nullptr || VBIB->IndexBufferSize < draw_data->TotalIdxCount)
	{
		XRHIResourceCreateData NullData;
		VBIB->IndexBufferSize = draw_data->TotalIdxCount + 10000;
		VBIB->IndexBuffer = RHICreateIndexBuffer(sizeof(ImDrawIdx), sizeof(ImDrawIdx)*VBIB->IndexBufferSize, EBufferUsage::BUF_Dynamic, NullData);
	}

	
	void* VertexResourceData = LockVertexBuffer(VBIB->VertexBuffer.get(), 0, VBIB->VertexBufferSize);
	void* IndexResourceData = LockIndexBuffer(VBIB->IndexBuffer.get(), 0, VBIB->IndexBufferSize);

	
	ImDrawVert* VtxDst = (ImDrawVert*)VertexResourceData;
	ImDrawIdx* IdxDst = (ImDrawIdx*)IndexResourceData;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* DrawList = draw_data->CmdLists[n];
		memcpy(VtxDst, DrawList->VtxBuffer.Data, DrawList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(IdxDst, DrawList->IdxBuffer.Data, DrawList->IdxBuffer.Size * sizeof(ImDrawIdx));

		VtxDst += DrawList->VtxBuffer.Size;
		IdxDst += DrawList->IdxBuffer.Size;
	}

	
	UnLockIndexBuffer(VBIB->IndexBuffer.get());
	UnLockVertexBuffer(VBIB->VertexBuffer.get());

	//ImGui_Impl_RHI_SetupRenderState(draw_data, RHICmdList, SceneColorTex);

	
	int GloablVtxOffset = 0;
	int GloablIdxOffset = 0;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != NULL)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
				{
					XASSERT(false);
					ImGui_Impl_RHI_SetupRenderState(draw_data, RHICmdList, SceneColorTex);
				}
				else
					pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				ImGui_Impl_RHI_SetupRenderState(draw_data, RHICmdList, SceneColorTex);
				RHICmdList->SetVertexBuffer(VBIB->VertexBuffer.get(), 0, 0);
				RHICmdList->RHIDrawIndexedPrimitive(VBIB->IndexBuffer.get(), pcmd->ElemCount, 1, 
					pcmd->IdxOffset + GloablIdxOffset, pcmd->VtxOffset + GloablVtxOffset, 0);
			}
		}
		GloablIdxOffset += cmd_list->IdxBuffer.Size;
		GloablVtxOffset += cmd_list->VtxBuffer.Size;
	}



}

void XUIRender::SetDefaltStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text] = ImVec4(0.4745f, 0.4745f, 0.4745f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.0078f, 0.0078f, 0.0078f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.047f, 0.047f, 0.047f, 0.5411f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.196f, 0.196f, 0.196f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.294f, 0.294f, 0.294f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.0039f, 0.0039f, 0.0039f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.0039f, 0.0039f, 0.0039f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(93.0f / 255.0f, 10.0f / 255.0f, 66.0f / 255.0f, 1.00f);
	colors[ImGuiCol_SliderGrab] = colors[ImGuiCol_CheckMark];
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.3647f, 0.0392f, 0.2588f, 0.50f);
	colors[ImGuiCol_Button] = ImVec4(0.0117f, 0.0117f, 0.0117f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.0235f, 0.0235f, 0.0235f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.0353f, 0.0196f, 0.0235f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.1137f, 0.0235f, 0.0745f, 0.588f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(5.0f / 255.0f, 5.0f / 255.0f, 5.0f / 255.0f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.0f, 0.0f, 0.0f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(6.0f / 255.0f, 6.0f / 255.0f, 8.0f / 255.0f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 150.0f / 255.0f);
	colors[ImGuiCol_TabActive] = ImVec4(47.0f / 255.0f, 6.0f / 255.0f, 29.0f / 255.0f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(45.0f / 255.0f, 7.0f / 255.0f, 26.0f / 255.0f, 25.0f / 255.0f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(6.0f / 255.0f, 6.0f / 255.0f, 8.0f / 255.0f, 200.0f / 255.0f);
	//colors[ImGuiCol_DockingPreview] = ImVec4(47.0f / 255.0f, 6.0f / 255.0f, 29.0f / 255.0f, 0.7f);
	//colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(2.0f / 255.0f, 2.0f / 255.0f, 2.0f / 255.0f, 1.0f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}
